/***************************************************************************
 *  qgsgeoadminrestcatalogprovider.cpp                                     *
 *  ----------------------------------                                     *
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
#include "qgsgeoadminrestcatalogprovider.h"
#include "qgsnetworkaccessmanager.h"
#include <QDomDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>

QgsGeoAdminRestCatalogProvider::QgsGeoAdminRestCatalogProvider( const QString &baseUrl, QgsCatalogBrowser *browser )
    : QgsCatalogProvider( browser ), mBaseUrl( baseUrl )
{
}

void QgsGeoAdminRestCatalogProvider::load()
{
  QNetworkRequest req( mBaseUrl );
  req.setRawHeader( "Referer", QSettings().value( "search/referrer", "http://localhost" ).toByteArray() );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  connect( reply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
}

void QgsGeoAdminRestCatalogProvider::parseTheme( QStandardItem *parent, const QDomElement& theme, QMap<QString, QStandardItem*>& layerParentMap )
{
  parent = mBrowser->addItem( parent, theme.firstChildElement( "ows:Title" ).text() );
  QDomNodeList layerRefs = theme.toElement().elementsByTagName( "LayerRef" );
  for ( int iLayerRef = 0, nLayerRefs = layerRefs.count(); iLayerRef < nLayerRefs; ++iLayerRef )
  {
    QDomNode layerRef = layerRefs.item( iLayerRef );
    layerParentMap.insert( layerRef.toElement().text(), parent );
  }

  foreach ( const QDomNode& theme, childrenByTagName( theme, "Theme" ) )
  {
    parseTheme( parent, theme.toElement(), layerParentMap );
  }
}

void QgsGeoAdminRestCatalogProvider::replyFinished()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  if ( reply->error() != QNetworkReply::NoError )
  {
    return;
  }
  QDomDocument doc;
  doc.setContent( reply->readAll() );
  reply->deleteLater();

  if ( doc.isNull() )
  {
    return;
  }

  QString referer = QSettings().value( "search/referrer", "http://localhost" ).toString();

  // Categories
  QMap<QString, QStandardItem*> layerParentMap;
  QDomElement themes = doc.firstChildElement( "Capabilities" ).firstChildElement( "Themes" );
  foreach ( const QDomNode& theme, childrenByTagName( themes, "Theme" ) )
  {
    parseTheme( 0, theme.toElement(), layerParentMap );
  }

  // Tile matrix sets
  QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );

  // Layers
  QList<QDomNode> layerItems = childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" );
  foreach ( const QDomNode& layerItem, layerItems )
  {
    QString title, layerid;
    QMimeData* mimeData;
    parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, mBaseUrl, QString( "&referer=%1" ).arg( referer ), title, layerid, mimeData );

    // Determine paren
    QStandardItem* parent = 0;
    if ( layerParentMap.contains( layerid ) )
    {
      parent = layerParentMap.value( layerid );
    }
    else
    {
      parent = mBrowser->addItem( 0, tr( "Uncategorized" ) );
    }
    mBrowser->addItem( parent, title, true, mimeData );
  }
  emit finished();
}
