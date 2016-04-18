/***************************************************************************
 *  qgscatalogprovider.cpp                                                 *
 *  ----------------------                                                 *
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
#include "qgscatalogprovider.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsmimedatautils.h"
#include <QDomElement>


QgsCatalogProvider::QgsCatalogProvider( QgsCatalogBrowser* browser )
    : QObject( browser ), mBrowser( browser )
{}

QList<QDomNode> QgsCatalogProvider::childrenByTagName( const QDomElement& element, const QString& tagName ) const
{
  QDomElement el = element.firstChildElement( tagName );
  QList<QDomNode> children;
  while ( !el.isNull() )
  {
    children.append( el );
    el = el.nextSiblingElement( tagName );
  }
  return children;
}

QMap<QString, QString> QgsCatalogProvider::parseWMTSTileMatrixSets( const QDomDocument& doc ) const
{
  QMap<QString, QString> tileMatrixSetMap;
  foreach ( const QDomNode& tileMatrixSet, childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "TileMatrixSet" ) )
  {
    QString identifier = tileMatrixSet.firstChildElement( "ows:Identifier" ).text();
    QgsCoordinateReferenceSystem crs;
    crs.createFromOgcWmsCrs( tileMatrixSet.firstChildElement( "ows:SupportedCRS" ).text() );
    tileMatrixSetMap.insert( identifier, crs.authid() );
  }
  return tileMatrixSetMap;
}

void QgsCatalogProvider::parseWMTSLayerCapabilities( const QDomNode& layerItem, const QMap<QString, QString>& tileMatrixSetMap, const QString& url, const QString& extraParams, QString& title, QString& layerid, QMimeData*& mimeData ) const
{
  title = layerItem.firstChildElement( "ows:Title" ).text();
  layerid = layerItem.firstChildElement( "ows:Identifier" ).text();
  QString imgFormat = layerItem.firstChildElement( "Format" ).text();
  QString tileMatrixSet = layerItem.firstChildElement( "TileMatrixSetLink" ).firstChildElement( "TileMatrixSet" ).text();
  QString styleId = layerItem.firstChildElement( "Style" ).firstChildElement( "ows:Identifier" ).text();
  QString supportedCrs = tileMatrixSetMap[tileMatrixSet];
  QString dimensionParams = "";

  foreach ( const QDomNode& dim, childrenByTagName( layerItem.toElement(), "Dimension" ) )
  {
    QString dimId = dim.firstChildElement( "ows:Identifier" ).text();
    QString dimVal = dim.firstChildElement( "Default" ).text();
    dimensionParams += dimId + "%3D" + dimVal;
  }

  QgsMimeDataUtils::Uri mimeDataUri;
  mimeDataUri.layerType = "raster";
  mimeDataUri.providerKey = "wms";
  mimeDataUri.name = title;
  mimeDataUri.supportedCrs.append( supportedCrs );
  mimeDataUri.supportedFormats.append( imgFormat );
  mimeDataUri.uri = QString(
                      "contextualWMSLegend=0&featureCount=10&dpiMode=7&smoothPixmapTransform=1"
                      "&layers=%1&crs=%2&format=%3&tileMatrixSet=%4"
                      "&styles=%5&url=%6%7"
                    ).arg( layerid ).arg( supportedCrs ).arg( imgFormat ).arg( tileMatrixSet ).arg( styleId ).arg( url ).arg( extraParams );
  mimeDataUri.uri += "&tileDimensions=" + dimensionParams; // Add this last because it contains % (from %3D) which confuses .arg
  mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
}

QStringList QgsCatalogProvider::parseWMSFormats( const QDomDocument& doc ) const
{
  QStringList formats;
  QDomElement getMapItem = doc.firstChildElement( "WMS_Capabilities" )
                           .firstChildElement( "Capability" )
                           .firstChildElement( "Request" )
                           .firstChildElement( "GetMap" );
  foreach ( const QDomNode& formatItem, childrenByTagName( getMapItem, "Format" ) )
  {
    formats.append( formatItem.toElement().text() );
  }
  return formats;
}

QString QgsCatalogProvider::parseWMSNestedLayer( const QDomNode& layerItem ) const
{
  QString subLayerParams;
  foreach ( const QDomNode& subLayerItem, childrenByTagName( layerItem.toElement(), "Layer" ) )
  {
    QString subLayerName = subLayerItem.firstChildElement( "Name" ).toElement().text();
    subLayerParams += "&layers=" + subLayerName;
    subLayerParams += "&styles=";
    subLayerParams += parseWMSNestedLayer( subLayerItem );
  }
  return subLayerParams;
}

void QgsCatalogProvider::parseWMSLayerCapabilities( const QDomNode& layerItem, const QStringList& imgFormats, const QString& url, QString& title, QMimeData*& mimeData ) const
{
  title = layerItem.firstChildElement( "Title" ).text();
  QString layerid = layerItem.firstChildElement( "Name" ).text();
  QString subLayerParams = QString( "&layers=%1&styles=" ).arg( layerid );
  QStringList supportedCrs;
  foreach ( const QDomNode& crsItem, childrenByTagName( layerItem.toElement(), "CRS" ) )
  {
    supportedCrs.append( crsItem.toElement().text() );
  }
  QString imgFormat = imgFormats[0];
  // Prefer jpeg or png
  if ( imgFormats.contains( "image/jpeg" ) )
  {
    imgFormat = "image/jpeg";
  }
  else if ( imgFormats.contains( "image/png" ) )
  {
    imgFormat = "image/png";
  }

  QgsMimeDataUtils::Uri mimeDataUri;
  mimeDataUri.layerType = "raster";
  mimeDataUri.providerKey = "wms";
  mimeDataUri.name = title;
  mimeDataUri.supportedCrs = supportedCrs;
  mimeDataUri.supportedFormats = imgFormats;
  mimeDataUri.uri = QString(
                      "contextualWMSLegend=0&featureCount=10&dpiMode=7"
                      "&crs=%1&format=%2"
                      "%3&url=%4" ).arg( supportedCrs[0] ).arg( imgFormat ).arg( subLayerParams ).arg( url );
  mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
}

QStandardItem* QgsCatalogProvider::getCategoryItem( const QStringList& titles )
{
  QStandardItem* cat = 0;
  foreach ( const QString& title, titles )
  {
    cat = mBrowser->addItem( cat, title );
  }
  return cat;
}

