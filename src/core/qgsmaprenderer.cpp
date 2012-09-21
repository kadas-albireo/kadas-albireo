/***************************************************************************
    qgsmaprender.cpp  -  class for rendering map layer set
    ----------------------
    begin                : January 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>
#include <cfloat>

#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"
#include "qgsmaprenderer.h"
#include "qgsscalecalculator.h"
#include "qgsmaptopixel.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsdistancearea.h"
#include "qgscentralpointpositionmanager.h"
#include "qgsoverlayobjectpositionmanager.h"
#include "qgspalobjectpositionmanager.h"
#include "qgsvectorlayer.h"
#include "qgsvectoroverlay.h"


#include <QDomDocument>
#include <QDomNode>
#include <QMutexLocker>
#include <QPainter>
#include <QListIterator>
#include <QSettings>
#include <QTime>
#include <QCoreApplication>

QgsMapRenderer::QgsMapRenderer()
{
  mScaleCalculator = new QgsScaleCalculator;
  mDistArea = new QgsDistanceArea;
  mCachedTrForLayer = 0;
  mCachedTr = 0;

  mDrawing = false;
  mOverview = false;

  // set default map units - we use WGS 84 thus use degrees
  setMapUnits( QGis::Degrees );

  mSize = QSize( 0, 0 );

  mProjectionsEnabled = false;
  mDestCRS = new QgsCoordinateReferenceSystem( GEOCRS_ID, QgsCoordinateReferenceSystem::InternalCrsId ); //WGS 84

  mOutputUnits = QgsMapRenderer::Millimeters;

  mLabelingEngine = NULL;
}

QgsMapRenderer::~QgsMapRenderer()
{
  delete mScaleCalculator;
  delete mDistArea;
  delete mDestCRS;
  delete mLabelingEngine;
  delete mCachedTr;
}

QgsRectangle QgsMapRenderer::extent() const
{
  return mExtent;
}

void QgsMapRenderer::updateScale()
{
  mScale = mScaleCalculator->calculate( mExtent, mSize.width() );
}

bool QgsMapRenderer::setExtent( const QgsRectangle& extent )
{
  //remember the previous extent
  mLastExtent = mExtent;

  // Don't allow zooms where the current extent is so small that it
  // can't be accurately represented using a double (which is what
  // currentExtent uses). Excluding 0 avoids a divide by zero and an
  // infinite loop when rendering to a new canvas. Excluding extents
  // greater than 1 avoids doing unnecessary calculations.

  // The scheme is to compare the width against the mean x coordinate
  // (and height against mean y coordinate) and only allow zooms where
  // the ratio indicates that there is more than about 12 significant
  // figures (there are about 16 significant figures in a double).

  if ( extent.width()  > 0 &&
       extent.height() > 0 &&
       extent.width()  < 1 &&
       extent.height() < 1 )
  {
    // Use abs() on the extent to avoid the case where the extent is
    // symmetrical about 0.
    double xMean = ( qAbs( extent.xMinimum() ) + qAbs( extent.xMaximum() ) ) * 0.5;
    double yMean = ( qAbs( extent.yMinimum() ) + qAbs( extent.yMaximum() ) ) * 0.5;

    double xRange = extent.width() / xMean;
    double yRange = extent.height() / yMean;

    static const double minProportion = 1e-12;
    if ( xRange < minProportion || yRange < minProportion )
      return false;
  }

  mExtent = extent;
  if ( !extent.isEmpty() )
    adjustExtentToSize();
  return true;
}



void QgsMapRenderer::setOutputSize( QSize size, int dpi )
{
  mSize = QSizeF( size.width(), size.height() );
  mScaleCalculator->setDpi( dpi );
  adjustExtentToSize();
}

void QgsMapRenderer::setOutputSize( QSizeF size, double dpi )
{
  mSize = size;
  mScaleCalculator->setDpi( dpi );
  adjustExtentToSize();
}

double QgsMapRenderer::outputDpi()
{
  return mScaleCalculator->dpi();
}

QSize QgsMapRenderer::outputSize()
{
  return mSize.toSize();
}

QSizeF QgsMapRenderer::outputSizeF()
{
  return mSize;
}

void QgsMapRenderer::adjustExtentToSize()
{
  double myHeight = mSize.height();
  double myWidth = mSize.width();

  QgsMapToPixel newCoordXForm;

  if ( !myWidth || !myHeight )
  {
    mScale = 1;
    newCoordXForm.setParameters( 0, 0, 0, 0 );
    return;
  }

  // calculate the translation and scaling parameters
  // mapUnitsPerPixel = map units per pixel
  double mapUnitsPerPixelY = mExtent.height() / myHeight;
  double mapUnitsPerPixelX = mExtent.width() / myWidth;
  mMapUnitsPerPixel = mapUnitsPerPixelY > mapUnitsPerPixelX ? mapUnitsPerPixelY : mapUnitsPerPixelX;

  // calculate the actual extent of the mapCanvas
  double dxmin, dxmax, dymin, dymax, whitespace;

  if ( mapUnitsPerPixelY > mapUnitsPerPixelX )
  {
    dymin = mExtent.yMinimum();
    dymax = mExtent.yMaximum();
    whitespace = (( myWidth * mMapUnitsPerPixel ) - mExtent.width() ) * 0.5;
    dxmin = mExtent.xMinimum() - whitespace;
    dxmax = mExtent.xMaximum() + whitespace;
  }
  else
  {
    dxmin = mExtent.xMinimum();
    dxmax = mExtent.xMaximum();
    whitespace = (( myHeight * mMapUnitsPerPixel ) - mExtent.height() ) * 0.5;
    dymin = mExtent.yMinimum() - whitespace;
    dymax = mExtent.yMaximum() + whitespace;
  }

  QgsDebugMsg( QString( "Map units per pixel (x,y) : %1, %2" ).arg( mapUnitsPerPixelX, 0, 'f', 8 ).arg( mapUnitsPerPixelY, 0, 'f', 8 ) );
  QgsDebugMsg( QString( "Pixmap dimensions (x,y) : %1, %2" ).arg( myWidth, 0, 'f', 8 ).arg( myHeight, 0, 'f', 8 ) );
  QgsDebugMsg( QString( "Extent dimensions (x,y) : %1, %2" ).arg( mExtent.width(), 0, 'f', 8 ).arg( mExtent.height(), 0, 'f', 8 ) );
  QgsDebugMsg( mExtent.toString() );

  // update extent
  mExtent.setXMinimum( dxmin );
  mExtent.setXMaximum( dxmax );
  mExtent.setYMinimum( dymin );
  mExtent.setYMaximum( dymax );

  // update the scale
  updateScale();

  QgsDebugMsg( QString( "Scale (assuming meters as map units) = 1:%1" ).arg( mScale, 0, 'f', 8 ) );

  newCoordXForm.setParameters( mMapUnitsPerPixel, dxmin, dymin, myHeight );
  mRenderContext.setMapToPixel( newCoordXForm );
  mRenderContext.setExtent( mExtent );
}


void QgsMapRenderer::render( QPainter* painter, double* forceWidthScale )
{
  //Lock render method for concurrent threads (e.g. from globe)
  QMutexLocker renderLock( &mRenderMutex );

  //flag to see if the render context has changed
  //since the last time we rendered. If it hasnt changed we can
  //take some shortcuts with rendering
  bool mySameAsLastFlag = true;

  QgsDebugMsg( "========== Rendering ==========" );

  if ( mExtent.isEmpty() )
  {
    QgsDebugMsg( "empty extent... not rendering" );
    return;
  }

  if ( mSize.width() == 1 && mSize.height() == 1 )
  {
    QgsDebugMsg( "size 1x1... not rendering" );
    return;
  }

  QPaintDevice* thePaintDevice = painter->device();
  if ( !thePaintDevice )
  {
    return;
  }

  // wait
  if ( mDrawing )
  {
    QgsDebugMsg( "already rendering" );
    QCoreApplication::processEvents();
  }

  if ( mDrawing )
  {
    QgsDebugMsg( "still rendering - skipping" );
    return;
  }

  mDrawing = true;

  QgsCoordinateTransform* ct;

#ifdef QGISDEBUG
  QgsDebugMsg( "Starting to render layer stack." );
  QTime renderTime;
  renderTime.start();
#endif

  if ( mOverview )
    mRenderContext.setDrawEditingInformation( !mOverview );

  mRenderContext.setPainter( painter );
  mRenderContext.setCoordinateTransform( 0 );
  //this flag is only for stopping during the current rendering progress,
  //so must be false at every new render operation
  mRenderContext.setRenderingStopped( false );

  //calculate scale factor
  //use the specified dpi and not those from the paint device
  //because sometimes QPainter units are in a local coord sys (e.g. in case of QGraphicsScene)
  double sceneDpi = mScaleCalculator->dpi();
  double scaleFactor = 1.0;
  if ( mOutputUnits == QgsMapRenderer::Millimeters )
  {
    if ( forceWidthScale )
    {
      scaleFactor = *forceWidthScale;
    }
    else
    {
      scaleFactor = sceneDpi / 25.4;
    }
  }
  double rasterScaleFactor = ( thePaintDevice->logicalDpiX() + thePaintDevice->logicalDpiY() ) / 2.0 / sceneDpi;
  if ( mRenderContext.rasterScaleFactor() != rasterScaleFactor )
  {
    mRenderContext.setRasterScaleFactor( rasterScaleFactor );
    mySameAsLastFlag = false;
  }
  if ( mRenderContext.scaleFactor() != scaleFactor )
  {
    mRenderContext.setScaleFactor( scaleFactor );
    mySameAsLastFlag = false;
  }
  if ( mRenderContext.rendererScale() != mScale )
  {
    //add map scale to render context
    mRenderContext.setRendererScale( mScale );
    mySameAsLastFlag = false;
  }
  if ( mLastExtent != mExtent )
  {
    mLastExtent = mExtent;
    mySameAsLastFlag = false;
  }

  mRenderContext.setLabelingEngine( mLabelingEngine );
  if ( mLabelingEngine )
    mLabelingEngine->init( this );

  // know we know if this render is just a repeat of the last time, we
  // can clear caches if it has changed
  if ( !mySameAsLastFlag )
  {
    //clear the cache pixmap if we changed resolution / extent
    QSettings mySettings;
    if ( mySettings.value( "/qgis/enable_render_caching", false ).toBool() )
    {
      QgsMapLayerRegistry::instance()->clearAllLayerCaches();
    }
  }

  QgsOverlayObjectPositionManager* overlayManager = overlayManagerFromSettings();
  QList<QgsVectorOverlay*> allOverlayList; //list of all overlays, used to draw them after layers have been rendered

  // render all layers in the stack, starting at the base
  QListIterator<QString> li( mLayerSet );
  li.toBack();

  QgsRectangle r1, r2;

  while ( li.hasPrevious() )
  {
    if ( mRenderContext.renderingStopped() )
    {
      break;
    }

    // Store the painter in case we need to swap it out for the
    // cache painter
    QPainter * mypContextPainter = mRenderContext.painter();

    QString layerId = li.previous();

    QgsDebugMsg( "Rendering at layer item " + layerId );

    // This call is supposed to cause the progress bar to
    // advance. However, it seems that updating the progress bar is
    // incompatible with having a QPainter active (the one that is
    // passed into this function), as Qt produces a number of errors
    // when try to do so. I'm (Gavin) not sure how to fix this, but
    // added these comments and debug statement to help others...
    QgsDebugMsg( "If there is a QPaintEngine error here, it is caused by an emit call" );

    //emit drawingProgress(myRenderCounter++, mLayerSet.size());
    QgsMapLayer *ml = QgsMapLayerRegistry::instance()->mapLayer( layerId );

    if ( !ml )
    {
      QgsDebugMsg( "Layer not found in registry!" );
      continue;
    }

    QgsDebugMsg( QString( "layer %1:  minscale:%2  maxscale:%3  scaledepvis:%4  extent:%5" )
                 .arg( ml->name() )
                 .arg( ml->minimumScale() )
                 .arg( ml->maximumScale() )
                 .arg( ml->hasScaleBasedVisibility() )
                 .arg( ml->extent().toString() )
               );

    if ( !ml->hasScaleBasedVisibility() || ( ml->minimumScale() <= mScale && mScale < ml->maximumScale() ) || mOverview )
    {
      connect( ml, SIGNAL( drawingProgress( int, int ) ), this, SLOT( onDrawingProgress( int, int ) ) );

      //
      // Now do the call to the layer that actually does
      // the rendering work!
      //

      bool split = false;

      if ( hasCrsTransformEnabled() )
      {
        r1 = mExtent;
        split = splitLayersExtent( ml, r1, r2 );
        ct = new QgsCoordinateTransform( ml->crs(), *mDestCRS );
        mRenderContext.setExtent( r1 );
        QgsDebugMsg( "  extent 1: " + r1.toString() );
        QgsDebugMsg( "  extent 2: " + r2.toString() );
        if ( !r1.isFinite() || !r2.isFinite() ) //there was a problem transforming the extent. Skip the layer
        {
          continue;
        }
      }
      else
      {
        ct = NULL;
      }

      mRenderContext.setCoordinateTransform( ct );

      //decide if we have to scale the raster
      //this is necessary in case QGraphicsScene is used
      bool scaleRaster = false;
      QgsMapToPixel rasterMapToPixel;
      QgsMapToPixel bk_mapToPixel;

      if ( ml->type() == QgsMapLayer::RasterLayer && qAbs( rasterScaleFactor - 1.0 ) > 0.000001 )
      {
        scaleRaster = true;
      }


      //create overlay objects for features within the view extent
      if ( ml->type() == QgsMapLayer::VectorLayer && overlayManager )
      {
        QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( ml );
        if ( vl )
        {
          QList<QgsVectorOverlay*> thisLayerOverlayList;
          vl->vectorOverlays( thisLayerOverlayList );

          QList<QgsVectorOverlay*>::iterator overlayIt = thisLayerOverlayList.begin();
          for ( ; overlayIt != thisLayerOverlayList.end(); ++overlayIt )
          {
            if (( *overlayIt )->displayFlag() )
            {
              ( *overlayIt )->createOverlayObjects( mRenderContext );
              allOverlayList.push_back( *overlayIt );
            }
          }

          overlayManager->addLayer( vl, thisLayerOverlayList );
        }
      }

      // Force render of layers that are being edited
      // or if there's a labeling engine that needs the layer to register features
      if ( ml->type() == QgsMapLayer::VectorLayer )
      {
        QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( ml );
        if ( vl->isEditable() ||
             ( mRenderContext.labelingEngine() && mRenderContext.labelingEngine()->willUseLayer( vl ) ) )
        {
          ml->setCacheImage( 0 );
        }
      }

      QSettings mySettings;
      if ( ! split )//render caching does not yet cater for split extents
      {
        if ( mySettings.value( "/qgis/enable_render_caching", false ).toBool() )
        {
          if ( !mySameAsLastFlag || ml->cacheImage() == 0 )
          {
            QgsDebugMsg( "Caching enabled but layer redraw forced by extent change or empty cache" );
            QImage * mypImage = new QImage( mRenderContext.painter()->device()->width(),
                                            mRenderContext.painter()->device()->height(), QImage::Format_ARGB32 );
            mypImage->fill( 0 );
            ml->setCacheImage( mypImage ); //no need to delete the old one, maplayer does it for you
            QPainter * mypPainter = new QPainter( ml->cacheImage() );
            // Changed to enable anti aliasing by default in QGIS 1.7
            if ( mySettings.value( "/qgis/enable_anti_aliasing", true ).toBool() )
            {
              mypPainter->setRenderHint( QPainter::Antialiasing );
            }
            mRenderContext.setPainter( mypPainter );
          }
          else if ( mySameAsLastFlag )
          {
            //draw from cached image
            QgsDebugMsg( "Caching enabled --- drawing layer from cached image" );
            mypContextPainter->drawImage( 0, 0, *( ml->cacheImage() ) );
            disconnect( ml, SIGNAL( drawingProgress( int, int ) ), this, SLOT( onDrawingProgress( int, int ) ) );
            //short circuit as there is nothing else to do...
            continue;
          }
        }
      }

      if ( scaleRaster )
      {
        bk_mapToPixel = mRenderContext.mapToPixel();
        rasterMapToPixel = mRenderContext.mapToPixel();
        rasterMapToPixel.setMapUnitsPerPixel( mRenderContext.mapToPixel().mapUnitsPerPixel() / rasterScaleFactor );
        rasterMapToPixel.setYMaximum( mSize.height() * rasterScaleFactor );
        mRenderContext.setMapToPixel( rasterMapToPixel );
        mRenderContext.painter()->save();
        mRenderContext.painter()->scale( 1.0 / rasterScaleFactor, 1.0 / rasterScaleFactor );
      }

      if ( !ml->draw( mRenderContext ) )
      {
        emit drawError( ml );
      }
      else
      {
        QgsDebugMsg( "Layer rendered without issues" );
      }

      if ( split )
      {
        mRenderContext.setExtent( r2 );
        if ( !ml->draw( mRenderContext ) )
        {
          emit drawError( ml );
        }
      }

      if ( scaleRaster )
      {
        mRenderContext.setMapToPixel( bk_mapToPixel );
        mRenderContext.painter()->restore();
      }

      if ( mySettings.value( "/qgis/enable_render_caching", false ).toBool() )
      {
        if ( !split )
        {
          // composite the cached image into our view and then clean up from caching
          // by reinstating the painter as it was swapped out for caching renders
          delete mRenderContext.painter();
          mRenderContext.setPainter( mypContextPainter );
          //draw from cached image that we created further up
          mypContextPainter->drawImage( 0, 0, *( ml->cacheImage() ) );
        }
      }
      disconnect( ml, SIGNAL( drawingProgress( int, int ) ), this, SLOT( onDrawingProgress( int, int ) ) );
    }
    else // layer not visible due to scale
    {
      QgsDebugMsg( "Layer not rendered because it is not within the defined "
                   "visibility scale range" );
    }

  } // while (li.hasPrevious())

  QgsDebugMsg( "Done rendering map layers" );

  if ( !mOverview )
  {
    // render all labels for vector layers in the stack, starting at the base
    li.toBack();
    while ( li.hasPrevious() )
    {
      if ( mRenderContext.renderingStopped() )
      {
        break;
      }

      QString layerId = li.previous();

      // TODO: emit drawingProgress((myRenderCounter++),zOrder.size());
      QgsMapLayer *ml = QgsMapLayerRegistry::instance()->mapLayer( layerId );

      if ( ml && ( ml->type() != QgsMapLayer::RasterLayer ) )
      {
        // only make labels if the layer is visible
        // after scale dep viewing settings are checked
        if ( !ml->hasScaleBasedVisibility() || ( ml->minimumScale() < mScale && mScale < ml->maximumScale() ) )
        {
          bool split = false;

          if ( hasCrsTransformEnabled() )
          {
            QgsRectangle r1 = mExtent;
            split = splitLayersExtent( ml, r1, r2 );
            ct = new QgsCoordinateTransform( ml->crs(), *mDestCRS );
            mRenderContext.setExtent( r1 );
          }
          else
          {
            ct = NULL;
          }

          mRenderContext.setCoordinateTransform( ct );

          ml->drawLabels( mRenderContext );
          if ( split )
          {
            mRenderContext.setExtent( r2 );
            ml->drawLabels( mRenderContext );
          }
        }
      }
    }
  } // if (!mOverview)

  //find overlay positions and draw the vector overlays
  if ( overlayManager && allOverlayList.size() > 0 )
  {
    overlayManager->findObjectPositions( mRenderContext, mScaleCalculator->mapUnits() );
    //draw all the overlays
    QList<QgsVectorOverlay*>::iterator allOverlayIt = allOverlayList.begin();
    for ( ; allOverlayIt != allOverlayList.end(); ++allOverlayIt )
    {
      ( *allOverlayIt )->drawOverlayObjects( mRenderContext );
    }
    overlayManager->removeLayers();
  }

  delete overlayManager;
  // make sure progress bar arrives at 100%!
  emit drawingProgress( 1, 1 );

  if ( mLabelingEngine )
  {
    // set correct extent
    mRenderContext.setExtent( mExtent );
    mRenderContext.setCoordinateTransform( NULL );

    mLabelingEngine->drawLabeling( mRenderContext );
    mLabelingEngine->exit();
  }

  QgsDebugMsg( "Rendering completed in (seconds): " + QString( "%1" ).arg( renderTime.elapsed() / 1000.0 ) );

  mDrawing = false;
}

void QgsMapRenderer::setMapUnits( QGis::UnitType u )
{
  mScaleCalculator->setMapUnits( u );

  // Since the map units have changed, force a recalculation of the scale.
  updateScale();

  emit mapUnitsChanged();
}

QGis::UnitType QgsMapRenderer::mapUnits() const
{
  return mScaleCalculator->mapUnits();
}

void QgsMapRenderer::onDrawingProgress( int current, int total )
{
  Q_UNUSED( current );
  Q_UNUSED( total );
  // TODO: emit signal with progress
// QgsDebugMsg(QString("onDrawingProgress: %1 / %2").arg(current).arg(total));
  emit updateMap();
}

void QgsMapRenderer::setProjectionsEnabled( bool enabled )
{
  if ( mProjectionsEnabled != enabled )
  {
    mProjectionsEnabled = enabled;
    QgsDebugMsg( "Adjusting DistArea projection on/off" );
    mDistArea->setEllipsoidalMode( enabled );
    updateFullExtent();
    mLastExtent.setMinimal();
    emit hasCrsTransformEnabled( enabled );
  }
}

bool QgsMapRenderer::hasCrsTransformEnabled() const
{
  return mProjectionsEnabled;
}

void QgsMapRenderer::setDestinationCrs( const QgsCoordinateReferenceSystem& crs )
{
  QgsDebugMsg( "* Setting destCRS : = " + crs.toProj4() );
  QgsDebugMsg( "* DestCRS.srsid() = " + QString::number( crs.srsid() ) );
  if ( *mDestCRS != crs )
  {
    QgsRectangle rect;
    if ( !mExtent.isEmpty() )
    {
      QgsCoordinateTransform transform( *mDestCRS, crs );
      rect = transform.transformBoundingBox( mExtent );
    }

    invalidateCachedLayerCrs();
    QgsDebugMsg( "Setting DistArea CRS to " + QString::number( crs.srsid() ) );
    mDistArea->setSourceCrs( crs.srsid() );
    *mDestCRS = crs;
    updateFullExtent();

    if ( !rect.isEmpty() )
    {
      setExtent( rect );
    }

    emit destinationSrsChanged();
  }
}

const QgsCoordinateReferenceSystem& QgsMapRenderer::destinationCrs() const
{
  QgsDebugMsgLevel( "* Returning destCRS", 3 );
  QgsDebugMsgLevel( "* DestCRS.srsid() = " + QString::number( mDestCRS->srsid() ), 3 );
  QgsDebugMsgLevel( "* DestCRS.proj4() = " + mDestCRS->toProj4(), 3 );
  return *mDestCRS;
}


bool QgsMapRenderer::splitLayersExtent( QgsMapLayer* layer, QgsRectangle& extent, QgsRectangle& r2 )
{
  bool split = false;

  if ( hasCrsTransformEnabled() )
  {
    try
    {
#ifdef QGISDEBUG
      // QgsLogger::debug<QgsRectangle>("Getting extent of canvas in layers CS. Canvas is ", extent, __FILE__, __FUNCTION__, __LINE__);
#endif
      // Split the extent into two if the source CRS is
      // geographic and the extent crosses the split in
      // geographic coordinates (usually +/- 180 degrees,
      // and is assumed to be so here), and draw each
      // extent separately.
      static const double splitCoord = 180.0;

      if ( mCachedTr->sourceCrs().geographicFlag() )
      {
        // Note: ll = lower left point
        //   and ur = upper right point
        QgsPoint ll = tr( layer )->transform( extent.xMinimum(), extent.yMinimum(),
                                              QgsCoordinateTransform::ReverseTransform );

        QgsPoint ur = tr( layer )->transform( extent.xMaximum(), extent.yMaximum(),
                                              QgsCoordinateTransform::ReverseTransform );

        extent = tr( layer )->transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform );

        if ( ll.x() > ur.x() )
        {
          r2 = extent;
          extent.setXMinimum( splitCoord );
          r2.setXMaximum( splitCoord );
          split = true;
        }
      }
      else // can't cross 180
      {
        extent = tr( layer )->transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform );
      }
    }
    catch ( QgsCsException &cse )
    {
      Q_UNUSED( cse );
      QgsDebugMsg( "Transform error caught" );
      extent = QgsRectangle( -DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX );
      r2     = QgsRectangle( -DBL_MAX, -DBL_MAX, DBL_MAX, DBL_MAX );
    }
  }
  return split;
}

QgsRectangle QgsMapRenderer::layerExtentToOutputExtent( QgsMapLayer* theLayer, QgsRectangle extent )
{
  QgsDebugMsg( QString( "sourceCrs = " + tr( theLayer )->sourceCrs().authid() ) );
  QgsDebugMsg( QString( "destCRS = " + tr( theLayer )->destCRS().authid() ) );
  QgsDebugMsg( QString( "extent = " + extent.toString() ) );
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      extent = tr( theLayer )->transformBoundingBox( extent );
    }
    catch ( QgsCsException &cse )
    {
      QgsMessageLog::logMessage( tr( "Transform error caught: %1" ).arg( cse.what() ), tr( "CRS" ) );
    }
  }

  QgsDebugMsg( QString( "proj extent = " + extent.toString() ) );

  return extent;
}

QgsRectangle QgsMapRenderer::outputExtentToLayerExtent( QgsMapLayer* theLayer, QgsRectangle extent )
{
  QgsDebugMsg( QString( "layer sourceCrs = " + tr( theLayer )->sourceCrs().authid() ) );
  QgsDebugMsg( QString( "layer destCRS = " + tr( theLayer )->destCRS().authid() ) );
  QgsDebugMsg( QString( "extent = " + extent.toString() ) );
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      extent = tr( theLayer )->transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform );
    }
    catch ( QgsCsException &cse )
    {
      QgsMessageLog::logMessage( tr( "Transform error caught: %1" ).arg( cse.what() ), tr( "CRS" ) );
    }
  }

  QgsDebugMsg( QString( "proj extent = " + extent.toString() ) );

  return extent;
}

QgsPoint QgsMapRenderer::layerToMapCoordinates( QgsMapLayer* theLayer, QgsPoint point )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      point = tr( theLayer )->transform( point, QgsCoordinateTransform::ForwardTransform );
    }
    catch ( QgsCsException &cse )
    {
      Q_UNUSED( cse );
      QgsDebugMsg( QString( "Transform error caught: %1" ).arg( cse.what() ) );
    }
  }
  else
  {
    // leave point without transformation
  }
  return point;
}

QgsPoint QgsMapRenderer::mapToLayerCoordinates( QgsMapLayer* theLayer, QgsPoint point )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      point = tr( theLayer )->transform( point, QgsCoordinateTransform::ReverseTransform );
    }
    catch ( QgsCsException &cse )
    {
      QgsDebugMsg( QString( "Transform error caught: %1" ).arg( cse.what() ) );
      throw cse; //let client classes know there was a transformation error
    }
  }
  else
  {
    // leave point without transformation
  }
  return point;
}

QgsRectangle QgsMapRenderer::mapToLayerCoordinates( QgsMapLayer* theLayer, QgsRectangle rect )
{
  if ( hasCrsTransformEnabled() )
  {
    try
    {
      rect = tr( theLayer )->transform( rect, QgsCoordinateTransform::ReverseTransform );
    }
    catch ( QgsCsException &cse )
    {
      QgsDebugMsg( QString( "Transform error caught: %1" ).arg( cse.what() ) );
      throw cse; //let client classes know there was a transformation error
    }
  }
  return rect;
}


void QgsMapRenderer::updateFullExtent()
{
  QgsDebugMsg( "called." );
  QgsMapLayerRegistry* registry = QgsMapLayerRegistry::instance();

  // reset the map canvas extent since the extent may now be smaller
  // We can't use a constructor since QgsRectangle normalizes the rectangle upon construction
  mFullExtent.setMinimal();

  // iterate through the map layers and test each layers extent
  // against the current min and max values
  QStringList::iterator it = mLayerSet.begin();
  QgsDebugMsg( QString( "Layer count: %1" ).arg( mLayerSet.count() ) );
  while ( it != mLayerSet.end() )
  {
    QgsMapLayer * lyr = registry->mapLayer( *it );
    if ( lyr == NULL )
    {
      QgsDebugMsg( QString( "WARNING: layer '%1' not found in map layer registry!" ).arg( *it ) );
    }
    else
    {
      QgsDebugMsg( "Updating extent using " + lyr->name() );
      QgsDebugMsg( "Input extent: " + lyr->extent().toString() );

      // Layer extents are stored in the coordinate system (CS) of the
      // layer. The extent must be projected to the canvas CS
      QgsRectangle extent = layerExtentToOutputExtent( lyr, lyr->extent() );

      QgsDebugMsg( "Output extent: " + extent.toString() );
      mFullExtent.unionRect( extent );

    }
    it++;
  }

  if ( mFullExtent.width() == 0.0 || mFullExtent.height() == 0.0 )
  {
    // If all of the features are at the one point, buffer the
    // rectangle a bit. If they are all at zero, do something a bit
    // more crude.

    if ( mFullExtent.xMinimum() == 0.0 && mFullExtent.xMaximum() == 0.0 &&
         mFullExtent.yMinimum() == 0.0 && mFullExtent.yMaximum() == 0.0 )
    {
      mFullExtent.set( -1.0, -1.0, 1.0, 1.0 );
    }
    else
    {
      const double padFactor = 1e-8;
      double widthPad = mFullExtent.xMinimum() * padFactor;
      double heightPad = mFullExtent.yMinimum() * padFactor;
      double xmin = mFullExtent.xMinimum() - widthPad;
      double xmax = mFullExtent.xMaximum() + widthPad;
      double ymin = mFullExtent.yMinimum() - heightPad;
      double ymax = mFullExtent.yMaximum() + heightPad;
      mFullExtent.set( xmin, ymin, xmax, ymax );
    }
  }

  QgsDebugMsg( "Full extent: " + mFullExtent.toString() );
}

QgsRectangle QgsMapRenderer::fullExtent()
{
  updateFullExtent();
  return mFullExtent;
}

void QgsMapRenderer::setLayerSet( const QStringList& layers )
{
  QgsDebugMsg( QString( "Entering: %1" ).arg( layers.join( ", " ) ) );
  mLayerSet = layers;
  updateFullExtent();
}

QStringList& QgsMapRenderer::layerSet()
{
  return mLayerSet;
}

QgsOverlayObjectPositionManager* QgsMapRenderer::overlayManagerFromSettings()
{
  QSettings settings;
  QString overlayAlgorithmQString = settings.value( "qgis/overlayPlacementAlgorithm", "Central point" ).toString();

  QgsOverlayObjectPositionManager* result = 0;

  if ( overlayAlgorithmQString != "Central point" )
  {
    QgsPALObjectPositionManager* palManager = new QgsPALObjectPositionManager();
    if ( overlayAlgorithmQString == "Chain" )
    {
      palManager->setPlacementAlgorithm( "Chain" );
    }
    else if ( overlayAlgorithmQString == "Popmusic tabu chain" )
    {
      palManager->setPlacementAlgorithm( "Popmusic tabu chain" );
    }
    else if ( overlayAlgorithmQString == "Popmusic tabu" )
    {
      palManager->setPlacementAlgorithm( "Popmusic tabu" );
    }
    else if ( overlayAlgorithmQString == "Popmusic chain" )
    {
      palManager->setPlacementAlgorithm( "Popmusic chain" );
    }
    result = palManager;
  }
  else
  {
    result = new QgsCentralPointPositionManager();
  }

  return result;
}

bool QgsMapRenderer::readXML( QDomNode & theNode )
{
  QDomNode myNode = theNode.namedItem( "units" );
  QDomElement element = myNode.toElement();

  // set units
  QGis::UnitType units;
  if ( "meters" == element.text() )
  {
    units = QGis::Meters;
  }
  else if ( "feet" == element.text() )
  {
    units = QGis::Feet;
  }
  else if ( "degrees" == element.text() )
  {
    units = QGis::Degrees;
  }
  else if ( "unknown" == element.text() )
  {
    units = QGis::UnknownUnit;
  }
  else
  {
    QgsDebugMsg( "Unknown map unit type " + element.text() );
    units = QGis::Degrees;
  }
  setMapUnits( units );

  // set projections flag
  QDomNode projNode = theNode.namedItem( "projections" );
  element = projNode.toElement();
  setProjectionsEnabled( element.text().toInt() );

  // set destination CRS
  QgsCoordinateReferenceSystem srs;
  QDomNode srsNode = theNode.namedItem( "destinationsrs" );
  srs.readXML( srsNode );
  setDestinationCrs( srs );

  // set extent
  QgsRectangle aoi;
  QDomNode extentNode = theNode.namedItem( "extent" );

  QDomNode xminNode = extentNode.namedItem( "xmin" );
  QDomNode yminNode = extentNode.namedItem( "ymin" );
  QDomNode xmaxNode = extentNode.namedItem( "xmax" );
  QDomNode ymaxNode = extentNode.namedItem( "ymax" );

  QDomElement exElement = xminNode.toElement();
  double xmin = exElement.text().toDouble();
  aoi.setXMinimum( xmin );

  exElement = yminNode.toElement();
  double ymin = exElement.text().toDouble();
  aoi.setYMinimum( ymin );

  exElement = xmaxNode.toElement();
  double xmax = exElement.text().toDouble();
  aoi.setXMaximum( xmax );

  exElement = ymaxNode.toElement();
  double ymax = exElement.text().toDouble();
  aoi.setYMaximum( ymax );

  setExtent( aoi );
  return true;
}

bool QgsMapRenderer::writeXML( QDomNode & theNode, QDomDocument & theDoc )
{
  // units

  QDomElement unitsNode = theDoc.createElement( "units" );
  theNode.appendChild( unitsNode );

  QString unitsString;

  switch ( mapUnits() )
  {
    case QGis::Meters:
      unitsString = "meters";
      break;
    case QGis::Feet:
      unitsString = "feet";
      break;
    case QGis::Degrees:
      unitsString = "degrees";
      break;
    case QGis::UnknownUnit:
    default:
      unitsString = "unknown";
      break;
  }
  QDomText unitsText = theDoc.createTextNode( unitsString );
  unitsNode.appendChild( unitsText );


  // Write current view extents
  QDomElement extentNode = theDoc.createElement( "extent" );
  theNode.appendChild( extentNode );

  QDomElement xMin = theDoc.createElement( "xmin" );
  QDomElement yMin = theDoc.createElement( "ymin" );
  QDomElement xMax = theDoc.createElement( "xmax" );
  QDomElement yMax = theDoc.createElement( "ymax" );

  QgsRectangle r = extent();
  QDomText xMinText = theDoc.createTextNode( QString::number( r.xMinimum(), 'f' ) );
  QDomText yMinText = theDoc.createTextNode( QString::number( r.yMinimum(), 'f' ) );
  QDomText xMaxText = theDoc.createTextNode( QString::number( r.xMaximum(), 'f' ) );
  QDomText yMaxText = theDoc.createTextNode( QString::number( r.yMaximum(), 'f' ) );

  xMin.appendChild( xMinText );
  yMin.appendChild( yMinText );
  xMax.appendChild( xMaxText );
  yMax.appendChild( yMaxText );

  extentNode.appendChild( xMin );
  extentNode.appendChild( yMin );
  extentNode.appendChild( xMax );
  extentNode.appendChild( yMax );

  // projections enabled
  QDomElement projNode = theDoc.createElement( "projections" );
  theNode.appendChild( projNode );

  QDomText projText = theDoc.createTextNode( QString::number( hasCrsTransformEnabled() ) );
  projNode.appendChild( projText );

  // destination CRS
  QDomElement srsNode = theDoc.createElement( "destinationsrs" );
  theNode.appendChild( srsNode );
  destinationCrs().writeXML( srsNode, theDoc );

  return true;
}

void QgsMapRenderer::setLabelingEngine( QgsLabelingEngineInterface* iface )
{
  if ( mLabelingEngine )
    delete mLabelingEngine;

  mLabelingEngine = iface;
}

QgsCoordinateTransform *QgsMapRenderer::tr( QgsMapLayer *layer )
{
  if ( mCachedTrForLayer != layer )
  {
    invalidateCachedLayerCrs();

    delete mCachedTr;
    mCachedTr = new QgsCoordinateTransform( layer->crs(), *mDestCRS );
    mCachedTrForLayer = layer;

    connect( layer, SIGNAL( layerCrsChanged() ), this, SLOT( invalidateCachedLayerCrs() ) );
    connect( layer, SIGNAL( destroyed() ), this, SLOT( cachedLayerDestroyed() ) );
  }

  return mCachedTr;
}

void QgsMapRenderer::cachedLayerDestroyed()
{
  if ( mCachedTrForLayer == sender() )
    mCachedTrForLayer = 0;
}

void QgsMapRenderer::invalidateCachedLayerCrs()
{
  if ( mCachedTrForLayer )
    disconnect( mCachedTrForLayer, SIGNAL( layerCrsChanged() ), this, SLOT( invalidateCachedLayerCrs() ) );

  mCachedTrForLayer = 0;
}

bool QgsMapRenderer::mDrawing = false;
