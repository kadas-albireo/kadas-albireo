/***************************************************************************
 *  qgsremotedatasearchprovider.cpp                                     *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsremotedatasearchprovider.h"
#include "qgsnetworkaccessmanager.h"
#include "qgscoordinatetransform.h"
#include "qgslinestringv2.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaplayer.h"
#include "qgspolygonv2.h"
#include "qgsrasterlayer.h"
#include "qgslogger.h"
#include "qgsdatasourceuri.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <qjson/parser.h>


const int QgsRemoteDataSearchProvider::sSearchTimeout = 2000;
const int QgsRemoteDataSearchProvider::sResultCountLimit = 100;


QgsRemoteDataSearchProvider::QgsRemoteDataSearchProvider( QgsMapCanvas* mapCanvas )
    : QgsSearchProvider( mapCanvas )
{
  mNetReply = 0;
  mReplyFilter = 0;

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, SIGNAL( timeout() ), this, SLOT( replyFinished() ) );
}

void QgsRemoteDataSearchProvider::startSearch( const QString &searchtext, const SearchRegion &searchRegion )
{
  QStringList remoteLayers;
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers() )
  {
    if ( layer->type() != QgsMapLayer::RasterLayer )
    {
      continue;
    }
    QgsRasterLayer* rasterLayer = static_cast<QgsRasterLayer*>( layer );
    QUrl url( QString( "?" ) + QgsDataSourceURI( rasterLayer->dataProvider()->dataSourceUri() ).uri() );
    if ( url.queryItemValue( "url" ).contains( "wmts.geo.admin.ch" ) ||
         url.queryItemValue( "url" ).contains( "wms.geo.admin.ch" ) )
    {
      remoteLayers.append( url.queryItemValue( "layers" ).split( "," ) );
    }
  }

  QUrl url( QSettings().value( "search/remotedatasearchurl", "https://api3.geo.admin.ch/rest/services/api/SearchServer" ).toString() );
  url.addQueryItem( "type", "featuresearch" );
  url.addQueryItem( "searchText", searchtext );
  url.addQueryItem( "features", remoteLayers.join( "," ) );
  if ( !searchRegion.polygon.isEmpty() )
  {
    QgsRectangle rect;
    rect.setMinimal();
    QgsLineStringV2* exterior = new QgsLineStringV2();
    QgsCoordinateTransform ct( searchRegion.crs, QgsCoordinateReferenceSystem( "EPSG:21781" ) );
    foreach ( const QgsPoint& p, searchRegion.polygon )
    {
      QgsPoint pt = ct.transform( p );
      rect.include( pt );
      exterior->addVertex( QgsPointV2( pt ) );
    }
    url.addQueryItem( "bbox", QString( "%1,%2,%3,%4" ).arg( rect.xMinimum() ).arg( rect.yMinimum() ).arg( rect.xMaximum() ).arg( rect.yMaximum() ) );
    QgsPolygonV2* poly = new QgsPolygonV2();
    poly->setExteriorRing( exterior );
    mReplyFilter = new QgsGeometry( poly );
  }

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QSettings().value( "search/referrer", "http://localhost" ).toByteArray() );
  mNetReply = QgsNetworkAccessManager::instance()->get( req );
  connect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
  mTimeoutTimer.start( sSearchTimeout );
}

void QgsRemoteDataSearchProvider::cancelSearch()
{
  if ( mNetReply )
  {
    mTimeoutTimer.stop();
    disconnect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
    mNetReply->close();
    mNetReply->deleteLater();
    mNetReply = 0;
    delete mReplyFilter;
    mReplyFilter = 0;
  }
}

void QgsRemoteDataSearchProvider::replyFinished()
{
  if ( !mNetReply )
    return;

  if ( mNetReply->error() != QNetworkReply::NoError || !mTimeoutTimer.isActive() )
  {
    mNetReply->deleteLater();
    mNetReply = 0;
    delete mReplyFilter;
    mReplyFilter = 0;
    emit searchFinished();
    return;
  }
  QStringList bboxStr = mNetReply->request().url().queryItemValue( "bbox" ).split( "," );
  QgsRectangle bbox;
  if ( bboxStr.size() == 4 )
  {
    bbox.setXMinimum( bboxStr[0].toDouble() );
    bbox.setYMinimum( bboxStr[1].toDouble() );
    bbox.setXMaximum( bboxStr[2].toDouble() );
    bbox.setYMaximum( bboxStr[3].toDouble() );
  }

  QByteArray replyText = mNetReply->readAll();
  QgsDebugMsg( replyText );
  QJson::Parser parser;
  QVariant result = parser.parse( replyText );
  if ( result.isNull() )
  {
    QgsDebugMsg( QString( "Error at line %1: %2" ).arg( parser.errorLine() ).arg( parser.errorString() ) );
  }
  foreach ( const QVariant& item, result.toMap()["results"].toList() )
  {
    QVariantMap itemMap = item.toMap();
    QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

    if ( !mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
    {
      QgsDebugMsg( "Box RegEx did not match " + itemAttrsMap["geom_st_box2d"].toString() );
      continue;
    }

    SearchResult searchResult;
    searchResult.bbox = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                      mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
    // When bbox is empty, fallback to pos + zoomScale is used
    searchResult.pos = QgsPoint( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
    searchResult.pos = QgsCoordinateTransform( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsCoordinateReferenceSystem( "EPSG:21781" ) ).transform( searchResult.pos );
    if ( !bbox.isEmpty() && !bbox.contains( searchResult.pos ) )
    {
      continue;
    }
    if ( mReplyFilter && !mReplyFilter->contains( &searchResult.pos ) )
    {
      continue;
    }

    searchResult.zoomScale = 1000;

    searchResult.category = tr( "Feature" );
    searchResult.text = itemAttrsMap["label"].toString() + " (" + itemAttrsMap["detail"].toString() + ")";
    searchResult.text.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags
    searchResult.crs = QgsCoordinateReferenceSystem( "EPSG:21781" );
    emit searchResultFound( searchResult );
  }
  mNetReply->deleteLater();
  mNetReply = 0;
  delete mReplyFilter;
  mReplyFilter = 0;
  emit searchFinished();
}
