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

#include "qgscrscache.h"
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
  mReplyFilter = 0;

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, SIGNAL( timeout() ), this, SLOT( searchTimeout() ) );
}

void QgsRemoteDataSearchProvider::startSearch( const QString &searchtext, const SearchRegion &searchRegion )
{
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers() )
  {
    if ( layer->type() != QgsMapLayer::RasterLayer )
    {
      continue;
    }
    QgsRasterLayer* rasterLayer = static_cast<QgsRasterLayer*>( layer );
    QUrl url( QString( "?" ) + QgsDataSourceURI( rasterLayer->dataProvider()->dataSourceUri() ).uri() );
    if ( url.queryItemValue( "url" ).contains( "geo.admin.ch" ) )
    {
      foreach ( const QString& layerId, url.queryItemValue( "layers" ).split( "," ) )
      {
        QUrl url( QSettings().value( "search/remotedatasearchurl", "https://api3.geo.admin.ch/rest/services/api/SearchServer" ).toString() );
        url.addQueryItem( "type", "featuresearch" );
        url.addQueryItem( "searchText", searchtext );
        url.addQueryItem( "features", layerId );
        if ( !searchRegion.polygon.isEmpty() )
        {
          QgsRectangle rect;
          rect.setMinimal();
          QgsLineStringV2* exterior = new QgsLineStringV2();
          const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( searchRegion.crs, "EPSG:21781" );
          foreach ( const QgsPoint& p, searchRegion.polygon )
          {
            QgsPoint pt = ct->transform( p );
            rect.include( pt );
            exterior->addVertex( QgsPointV2( pt ) );
          }
          url.addQueryItem( "bbox", QString( "%1,%2,%3,%4" ).arg( rect.xMinimum() ).arg( rect.yMinimum() ).arg( rect.xMaximum() ).arg( rect.yMaximum() ) );
          QgsPolygonV2* poly = new QgsPolygonV2();
          poly->setExteriorRing( exterior );
          mReplyFilter = new QgsGeometry( poly );
        }

        QNetworkRequest req( url );
        req.setRawHeader( "Referer", QSettings().value( "search/referer", "http://localhost" ).toByteArray() );
        QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
        reply->setProperty( "layerName", rasterLayer->name() );
        connect( reply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
        mNetReplies.append( reply );
      }
    }
  }
  mTimeoutTimer.start( sSearchTimeout );
}

void QgsRemoteDataSearchProvider::cancelSearch()
{
  mTimeoutTimer.stop();
  while ( !mNetReplies.isEmpty() )
  {
    QNetworkReply* reply = mNetReplies.front();
    disconnect( reply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
    reply->close();
    mNetReplies.removeAll( reply );
    reply->deleteLater();
  }
  delete mReplyFilter;
  mReplyFilter = 0;
}

void QgsRemoteDataSearchProvider::replyFinished()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  if ( !reply )
    return;

  if ( reply->error() == QNetworkReply::NoError )
  {
    QString layerName = reply->property( "layerName" ).toString();
    QStringList bboxStr = reply->request().url().queryItemValue( "bbox" ).split( "," );
    QgsRectangle bbox;
    if ( bboxStr.size() == 4 )
    {
      bbox.setXMinimum( bboxStr[0].toDouble() );
      bbox.setYMinimum( bboxStr[1].toDouble() );
      bbox.setXMaximum( bboxStr[2].toDouble() );
      bbox.setYMaximum( bboxStr[3].toDouble() );
    }

    QByteArray replyText = reply->readAll();
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
      searchResult.pos = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", "EPSG:21781" )->transform( searchResult.pos );
      if ( !bbox.isEmpty() && !bbox.contains( searchResult.pos ) )
      {
        continue;
      }
      if ( mReplyFilter && !mReplyFilter->contains( &searchResult.pos ) )
      {
        continue;
      }

      searchResult.zoomScale = 1000;
      searchResult.category = tr( "Layer %1" ).arg( layerName );
      searchResult.categoryPrecedence = 11;
      searchResult.text = itemAttrsMap["label"].toString() + " (" + itemAttrsMap["detail"].toString() + ")";
      searchResult.text.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags
      searchResult.crs = "EPSG:21781";
      searchResult.showPin = true;
      emit searchResultFound( searchResult );
    }
  }
  reply->deleteLater();
  mNetReplies.removeAll( reply );
  if ( mNetReplies.isEmpty() )
  {
    delete mReplyFilter;
    mReplyFilter = 0;
    emit searchFinished();
  }
}
