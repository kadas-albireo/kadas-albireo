/***************************************************************************
                         qgscomposermap.h
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSCOMPOSERMAP_H
#define QGSCOMPOSERMAP_H

//#include "ui_qgscomposermapbase.h"
#include "qgscomposeritem.h"
#include "qgsrectangle.h"
#include <QFont>
#include <QGraphicsRectItem>

class QgsComposition;
class QgsMapRenderer;
class QgsMapToPixel;
class QDomNode;
class QDomDocument;
class QGraphicsView;
class QPainter;
class QgsFillSymbolV2;
class QgsLineSymbolV2;
class QgsVectorLayer;

/** \ingroup MapComposer
 *  \class QgsComposerMap
 *  \brief Object representing map window.
 */
// NOTE: QgsComposerMapBase must be first, otherwise does not compile
class CORE_EXPORT QgsComposerMap : public QgsComposerItem
{
    Q_OBJECT

  public:
    /** Constructor. */
    QgsComposerMap( QgsComposition *composition, int x, int y, int width, int height );
    /** Constructor. Settings are read from project. */
    QgsComposerMap( QgsComposition *composition );
    virtual ~QgsComposerMap();

    /** return correct graphics item type. Added in v1.7 */
    virtual int type() const { return ComposerMap; }

    /** \brief Preview style  */
    enum PreviewMode
    {
      Cache = 0,   // Use raster cache
      Render,      // Render the map
      Rectangle    // Display only rectangle
    };

    enum GridStyle
    {
      Solid = 0, //solid lines
      Cross //only draw line crossings
    };

    enum GridAnnotationPosition
    {
      InsideMapFrame = 0,
      OutsideMapFrame,
      Disabled
    };

    enum GridAnnotationDirection
    {
      Horizontal = 0,
      Vertical,
      HorizontalAndVertical,
      BoundaryDirection
    };

    enum GridAnnotationFormat
    {
      Decimal = 0,
      DegreeMinute,
      DegreeMinuteSecond
    };

    enum GridFrameStyle
    {
      NoGridFrame = 0,
      Zebra // black/white pattern
    };

    /**Enum for different frame borders*/
    enum Border
    {
      Left,
      Right,
      Bottom,
      Top
    };

    /** Scaling modes used for the serial rendering (atlas)
     */
    enum AtlasScalingMode
    {
      Fixed,      /*< The current scale of the map is used for each feature of the atlas */
      Predefined, /*< A scale is chosen from the predefined scales. The smallest scale from
                    the list of scales where the atlas feature is fully visible is chosen.
                    @see QgsAtlasComposition::setPredefinedScales.
                    @note This mode is only valid for polygon or line atlas coverage layers
                */
      Auto        /*< The extent is adjusted so that each feature is fully visible.
                    A margin is applied around the center @see setAtlasMargin
                    @note This mode is only valid for polygon or line atlas coverage layers*/
    };

    /** \brief Draw to paint device
        @param painter painter
        @param extent map extent
        @param size size in scene coordinates
        @param dpi scene dpi
        @param forceWidthScale force wysiwyg line widths / marker sizes
    */
    void draw( QPainter *painter, const QgsRectangle& extent, const QSizeF& size, double dpi, double* forceWidthScale = 0 );

    /** \brief Reimplementation of QCanvasItem::paint - draw on canvas */
    void paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget );

    /** \brief Create cache image */
    void cache();

    /** \brief Get identification number*/
    int id() const {return mId;}

    /**True if a draw is already in progress*/
    bool isDrawing() const {return mDrawing;}

    /** resizes an item in x- and y direction (canvas coordinates)*/
    void resize( double dx, double dy );

    /**Move content of map
       @param dx move in x-direction (item and canvas coordinates)
       @param dy move in y-direction (item and canvas coordinates)*/
    void moveContent( double dx, double dy );

    /**Zoom content of map
     @param delta value from wheel event that describes magnitude and direction (positive /negative number)
    @param x x-coordinate of mouse position in item coordinates
    @param y y-coordinate of mouse position in item coordinates*/
    void zoomContent( int delta, double x, double y );

    /**Sets new scene rectangle bounds and recalculates hight and extent*/
    void setSceneRect( const QRectF& rectangle );

    /** \brief Scale */
    double scale() const;

    /**Sets new scale and changes only mExtent*/
    void setNewScale( double scaleDenominator );

    /**Sets new Extent and changes width, height (and implicitely also scale)*/
    void setNewExtent( const QgsRectangle& extent );

    /**Sets new Extent for the current atlas preview and changes width, height (and implicitely also scale).
      Atlas preview extents are only temporary, and are regenerated whenever the atlas feature changes
    */
    void setNewAtlasFeatureExtent( const QgsRectangle& extent );

    /**Called when atlas preview is toggled, to force map item to update its extent and redraw*/
    void toggleAtlasPreview();

    /**Returns a pointer to the current map extent, which is either the original user specified
     * extent or the temporary atlas-driven feature extent depending on the current atlas state
     * of the composition. Both a const and non-const version are included.
     * @returns pointer to current map extent
     * @see visibleExtentPolygon
    */
    QgsRectangle* currentMapExtent();
    const QgsRectangle* currentMapExtent() const;

    PreviewMode previewMode() const {return mPreviewMode;}
    void setPreviewMode( PreviewMode m );

    /**Getter for flag that determines if the stored layer set should be used or the current layer set of the qgis mapcanvas
    @note this function was added in version 1.2*/
    bool keepLayerSet() const {return mKeepLayerSet;}
    /**Setter for flag that determines if the stored layer set should be used or the current layer set of the qgis mapcanvas
    @note this function was added in version 1.2*/
    void setKeepLayerSet( bool enabled ) {mKeepLayerSet = enabled;}

    /**Getter for stored layer set that is used if mKeepLayerSet is true
    @note this function was added in version 1.2*/
    QStringList layerSet() const {return mLayerSet;}
    /**Setter for stored layer set that is used if mKeepLayerSet is true
    @note this function was added in version 1.2*/
    void setLayerSet( const QStringList& layerSet ) {mLayerSet = layerSet;}
    /**Stores the current layer set of the qgis mapcanvas in mLayerSet*/
    void storeCurrentLayerSet();

    // Set cache outdated
    void setCacheUpdated( bool u = false );

    QgsRectangle extent() const {return mExtent;}

    //! @deprecated since 2.4 - use mapSettings() - may return 0 if not initialized with QgsMapRenderer
    Q_DECL_DEPRECATED const QgsMapRenderer* mapRenderer() const;

    /**Sets offset values to shift image (useful for live updates when moving item content)*/
    void setOffset( double xOffset, double yOffset );

    /**True if composer map renders a WMS layer*/
    bool containsWMSLayer() const;

    /**True if composer map contains layers with blend modes or flattened layers for vectors */
    bool containsAdvancedEffects() const;

    /** stores state in Dom node
     * @param elem is Dom element corresponding to 'Composer' tag
     * @param doc Dom document
     */
    bool writeXML( QDomElement& elem, QDomDocument & doc ) const;

    /** sets state from Dom document
     * @param itemElem is Dom node corresponding to 'ComposerMap' tag
     * @param doc is Dom document
     */
    bool readXML( const QDomElement& itemElem, const QDomDocument& doc );

    /**Enables a coordinate grid that is shown on top of this composermap.
        @note this function was added in version 1.4*/
    void setGridEnabled( bool enabled ) {mGridEnabled = enabled;}
    bool gridEnabled() const { return mGridEnabled; }

    /**Sets coordinate grid style to solid or cross
        @note this function was added in version 1.4*/
    void setGridStyle( GridStyle style ) {mGridStyle = style;}
    GridStyle gridStyle() const { return mGridStyle; }

    /**Sets coordinate interval in x-direction for composergrid.
        @note this function was added in version 1.4*/
    void setGridIntervalX( double interval ) { mGridIntervalX = interval;}
    double gridIntervalX() const { return mGridIntervalX; }

    /**Sets coordinate interval in y-direction for composergrid.
    @note this function was added in version 1.4*/
    void setGridIntervalY( double interval ) { mGridIntervalY = interval;}
    double gridIntervalY() const { return mGridIntervalY; }

    /**Sets x-coordinate offset for composer grid
    @note this function was added in version 1.4*/
    void setGridOffsetX( double offset ) { mGridOffsetX = offset; }
    double gridOffsetX() const { return mGridOffsetX; }

    /**Sets y-coordinate offset for composer grid
    @note this function was added in version 1.4*/
    void setGridOffsetY( double offset ) { mGridOffsetY = offset; }
    double gridOffsetY() const { return mGridOffsetY; }

    /**Sets the pen to draw composer grid
    @note this function was added in version 1.4*/
    void setGridPen( const QPen& p );
    QPen gridPen() const;

    /**Sets with of grid pen
    @note this function was added in version 1.4*/
    void setGridPenWidth( double w );

    /**Sets the color of the grid pen
    @note this function was added in version 1.4*/
    void setGridPenColor( const QColor& c );

    /**Sets font for grid annotations
    @note this function was added in version 1.4*/
    void setGridAnnotationFont( const QFont& f ) { mGridAnnotationFont = f; }
    QFont gridAnnotationFont() const { return mGridAnnotationFont; }

    /**Sets font color for grid annotations
        @note this function was added in version 2.0*/
    void setAnnotationFontColor( const QColor& c ) {mGridAnnotationFontColor = c;}
    /**Get font color for grid annotations
        @note: this function was added in version 2.0*/
    QColor annotationFontColor() const {return mGridAnnotationFontColor;}

    /**Sets coordinate precision for grid annotations
    @note this function was added in version 1.4*/
    void setGridAnnotationPrecision( int p ) {mGridAnnotationPrecision = p;}
    int gridAnnotationPrecision() const {return mGridAnnotationPrecision;}

    /**Sets flag if grid annotation should be shown
    @note this function was added in version 1.4*/
    void setShowGridAnnotation( bool show ) {mShowGridAnnotation = show;}
    bool showGridAnnotation() const {return mShowGridAnnotation;}

    void setGridAnnotationPosition( GridAnnotationPosition p, QgsComposerMap::Border border );
    GridAnnotationPosition gridAnnotationPosition( QgsComposerMap::Border border ) const;

    /**Sets distance between map frame and annotations
    @note this function was added in version 1.4*/
    void setAnnotationFrameDistance( double d ) {mAnnotationFrameDistance = d;}
    double annotationFrameDistance() const {return mAnnotationFrameDistance;}

    void setGridAnnotationDirection( GridAnnotationDirection d, QgsComposerMap::Border border );
    GridAnnotationDirection gridAnnotationDirection( QgsComposerMap::Border border ) const;

    void setGridAnnotationFormat( GridAnnotationFormat f ) { mGridAnnotationFormat = f; }
    GridAnnotationFormat gridAnnotationFormat() const { return mGridAnnotationFormat; }

    /**Set grid frame style (NoGridFrame or Zebra)
        @note: this function was added in version 1.9*/
    void setGridFrameStyle( GridFrameStyle style ) { mGridFrameStyle = style; }
    GridFrameStyle gridFrameStyle() const { return mGridFrameStyle; }

    /**Set grid frame width
        @note: this function was added in version 1.9*/
    void setGridFrameWidth( double w ) { mGridFrameWidth = w; }
    double gridFrameWidth() const { return mGridFrameWidth; }

    /**Set grid frame pen thickness
        @note: this function was added in version 2.1*/
    void setGridFramePenSize( double w ) { mGridFramePenThickness = w; }
    double gridFramePenSize() const { return mGridFramePenThickness; }

    /**Sets pen color for grid frame
        @note: this function was added in version 2.1*/
    void setGridFramePenColor( const QColor& c ) { mGridFramePenColor = c;}
    /**Get pen color for grid frame
        @note: this function was added in version 2.1*/
    QColor gridFramePenColor() const {return mGridFramePenColor;}

    /**Sets first fill color for grid zebra frame
        @note: this function was added in version 2.1*/
    void setGridFrameFillColor1( const QColor& c ) { mGridFrameFillColor1 = c;}
    /**Get first fill color for grid zebra frame
        @note: this function was added in version 2.1*/
    QColor gridFrameFillColor1() const {return mGridFrameFillColor1;}

    /**Sets second fill color for grid zebra frame
        @note: this function was added in version 2.1*/
    void setGridFrameFillColor2( const QColor& c ) { mGridFrameFillColor2 = c;}
    /**Get second fill color for grid zebra frame
        @note: this function was added in version 2.1*/
    QColor gridFrameFillColor2() const {return mGridFrameFillColor2;}

    /**In case of annotations, the bounding rectangle can be larger than the map item rectangle
    @note this function was added in version 1.4*/
    QRectF boundingRect() const;
    /**Updates the bounding rect of this item. Call this function before doing any changes related to annotation out of the map rectangle
    @note this function was added in version 1.4*/
    void updateBoundingRect();

    /* reimplement setFrameOutlineWidth, so that updateBoundingRect() is called after setting the frame width */
    virtual void setFrameOutlineWidth( double outlineWidth );

    /**Sets length of the cros segments (if grid style is cross)
    @note this function was added in version 1.4*/
    void setCrossLength( double l ) {mCrossLength = l;}
    double crossLength() {return mCrossLength;}

    /**Sets rotation for the map - this does not affect the composer item shape, only the
      way the map is drawn within the item
     * @deprecated Use setMapRotation( double rotation ) instead
     */
    Q_DECL_DEPRECATED void setRotation( double r );

    /**Returns the rotation used for drawing the map within the composer item
     * @deprecated Use mapRotation() instead
     */
    Q_DECL_DEPRECATED double rotation() const { return mMapRotation;};

    /**Sets rotation for the map - this does not affect the composer item shape, only the
      way the map is drawn within the item
      @note this function was added in version 2.1*/
    void setMapRotation( double r );
    /**Returns the rotation used for drawing the map within the composer item*/
    double mapRotation() const { return mMapRotation;};

    void updateItem();

    /**Sets canvas pointer (necessary to query and draw map canvas items)*/
    void setMapCanvas( QGraphicsView* canvas ) { mMapCanvas = canvas; }

    void setDrawCanvasItems( bool b ) { mDrawCanvasItems = b; }
    bool drawCanvasItems() const { return mDrawCanvasItems; }

    /**Returns the conversion factor map units -> mm*/
    double mapUnitsToMM() const;

    /**Sets overview frame map. -1 disables the overview frame
    @note: this function was added in version 1.9*/
    void setOverviewFrameMap( int mapId );
    /**Returns id of overview frame (or -1 if no overfiew frame)
    @note: this function was added in version 1.9*/
    int overviewFrameMapId() const { return mOverviewFrameMapId; }

    void setOverviewFrameMapSymbol( QgsFillSymbolV2* symbol );
    QgsFillSymbolV2* overviewFrameMapSymbol() { return mOverviewFrameMapSymbol; }

    /** Returns the overview's blending mode */
    QPainter::CompositionMode overviewBlendMode() const {return mOverviewBlendMode;}
    /** Sets the overview's blending mode*/
    void setOverviewBlendMode( QPainter::CompositionMode blendMode );

    /** Returns true if the overview frame is inverted */
    bool overviewInverted() const {return mOverviewInverted;}
    /** Sets the overview's inversion mode*/
    void setOverviewInverted( bool inverted );

    /** Returns true if the extent is forced to center on the overview */
    bool overviewCentered() const { return mOverviewCentered; }
    /** Set the overview's centering mode */
    void setOverviewCentered( bool centered );

    void setGridLineSymbol( QgsLineSymbolV2* symbol );
    QgsLineSymbolV2* gridLineSymbol() { return mGridLineSymbol; }

    /** Returns the grid's blending mode */
    QPainter::CompositionMode gridBlendMode() const {return mGridBlendMode;}
    /** Sets the grid's blending mode*/
    void setGridBlendMode( QPainter::CompositionMode blendMode );

    /**Sets mId to a number not yet used in the composition. mId is kept if it is not in use.
        Usually, this function is called before adding the composer map to the composition*/
    void assignFreeId();

    /**Calculates width and hight of the picture (in mm) such that it fits into the item frame with the given rotation
     * @deprecated Use bool QgsComposerItem::imageSizeConsideringRotation( double& width, double& height, double rotation )
     * instead
     */
    Q_DECL_DEPRECATED bool imageSizeConsideringRotation( double& width, double& height ) const;
    /**Calculates corner point after rotation and scaling
     * @deprecated Use QgsComposerItem::cornerPointOnRotatedAndScaledRect( double& x, double& y, double width, double height, double rotation )
     * instead
     */
    Q_DECL_DEPRECATED bool cornerPointOnRotatedAndScaledRect( double& x, double& y, double width, double height ) const;
    /**Calculates width / height of the bounding box of a rotated rectangle
    * @deprecated Use QgsComposerItem::sizeChangedByRotation( double& width, double& height, double rotation )
    * instead
    */
    Q_DECL_DEPRECATED void sizeChangedByRotation( double& width, double& height );

    /**Returns whether the map extent is set to follow the current atlas feature.
     * @returns true if map will follow the current atlas feature.
     * @see setAtlasDriven
     * @see atlasScalingMode
    */
    bool atlasDriven() const { return mAtlasDriven; }

    /**Sets whether the map extent will follow the current atlas feature.
     * @param enabled set to true if the map extents should be set by the current atlas feature.
     * @see atlasDriven
     * @see setAtlasScalingMode
    */
    void setAtlasDriven( bool enabled ) { mAtlasDriven = enabled; }

    /**Returns true if the map uses a fixed scale when in atlas mode
     * @deprecated since 2.4 Use atlasScalingMode() instead
    */
    Q_DECL_DEPRECATED bool atlasFixedScale() const;

    /**Set to true if the map should use a fixed scale when in atlas mode
     * @deprecated since 2.4 Use setAtlasScalingMode() instead
    */
    Q_DECL_DEPRECATED void setAtlasFixedScale( bool fixed );

    /**Returns the current atlas scaling mode. This controls how the map's extents
     * are calculated for the current atlas feature when an atlas composition
     * is enabled.
     * @returns the current scaling mode
     * @note this parameter is only used if atlasDriven() is true
     * @see setAtlasScalingMode
     * @see atlasDriven
    */
    AtlasScalingMode atlasScalingMode() const { return mAtlasScalingMode; }

    /**Sets the current atlas scaling mode. This controls how the map's extents
     * are calculated for the current atlas feature when an atlas composition
     * is enabled.
     * @param mode atlas scaling mode to set
     * @note this parameter is only used if atlasDriven() is true
     * @see atlasScalingMode
     * @see atlasDriven
    */
    void setAtlasScalingMode( AtlasScalingMode mode ) { mAtlasScalingMode = mode; }

    /**Returns the margin size (percentage) used when the map is in atlas mode.
     * @returns margin size in percentage to leave around the atlas feature's extent
     * @note this is only used if atlasScalingMode() is Auto.
     * @see atlasScalingMode
     * @see setAtlasMargin
    */
    double atlasMargin() const { return mAtlasMargin; }

    /**Sets the margin size (percentage) used when the map is in atlas mode.
     * @param margin size in percentage to leave around the atlas feature's extent
     * @note this is only used if atlasScalingMode() is Auto.
     * @see atlasScalingMode
     * @see atlasMargin
    */
    void setAtlasMargin( double margin ) { mAtlasMargin = margin; }

    /** Sets whether updates to the composer map are enabled. */
    void setUpdatesEnabled( bool enabled ) { mUpdatesEnabled = enabled; }

    /** Returns whether updates to the composer map are enabled. */
    bool updatesEnabled() const { return mUpdatesEnabled; }

    /**Get the number of layers that this item requires for exporting as layers
     * @returns 0 if this item is to be placed on the same layer as the previous item,
     * 1 if it should be placed on its own layer, and >1 if it requires multiple export layers
     * @note this method was added in version 2.4
    */
    int numberExportLayers() const;

    /**Returns a polygon representing the current visible map extent, considering map extents and rotation.
     * If the map rotation is 0, the result is the same as currentMapExtent
     * @returns polygon with the four corner points representing the visible map extent. The points are
     * clockwise, starting at the top-left point
     * @see currentMapExtent
    */
    QPolygonF visibleExtentPolygon() const;

  signals:
    void extentChanged();

    /**Is emitted on rotation change to notify north arrow pictures*/
    void mapRotationChanged( double newRotation );

    /**Is emitted when the map has been prepared for atlas rendering, just before actual rendering*/
    void preparedForAtlas();

  public slots:

    /**Called if map canvas has changed*/
    void updateCachedImage( );
    /**Call updateCachedImage if item is in render mode*/
    void renderModeUpdateCachedImage();

    void overviewExtentChanged();

  private:

    enum AnnotationCoordinate
    {
      Longitude = 0,
      Latitude
    };

    /**Unique identifier*/
    int mId;

    // Map region in map units realy used for rendering
    // It can be the same as mUserExtent, but it can be bigger in on dimension if mCalculate==Scale,
    // so that full rectangle in paper is used.
    QgsRectangle mExtent;

    // Current temporary map region in map units. This is overwritten when atlas feature changes. It's also
    // used when the user changes the map extent and an atlas preview is enabled. This allows the user
    // to manually tweak each atlas preview page without affecting the actual original map extent.
    QgsRectangle mAtlasFeatureExtent;

    // Cache used in composer preview
    QImage mCacheImage;

    // Is cache up to date
    bool mCacheUpdated;

    /** \brief Preview style  */
    PreviewMode mPreviewMode;

    /** \brief Number of layers when cache was created  */
    int mNumCachedLayers;

    /** \brief set to true if in state of drawing. Concurrent requests to draw method are returned if set to true */
    bool mDrawing;

    /**Offset in x direction for showing map cache image*/
    double mXOffset;
    /**Offset in y direction for showing map cache image*/
    double mYOffset;

    /**Map rotation*/
    double mMapRotation;

    /**Flag if layers to be displayed should be read from qgis canvas (true) or from stored list in mLayerSet (false)*/
    bool mKeepLayerSet;

    /**Stored layer list (used if layer live-link mKeepLayerSet is disabled)*/
    QStringList mLayerSet;

    /**Id of map which displays its extent rectangle into this composer map (overview map functionality). -1 if not present*/
    int mOverviewFrameMapId;
    /**Drawing style for overview farme*/
    QgsFillSymbolV2* mOverviewFrameMapSymbol;
    QgsLineSymbolV2* mGridLineSymbol;
    /**Blend mode for overview*/
    QPainter::CompositionMode mOverviewBlendMode;
    bool mOverviewInverted;
    /** Centering mode for overview */
    bool mOverviewCentered;

    /** Whether updates to the map are enabled */
    bool mUpdatesEnabled;

    /**Establishes signal/slot connection for update in case of layer change*/
    void connectUpdateSlot();

    /**Removes layer ids from mLayerSet that are no longer present in the qgis main map*/
    void syncLayerSet();

    /**True if coordinate grid has to be displayed*/
    bool mGridEnabled;
    /**Solid or crosses*/
    GridStyle mGridStyle;
    /**Grid line interval in x-direction (map units)*/
    double mGridIntervalX;
    /**Grid line interval in y-direction (map units)*/
    double mGridIntervalY;
    /**Grid line offset in x-direction*/
    double mGridOffsetX;
    /**Grid line offset in y-direction*/
    double mGridOffsetY;
    /**Font for grid line annotation*/
    QFont mGridAnnotationFont;
    /**Font color for grid line annotation*/
    QColor mGridAnnotationFontColor;
    /**Digits after the dot*/
    int mGridAnnotationPrecision;
    /**True if coordinate values should be drawn*/
    bool mShowGridAnnotation;
    /**Blend mode for grid*/
    QPainter::CompositionMode mGridBlendMode;

    /**Annotation position for left map side (inside / outside / not shown)*/
    GridAnnotationPosition mLeftGridAnnotationPosition;
    /**Annotation position for right map side (inside / outside / not shown)*/
    GridAnnotationPosition mRightGridAnnotationPosition;
    /**Annotation position for top map side (inside / outside / not shown)*/
    GridAnnotationPosition mTopGridAnnotationPosition;
    /**Annotation position for bottom map side (inside / outside / not shown)*/
    GridAnnotationPosition mBottomGridAnnotationPosition;

    /**Distance between map frame and annotation*/
    double mAnnotationFrameDistance;

    /**Annotation direction on left side ( horizontal or vertical )*/
    GridAnnotationDirection mLeftGridAnnotationDirection;
    /**Annotation direction on right side ( horizontal or vertical )*/
    GridAnnotationDirection mRightGridAnnotationDirection;
    /**Annotation direction on top side ( horizontal or vertical )*/
    GridAnnotationDirection mTopGridAnnotationDirection;
    /**Annotation direction on bottom side ( horizontal or vertical )*/
    GridAnnotationDirection mBottomGridAnnotationDirection;

    GridAnnotationFormat mGridAnnotationFormat;

    GridFrameStyle mGridFrameStyle;
    double mGridFrameWidth;
    double mGridFramePenThickness;
    QColor mGridFramePenColor;
    QColor mGridFrameFillColor1;
    QColor mGridFrameFillColor2;

    /**Current bounding rectangle. This is used to check if notification to the graphics scene is necessary*/
    QRectF mCurrentRectangle;
    /**The length of the cross sides for mGridStyle Cross*/
    double mCrossLength;
    QGraphicsView* mMapCanvas;
    /**True if annotation items, rubber band, etc. from the main canvas should be displayed*/
    bool mDrawCanvasItems;

    /**Adjusts an extent rectangle to match the provided item width and height, so that extent
     * center of extent remains the same */
    void adjustExtentToItemShape( double itemWidth, double itemHeight, QgsRectangle& extent ) const;

    /**True if map is being controlled by an atlas*/
    bool mAtlasDriven;
    /**Current atlas scaling mode*/
    AtlasScalingMode mAtlasScalingMode;
    /**Margin size for atlas driven extents (percentage of feature size) - when in auto scaling mode*/
    double mAtlasMargin;

    /**Returns a list of the layers to render for this map item*/
    QStringList layersToRender() const;

    /**Draws the map grid*/
    void drawGrid( QPainter* p );
    void drawGridFrame( QPainter* p, const QList< QPair< double, QLineF > >& hLines, const QList< QPair< double, QLineF > >& vLines );
    /**Draw coordinates for mGridAnnotationType Coordinate
        @param p drawing painter
        @param hLines horizontal coordinate lines in item coordinates
        @param vLines vertical coordinate lines in item coordinates*/
    void drawCoordinateAnnotations( QPainter* p, const QList< QPair< double, QLineF > >& hLines, const QList< QPair< double, QLineF > >& vLines );
    void drawCoordinateAnnotation( QPainter* p, const QPointF& pos, QString annotationString );
    /**Draws a single annotation
        @param p drawing painter
        @param pos item coordinates where to draw
        @param rotation text rotation
        @param annotationText the text to draw*/
    void drawAnnotation( QPainter* p, const QPointF& pos, int rotation, const QString& annotationText );
    QString gridAnnotationString( double value, AnnotationCoordinate coord ) const;
    /**Returns the grid lines with associated coordinate value
        @return 0 in case of success*/
    int xGridLines( QList< QPair< double, QLineF > >& lines ) const;
    /**Returns the grid lines for the y-coordinates. Not vertical in case of rotation
        @return 0 in case of success*/
    int yGridLines( QList< QPair< double, QLineF > >& lines ) const;
    /**Returns extent that considers mOffsetX / mOffsetY (during content move)*/
    QgsRectangle transformedExtent() const;
    /**Returns extent that considers rotation and shift with mOffsetX / mOffsetY*/
    QPolygonF transformedMapPolygon() const;
    double maxExtension() const;

    /** mapPolygon variant using a given extent */
    void mapPolygon( const QgsRectangle& extent, QPolygonF& poly ) const;

    /**Calculates the extent to request and the yShift of the top-left point in case of rotation.*/
    void requestedExtent( QgsRectangle& extent ) const;
    /**Scales a composer map shift (in MM) and rotates it by mRotation
        @param xShift in: shift in x direction (in item units), out: xShift in map units
        @param yShift in: shift in y direction (in item units), out: yShift in map units*/
    void transformShift( double& xShift, double& yShift ) const;
    /**Transforms map coordinates to item coordinates (considering rotation and move offset)*/
    QPointF mapToItemCoords( const QPointF& mapCoords ) const;
    /**Returns the item border of a point (in item coordinates)*/
    Border borderForLineCoord( const QPointF& p ) const;

    void drawCanvasItems( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle );
    void drawCanvasItem( QGraphicsItem* item, QPainter* painter, const QStyleOptionGraphicsItem* itemStyle );
    QPointF composerMapPosForItem( const QGraphicsItem* item ) const;
    void sortGridLinesOnBorders( const QList< QPair< double, QLineF > >& hLines, const QList< QPair< double, QLineF > >& vLines,  QMap< double, double >& leftFrameEntries,
                                 QMap< double, double >& rightFrameEntries, QMap< double, double >& topFrameEntries, QMap< double, double >& bottomFrameEntries ) const;
    void drawGridFrameBorder( QPainter* p, const QMap< double, double >& borderPos, Border border );
    void drawGridLine( const QLineF& line, QgsRenderContext& context );
    void drawOverviewMapExtent( QPainter* p );
    void createDefaultOverviewFrameSymbol();
    void createDefaultGridLineSymbol();
    void initGridAnnotationFormatFromProject();

    enum PartType
    {
      Background,
      Layer,
      Grid,
      OverviewMapExtent,
      Frame,
      SelectionBoxes
    };

    /**Test if a part of the copmosermap needs to be drawn, considering mCurrentExportLayer*/
    bool shouldDrawPart( PartType part ) const;
};

#endif

