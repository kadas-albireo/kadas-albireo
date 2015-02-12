/***************************************************************************
  qgsmaprenderersequentialjob.cpp
  --------------------------------------
  Date                 : December 2013
  Copyright            : (C) 2013 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaprenderersequentialjob.h"

#include "qgslogger.h"
#include "qgsmaprenderercustompainterjob.h"
#include "qgspallabeling.h"


QgsMapRendererSequentialJob::QgsMapRendererSequentialJob( const QgsMapSettings& settings )
    : QgsMapRendererQImageJob( settings )
    , mInternalJob( 0 )
    , mPainter( 0 )
    , mLabelingResults( 0 )
{
  QgsDebugMsg( "SEQUENTIAL construct" );

  mImage = QImage( mSettings.outputSize(), mSettings.outputImageFormat() );
  mImage.setDotsPerMeterX( 1000 * settings.outputDpi() / 25.4 );
  mImage.setDotsPerMeterY( 1000 * settings.outputDpi() / 25.4 );
}

QgsMapRendererSequentialJob::~QgsMapRendererSequentialJob()
{
  QgsDebugMsg( "SEQUENTIAL destruct" );
  if ( isActive() )
  {
    // still running!
    QgsDebugMsg( "SEQUENTIAL destruct -- still running! (cancelling)" );
    cancel();
  }

  Q_ASSERT( mInternalJob == 0 && mPainter == 0 );

  delete mLabelingResults;
  mLabelingResults = 0;
}


void QgsMapRendererSequentialJob::start()
{
  if ( isActive() )
    return; // do nothing if we are already running

  mRenderingStart.start();

  mErrors.clear();

  QgsDebugMsg( "SEQUENTIAL START" );

  Q_ASSERT( mInternalJob == 0  && mPainter == 0 );

  mPainter = new QPainter( &mImage );

  mInternalJob = new QgsMapRendererCustomPainterJob( mSettings, mPainter );
  mInternalJob->setCache( mCache );

  connect( mInternalJob, SIGNAL( finished() ), SLOT( internalFinished() ) );

  mInternalJob->start();
}


void QgsMapRendererSequentialJob::cancel()
{
  if ( !isActive() )
    return;

  QgsDebugMsg( "sequential - cancel internal" );
  mInternalJob->cancel();

  Q_ASSERT( mInternalJob == 0 && mPainter == 0 );
}

void QgsMapRendererSequentialJob::waitForFinished()
{
  if ( !isActive() )
    return;

  mInternalJob->waitForFinished();
}

bool QgsMapRendererSequentialJob::isActive() const
{
  return mInternalJob != 0;
}

QgsLabelingResults* QgsMapRendererSequentialJob::takeLabelingResults()
{
  QgsLabelingResults* tmp = mLabelingResults;
  mLabelingResults = 0;
  return tmp;
}


QImage QgsMapRendererSequentialJob::renderedImage()
{
  if ( isActive() && mCache )
    // this will allow immediate display of cached layers and at the same time updates of the layer being rendered
    return composeImage( mSettings, mInternalJob->jobs() );
  else
    return mImage;
}


void QgsMapRendererSequentialJob::internalFinished()
{
  QgsDebugMsg( "SEQUENTIAL finished" );

  mPainter->end();
  delete mPainter;
  mPainter = 0;

  mLabelingResults = mInternalJob->takeLabelingResults();

  mErrors = mInternalJob->errors();

  // now we are in a slot called from mInternalJob - do not delete it immediately
  // so the class is still valid when the execution returns to the class
  mInternalJob->deleteLater();
  mInternalJob = 0;

  mRenderingTime = mRenderingStart.elapsed();

  emit finished();
}

