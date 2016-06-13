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
#include "qgsvbscatalogprovider.h"
#include "qgsnetworkaccessmanager.h"

#include <QDomDocument>
#include <QDomNode>
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
  QNetworkRequest req( mBaseUrl );
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
