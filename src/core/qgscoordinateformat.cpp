/***************************************************************************
 *  qgscoordinateformat.cpp                                                *
 *  -----------------------                                                *
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

#include "qgscoordinateformat.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgscrscache.h"
#include "qgslatlontoutm.h"
#include "qgsmaplayerregistry.h"
#include "qgspoint.h"
#include "qgsproject.h"
#include "qgslogger.h"

#include <gdal.h>
#include <qmath.h>

QgsCoordinateFormat::QgsCoordinateFormat()
{
  mFormat = Format::Default;
  mEpsg = "EPSG:4326";
}

QgsCoordinateFormat* QgsCoordinateFormat::instance()
{
  static QgsCoordinateFormat instance;
  return &instance;
}

void QgsCoordinateFormat::setCoordinateDisplayFormat( Format format, const QString& epsg )
{
  mFormat = format;
  mEpsg = epsg;
  emit coordinateDisplayFormatChanged(format, epsg);
}

void QgsCoordinateFormat::getCoordinateDisplayFormat( Format& format, QString& epsg ) const
{
  format = mFormat;
  epsg = mEpsg;
}

QString QgsCoordinateFormat::getDisplayString(const QgsPoint& p , const QgsCoordinateReferenceSystem &sSrs) const
{
  return getDisplayString(p, sSrs, mFormat, mEpsg);
}

QString QgsCoordinateFormat::getDisplayString(const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs , Format format, const QString &epsg)
{
  QgsPoint pTrans = QgsCoordinateTransformCache::instance()->transform( sSrs.authid(), epsg )->transform( p );
  switch ( format )
  {
    case Default:
    {
      const QgsCoordinateReferenceSystem& crs = QgsCRSCache::instance()->crsByAuthId(epsg);
      int prec = crs.mapUnits() == QGis::Degrees ? 4 : 0;
      return QString( "%1, %2" ).arg( pTrans.x(), 0, 'f', prec ).arg( pTrans.y(), 0, 'f', prec );
    }
    case DegMinSec:
    {
      return pTrans.toDegreesMinutesSeconds( 1 );
    }
    case DegMin:
    {
      return pTrans.toDegreesMinutes( 3 );
    }
    case DecDeg:
    {
      return QString( "%1%2,%3%4" ).arg( pTrans.x(), 0, 'f', 5 ).arg( QChar( 176 ) )
             .arg( pTrans.y(), 0, 'f', 5 ).arg( QChar( 176 ) );
    }
    case UTM:
    {
      QgsLatLonToUTM::UTMCoo coo = QgsLatLonToUTM::LL2UTM( pTrans );
      return QString( "%1, %2 (zone %3%4)" ).arg( coo.easting ).arg( coo.northing ).arg( coo.zoneNumber ).arg( coo.zoneLetter );
    }
    case MGRS:
    {
      QgsLatLonToUTM::UTMCoo utm = QgsLatLonToUTM::LL2UTM( pTrans );
      QgsLatLonToUTM::MGRSCoo mgrs = QgsLatLonToUTM::UTM2MGRS( utm );
      if ( mgrs.letter100kID.isEmpty() )
        return QString();

      return QString( "%1%2%3 %4 %5" ).arg( mgrs.zoneNumber ).arg( mgrs.zoneLetter ).arg( mgrs.letter100kID ).arg( mgrs.easting, 5, 10, QChar( '0' ) ).arg( mgrs.northing, 5, 10, QChar( '0' ) );
    }
  }
  return "";
}

double QgsCoordinateFormat::getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QGis::UnitType unit, QString* errMsg )
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

  // Get vertical unit
  QGis::UnitType vertUnit = strcmp( GDALGetRasterUnitType( band ), "ft" ) == 0 ? QGis::Feet : QGis::Meters;

  // Transform geo position to raster CRS
  QgsPoint pRaster = QgsCoordinateTransformCache::instance()->transform( crs.authid(), rasterCrs.authid() )->transform( p );
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
    value *= QGis::fromUnitToUnitFactor( vertUnit, unit );
  }
  return value;
}
