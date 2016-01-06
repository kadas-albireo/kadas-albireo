/***************************************************************************
 *  qgscoordinateutils.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
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

#include "qgscoordinateutils.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgslatlontoutm.h"
#include "qgsmaplayerregistry.h"
#include "qgspoint.h"
#include "qgsproject.h"
#include "qgslogger.h"

#include <gdal.h>
#include <qmath.h>


double QgsCoordinateUtils::getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QGis::UnitType unit, QString* errMsg )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    if ( errMsg )
      *errMsg = tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." );
    return 0;
  }
  QString rasterFile = layer->source();
  GDALDatasetH raster = GDALOpen( rasterFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( !raster )
  {
    if ( errMsg )
      *errMsg = tr( "Failed to open raster file: %1" ).arg( rasterFile );
    return 0;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    if ( errMsg )
      *errMsg = tr( "Failed to get raster geotransform" );
    GDALClose( raster );
    return 0;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs( proj );
  if ( !rasterCrs.isValid() )
  {
    if ( errMsg )
      *errMsg = tr( "Failed to get raster CRS" );
    GDALClose( raster );
    return 0;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !raster )
  {
    if ( errMsg )
      *errMsg = tr( "Failed to open raster band 0" );
    GDALClose( raster );
    return 0;
  }

  // Transform geo position to raster CRS
  QgsPoint pRaster = QgsCoordinateTransform( crs, rasterCrs ).transform( p );
  QgsDebugMsg( QString( "Transform %1 from %2 to %3 gives %4" ).arg( p.toString() )
               .arg( crs.authid() ).arg( rasterCrs.authid() ).arg( pRaster.toString() ) );

  // Transform raster geo position to pixel coordinates
  double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
  double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );

  double pixValues[4] = {};
  if ( CE_None != GDALRasterIO( band, GF_Read,
                                qFloor( col ), qFloor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
  {
    if ( errMsg )
      *errMsg = tr( "Failed to read pixel values" );
    GDALClose( raster );
    return 0;
  }

  GDALClose( raster );

  // Interpolate values
  double lambdaR = row - qFloor( row );
  double lambdaC = col - qFloor( col );

  double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                 + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
  if ( rasterCrs.mapUnits() != unit )
  {
    value *= QGis::fromUnitToUnitFactor( rasterCrs.mapUnits(), unit );
  }
  return value;
}

QString QgsCoordinateUtils::getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, TargetFormat targetFormat, const QString& targetEpsg )
{
  QgsCoordinateReferenceSystem targetCrs( targetFormat == EPSG ? targetEpsg : "EPSG:4326" );
  switch ( targetFormat )
  {
    case EPSG:
    {
      QgsPoint pOut = QgsCoordinateTransform( sSrs, targetCrs ).transform( p );
      return QString( "%1, %2" ).arg( pOut.x(), 0, 'f', 0 ).arg( pOut.y(), 0, 'f', 0 );
    }
    case DegMinSec:
    {
      QgsPoint pOut = QgsCoordinateTransform( sSrs, targetCrs ).transform( p );
      return pOut.toDegreesMinutesSeconds( 1 );
    }
    case DegMin:
    {
      QgsPoint pOut = QgsCoordinateTransform( sSrs, targetCrs ).transform( p );
      return pOut.toDegreesMinutes( 3 );
    }
    case DecDeg:
    {
      QgsPoint pOut = QgsCoordinateTransform( sSrs, targetCrs ).transform( p );
      return QString( "%1%2,%3%4" ).arg( pOut.x(), 0, 'f', 5 ).arg( QChar( 176 ) )
             .arg( pOut.y(), 0, 'f', 5 ).arg( QChar( 176 ) );
    }
    case UTM:
    {
      QgsPoint pLatLong = QgsCoordinateTransform( sSrs, targetCrs ).transform( p );
      QgsLatLonToUTM::UTMCoo coo = QgsLatLonToUTM::LL2UTM( pLatLong );
      return QString( "%1, %2 (zone %3%4)" ).arg( coo.easting ).arg( coo.northing ).arg( coo.zoneNumber ).arg( coo.zoneLetter );
    }
    case MGRS:
    {
      QgsPoint pLatLong = QgsCoordinateTransform( sSrs, targetCrs ).transform( p );
      QgsLatLonToUTM::UTMCoo utm = QgsLatLonToUTM::LL2UTM( pLatLong );
      QgsLatLonToUTM::MGRSCoo mgrs = QgsLatLonToUTM::UTM2MGRS( utm );
      if ( mgrs.letter100kID.isEmpty() )
        return QString();

      return QString( "%1%2%3 %4 %5" ).arg( mgrs.zoneNumber ).arg( mgrs.zoneLetter ).arg( mgrs.letter100kID ).arg( mgrs.easting, 5, 10, QChar( '0' ) ).arg( mgrs.northing, 5, 10, QChar( '0' ) );
    }
  }
  return "";
}

