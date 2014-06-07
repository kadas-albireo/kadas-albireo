/***************************************************************************
                         qgscomposeritem.h
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
#ifndef QGSCOMPOSERITEM_H
#define QGSCOMPOSERITEM_H

#include "qgscomposeritemcommand.h"
#include "qgscomposereffect.h"
#include "qgsmaprenderer.h" // for blend mode functions & enums
#include <QGraphicsRectItem>
#include <QObject>

class QgsComposition;
class QWidget;
class QDomDocument;
class QDomElement;
class QGraphicsLineItem;
class QgsComposerItemGroup;

/** \ingroup MapComposer
 * A item that forms part of a map composition.
 */
class CORE_EXPORT QgsComposerItem: public QObject, public QGraphicsRectItem
{
    Q_OBJECT
  public:

    enum ItemType
    {
      // base class for the items
      ComposerItem = UserType + 100,

      // derived classes
      ComposerArrow,
      ComposerItemGroup,
      ComposerLabel,
      ComposerLegend,
      ComposerMap,
      ComposerPaper,  // QgsPaperItem
      ComposerPicture,
      ComposerScaleBar,
      ComposerShape,
      ComposerTable,
      ComposerAttributeTable,
      ComposerTextTable,
      ComposerFrame
    };

    /**Describes the action (move or resize in different directon) to be done during mouse move*/
    enum MouseMoveAction
    {
      MoveItem,
      ResizeUp,
      ResizeDown,
      ResizeLeft,
      ResizeRight,
      ResizeLeftUp,
      ResizeRightUp,
      ResizeLeftDown,
      ResizeRightDown,
      NoAction
    };

    enum ItemPositionMode
    {
      UpperLeft,
      UpperMiddle,
      UpperRight,
      MiddleLeft,
      Middle,
      MiddleRight,
      LowerLeft,
      LowerMiddle,
      LowerRight
    };

    /**Constructor
     @param composition parent composition
     @param manageZValue true if the z-Value of this object should be managed by mComposition*/
    QgsComposerItem( QgsComposition* composition, bool manageZValue = true );
    /**Constructor with box position and composer object
     @param x x coordinate of item
     @param y y coordinate of item
     @param width width of item
     @param height height of item
     @param composition parent composition
     @param manageZValue true if the z-Value of this object should be managed by mComposition*/
    QgsComposerItem( qreal x, qreal y, qreal width, qreal height, QgsComposition* composition, bool manageZValue = true );
    virtual ~QgsComposerItem();

    /** return correct graphics item type. Added in v1.7 */
    virtual int type() const { return ComposerItem; }

    /** \brief Set selected, selected item should be highlighted */
    virtual void setSelected( bool s );

    /** \brief Is selected */
    virtual bool selected() const {return QGraphicsRectItem::isSelected();};

    /** stores state in project */
    virtual bool writeSettings();

    /** read state from project */
    virtual bool readSettings();

    /** delete settings from project file  */
    virtual bool removeSettings();

    /**Moves item in canvas coordinates*/
    void move( double dx, double dy );

    /**Move Content of item. Does nothing per default (but implemented in composer map)
       @param dx move in x-direction (canvas coordinates)
       @param dy move in y-direction(canvas coordinates)*/
    virtual void moveContent( double dx, double dy ) { Q_UNUSED( dx ); Q_UNUSED( dy ); }

    /**Zoom content of item. Does nothing per default (but implemented in composer map)
      @param delta value from wheel event that describes magnitude and direction (positive /negative number)
      @param x x-position of mouse cursor (in item coordinates)
      @param y y-position of mouse cursor (in item coordinates)*/
    virtual void zoomContent( int delta, double x, double y ) { Q_UNUSED( delta ); Q_UNUSED( x ); Q_UNUSED( y ); }

    /**Gets the page the item is currently on.
     * @returns page number for item
     * @see pagePos
     * @see updatePagePos
     * @note this method was added in version 2.4
    */
    int page() const;

    /**Returns the item's position relative to its current page.
     * @returns position relative to the page's top left corner.
     * @see page
     * @see updatePagePos
     * @note this method was added in version 2.4
    */
    QPointF pagePos() const;

    /**Moves the item so that it retains its relative position on the page
     * when the paper size changes.
     * @param newPageWidth new width of the page in mm
     * @param newPageHeight new height of the page in mm
     * @see page
     * @see pagePos
     * @note this method was added in version 2.4
    */
    void updatePagePos( double newPageWidth, double newPageHeight );

    /**Moves the item to a new position (in canvas coordinates)*/
    void setItemPosition( double x, double y, ItemPositionMode itemPoint = UpperLeft, int page = -1 );

    /**Sets item position and width / height in one go
      @param x item position x
      @param y item position y
      @param width item width
      @param height item height
      @param itemPoint item position mode
      @param posIncludesFrame set to true if the position and size arguments include the item's frame border
      @param page if page > 0, y is interpreted as relative to the origin of the specified page, if page <= 0, y is in absolute canvas coordinates
      @note: this method was added in version 1.6*/
    void setItemPosition( double x, double y, double width, double height, ItemPositionMode itemPoint = UpperLeft, bool posIncludesFrame = false, int page = -1 );

    /**Returns item's last used position mode.
      @note: This property has no effect on actual's item position, which is always the top-left corner.
      @note: this method was added in version 2.0*/
    ItemPositionMode lastUsedPositionMode() { return mLastUsedPositionMode; }

    /**Sets this items bound in scene coordinates such that 1 item size units
     corresponds to 1 scene size unit*/
    virtual void setSceneRect( const QRectF& rectangle );

    /** stores state in Dom element
     * @param elem is Dom element corresponding to 'Composer' tag
     * @param doc is the Dom document
     */
    virtual bool writeXML( QDomElement& elem, QDomDocument & doc ) const = 0;

    /**Writes parameter that are not subclass specific in document. Usually called from writeXML methods of subclasses*/
    bool _writeXML( QDomElement& itemElem, QDomDocument& doc ) const;

    /** sets state from Dom document
     * @param itemElem is Dom node corresponding to item tag
     * @param doc is Dom document
     */
    virtual bool readXML( const QDomElement& itemElem, const QDomDocument& doc ) = 0;

    /**Reads parameter that are not subclass specific in document. Usually called from readXML methods of subclasses*/
    bool _readXML( const QDomElement& itemElem, const QDomDocument& doc );

    /** Whether this item has a frame or not.
     * @returns true if there is a frame around this item, otherwise false.
     * @note introduced since 1.8
     * @see hasFrame
     */
    bool hasFrame() const {return mFrame;}

    /** Set whether this item has a frame drawn around it or not.
     * @param drawFrame draw frame
     * @returns nothing
     * @note introduced in 1.8
     * @see hasFrame
     */
    void setFrameEnabled( bool drawFrame );

    /** Sets frame outline width
     * @param outlineWidth new width for outline frame
     * @returns nothing
     * @note introduced in 2.2
     * @see setFrameEnabled
     */
    virtual void setFrameOutlineWidth( double outlineWidth );

    /** Returns the frame's outline width. Only used if hasFrame is true.
     * @returns Frame outline width
     * @note introduced in 2.3
     * @see hasFrame
     * @see setFrameOutlineWidth
     */
    double frameOutlineWidth() const { return pen().widthF(); }

    /** Returns the join style used for drawing the item's frame
     * @returns Join style for outline frame
     * @note introduced in 2.3
     * @see hasFrame
     * @see setFrameJoinStyle
     */
    Qt::PenJoinStyle frameJoinStyle() const { return mFrameJoinStyle; }
    /** Sets join style used when drawing the item's frame
     * @param style Join style for outline frame
     * @returns nothing
     * @note introduced in 2.3
     * @see setFrameEnabled
     * @see frameJoinStyle
     */
    void setFrameJoinStyle( Qt::PenJoinStyle style );

    /** Returns the estimated amount the item's frame bleeds outside the item's
     *  actual rectangle. For instance, if the item has a 2mm frame outline, then
     *  1mm of this frame is drawn outside the item's rect. In this case the
     *  return value will be 1.0
     * @note introduced in 2.2
     */
    virtual double estimatedFrameBleed() const;

    /** Returns the item's rectangular bounds, including any bleed caused by the item's frame.
     *  The bounds are returned in the item's coordinate system (see Qt's QGraphicsItem docs for
     *  more details about QGraphicsItem coordinate systems). The results differ from Qt's rect()
     *  function, as rect() makes no allowances for the portion of outlines which are drawn
     *  outside of the item.
     * @note introduced in 2.2
     * @see estimatedFrameBleed
     */
    virtual QRectF rectWithFrame() const;

    /** Whether this item has a Background or not.
     * @returns true if there is a Background around this item, otherwise false.
     * @note introduced since 2.0
     * @see hasBackground
     */
    bool hasBackground() const {return mBackground;}

    /** Set whether this item has a Background drawn around it or not.
     * @param drawBackground draw Background
     * @returns nothing
     * @note introduced in 2.0
     * @see hasBackground
     */
    void setBackgroundEnabled( bool drawBackground ) { mBackground = drawBackground; }

    /** Gets the background color for this item
     * @returns background color
     * @note introduced in 2.0
     */
    QColor backgroundColor() const { return mBackgroundColor; }

    /** Sets the background color for this item
     * @param backgroundColor new background color
     * @returns nothing
     * @note introduced in 2.0
     */
    void setBackgroundColor( const QColor& backgroundColor );

    /** Returns the item's composition blending mode */
    QPainter::CompositionMode blendMode() const { return mBlendMode; }

    /** Sets the item's composition blending mode*/
    void setBlendMode( QPainter::CompositionMode blendMode );

    /** Returns the item's transparency */
    int transparency() const { return mTransparency; }
    /** Sets the item's transparency */
    void setTransparency( int transparency );

    /** Returns true if effects (eg blend modes) are enabled for the item
     * @note introduced in 2.0
    */
    bool effectsEnabled() const { return mEffectsEnabled; }
    /** Sets whether effects (eg blend modes) are enabled for the item
     * @note introduced in 2.0
    */
    void setEffectsEnabled( bool effectsEnabled );

    /**Composite operations for item groups do nothing per default*/
    virtual void addItem( QgsComposerItem* item ) { Q_UNUSED( item ); }
    virtual void removeItems() {}

    const QgsComposition* composition() const { return mComposition; }
    QgsComposition* composition() {return mComposition;}

    virtual void beginItemCommand( const QString& text ) { beginCommand( text ); }

    /**Starts new composer undo command
      @param commandText command title
      @param c context for mergeable commands (unknown for non-mergeable commands*/
    void beginCommand( const QString& commandText, QgsComposerMergeCommand::Context c = QgsComposerMergeCommand::Unknown );

    virtual void endItemCommand() { endCommand(); }
    /**Finish current command and push it onto the undo stack */
    void endCommand();
    void cancelCommand();

    //functions that encapsulate the workaround for the Qt font bug (that is to scale the font size up and then scale the
    //painter down by the same factor for drawing

    /**Draws Text. Takes care about all the composer specific issues (calculation to pixel, scaling of font and painter
     to work around the Qt font bug)*/
    void drawText( QPainter* p, double x, double y, const QString& text, const QFont& font ) const;

    /**Like the above, but with a rectangle for multiline text
     * @param p painter to use
     * @param rect rectangle to draw into
     * @param text text to draw
     * @param font font to use
     * @param halignment optional horizontal alignment
     * @param valignment optional vertical alignment
     * @param flags allows for passing Qt::TextFlags to control appearance of rendered text
    */
    void drawText( QPainter* p, const QRectF& rect, const QString& text, const QFont& font, Qt::AlignmentFlag halignment = Qt::AlignLeft, Qt::AlignmentFlag valignment = Qt::AlignTop, int flags = Qt::TextWordWrap ) const;

    /**Returns the font width in millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE*/
    double textWidthMillimeters( const QFont& font, const QString& text ) const;

    /**Returns the font height of a character in millimeters
      @note this method was added in version 1.7*/
    double fontHeightCharacterMM( const QFont& font, const QChar& c ) const;

    /**Returns the font ascent in Millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE*/
    double fontAscentMillimeters( const QFont& font ) const;

    /**Returns the font descent in Millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE*/
    double fontDescentMillimeters( const QFont& font ) const;

    /**Returns the font height in Millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE.
     * Font height equals the font ascent+descent+1 (for baseline).
     * @note Added in version 2.4
    */
    double fontHeightMillimeters( const QFont& font ) const;

    /**Calculates font to from point size to pixel size*/
    double pixelFontSize( double pointSize ) const;

    /**Returns a font where size is in pixel and font size is upscaled with FONT_WORKAROUND_SCALE*/
    QFont scaledFontPixelSize( const QFont& font ) const;

    /**Locks / unlocks the item position for mouse drags
    @note this method was added in version 1.2*/
    void setPositionLock( bool lock );

    /**Returns position lock for mouse drags (true means locked)
    @note this method was added in version 1.2*/
    bool positionLock() const { return mItemPositionLocked; }

    /**Returns the rotation for the composer item
    @note this method was added in version 2.1*/
    double itemRotation() const { return mItemRotation; }

    /**Returns the rotation for the composer item
     * @deprecated Use itemRotation()
     *             instead
     */
    Q_DECL_DEPRECATED double rotation() const { return mItemRotation; }

    /**Updates item, with the possibility to do custom update for subclasses*/
    virtual void updateItem() { QGraphicsRectItem::update(); }

    /**Get item's id (which is not necessarly unique)
      @note this method was added in version 1.7*/
    QString id() const { return mId; }

    /**Set item's id (which is not necessarly unique)
      @note this method was added in version 1.7*/
    virtual void setId( const QString& id );

    /**Get item identification name
      @note this method was added in version 2.0
      @note there is not setter since one can't manually set the id*/
    QString uuid() const { return mUuid; }

    /**Get the number of layers that this item requires for exporting as layers
     * @returns 0 if this item is to be placed on the same layer as the previous item,
     * 1 if it should be placed on its own layer, and >1 if it requires multiple export layers
     * @note this method was added in version 2.4
    */
    virtual int numberExportLayers() const { return 0; }

    /**Sets the current layer to draw for exporting
     * @param layerIdx can be set to -1 to draw all item layers, and must be less than numberExportLayers()
     * @note this method was added in version 2.4
    */
    virtual void setCurrentExportLayer( int layerIdx = -1 ) { mCurrentExportLayer = layerIdx; }

  public slots:
    /**Sets the item rotation
     * @deprecated Use setItemRotation( double rotation ) instead
     */
    virtual void setRotation( double r );

    /**Sets the item rotation
      @param r item rotation in degrees
      @param adjustPosition set to true if item should be shifted so that rotation occurs
       around item center. If false, rotation occurs around item origin
      @note this method was added in version 2.1
    */
    virtual void setItemRotation( double r, bool adjustPosition = false );

    void repaint();

  protected:

    QgsComposition* mComposition;

    QgsComposerItem::MouseMoveAction mCurrentMouseMoveAction;
    /**Start point of the last mouse move action (in scene coordinates)*/
    QPointF mMouseMoveStartPos;
    /**Position of the last mouse move event (in scene coordinates)*/
    QPointF mLastMouseEventPos;

    /**Rectangle used during move and resize actions*/
    QGraphicsRectItem* mBoundingResizeRectangle;
    QGraphicsLineItem* mHAlignSnapItem;
    QGraphicsLineItem* mVAlignSnapItem;

    /**True if item fram needs to be painted*/
    bool mFrame;
    /**True if item background needs to be painted*/
    bool mBackground;
    /**Background color*/
    QColor mBackgroundColor;
    /**Frame join style*/
    Qt::PenJoinStyle mFrameJoinStyle;

    /**True if item position  and size cannot be changed with mouse move
    @note: this member was added in version 1.2*/
    bool mItemPositionLocked;

    /**Backup to restore item appearance if no view scale factor is available*/
    mutable double mLastValidViewScaleFactor;

    /**Item rotation in degrees, clockwise*/
    double mItemRotation;

    /**Composition blend mode for item*/
    QPainter::CompositionMode mBlendMode;
    bool mEffectsEnabled;
    QgsComposerEffect *mEffect;

    /**Item transparency*/
    int mTransparency;

    /**The item's position mode
    @note: this member was added in version 2.0*/
    ItemPositionMode mLastUsedPositionMode;

    /**The layer that needs to be exported
    @note: if -1, all layers are to be exported
    @note: this member was added in version 2.4*/
    int mCurrentExportLayer;

    /**Draw selection boxes around item*/
    virtual void drawSelectionBoxes( QPainter* p );

    /**Draw black frame around item*/
    virtual void drawFrame( QPainter* p );

    /**Draw background*/
    virtual void drawBackground( QPainter* p );

    /**Draws arrowhead*/
    void drawArrowHead( QPainter* p, double x, double y, double angle, double arrowHeadWidth ) const;

    /**Returns angle of the line from p1 to p2 (clockwise, starting at N)*/
    double angle( const QPointF& p1, const QPointF& p2 ) const;

    /**Returns the current (zoom level dependent) tolerance to decide if mouse position is close enough to the
    item border for resizing*/
    double rectHandlerBorderTolerance() const;

    /**Returns the size of the lock symbol depending on the composer zoom level and the item size
    @note: this function was introduced in version 1.2*/
    double lockSymbolSize() const;

    /**Returns the zoom factor of the graphics view.
      @return the factor or -1 in case of error (e.g. graphic view does not exist)
    @note: this function was introduced in version 1.2*/
    double horizontalViewScaleFactor() const;

    //some utility functions

    /**Calculates width and hight of the picture (in mm) such that it fits into the item frame with the given rotation*/
    bool imageSizeConsideringRotation( double& width, double& height, double rotation ) const;
    /**Calculates width and hight of the picture (in mm) such that it fits into the item frame with the given rotation
     * @deprecated Use bool imageSizeConsideringRotation( double& width, double& height, double rotation )
     * instead
     */
    Q_DECL_DEPRECATED bool imageSizeConsideringRotation( double& width, double& height ) const;

    /**Calculates the largest scaled version of originalRect which fits within boundsRect, when it is rotated by
     * a specified amount
        @param originalRect QRectF to be rotated and scaled
        @param boundsRect QRectF specifying the bounds which the rotated and scaled rectangle must fit within
        @param rotation the rotation in degrees to be applied to the rectangle
    */
    QRectF largestRotatedRectWithinBounds( QRectF originalRect, QRectF boundsRect, double rotation ) const;

    /**Calculates corner point after rotation and scaling*/
    bool cornerPointOnRotatedAndScaledRect( double& x, double& y, double width, double height, double rotation ) const;
    /**Calculates corner point after rotation and scaling
     * @deprecated Use bool cornerPointOnRotatedAndScaledRect( double& x, double& y, double width, double height, double rotation )
     * instead
     */
    Q_DECL_DEPRECATED bool cornerPointOnRotatedAndScaledRect( double& x, double& y, double width, double height ) const;

    /**Calculates width / height of the bounding box of a rotated rectangle*/
    void sizeChangedByRotation( double& width, double& height, double rotation );
    /**Calculates width / height of the bounding box of a rotated rectangle
    * @deprecated Use void sizeChangedByRotation( double& width, double& height, double rotation )
    * instead
    */
    Q_DECL_DEPRECATED void sizeChangedByRotation( double& width, double& height );

    /**Rotates a point / vector
        @param angle rotation angle in degrees, counterclockwise
        @param x in/out: x coordinate before / after the rotation
        @param y in/out: y cooreinate before / after the rotation*/
    void rotate( double angle, double& x, double& y ) const;

    /**Return horizontal align snap item. Creates a new graphics line if 0*/
    QGraphicsLineItem* hAlignSnapItem();
    void deleteHAlignSnapItem();
    /**Return vertical align snap item. Creates a new graphics line if 0*/
    QGraphicsLineItem* vAlignSnapItem();
    void deleteVAlignSnapItem();
    void deleteAlignItems();

  signals:
    /**Is emitted on item rotation change*/
    void itemRotationChanged( double newRotation );
    /**Used e.g. by the item widgets to update the gui elements*/
    void itemChanged();
    /**Emitted if the rectangle changes*/
    void sizeChanged();
    /**Emitted if the item's frame style changes
     * @note: this function was introduced in version 2.2
    */
    void frameChanged();
  private:
    // id (not unique)
    QString mId;
    // name (unique)
    QString mUuid;
    // name (temporary when loaded from template)
    QString mTemplateUuid;

    void init( bool manageZValue );

    friend class QgsComposerItemGroup; // to access mTemplateUuid
};

#endif
