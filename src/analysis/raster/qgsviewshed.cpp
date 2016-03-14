/***************************************************************************
                          qgsviewshed.cpp  -  Viewshed computation
                          ---------------------------
    begin                : November 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsviewshed.h"
#include "qgslogger.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgsdistancearea.h"
#include <qmath.h>
#include <gdal.h>
#include <cpl_string.h>
#include <cstring>
#include <limits>
#include <QProgressDialog>
#include <QVector>


static inline double geoToPixelX( double gtrans[6], double x, double y )
{
  return ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * y + gtrans[5] * x ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
}

static inline double geoToPixelY( double gtrans[6], double x, double y )
{
  return ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * y + gtrans[4] * x ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
}

static inline double pixelToGeoX( double gtrans[6], double px, double py )
{
  return gtrans[0] + px * gtrans[1] + py * gtrans[2];
}

static inline double pixelToGeoY( double gtrans[6], double px, double py )
{
  return gtrans[3] + px * gtrans[4] + py * gtrans[5];
}

bool QgsViewshed::computeViewshed( const QString &inputFile, const QString &outputFile, const QString &outputFormat, QgsPoint observerPos, const QgsCoordinateReferenceSystem &observerPosCrs, double observerHeight, double targetHeight, double radius, const QGis::UnitType distanceElevUnit, const QVector<QgsPoint> &filterRegion, bool displayVisible, QProgressDialog *progress )
{
  // Open input file
  GDALDatasetH inputDataset = GDALOpen( inputFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( inputDataset == 0 )
  {
    QgsDebugMsg( "Failed to open input dataset" );
    return false;
  }

  // Transform positions and measurements to dataset CRS
  QgsCoordinateReferenceSystem datasetCrs( QString( GDALGetProjectionRef( inputDataset ) ) );
  if ( !datasetCrs.isValid() )
  {
    QgsDebugMsg( "Could not determine input dataset CRS" );
    GDALClose( inputDataset );
    return false;
  }
  QgsCoordinateTransform ct( observerPosCrs, datasetCrs );
  observerPos = ct.transform( observerPos );
  if ( datasetCrs.mapUnits() != distanceElevUnit )
  {
    observerHeight *= QGis::fromUnitToUnitFactor( distanceElevUnit, datasetCrs.mapUnits() );
    targetHeight *= QGis::fromUnitToUnitFactor( distanceElevUnit, datasetCrs.mapUnits() );
    radius *= QGis::fromUnitToUnitFactor( distanceElevUnit, datasetCrs.mapUnits() );
  }


  // Open input band
  GDALRasterBandH inputBand = GDALGetRasterBand( inputDataset, 1 );
  if ( inputBand == NULL )
  {
    GDALClose( inputDataset );
    QgsDebugMsg( "Failed to open input dataset band 1" );
    return false;
  }
  float noDataValue = GDALGetRasterNoDataValue( inputBand, NULL );


  // Compute window of raster to read
  double gtrans[6] = {};
  if ( GDALGetGeoTransform( inputDataset, &gtrans[0] ) != CE_None )
  {
    QgsDebugMsg( "Failed to query input dataset geotransform" );
    return false;
  }
  int terWidth = GDALGetRasterXSize( inputDataset );
  int terHeight = GDALGetRasterYSize( inputDataset );

  int obs[2] =
  {
    qRound( geoToPixelX( gtrans, observerPos.x(), observerPos.y() ) ),
    qRound( geoToPixelY( gtrans, observerPos.x(), observerPos.y() ) )
  };
  double earthRadius = 6370000;
  if ( datasetCrs.mapUnits() != QGis::Meters )
  {
    earthRadius *= QGis::fromUnitToUnitFactor( QGis::Meters, datasetCrs.mapUnits() );
  }

  QList<QgsPoint> cornerPoints = QList<QgsPoint>()
                                 << QgsPoint( observerPos.x() - radius, observerPos.y() - radius )
                                 << QgsPoint( observerPos.x() + radius, observerPos.y() - radius )
                                 << QgsPoint( observerPos.x() + radius, observerPos.y() + radius )
                                 << QgsPoint( observerPos.x() - radius, observerPos.y() + radius );
  int colStart = std::numeric_limits<int>::max();
  int rowStart = std::numeric_limits<int>::max();
  int colEnd = -std::numeric_limits<int>::max();
  int rowEnd = -std::numeric_limits<int>::max();
  foreach ( const QgsPoint& p, cornerPoints )
  {
    double x = geoToPixelX( gtrans, p.x(), p.y() );
    double y = geoToPixelY( gtrans, p.x(), p.y() );
    colStart = qMin( colStart, qFloor( x ) );
    colEnd = qMax( colEnd, qCeil( x ) );
    rowStart = qMin( rowStart, qFloor( y ) );
    rowEnd = qMax( rowEnd, qCeil( y ) );
  }
  colStart = qMax( 0, colStart );
  colEnd = qMin( terWidth - 1, colEnd );
  rowStart = qMax( 0, rowStart );
  rowEnd = qMin( terHeight - 1, rowEnd );
  int hmapWidth = colEnd - colStart + 1;
  int hmapHeight = rowEnd - rowStart + 1;
  int roi = .5 * qMin( hmapWidth, hmapHeight );
  QPolygon filterPoly;
  for ( int i = 0, n = filterRegion.size(); i < n; ++i )
  {
    QgsPoint p = ct.transform( filterRegion[i] );
    filterPoly.append( QPoint( geoToPixelX( gtrans, p.x(), p.y() ), geoToPixelY( gtrans, p.x(), p.y() ) ) );
  }


  // Prepare output
  GDALDriverH outputDriver = GDALGetDriverByName( outputFormat.toLocal8Bit().data() );
  if ( outputDriver == 0 )
  {
    GDALClose( inputDataset );
    QgsDebugMsg( "Failed to get driver for output" );
    return false;
  }
  if ( !CSLFetchBoolean( GDALGetMetadata( outputDriver, NULL ), GDAL_DCAP_CREATE, false ) )
  {
    GDALClose( inputDataset );
    QgsDebugMsg( "Driver for output does not support creation" );
    return false;
  }
  char **papszOptions = CSLSetNameValue( 0, "COMPRESS", "LZW" );
  GDALDatasetH outputDataset = GDALCreate( outputDriver, outputFile.toLocal8Bit().data(), hmapWidth, hmapHeight, 1, GDT_Byte, papszOptions );
  if ( outputDataset == NULL )
  {
    GDALClose( inputDataset );
    QgsDebugMsg( "Failed to open output dataset" );
    return false;
  }

  double outgtrans[6];
  std::memcpy( outgtrans, gtrans, sizeof( gtrans ) );

  // Shift for origin of window
  outgtrans[0] += colStart * outgtrans[1] + rowStart * outgtrans[2];
  outgtrans[3] += colStart * outgtrans[4] + rowStart * outgtrans[5];

  GDALSetGeoTransform( outputDataset, outgtrans );
  GDALSetProjection( outputDataset, GDALGetProjectionRef( inputDataset ) );

  GDALRasterBandH outputBand = GDALGetRasterBand( outputDataset, 1 );
  if ( outputBand == 0 )
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    QgsDebugMsg( "Failed to get output dataset band 1" );
    return false;
  }
  GDALSetRasterNoDataValue( outputBand, 255 * !displayVisible );


  // Read input heightmap
  QVector<float> heightmap( hmapWidth * hmapHeight, noDataValue );
  CPLErr err = GDALRasterIO( inputBand, GF_Read, colStart, rowStart, hmapWidth, hmapHeight, heightmap.data(), hmapWidth, hmapHeight, GDT_Float32, 0, 0 );
  if ( err != CE_None )
  {
    GDALClose( inputDataset );
    QgsDebugMsg( "Failed to fetch raster pixels" );
    return false;
  }
  // Offset observer elevation by position at point
  observerHeight += heightmap[( obs[1] - rowStart ) * hmapWidth + ( obs[0] - colStart )];


  // Compute viewshed
  if ( progress )
  {
    progress->setRange( 0, 8 * roi );
  }
  QVector<unsigned char> viewshed( hmapWidth * hmapHeight, 255 * !displayVisible );
  for ( int radiusNumber = 0; radiusNumber < 8 * roi; ++radiusNumber )
  {
    if ( progress )
    {
      if ( progress->wasCanceled() )
      {
        QgsDebugMsg( "Canceled" );
        GDALClose( inputDataset );
        GDALClose( outputDataset );
        return false;
      }
      progress->setValue( radiusNumber );
    }
    int target[2];
    if ( radiusNumber <= roi )
    {
      target[0] = obs[0] + roi;
      target[1] = obs[1] + radiusNumber;
    }
    else if ( radiusNumber <= 3 * roi )
    {
      target[0] = obs[0] + 2 * roi - radiusNumber;
      target[1] = obs[1] + roi;
    }
    else if ( radiusNumber <= 5 * roi )
    {
      target[0] = obs[0] - roi;
      target[1] = obs[1] + 4 * roi - radiusNumber;
    }
    else if ( radiusNumber <= 7 * roi )
    {
      target[0] = obs[0] + radiusNumber - 6 * roi;
      target[1] = obs[1] - roi;
    }
    else if ( radiusNumber < 8 * roi )
    {
      target[0] = obs[0] + roi;
      target[1] = obs[1] + radiusNumber - 8 * roi;
    }
    else
    {
      break; // All terrain points processed.
    }

    // Line of sight from observer to target.
    int delta[2] = {target[0] - obs[0], target[1] - obs[1]};
    int inciny = qAbs( delta[0] ) < qAbs( delta[1] );

    // Step along coord (X or Y) that varies most from observer to target.
    // That coord is inciny. Slope is how fast the other coord varies.
    double slope = ( double ) delta[1 - inciny] / ( double ) delta[inciny];
    int step = delta[inciny] > 0 ? 1 : -1;
    double horizon_slope = -99999; // Slope (in vertical plane) to horizon so far.

    // i = 0 would be the observer, which is always visible.
    for ( int i = step; true; i += step )
    {
      int p[2];
      p[inciny] = obs[inciny] + i;

      if ( i * slope > 0 )
      {
        p[1 - inciny] = obs[1 - inciny] + int( qCeil( i * slope - 0.5 ) );
      }
      else
      {
        p[1 - inciny] = obs[1 - inciny] + int( qFloor( i * slope + 0.5 ) );
      }

      if ( p[0] < colStart || p[0] > colEnd || p[1] < rowStart || p[1] > rowEnd )
        break;

      //Is the point in the outside of the viewshed area?
      double dx = qAbs( p[0] - obs[0] ), dy = qAbs( p[1] - obs[1] );
      if ( !( dx <= roi && dy <= roi && dx * dx + dy * dy <= double( roi ) * double( roi ) ) )
      {
        break;
      }
      if ( !filterPoly.isEmpty() && !filterPoly.containsPoint( QPoint( p[0], p[1] ), Qt::OddEvenFill ) )
      {
        break;
      }

      float pElev = heightmap[( p[1] - rowStart ) * hmapWidth + ( p[0] - colStart )];
      if ( pElev == noDataValue )
      {
        continue;
      }

      // Earth curvature correction
      double pGeoX = pixelToGeoX( gtrans, p[0], p[1] );
      double pGeoY = pixelToGeoY( gtrans, p[0], p[1] );
      double geoDistSqr = ( observerPos.x() - pGeoX ) * ( observerPos.x() - pGeoX ) + ( observerPos.y() - pGeoY ) * ( observerPos.y() - pGeoY );
      // http://www.swisstopo.admin.ch/internet/swisstopo/de/home/topics/survey/faq/curvature.html
      pElev -= 0.87 * geoDistSqr / ( 2 * earthRadius );

      // Update the slope if the current slope is greater than the old one
      double s = double( pElev - observerHeight ) / double( qAbs( p[inciny] - obs[inciny] ) );
      horizon_slope = qMax( horizon_slope, s );

      double horizon_alt =  observerHeight + horizon_slope * qAbs( p[inciny] - obs[inciny] );
      if ( pElev + targetHeight >= horizon_alt )
      {
        viewshed[( p[1] - rowStart ) * hmapWidth + ( p[0] - colStart )] = 255;
      }
      else
      {
        viewshed[( p[1] - rowStart ) * hmapWidth + ( p[0] - colStart )] = 0;
      }
    }
  }
  // The observer is always visible from itself
  viewshed[( obs[1] - rowStart ) * hmapWidth + ( obs[0] - colStart )] = 255;


  // Write output
  err = GDALRasterIO( outputBand, GF_Write, 0, 0, hmapWidth, hmapHeight, viewshed.data(), hmapWidth, hmapHeight, GDT_Byte, 0, 0 );
  GDALClose( inputDataset );
  GDALClose( outputDataset );
  if ( err != CE_None )
  {
    QgsDebugMsg( "Failed to write to output dataset" );
    return false;
  }
  return true;
}
