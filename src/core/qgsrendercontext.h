/***************************************************************************
                              qgsrendercontext.h
                              ------------------
  begin                : March 16, 2008
  copyright            : (C) 2008 by Marco Hugentobler
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

#ifndef QGSRENDERCONTEXT_H
#define QGSRENDERCONTEXT_H

#include <QColor>

#include "qgscoordinatetransform.h"
#include "qgsmaptopixel.h"
#include "qgsrectangle.h"

class QPainter;

class QgsLabelingEngineInterface;

/** \ingroup core
 * Contains information about the context of a rendering operation.
 * The context of a rendering operation defines properties such as
 * the conversion ratio between screen and map units, the extents /
 * bounding box to be rendered etc.
 **/
class CORE_EXPORT QgsRenderContext
{
  public:
    QgsRenderContext();
    ~QgsRenderContext();

    //getters

    QPainter* painter() {return mPainter;}
    const QPainter* constPainter() const { return mPainter; }

    const QgsCoordinateTransform* coordinateTransform() const {return mCoordTransform;}

    const QgsRectangle& extent() const {return mExtent;}

    const QgsMapToPixel& mapToPixel() const {return mMapToPixel;}

    double scaleFactor() const {return mScaleFactor;}

    double rasterScaleFactor() const {return mRasterScaleFactor;}

    bool renderingStopped() const {return mRenderingStopped;}

    bool forceVectorOutput() const {return mForceVectorOutput;}

    /**Returns true if advanced effects such as blend modes such be used
      @note added in 1.9*/
    bool useAdvancedEffects() const {return mUseAdvancedEffects;}
    /**Used to enable or disable advanced effects such as blend modes
      @note: added in version 1.9*/
    void setUseAdvancedEffects( bool enabled ) { mUseAdvancedEffects = enabled; }

    bool drawEditingInformation() const {return mDrawEditingInformation;}

    double rendererScale() const {return mRendererScale;}

    //! Added in QGIS v1.4
    QgsLabelingEngineInterface* labelingEngine() const { return mLabelingEngine; }

    //! Added in QGIS v2.0
    QColor selectionColor() const { return mSelectionColor; }

    //setters

    /**Sets coordinate transformation. QgsRenderContext does not take ownership*/
    void setCoordinateTransform( const QgsCoordinateTransform* t );
    void setMapToPixel( const QgsMapToPixel& mtp ) {mMapToPixel = mtp;}
    void setExtent( const QgsRectangle& extent ) {mExtent = extent;}
    void setDrawEditingInformation( bool b ) {mDrawEditingInformation = b;}
    void setRenderingStopped( bool stopped ) {mRenderingStopped = stopped;}
    void setScaleFactor( double factor ) {mScaleFactor = factor;}
    void setRasterScaleFactor( double factor ) {mRasterScaleFactor = factor;}
    void setRendererScale( double scale ) {mRendererScale = scale;}
    void setPainter( QPainter* p ) {mPainter = p;}
    //! Added in QGIS v1.5
    void setForceVectorOutput( bool force ) {mForceVectorOutput = force;}
    //! Added in QGIS v1.4
    void setLabelingEngine( QgsLabelingEngineInterface* iface ) { mLabelingEngine = iface; }
    //! Added in QGIS v2.0
    void setSelectionColor( const QColor& color ) { mSelectionColor = color; }

  private:

    /**Painter for rendering operations*/
    QPainter* mPainter;

    /**For transformation between coordinate systems. Can be 0 if on-the-fly reprojection is not used*/
    const QgsCoordinateTransform* mCoordTransform;

    /**True if vertex markers for editing should be drawn*/
    bool mDrawEditingInformation;

    QgsRectangle mExtent;

    /**If true then no rendered vector elements should be cached as image*/
    bool mForceVectorOutput;

    /**Flag if advanced visual effects such as blend modes should be used. True by default*/
    bool mUseAdvancedEffects;

    QgsMapToPixel mMapToPixel;

    /**True if the rendering has been canceled*/
    bool mRenderingStopped;

    /**Factor to scale line widths and point marker sizes*/
    double mScaleFactor;

    /**Factor to scale rasters*/
    double mRasterScaleFactor;

    /**Map scale*/
    double mRendererScale;

    /**Labeling engine (can be NULL)*/
    QgsLabelingEngineInterface* mLabelingEngine;

    /** Color used for features that are marked as selected */
    QColor mSelectionColor;
};

#endif
