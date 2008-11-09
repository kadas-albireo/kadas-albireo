/***************************************************************************
    qgsmaprender.h  -  class for rendering map layer set
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
/* $Id$ */

#ifndef QGSMAPRENDER_H
#define QGSMAPRENDER_H

#include <QSize>
#include <QStringList>

#include "qgis.h"
#include "qgsrectangle.h"
#include "qgsrendercontext.h"

class QDomDocument;
class QDomNode;
class QPainter;

class QgsMapToPixel;
class QgsMapLayer;
class QgsScaleCalculator;
class QgsCoordinateReferenceSystem;
class QgsDistanceArea;

/** \ingroup core
 * A non GUI class for rendering a map layer set onto a QPainter.
 */

class CORE_EXPORT QgsMapRenderer : public QObject
{
    Q_OBJECT

  public:

    /**Output units for pen width and point marker width/height*/
    enum OutputUnits
    {
      Millimeters,
      Pixels
      //MAP_UNITS probably supported in future versions
    };

    //! constructor
    QgsMapRenderer();

    //! destructor
    ~QgsMapRenderer();

    //! starts rendering
    void render( QPainter* painter );

    //! sets extent and checks whether suitable (returns false if not)
    bool setExtent( const QgsRectangle& extent );

    //! returns current extent
    QgsRectangle extent() const;

    const QgsMapToPixel* coordinateTransform() { return &( mRenderContext.mapToPixel() ); }

    double scale() const { return mScale; }
    /**Sets scale for scale based visibility. Normally, the scale is calculated automatically. This
     function is only used to force a preview scale (e.g. for print composer)*/
    void setScale( double scale ) {mScale = scale;}
    double mapUnitsPerPixel() const { return mMapUnitsPerPixel; }

    int width() const { return mSize.width(); };
    int height() const { return mSize.height(); };

    //! Recalculate the map scale
    void updateScale();

    //! Return the measuring object
    QgsDistanceArea* distanceArea() { return mDistArea; }
    QGis::UnitType mapUnits() const;
    void setMapUnits( QGis::UnitType u );

    //! sets whether map image will be for overview
    void enableOverviewMode( bool isOverview = true ) { mOverview = isOverview; }

    void setOutputSize( QSize size, int dpi );

    //!accessor for output dpi
    int outputDpi();
    //!accessor for output size
    QSize outputSize();

    //! transform extent in layer's CRS to extent in output CRS
    QgsRectangle layerExtentToOutputExtent( QgsMapLayer* theLayer, QgsRectangle extent );

    //! transform coordinates from layer's CRS to output CRS
    QgsPoint layerToMapCoordinates( QgsMapLayer* theLayer, QgsPoint point );

    //! transform coordinates from output CRS to layer's CRS
    QgsPoint mapToLayerCoordinates( QgsMapLayer* theLayer, QgsPoint point );

    //! transform rect's coordinates from output CRS to layer's CRS
    QgsRectangle mapToLayerCoordinates( QgsMapLayer* theLayer, QgsRectangle rect );

    //! sets whether to use projections for this layer set
    void setProjectionsEnabled( bool enabled );

    //! returns true if projections are enabled for this layer set
    bool hasCrsTransformEnabled();

    //! sets destination spatial reference system
    void setDestinationSrs( const QgsCoordinateReferenceSystem& srs );

    //! returns CRS ID of destination spatial reference system
    const QgsCoordinateReferenceSystem& destinationSrs();

    void setOutputUnits( OutputUnits u ) {mOutputUnits = u;}

    OutputUnits outputUnits() const {return mOutputUnits;}

    //! returns current extent of layer set
    QgsRectangle fullExtent();

    //! returns current layer set
    QStringList& layerSet();

    //! change current layer set
    void setLayerSet( const QStringList& layers );

    //! updates extent of the layer set
    void updateFullExtent();

    //! read settings
    bool readXML( QDomNode & theNode );

    //! write settings
    bool writeXML( QDomNode & theNode, QDomDocument & theDoc );

    //! Accessor for render context
    QgsRenderContext* rendererContext() {return &mRenderContext;}

  signals:

    void drawingProgress( int current, int total );

    void hasCrsTransformEnabled( bool flag );

    void destinationSrsChanged();

    void updateMap();

    void mapUnitsChanged();

    //! emitted when layer's draw() returned FALSE
    void drawError( QgsMapLayer* );

  public slots:

    //! called by signal from layer current being drawn
    void onDrawingProgress( int current, int total );

  protected:

    //! adjust extent to fit the pixmap size
    void adjustExtentToSize();

    /** Convenience function to project an extent into the layer source
     * CRS, but also split it into two extents if it crosses
     * the +/- 180 degree line. Modifies the given extent to be in the
     * source CRS coordinates, and if it was split, returns true, and
     * also sets the contents of the r2 parameter
     */
    bool splitLayersExtent( QgsMapLayer* layer, QgsRectangle& extent, QgsRectangle& r2 );

  protected:

    //! indicates drawing in progress
    bool mDrawing;

    //! map units per pixel
    double mMapUnitsPerPixel;

    //! Map scale at its current zool level
    double mScale;

    //! scale calculator
    QgsScaleCalculator * mScaleCalculator;

    //! current extent to be drawn
    QgsRectangle mExtent;

    //! indicates whether it's map image for overview
    bool mOverview;

    QSize mSize;

    //! detemines whether on the fly projection support is enabled
    bool mProjectionsEnabled;

    //! destination spatial reference system of the projection
    QgsCoordinateReferenceSystem* mDestCRS;

    //! stores array of layers to be rendered (identified by string)
    QStringList mLayerSet;

    //! full extent of the layer set
    QgsRectangle mFullExtent;

    //! tool for measuring
    QgsDistanceArea* mDistArea;

    //!Encapsulates context of rendering
    QgsRenderContext mRenderContext;

    //!Output units
    OutputUnits mOutputUnits;
};

#endif

