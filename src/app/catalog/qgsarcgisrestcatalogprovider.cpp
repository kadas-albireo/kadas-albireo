/***************************************************************************
 *  qgsarcgisrestcatalogprovider.cpp                                       *
 *  --------------------------------                                       *
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

#include "qgsarcgisrestcatalogprovider.h"
#include "qgscatalogbrowser.h"
#include "qgsnetworkaccessmanager.h"
#include <QDomDocument>
#include <QDomNode>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <qjson/parser.h>

QgsArcGisRestCatalogProvider::QgsArcGisRestCatalogProvider( const QString &baseUrl, QgsCatalogBrowser *browser )
    : QgsCatalogProvider( browser ), mBaseUrl( baseUrl )
{
}

void QgsArcGisRestCatalogProvider::load()
{
  mPendingTasks = 0;
  parseFolder( "/" );
}

void QgsArcGisRestCatalogProvider::endTask()
{
  mPendingTasks -= 1;
  if ( mPendingTasks == 0 )
  {
    emit finished();
  }
}

void QgsArcGisRestCatalogProvider::parseFolder( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( mBaseUrl + QString( "/rest/services%1?f=json" ).arg( path ) ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  connect( reply, SIGNAL( finished() ), this, SLOT( parseFolderDo() ) );
}

void QgsArcGisRestCatalogProvider::parseFolderDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QJson::Parser parser;
    QVariantMap folderData = parser.parse( reply->readAll() ).toMap();
    QString catName = QFileInfo( path ).baseName();
    if ( !catName.isEmpty() )
    {
      catTitles.append( catName );
    }
    foreach ( const QVariant& folderName, folderData["folders"].toList() )
    {
      parseFolder( path + "/" + folderName.toString(), catTitles );
    }
    foreach ( const QVariant& serviceData, folderData["services"].toList() )
    {
      parseService( QString( "/" ) + serviceData.toMap()["name"].toString(), catTitles );
    }
  }

  reply->deleteLater();
  endTask();
}

void QgsArcGisRestCatalogProvider::parseService( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( mBaseUrl + QString( "/rest/services%1/MapServer?f=json" ).arg( path ) ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  connect( reply, SIGNAL( finished() ), this, SLOT( parseServiceDo() ) );
}

void QgsArcGisRestCatalogProvider::parseServiceDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QJson::Parser parser;
    QVariantMap serviceData = parser.parse( reply->readAll() ).toMap();
    if ( serviceData.contains( "singleFusedMapCache" ) )
    {
      QString catName = serviceData["documentInfo"].toMap()["Title"].toString();
      if ( catName.isEmpty() )
      {
        catName = QFileInfo( path ).baseName();
      }
      catTitles.append( catName );

      bool isWMTS = serviceData.value( "singleFusedMapCache", 0 ).toInt();
      if ( isWMTS )
      {
        parseWMTS( path, catTitles );
      }
      else
      {
        parseWMS( path, catTitles );
      }
    }
  }

  reply->deleteLater();
  endTask();
}

void QgsArcGisRestCatalogProvider::parseWMTS( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QString url = mBaseUrl + QString( "/rest/services/%1/MapServer/WMTS/1.0.0/WMTSCapabilities.xml" ).arg( path );
  QNetworkRequest req = QNetworkRequest( QUrl( url ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  reply->setProperty( "url", url );
  connect( reply, SIGNAL( finished() ), this, SLOT( parseWMTSDo() ) );
}

void QgsArcGisRestCatalogProvider::parseWMTSDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    if ( !doc.isNull() )
    {
      QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );

      // Layers
      foreach ( const QDomNode& layerItem, childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" ) )
      {
        QString title, layerid;
        QMimeData* mimeData;
        parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, url, "", title, layerid, mimeData );
        mBrowser->addItem( getCategoryItem( catTitles ), title, true, mimeData );
      }
    }
  }

  reply->deleteLater();
  endTask();
}

void QgsArcGisRestCatalogProvider::parseWMS( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QString url = mBaseUrl + QString( "/services/%1/MapServer/WMSServer?request=GetCapabilities&service=WMS" ).arg( path );
  QNetworkRequest req = QNetworkRequest( QUrl( url ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  reply->setProperty( "url", url );
  connect( reply, SIGNAL( finished() ), this, SLOT( parseWMSDo() ) );
}

void QgsArcGisRestCatalogProvider::parseWMSDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QStringList imgFormats = parseWMSFormats( doc );
    foreach ( const QDomNode& layerItem, childrenByTagName( doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ), "Layer" ) )
    {
      QString title;
      QMimeData* mimeData;
      parseWMSLayerCapabilities( layerItem, imgFormats, url, title, mimeData );
      mBrowser->addItem( getCategoryItem( catTitles ), title, mimeData );
    }
  }

  reply->deleteLater();
  endTask();
}
