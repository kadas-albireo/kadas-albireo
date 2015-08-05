/***************************************************************************
 *  qgsvbslocationsearchprovider.cpp                                       *
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

#include "qgsvbslocationsearchprovider.h"
#include "qgsnetworkaccessmanager.h"
#include "qgslogger.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <qjson/parser.h>


const int QgsVBSLocationSearchProvider::sSearchTimeout = 2000;
const int QgsVBSLocationSearchProvider::sResultCountLimit = 100;
const QByteArray QgsVBSLocationSearchProvider::sGeoAdminUrl = "https://api3.geo.admin.ch/rest/services/api/SearchServer";
const QByteArray QgsVBSLocationSearchProvider::sGeoAdminReferrer = "http://localhost";


QgsVBSLocationSearchProvider::QgsVBSLocationSearchProvider( )
{
  mNetReply = 0;

  mCategoryMap.insert( "sn25", tr( "Places" ) );
  mCategoryMap.insert( "gg25", tr( "Municipalities" ) );
  mCategoryMap.insert( "kantone", tr( "Cantons" ) );
  mCategoryMap.insert( "district", tr( "Districts" ) );
  mCategoryMap.insert( "address", tr( "Address" ) );
  mCategoryMap.insert( "zipcode", tr( "Zip Codes" ) );

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, SIGNAL( timeout() ), this, SLOT( replyFinished() ) );
}

void QgsVBSLocSearchProvider::startSearch( const QString &searchtext )
{
  QUrl url( sGeoAdminUrl );
  url.addQueryItem( "type", "locations" );
  url.addQueryItem( "searchText", searchtext );
  url.addQueryItem( "limit", QString::number( sResultCountLimit ) );

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", sGeoAdminReferrer );
  mNetReply = QgsNetworkAccessManager::instance()->get( req );
  connect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
  mTimeoutTimer.start( sSearchTimeout );
}

void QgsVBSLocationSearchProvider::cancelSearch()
{
  if ( mNetReply )
  {
    mTimeoutTimer.stop();
    disconnect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
    mNetReply->close();
    mNetReply->deleteLater();
    mNetReply = 0;
  }
}

void QgsVBSLocationSearchProvider::replyFinished()
{
  if ( !mNetReply )
    return;

  if ( mNetReply->error() != QNetworkReply::NoError || !mTimeoutTimer.isActive() )
  {
    mNetReply->deleteLater();
    mNetReply = 0;
    emit searchFinished();
    return;
  }

  QVariantMap result = QJson::Parser().parse( mNetReply ).toMap();
  foreach ( const QVariant& item, result["results"].toList() )
  {
    QVariantMap itemMap = item.toMap();
    QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

    if ( !mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
    {
      QgsDebugMsg( "Box RegEx did not match " + itemAttrsMap["geom_st_box2d"].toString() );
      continue;
    }
    QString origin = itemAttrsMap["origin"].toString();

    SearchResult searchResult;
    searchResult.bbox = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                      mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
    // When bbox is empty, fallback to pos + zoomScale is used
    searchResult.pos = QgsPoint( itemAttrsMap["y"].toDouble(), itemAttrsMap["x"].toDouble() );
    searchResult.zoomScale = 1000;

    searchResult.category = mCategoryMap.contains( origin ) ? mCategoryMap[origin] : origin;
    searchResult.text = itemAttrsMap["label"].toString();
    searchResult.text.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags
    searchResult.crs = QgsCoordinateReferenceSystem( "EPSG:21781" );
    emit searchResultFound( searchResult );
    QgsDebugMsg( QString( "Result found: %1 added to category %2" ).arg( searchResult.text ).arg( searchResult.category ) );
  }
  mNetReply->deleteLater();
  mNetReply = 0;
  emit searchFinished();
}
