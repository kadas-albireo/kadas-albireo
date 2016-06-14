/***************************************************************************
 *  qgsvbscatalogprovider.cpp                                              *
 *  -------------------------                                              *
 *  begin                : January, 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#include "qgscatalogbrowser.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsvbscatalogprovider.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsmimedatautils.h"

#include <QDomDocument>
#include <QDomNode>
#include <QImageReader>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <qjson/parser.h>


QgsVBSCatalogProvider::QgsVBSCatalogProvider( const QString &baseUrl, QgsCatalogBrowser *browser )
    : QgsCatalogProvider( browser ), mBaseUrl( baseUrl )
{
}

void QgsVBSCatalogProvider::load()
{
  mPendingTasks = 0;
  QUrl url( mBaseUrl );
  QString lang = QSettings().value( "/locale/currentLang", "en" ).toString().left( 2 ).toUpper();
  url.addQueryItem( "lang", lang );
  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  connect( reply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
}

void QgsVBSCatalogProvider::replyFinished()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  if ( reply->error() != QNetworkReply::NoError )
  {
    return;
  }

  QJson::Parser parser;
  QMap<QString, EntryMap> wmtsLayers;
  QMap<QString, EntryMap> wmsLayers;
  QMap<QString, EntryMap> amsLayers;
  QVariantMap listData = parser.parse( reply->readAll() ).toMap();
  foreach ( const QVariant& resultData, listData["results"].toList() )
  {
    QVariantMap resultMap = resultData.toMap();
    if ( resultMap["type"].toString() == "wmts" )
    {
      wmtsLayers[resultMap["url"].toString()].insert( resultMap["layerName"].toString(), ResultEntry( resultMap["category"].toString(), resultMap["title"].toString() ) );
    }
    else if ( resultMap["type"].toString() == "wms" )
    {
      wmsLayers[resultMap["url"].toString()].insert( resultMap["layerName"].toString(), ResultEntry( resultMap["category"].toString(), resultMap["title"].toString() ) );
    }
    else if ( resultMap["type"].toString() == "ams" )
    {
      amsLayers[resultMap["url"].toString()].insert( resultMap["layerName"].toString(), ResultEntry( resultMap["category"].toString(), resultMap["title"].toString() ) );
    }
  }

  mPendingTasks += 1;
  foreach ( const QString& wmtsUrl, wmtsLayers.keys() )
  {
    readWMTSCapabilities( wmtsUrl, wmtsLayers[wmtsUrl] );
  }
  foreach ( const QString& wmsUrl, wmsLayers.keys() )
  {
    readWMSCapabilities( wmsUrl, wmsLayers[wmsUrl] );
  }
  foreach ( const QString& amsUrl, amsLayers.keys() )
  {
    readAMSCapabilities( amsUrl, amsLayers[amsUrl] );
  }

  endTask();
}

void QgsVBSCatalogProvider::endTask()
{
  mPendingTasks -= 1;
  if ( mPendingTasks == 0 )
  {
    emit finished();
  }
}

void QgsVBSCatalogProvider::readWMTSCapabilities( const QString& wmtsUrl, const EntryMap& entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( wmtsUrl + "/1.0.0/WMTSCapabilities.xml" ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "entries", QVariant::fromValue<void*>( reinterpret_cast<void*>( new EntryMap( entries ) ) ) );
  connect( reply, SIGNAL( finished() ), this, SLOT( readWMTSCapabilitiesDo() ) );
}

void QgsVBSCatalogProvider::readWMTSCapabilitiesDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  EntryMap* entries = reinterpret_cast<EntryMap*>( reply->property( "entries" ).value<void*>() );
  QString referer = QSettings().value( "search/referer", "http://localhost" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    if ( !doc.isNull() )
    {
      QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );
      foreach ( const QDomNode& layerItem, childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" ) )
      {
        QString layerid = layerItem.firstChildElement( "ows:Identifier" ).text();
        if ( entries->contains( layerid ) )
        {
          QString title;
          QMimeData* mimeData;
          parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, reply->request().url().toString(), QString( "&referer=%1" ).arg( referer ), title, layerid, mimeData );
          mBrowser->addItem( getCategoryItem(( *entries )[layerid].category.split( "/" ) ), ( *entries )[layerid].title, true, mimeData );
        }
      }
    }
  }

  delete entries;
  endTask();
}

void QgsVBSCatalogProvider::readWMSCapabilities( const QString& wmsUrl, const EntryMap& entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( wmsUrl + "?request=GetCapabilities&service=WMS" ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "url", wmsUrl );
  reply->setProperty( "entries", QVariant::fromValue<void*>( reinterpret_cast<void*>( new EntryMap( entries ) ) ) );
  connect( reply, SIGNAL( finished() ), this, SLOT( readWMSCapabilitiesDo() ) );
}

void QgsVBSCatalogProvider::readWMSCapabilitiesDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  EntryMap* entries = reinterpret_cast<EntryMap*>( reply->property( "entries" ).value<void*>() );
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QStringList imgFormats = parseWMSFormats( doc );
    foreach ( const QDomNode& layerItem, childrenByTagName( doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ), "Layer" ) )
    {
      if ( searchMatchingWMSLayer( layerItem, *entries, url, imgFormats ) )
      {
        break;
      }
    }
  }

  delete entries;
  endTask();
}

void QgsVBSCatalogProvider::readAMSCapabilities( const QString& amsUrl, const EntryMap& entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( amsUrl + "?f=json" ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "url", amsUrl );
  reply->setProperty( "entries", QVariant::fromValue<void*>( reinterpret_cast<void*>( new EntryMap( entries ) ) ) );
  connect( reply, SIGNAL( finished() ), this, SLOT( readAMSCapabilitiesDo() ) );
}

void QgsVBSCatalogProvider::readAMSCapabilitiesDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  EntryMap* entries = reinterpret_cast<EntryMap*>( reply->property( "entries" ).value<void*>() );
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QJson::Parser parser;
    QVariantMap serviceInfoMap = parser.parse( reply->readAll() ).toMap();

    // Parse spatial reference
    QVariantMap spatialReferenceMap = serviceInfoMap["spatialReference"].toMap();
    QString spatialReference = spatialReferenceMap["latestWkid"].toString();
    if ( spatialReference.isEmpty() )
      spatialReference = spatialReferenceMap["wkid"].toString();
    if ( spatialReference.isEmpty() )
      spatialReference = spatialReferenceMap["wkt"].toString();
    else
      spatialReference = QString( "EPSG:%1" ).arg( spatialReference );
    QgsCoordinateReferenceSystem crs;
    crs.createFromString( spatialReference );
    if ( crs.authid().startsWith( "USER:" ) )
      crs.createFromString( "EPSG:4326" ); // If we can't recognize the SRS, fall back to WGS84

    // Parse formats
    QSet<QString> filteredEncodings;
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    foreach ( const QString& encoding, serviceInfoMap["supportedImageFormatTypes"].toString().split( "," ) )
    {
      foreach ( const QByteArray& fmt, supportedFormats )
      {
        if ( encoding.startsWith( fmt, Qt::CaseInsensitive ) )
        {
          filteredEncodings.insert( encoding.toLower() );
        }
      }
    }

    foreach ( const QString& layerName, entries->keys() )
    {
      QgsMimeDataUtils::Uri mimeDataUri;
      mimeDataUri.layerType = "raster";
      mimeDataUri.providerKey = "arcgismapserver";
      mimeDataUri.name = entries->value( layerName ).title;
      QString format = filteredEncodings.contains( "jpg" ) ? "jpg" : filteredEncodings.toList().front();
      mimeDataUri.uri = QString( "crs='%1' format='%2' url='%3' layer='%4'" ).arg( crs.authid() ).arg( format ).arg( url ).arg( layerName );
      QMimeData* mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
      mBrowser->addItem( getCategoryItem( entries->value( layerName ).category.split( "/" ) ), mimeDataUri.name, true, mimeData );
    }
  }

  delete entries;
  endTask();
}

bool QgsVBSCatalogProvider::searchMatchingWMSLayer( const QDomNode& layerItem, const EntryMap& entries, const QString& url, const QStringList& imgFormats )
{
  QString layerid = layerItem.firstChildElement( "Name" ).text();
  if ( entries.contains( layerid ) )
  {
    QString title;
    QMimeData* mimeData;
    parseWMSLayerCapabilities( layerItem, imgFormats, url, title, mimeData );
    mBrowser->addItem( getCategoryItem( entries[layerid].category.split( "/" ) ), entries[layerid].title, true, mimeData );
    return true;
  }
  else
  {
    foreach ( const QDomNode& subLayerItem, childrenByTagName( layerItem.toElement(), "Layer" ) )
    {
      if ( searchMatchingWMSLayer( subLayerItem, entries, url, imgFormats ) )
      {
        return true;
      }
    }
  }
  return false;
}
