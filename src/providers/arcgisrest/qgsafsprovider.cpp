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
#include "qgscrscache.h"
#include "qgsdatasourceuri.h"
#include "qgslogger.h"
#include "geometry/qgsgeometry.h"
#include "qgsnetworkaccessmanager.h"

#include <qjson/parser.h>
#include <QEventLoop>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>


QgsAfsProvider::QgsAfsProvider( const QString &uri )
    : QgsVectorDataProvider( uri )
    , mValid( false )
    , mObjectIdFieldIdx( -1 )
{
  mSharedData = QSharedPointer<QgsAfsSharedData>( new QgsAfsSharedData() );
  mSharedData->mGeometryType = QgsWKBTypes::Unknown;
  mSharedData->mDataSource = QgsDataSourceURI( uri );

  // Set CRS
  mSharedData->mSourceCRS = QgsCRSCache::instance()->crsByAuthId( mSharedData->mDataSource.param( "crs" ) );

  // Get layer info
  QString errorTitle, errorMessage;
  QVariantMap layerData = QgsArcGisRestUtils::getLayerInfo( mSharedData->mDataSource.param( "url" ), errorTitle, errorMessage );
  if ( layerData.isEmpty() )
  {
    pushError( errorTitle + ": " + errorMessage );
    appendError( QgsErrorMessage( tr( "getLayerInfo failed" ), "AFSProvider" ) );
    return;
  }
  mLayerName = layerData["name"].toString();
  mLayerDescription = layerData["description"].toString();

  // Set extent
  QStringList coords = mSharedData->mDataSource.param( "bbox" ).split( "," );
  if ( coords.size() == 4 )
  {
    bool xminOk = false, yminOk = false, xmaxOk = false, ymaxOk = false;
    mSharedData->mExtent.setXMinimum( coords[0].toDouble( &xminOk ) );
    mSharedData->mExtent.setYMinimum( coords[1].toDouble( &yminOk ) );
    mSharedData->mExtent.setXMaximum( coords[2].toDouble( &xmaxOk ) );
    mSharedData->mExtent.setYMaximum( coords[3].toDouble( &ymaxOk ) );
    if ( !xminOk || !yminOk || !xmaxOk || !ymaxOk )
      mSharedData->mExtent = QgsRectangle();
  }
  if ( mSharedData->mExtent.isEmpty() )
  {
    QVariantMap layerExtentMap = layerData["extent"].toMap();
    bool xminOk = false, yminOk = false, xmaxOk = false, ymaxOk = false;
    mSharedData->mExtent.setXMinimum( layerExtentMap["xmin"].toDouble( &xminOk ) );
    mSharedData->mExtent.setYMinimum( layerExtentMap["ymin"].toDouble( &yminOk ) );
    mSharedData->mExtent.setXMaximum( layerExtentMap["xmax"].toDouble( &xmaxOk ) );
    mSharedData->mExtent.setYMaximum( layerExtentMap["ymax"].toDouble( &ymaxOk ) );
    if ( !xminOk || !yminOk || !xmaxOk || !ymaxOk )
    {
      appendError( QgsErrorMessage( tr( "Could not retreive layer extent" ), "AFSProvider" ) );
      return;
    }
    QgsCoordinateReferenceSystem extentCrs = QgsArcGisRestUtils::parseSpatialReference( layerExtentMap["spatialReference"].toMap() );
    if ( !extentCrs.isValid() )
    {
      appendError( QgsErrorMessage( tr( "Could not parse spatial reference" ), "AFSProvider" ) );
      return;
    }
    mSharedData->mExtent = QgsCoordinateTransform( extentCrs, mSharedData->mSourceCRS ).transformBoundingBox( mSharedData->mExtent );
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
    mSharedData->mFields.append( field );
  }

  // Determine geometry type
  bool hasM = layerData["hasM"].toBool();
  bool hasZ = layerData["hasZ"].toBool();
  mSharedData->mGeometryType = QgsArcGisRestUtils::mapEsriGeometryType( layerData["geometryType"].toString() );
  if ( mSharedData->mGeometryType == QgsWKBTypes::Unknown )
  {
    appendError( QgsErrorMessage( tr( "Failed to determine geometry type" ), "AFSProvider" ) );
    return;
  }
  mSharedData->mGeometryType = QgsWKBTypes::zmType( mSharedData->mGeometryType, hasZ, hasM );

  // Read OBJECTIDs of all features: these may not be a continuous sequence,
  // and we need to store these to iterate through the features. This query
  // also returns the name of the ObjectID field.
  QVariantMap objectIdData = QgsArcGisRestUtils::getObjectIds( mSharedData->mDataSource.param( "url" ), errorTitle, errorMessage );
  if ( objectIdData.isEmpty() )
  {
    appendError( QgsErrorMessage( tr( "getObjectIds failed: %1 - %2" ).arg( errorTitle ).arg( errorMessage ), "AFSProvider" ) );
    return;
  }
  if ( !objectIdData["objectIdFieldName"].isValid() || !objectIdData["objectIds"].isValid() )
  {
    appendError( QgsErrorMessage( tr( "Failed to determine objectIdFieldName and/or objectIds" ), "AFSProvider" ) );
    return;
  }
  QString objectIdFieldName = objectIdData["objectIdFieldName"].toString();
  for ( int idx = 0, nIdx = mSharedData->mFields.count(); idx < nIdx; ++idx )
  {
    if ( mSharedData->mFields.at( idx ).name() == objectIdFieldName )
    {
      mObjectIdFieldIdx = idx;
      break;
    }
  }
  foreach ( const QVariant& objectId, objectIdData["objectIds"].toList() )
  {
    mSharedData->mObjectIds.append( objectId.toInt() );
  }

  mValid = true;
}

QgsAbstractFeatureSource* QgsAfsProvider::featureSource() const
{
  return new QgsAfsFeatureSource( mSharedData );
}

QgsFeatureIterator QgsAfsProvider::getFeatures( const QgsFeatureRequest& request )
{
  return new QgsAfsFeatureIterator( new QgsAfsFeatureSource( mSharedData ), true, request );
}

QGis::WkbType QgsAfsProvider::geometryType() const
{
  return static_cast<QGis::WkbType>( mSharedData->mGeometryType );
}

long QgsAfsProvider::featureCount() const
{
  return mSharedData->mObjectIds.size();
}

const QgsFields & QgsAfsProvider::fields() const
{
  return mSharedData->mFields;
}

void QgsAfsProvider::setDataSourceUri( const QString &uri )
{
  mSharedData->mDataSource = QgsDataSourceURI( uri );
  QgsDataProvider::setDataSourceUri( uri );
}

QgsCoordinateReferenceSystem QgsAfsProvider::crs()
{
  return mSharedData->crs();
}

QgsRectangle QgsAfsProvider::extent()
{
  return mSharedData->extent();
}

void QgsAfsProvider::reloadData()
{
  mSharedData->mCache.clear();
}
