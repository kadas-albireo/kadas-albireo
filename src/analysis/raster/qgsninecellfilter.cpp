/***************************************************************************
                          qgsninecellfilter.h  -  description
                             -------------------
    begin                : August 6th, 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsninecellfilter.h"
#include "qgscoordinatetransform.h"
#include "cpl_string.h"
#include <qmath.h>
#include <QProgressDialog>
#include <QFile>

#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_NUM >= 1800
#define TO8F(x) (x).toUtf8().constData()
#else
#define TO8F(x) QFile::encodeName( x ).constData()
#endif

QgsNineCellFilter::QgsNineCellFilter( const QString& inputFile, const QString& outputFile, const QString& outputFormat, const QgsRectangle &region, const QgsCoordinateReferenceSystem &regionCrs )
    : mInputFile( inputFile )
    , mOutputFile( outputFile )
    , mOutputFormat( outputFormat )
    , mFilterRegion( region )
    , mFilterRegionCrs( regionCrs )
    , mCellSizeX( -1.0 )
    , mCellSizeY( -1.0 )
    , mInputNodataValue( -1.0 )
    , mOutputNodataValue( -1.0 )
    , mZFactor( 1.0 )
{

}

QgsNineCellFilter::QgsNineCellFilter()
    : mCellSizeX( -1.0 )
    , mCellSizeY( -1.0 )
    , mInputNodataValue( -1.0 )
    , mOutputNodataValue( -1.0 )
    , mZFactor( 1.0 )
{
}

QgsNineCellFilter::~QgsNineCellFilter()
{

}

int QgsNineCellFilter::processRaster( QProgressDialog* p )
{
  GDALAllRegister();

  //open input file
  int xSize, ySize;
  GDALDatasetH  inputDataset = openInputFile( xSize, ySize );
  if ( inputDataset == NULL )
  {
    return 1; //opening of input file failed
  }
  double gtrans[6] = {};
  if ( GDALGetGeoTransform( inputDataset, &gtrans[0] ) != CE_None )
  {
    GDALClose( inputDataset );
    return 1;
  }

  //output driver
  GDALDriverH outputDriver = openOutputDriver();
  if ( outputDriver == 0 )
  {
    GDALClose( inputDataset );
    return 2;
  }


  //determine the window
  int rowStart, rowEnd, colStart, colEnd;
  if ( !computeWindow( inputDataset, mFilterRegion, mFilterRegionCrs, rowStart, rowEnd, colStart, colEnd ) )
  {
    GDALClose( inputDataset );
    return 2;
  }
  xSize = colEnd - colStart;
  ySize = rowEnd - rowStart;

  GDALDatasetH outputDataset = openOutputFile( inputDataset, outputDriver, colStart, rowStart, xSize, ySize );
  if ( outputDataset == NULL )
  {
    GDALClose( inputDataset );
    return 3; //create operation on output file failed
  }

  //open first raster band for reading (operation is only for single band raster)
  GDALRasterBandH rasterBand = GDALGetRasterBand( inputDataset, 1 );
  if ( rasterBand == NULL )
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    return 4;
  }
  mInputNodataValue = GDALGetRasterNoDataValue( rasterBand, NULL );

  GDALRasterBandH outputRasterBand = GDALGetRasterBand( outputDataset, 1 );
  if ( outputRasterBand == NULL )
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    return 5;
  }
  //try to set -9999 as nodata value
  GDALSetRasterNoDataValue( outputRasterBand, -9999 );
  mOutputNodataValue = GDALGetRasterNoDataValue( outputRasterBand, NULL );

  if ( ySize < 3 ) //we require at least three rows (should be true for most datasets)
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    return 6;
  }

  //keep only three scanlines in memory at a time
  float* scanLine1 = ( float * ) CPLMalloc( sizeof( float ) * xSize );
  float* scanLine2 = ( float * ) CPLMalloc( sizeof( float ) * xSize );
  float* scanLine3 = ( float * ) CPLMalloc( sizeof( float ) * xSize );

  float* resultLine = ( float * ) CPLMalloc( sizeof( float ) * xSize );

  if ( p )
  {
    p->setMaximum( ySize );
  }

  //values outside the layer extent (if the 3x3 window is on the border) are sent to the processing method as (input) nodata values
  for ( int i = 0; i < ySize; ++i )
  {
    if ( p )
    {
      p->setValue( i );
    }

    if ( p && p->wasCanceled() )
    {
      break;
    }

    if ( i == 0 )
    {
      //fill scanline 1 with (input) nodata for the values above the first row and feed scanline2 with the first row
      for ( int a = 0; a < xSize; ++a )
      {
        scanLine1[a] = mInputNodataValue;
      }
      GDALRasterIO( rasterBand, GF_Read, colStart, rowStart, xSize, 1, scanLine2, xSize, 1, GDT_Float32, 0, 0 );
    }
    else
    {
      //normally fetch only scanLine3 and release scanline 1 if we move forward one row
      CPLFree( scanLine1 );
      scanLine1 = scanLine2;
      scanLine2 = scanLine3;
      scanLine3 = ( float * ) CPLMalloc( sizeof( float ) * xSize );
    }

    if ( i == ySize - 1 ) //fill the row below the bottom with nodata values
    {
      for ( int a = 0; a < xSize; ++a )
      {
        scanLine3[a] = mInputNodataValue;
      }
    }
    else
    {
      GDALRasterIO( rasterBand, GF_Read, colStart, rowStart + i + 1, xSize, 1, scanLine3, xSize, 1, GDT_Float32, 0, 0 );
    }

#pragma omp parallel for schedule(static)
    for ( int j = 0; j < xSize; ++j )
    {
      if ( j == 0 )
      {
        resultLine[j] = processNineCellWindow( &mInputNodataValue, &scanLine1[j], &scanLine1[j+1], &mInputNodataValue, &scanLine2[j],
                                               &scanLine2[j+1], &mInputNodataValue, &scanLine3[j], &scanLine3[j+1] );
      }
      else if ( j == xSize - 1 )
      {
        resultLine[j] = processNineCellWindow( &scanLine1[j-1], &scanLine1[j], &mInputNodataValue, &scanLine2[j-1], &scanLine2[j],
                                               &mInputNodataValue, &scanLine3[j-1], &scanLine3[j], &mInputNodataValue );
      }
      else
      {
        resultLine[j] = processNineCellWindow( &scanLine1[j-1], &scanLine1[j], &scanLine1[j+1], &scanLine2[j-1], &scanLine2[j],
                                               &scanLine2[j+1], &scanLine3[j-1], &scanLine3[j], &scanLine3[j+1] );
      }
    }

    GDALRasterIO( outputRasterBand, GF_Write, 0, i, xSize, 1, resultLine, xSize, 1, GDT_Float32, 0, 0 );
  }

  if ( p )
  {
    p->setValue( ySize );
  }

  CPLFree( resultLine );
  CPLFree( scanLine1 );
  CPLFree( scanLine2 );
  CPLFree( scanLine3 );

  GDALClose( inputDataset );

  if ( p && p->wasCanceled() )
  {
    //delete the dataset without closing (because it is faster)
    GDALDeleteDataset( outputDriver, TO8F( mOutputFile ) );
    return 7;
  }
  GDALClose( outputDataset );

  return 0;
}

GDALDatasetH QgsNineCellFilter::openInputFile( int& nCellsX, int& nCellsY )
{
  GDALDatasetH inputDataset = GDALOpen( TO8F( mInputFile ), GA_ReadOnly );
  if ( inputDataset != NULL )
  {
    nCellsX = GDALGetRasterXSize( inputDataset );
    nCellsY = GDALGetRasterYSize( inputDataset );

    //we need at least one band
    if ( GDALGetRasterCount( inputDataset ) < 1 )
    {
      GDALClose( inputDataset );
      return NULL;
    }
  }
  return inputDataset;
}

bool QgsNineCellFilter::computeWindow( GDALDatasetH dataset, const QgsRectangle &region, const QgsCoordinateReferenceSystem& regionCrs, int &rowStart, int &rowEnd, int &colStart, int &colEnd )
{
  int nCellsX = GDALGetRasterXSize( dataset );
  int nCellsY = GDALGetRasterYSize( dataset );

  if ( region.isEmpty() )
  {
    rowStart = 0;
    rowEnd = nCellsY;
    colStart = 0;
    colEnd = nCellsX;
    return true;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( dataset, &gtrans[0] ) != CE_None )
  {
    return false;
  }

  QgsCoordinateReferenceSystem rasterCrs( QString( GDALGetProjectionRef( dataset ) ) );
  if ( !rasterCrs.isValid() )
  {
    return false;
  }
  QgsCoordinateTransform ct( regionCrs, rasterCrs );

  // Transform raster geo position to pixel coordinates
  QgsPoint regionPoints[4] =
  {
    QgsPoint( region.xMinimum(), region.yMinimum() ),
    QgsPoint( region.xMaximum(), region.yMinimum() ),
    QgsPoint( region.xMaximum(), region.yMaximum() ),
    QgsPoint( region.xMinimum(), region.yMaximum() )
  };
  QgsPoint pRaster = ct.transform( regionPoints[0] );
  double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
  double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
  colStart = qFloor( col );
  colEnd = qCeil( col );
  rowStart = qFloor( row );
  rowEnd = qCeil( row );

  for ( int i = 1; i < 4; ++i )
  {
    pRaster = ct.transform( QgsPoint( regionPoints[i] ) );
    col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
    row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
    colStart  = qMin( colStart , qFloor( col ) );
    colEnd  = qMax( colEnd , qCeil( col ) );
    rowStart = qMin( rowStart, qFloor( row ) );
    rowEnd = qMax( rowEnd, qCeil( row ) );
  }

  colStart = qMax( colStart, 0 );
  colEnd = qMin( colEnd + 1, nCellsX );
  rowStart = qMax( rowStart, 0 );
  rowEnd = qMin( rowEnd + 1, nCellsY );

  return true;
}

GDALDriverH QgsNineCellFilter::openOutputDriver()
{
  char **driverMetadata;

  //open driver
  GDALDriverH outputDriver = GDALGetDriverByName( mOutputFormat.toLocal8Bit().data() );

  if ( outputDriver == NULL )
  {
    return outputDriver; //return NULL, driver does not exist
  }

  driverMetadata = GDALGetMetadata( outputDriver, NULL );
  if ( !CSLFetchBoolean( driverMetadata, GDAL_DCAP_CREATE, false ) )
  {
    return NULL; //driver exist, but it does not support the create operation
  }

  return outputDriver;
}

GDALDatasetH QgsNineCellFilter::openOutputFile( GDALDatasetH inputDataset, GDALDriverH outputDriver, int colStart, int rowStart, int xSize, int ySize )
{
  if ( inputDataset == NULL )
  {
    return NULL;
  }

  //open output file
  char **papszOptions = NULL;
  papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "LZW" );
  GDALDatasetH outputDataset = GDALCreate( outputDriver, TO8F( mOutputFile ), xSize, ySize, 1, GDT_Float32, papszOptions );
  if ( outputDataset == NULL )
  {
    return outputDataset;
  }


  //get geotransform from inputDataset
  double geotransform[6];
  if ( GDALGetGeoTransform( inputDataset, geotransform ) != CE_None )
  {
    GDALClose( outputDataset );
    return NULL;
  }

  // Shift for origin of window
  geotransform[0] += colStart * geotransform[1] + rowStart * geotransform[2];
  geotransform[3] += colStart * geotransform[4] + rowStart * geotransform[5];

  GDALSetGeoTransform( outputDataset, geotransform );

  //make sure mCellSizeX and mCellSizeY are always > 0
  mCellSizeX = geotransform[1];
  if ( mCellSizeX < 0 )
  {
    mCellSizeX = -mCellSizeX;
  }
  mCellSizeY = geotransform[5];
  if ( mCellSizeY < 0 )
  {
    mCellSizeY = -mCellSizeY;
  }

  const char* projection = GDALGetProjectionRef( inputDataset );
  GDALSetProjection( outputDataset, projection );

  return outputDataset;
}

