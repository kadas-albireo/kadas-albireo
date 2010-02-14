/***************************************************************************
     qgsimagewarper.cpp
     --------------------------------------
    Date                 : Sun Sep 16 12:03:14 AKDT 2007
    Copyright            : (C) 2007 by Gary E. Sherman
    Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>
#include <iostream>

#include <cpl_conv.h>
#include <cpl_string.h>
#include <gdal.h>
#include <gdalwarper.h>

#include <QFile>
#include <QProgressDialog>

#include "qgsimagewarper.h"
#include "qgsgeoreftransform.h"

QgsImageWarper::QgsImageWarper(QWidget *theParent) : mParent(theParent)
{
}

bool QgsImageWarper::openSrcDSAndGetWarpOpt( const QString &input, const QString &output,
    const ResamplingMethod &resampling, const GDALTransformerFunc &pfnTransform,
    GDALDatasetH &hSrcDS, GDALWarpOptions *&psWarpOptions )
{
  // Open input file
  GDALAllRegister();
  hSrcDS = GDALOpen( QFile::encodeName( input ).constData(), GA_ReadOnly );
  if ( hSrcDS == NULL ) return false;

  // Setup warp options.
  psWarpOptions = GDALCreateWarpOptions();
  psWarpOptions->hSrcDS = hSrcDS;
  psWarpOptions->nBandCount = GDALGetRasterCount( hSrcDS );
  psWarpOptions->panSrcBands =
    ( int * ) CPLMalloc( sizeof( int ) * psWarpOptions->nBandCount );
  psWarpOptions->panDstBands =
    ( int * ) CPLMalloc( sizeof( int ) * psWarpOptions->nBandCount );
  for ( int i = 0; i < psWarpOptions->nBandCount; ++i )
  {
    psWarpOptions->panSrcBands[i] = i + 1;
    psWarpOptions->panDstBands[i] = i + 1;
  }
  psWarpOptions->pfnProgress = GDALTermProgress;
  psWarpOptions->pfnTransformer = pfnTransform;
  psWarpOptions->eResampleAlg = GDALResampleAlg( resampling );

  return true;
}

bool QgsImageWarper::createDestinationDataset(const QString &outputName, GDALDatasetH hSrcDS, GDALDatasetH &hDstDS, uint resX, uint resY, double *adfGeoTransform, bool useZeroAsTrans, const QString& compression)
{
  // create the output file
  GDALDriverH driver = GDALGetDriverByName( "GTiff" );
  if (driver == NULL)
  {
    return false;
  }
  char **papszOptions = NULL;
  papszOptions = CSLSetNameValue( papszOptions, "INIT_DEST", "NO_DATA" );
  papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", compression.toAscii() );
  hDstDS = GDALCreate( driver,
                       QFile::encodeName( outputName ).constData(), resX, resY,
                       GDALGetRasterCount( hSrcDS ),
                       GDALGetRasterDataType( GDALGetRasterBand( hSrcDS, 1 ) ),
                       papszOptions );
  if (hDstDS == NULL)
  {
    return false;
  }

  if (CE_None != GDALSetGeoTransform(hDstDS, adfGeoTransform))
  {
    return false;
  }

  for ( int i = 0; i < GDALGetRasterCount( hSrcDS ); ++i )
  {
    GDALRasterBandH hSrcBand = GDALGetRasterBand( hSrcDS, i + 1 );
    GDALRasterBandH hDstBand = GDALGetRasterBand( hDstDS, i + 1 );
    GDALColorTableH cTable = GDALGetRasterColorTable( hSrcBand );
    GDALSetRasterColorInterpretation( hDstBand, GDALGetRasterColorInterpretation( hSrcBand ) );
    if ( cTable )
    {
      GDALSetRasterColorTable( hDstBand, cTable );
    }

    int success;
    double noData = GDALGetRasterNoDataValue( hSrcBand, &success );
    if (success)
    {
      GDALSetRasterNoDataValue( hDstBand, noData );
    }
    else if (useZeroAsTrans)
    {
      GDALSetRasterNoDataValue( hDstBand, 0 );
    }
  }
  return true;
}

bool QgsImageWarper::warpFile( const QString& input, const QString& output, const QgsGeorefTransform &georefTransform,
                               ResamplingMethod resampling, bool useZeroAsTrans, const QString& compression)
{
  if (!georefTransform.parametersInitialized())
    return false;

  CPLErr eErr;
  GDALDatasetH hSrcDS, hDstDS;
  GDALWarpOptions *psWarpOptions;
  if (!openSrcDSAndGetWarpOpt(input, output, resampling, georefTransform.GDALTransformer(), hSrcDS, psWarpOptions))
  {
    // TODO: be verbose about failures
    return false;
  }

  double adfGeoTransform[6];
  int destPixels, destLines;
  eErr = GDALSuggestedWarpOutput(hSrcDS, georefTransform.GDALTransformer(), georefTransform.GDALTransformerArgs(), 
                                 adfGeoTransform, &destPixels, &destLines);
  if (eErr != CE_None)
  {
    GDALClose( hSrcDS );
    GDALDestroyWarpOptions( psWarpOptions );
    return false;
  }

  if (!createDestinationDataset(output, hSrcDS, hDstDS, destPixels, destLines, adfGeoTransform, useZeroAsTrans, compression))
  {
    GDALClose( hSrcDS );
    GDALDestroyWarpOptions( psWarpOptions );
    return false;
  }

  // Create a QT progress dialog
  QProgressDialog *progressDialog = new QProgressDialog(mParent);
  progressDialog->setRange(0, 100);
  progressDialog->setAutoClose(true);
  progressDialog->setModal(true);
  progressDialog->setMinimumDuration(0);

  // Set GDAL callbacks for the progress dialog
  psWarpOptions->pProgressArg = createWarpProgressArg(progressDialog);
  psWarpOptions->pfnProgress  = updateWarpProgress;

  psWarpOptions->hSrcDS = hSrcDS;
  psWarpOptions->hDstDS = hDstDS;

  // Create a transformer which transforms from source to destination pixels (and vice versa)
  psWarpOptions->pfnTransformer  = GeoToPixelTransform;
  psWarpOptions->pTransformerArg = addGeoToPixelTransform(georefTransform.GDALTransformer(),
                                                          georefTransform.GDALTransformerArgs(),
                                                          adfGeoTransform);

  // Initialize and execute the warp operation.
  GDALWarpOperation oOperation;
  oOperation.Initialize( psWarpOptions );

  progressDialog->show();
  progressDialog->raise();
  progressDialog->activateWindow();

  eErr = oOperation.ChunkAndWarpImage(0, 0, destPixels, destLines);

  destroyGeoToPixelTransform(psWarpOptions->pTransformerArg);
  GDALDestroyWarpOptions( psWarpOptions );
  delete progressDialog;

  GDALClose( hSrcDS );
  GDALClose( hDstDS );
  if (eErr != CE_None)
  {
    return false;
  }
  return true;
}


void *QgsImageWarper::addGeoToPixelTransform(GDALTransformerFunc GDALTransformer, void *GDALTransformerArg, double *padfGeotransform) const
{
  TransformChain *chain = new TransformChain;
  chain->GDALTransformer = GDALTransformer;
  chain->GDALTransformerArg = GDALTransformerArg;
  memcpy(chain->adfGeotransform, padfGeotransform, sizeof(double)*6);
  // TODO: In reality we don't require the full homogeneous matrix, so GeoToPixelTransform and matrix inversion could
  // be optimized for simple scale+offset if there's the need (e.g. for numerical or performance reasons).
  if (!GDALInvGeoTransform(chain->adfGeotransform, chain->adfInvGeotransform))
  {
    // Error handling if inversion fails - although the inverse transform is not needed for warp operation
    return NULL;
  }
  return (void*)chain;
}

void QgsImageWarper::destroyGeoToPixelTransform(void *GeoToPixelTransfomArg) const
{
  delete static_cast<TransformChain *>(GeoToPixelTransfomArg);
}

int QgsImageWarper::GeoToPixelTransform( void *pTransformerArg, int bDstToSrc, int nPointCount,
                                         double *x, double *y, double *z, int *panSuccess   )
{
  TransformChain *chain = static_cast<TransformChain*>(pTransformerArg);
  if (chain == NULL)
  {
    return FALSE;
  }

  if ( bDstToSrc == FALSE )
  {
    // Transform to georeferenced coordinates
    if (!chain->GDALTransformer(chain->GDALTransformerArg, bDstToSrc, nPointCount, x, y, z, panSuccess))
    {
      return FALSE;
    }
    // Transform from georeferenced to pixel/line
    for (int i = 0; i < nPointCount; ++i)
    {
      if (!panSuccess[i]) continue;
      double xP = x[i];
      double yP = y[i];
      x[i] = chain->adfInvGeotransform[0] + xP*chain->adfInvGeotransform[1] + yP*chain->adfInvGeotransform[2];
      y[i] = chain->adfInvGeotransform[3] + xP*chain->adfInvGeotransform[4] + yP*chain->adfInvGeotransform[5];
    }
  }
  else
  {
    // Transform from pixel/line to georeferenced coordinates
    for (int i = 0; i < nPointCount; ++i)
    {
      double P = x[i];
      double L = y[i];
      x[i] = chain->adfGeotransform[0] + P*chain->adfGeotransform[1] + L*chain->adfGeotransform[2];
      y[i] = chain->adfGeotransform[3] + P*chain->adfGeotransform[4] + L*chain->adfGeotransform[5];
    }
    // Transform from georeferenced coordinates to source pixel/line
    if (!chain->GDALTransformer(chain->GDALTransformerArg, bDstToSrc, nPointCount, x, y, z, panSuccess))
    {
      return FALSE;
    }
  }
  return TRUE;
}

void *QgsImageWarper::createWarpProgressArg(QProgressDialog *progressDialog) const
{
  return (void *)progressDialog;
}

int CPL_STDCALL QgsImageWarper::updateWarpProgress(double dfComplete, const char *pszMessage, void *pProgressArg)
{
  QProgressDialog *progress = static_cast<QProgressDialog*>(pProgressArg);
  progress->setValue(std::min(100u, (uint)(dfComplete*100.0)));
  // TODO: call QEventLoop manually to make "cancel" button more responsive
  if (progress->wasCanceled())
  {
    //TODO: delete resulting file?
    return FALSE;
  }
  return TRUE;
}