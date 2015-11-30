/***************************************************************************
      qgsafsprovider.cpp
      ------------------
    begin                : May 27, 2015
    copyright            : (C) 2015 Sandro Mani
    email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsafsprovider.h"
#include "qgsarcgisrestutils.h"
#include "qgsafsfeatureiterator.h"
#include "qgsdatasourceuri.h"
#include "qgslogger.h"
#include "geometry/qgsgeometry.h"
#include "qgsnetworkaccessmanager.h"

#include <qjson/parser.h>
#include <QEventLoop>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>


QgsAfsProvider::QgsAfsProvider( const QString& uri )
    : QgsVectorDataProvider( uri )
    , mValid( false )
{
  mDataSource = QgsDataSourceURI( uri );

  // Set CRS
  mSourceCRS = QgsCoordinateReferenceSystem( mDataSource.param( "crs" ) );

  // Get layer info
  QString errorTitle, errorMessage;
  QVariantMap layerData = QgsArcGisRestUtils::getLayerInfo( mDataSource.param( "url" ), errorTitle, errorMessage );
  if ( layerData.isEmpty() )
  {
    pushError( errorTitle + ": " + errorMessage );
    QgsDebugMsg( "getLayerInfo failed" );
    return;
  }
  mLayerName = layerData["name"].toString();
  mLayerDescription = layerData["description"].toString();

  // Set extent
  QStringList coords = mDataSource.param( "bbox" ).split( "," );
  if ( coords.size() == 4 )
  {
    bool xminOk = false, yminOk = false, xmaxOk = false, ymaxOk = false;
    mExtent.setXMinimum( coords[0].toDouble( &xminOk ) );
    mExtent.setYMinimum( coords[1].toDouble( &yminOk ) );
    mExtent.setXMaximum( coords[2].toDouble( &xmaxOk ) );
    mExtent.setYMaximum( coords[3].toDouble( &ymaxOk ) );
    if ( !xminOk || !yminOk || !xmaxOk || !ymaxOk )
      mExtent = QgsRectangle();
  }
  if ( mExtent.isEmpty() )
  {
    QVariantMap layerExtentMap = layerData["extent"].toMap();
    bool xminOk = false, yminOk = false, xmaxOk = false, ymaxOk = false;
    mExtent.setXMinimum( layerExtentMap["xmin"].toDouble( &xminOk ) );
    mExtent.setYMinimum( layerExtentMap["ymin"].toDouble( &yminOk ) );
    mExtent.setXMaximum( layerExtentMap["xmax"].toDouble( &xmaxOk ) );
    mExtent.setYMaximum( layerExtentMap["ymax"].toDouble( &ymaxOk ) );
    QString wkid = layerExtentMap["spatialReference"].toMap()["wkid"].toString();
    if ( !xminOk || !yminOk || !xmaxOk || !ymaxOk || wkid.isEmpty() )
      return;
    QgsCoordinateReferenceSystem extentCRS = QgsCoordinateReferenceSystem( QString( "epsg:%1" ).arg( wkid ) );
    mExtent = QgsCoordinateTransform( extentCRS, mSourceCRS ).transformBoundingBox( mExtent );
  }

  // Read fields
  foreach ( const QVariant& fieldData, layerData["fields"].toList() )
  {
    QVariantMap fieldDataMap = fieldData.toMap();
    QString fieldName = fieldDataMap["name"].toString();
    QVariant::Type type = QgsArcGisRestUtils::mapEsriFieldType( fieldDataMap["type"].toString() );
    if ( fieldName == "geometry" || type == QVariant::Invalid )
    {
      QgsDebugMsg( QString( "Skipping unsupported (or possibly geometry) field" ).arg( fieldName ) );
      continue;
    }
    QgsField field( fieldName, type, fieldDataMap["type"].toString(), fieldDataMap["length"].toInt() );
    mFields.append( field );
    QVariantList codedValues = fieldDataMap["domain"].toMap()["codedValues"].toList();
    if ( !codedValues.isEmpty() )
    {
      QVector<QString> enumValues;
      foreach ( const QVariant& codedValue, codedValues )
      {
        QVariantMap codedValueMap = codedValue.toMap();
        if ( codedValueMap.isEmpty() )
          continue;
        int code = codedValueMap["code"].toInt();
        if ( code >= enumValues.size() )
          enumValues.resize( code + 1 );
        enumValues[code] = codedValueMap["name"].toString();
      }
      mEnumValues[mFields.size() - 1] = enumValues.toList();
    }
  }

  // Determine geometry type
  bool hasM = layerData["hasM"].toBool();
  bool hasZ = layerData["hasZ"].toBool();
  mGeometryType = QgsArcGisRestUtils::mapEsriGeometryType( layerData["geometryType"].toString() );
  if ( mGeometryType == QgsWKBTypes::Unknown )
  {
    QgsDebugMsg( "Failed to determine geometry type" );
    return;
  }
  mGeometryType = QgsWKBTypes::zmType( mGeometryType, hasZ, hasM );

  // Read OBJECTIDs of all features: these may not be a continuous sequence,
  // and we need to store these to iterate through the features. This query
  // also returns the name of the ObjectID field.
  QVariantMap objectIdData = QgsArcGisRestUtils::getObjectIds( mDataSource.param( "url" ), errorTitle, errorMessage );
  if ( objectIdData.isEmpty() )
  {
    QgsDebugMsg( QString( "getObjectIds failed: %1 - %2" ).arg( errorTitle ).arg( errorMessage ) );
    return;
  }
  if ( !objectIdData["objectIdFieldName"].isValid() || !objectIdData["objectIds"].isValid() )
  {
    QgsDebugMsg( "Failed to determine objectIdFieldName and/or objectIds" );
    return;
  }
  mObjectIdFieldName = objectIdData["objectIdFieldName"].toString();
  for ( int idx = 0, nIdx = mFields.count(); idx < nIdx; ++idx )
  {
    if ( mFields.at( idx ).name() == mObjectIdFieldName )
    {
      mObjectIdFieldIdx = idx;
      break;
    }
  }
  foreach ( const QVariant& objectId, objectIdData["objectIds"].toList() )
  {
    mObjectIds.append( objectId.toInt() );
  }

  mValid = true;
}

QgsAbstractFeatureSource* QgsAfsProvider::featureSource() const
{
  return new QgsAfsFeatureSource( this );
}

QgsFeatureIterator QgsAfsProvider::getFeatures( const QgsFeatureRequest& request )
{
  return new QgsAfsFeatureIterator( new QgsAfsFeatureSource( this ), true, request );
}

bool QgsAfsProvider::getFeature( const QgsFeatureId &id, QgsFeature &f, bool fetchGeometry, const QList<int>& /*fetchAttributes*/, const QgsRectangle filterRect )
{
  // If cached, return cached feature
  QMap<QgsFeatureId, QgsFeature>::const_iterator it = mCache.find( id );
  if ( it != mCache.end() )
  {
    f = it.value();
    return true;
  }

  // Determine attributes to fetch
  /*QStringList fetchAttribNames;
  foreach ( int idx, fetchAttributes )
    fetchAttribNames.append( mFields.at( idx ).name() );
  */

  // When fetching from server, fetch all attributes and geometry by default so that we can cache them
  QStringList fetchAttribNames;
  QList<int> fetchAttribIdx;
  for ( int idx = 0, n = mFields.size(); idx < n; ++idx )
  {
    fetchAttribNames.append( mFields.at( idx ).name() );
    fetchAttribIdx.append( idx );
  }
  fetchGeometry = true;


  // Query
  QString errorTitle, errorMessage;
  QVariantMap queryData = QgsArcGisRestUtils::getObject(
                            mDataSource.param( "url" ), mObjectIds[id], mDataSource.param( "crs" ), fetchGeometry,
                            fetchAttribNames, QgsWKBTypes::hasM( mGeometryType ), QgsWKBTypes::hasZ( mGeometryType ),
                            filterRect, errorTitle, errorMessage );
  if ( queryData.isEmpty() )
  {
    const_cast<QgsAfsProvider*>( this )->pushError( errorTitle + ": " + errorMessage );
    QgsDebugMsg( "Query returned empty result" );
    return false;
  }

  QVariantList featuresData = queryData["features"].toList();
  if ( featuresData.isEmpty() )
  {
    QgsDebugMsg( "Query returned no features" );
    return false;
  }
  QVariantMap featureData = featuresData[0].toMap();

  // Set FID
  f.setFeatureId( id );

  // Set attributes
  if ( !fetchAttribIdx.isEmpty() )
  {
    QVariantMap attributesData = featureData["attributes"].toMap();
    f.setFields( &mFields );
    QgsAttributes attributes( mFields.size() );
    foreach ( int idx, fetchAttribIdx )
    {
      attributes[idx] = attributesData[mFields.at( idx ).name()];
    }
    f.setAttributes( attributes );
  }

  // Set geometry
  if ( fetchGeometry )
  {
    QVariantMap geometryData = featureData["geometry"].toMap();
    QgsAbstractGeometryV2* geometry = QgsArcGisRestUtils::parseEsriGeoJSON( geometryData, queryData["geometryType"].toString(),
                                      QgsWKBTypes::hasM( mGeometryType ), QgsWKBTypes::hasZ( mGeometryType ) );
    // Above might return 0, which is ok since in theory empty geometries are allowed
    f.setGeometry( new QgsGeometry( geometry ) );
  }
  mCache.insert( f.id(), f );
  return true;
}

void QgsAfsProvider::setDataSourceUri( const QString &uri )
{
  mDataSource = QgsDataSourceURI( uri );
  QgsDataProvider::setDataSourceUri( uri );
}
