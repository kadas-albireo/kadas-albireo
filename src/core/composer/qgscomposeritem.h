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
    virtual bool selected() {return QGraphicsRectItem::isSelected();}

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

    /**Moves the item to a new position (in canvas coordinates)*/
    void setItemPosition( double x, double y, ItemPositionMode itemPoint = UpperLeft );

    /**Sets item position and width / height in one go
      @note: this method was added in version 1.6*/
    void setItemPosition( double x, double y, double width, double height, ItemPositionMode itemPoint = UpperLeft );

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
    void setFrameEnabled( bool drawFrame ) {mFrame = drawFrame;}


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
    void setBackgroundEnabled( bool drawBackground ) {mBackground = drawBackground;}

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
    QPainter::CompositionMode blendMode() const {return mBlendMode;}

    /** Sets the item's composition blending mode*/
    void setBlendMode( QPainter::CompositionMode blendMode );

    /** Returns the item's transparency */
    int transparency() const {return mTransparency;}
    /** Sets the item's transparency */
    void setTransparency( int transparency );

    /** Returns true if effects (eg blend modes) are enabled for the item
     * @note introduced in 2.0
    */
    bool effectsEnabled() const {return mEffectsEnabled;}
    /** Sets whether effects (eg blend modes) are enabled for the item
     * @note introduced in 2.0
    */
    void setEffectsEnabled( bool effectsEnabled );

    /**Composite operations for item groups do nothing per default*/
    virtual void addItem( QgsComposerItem* item ) { Q_UNUSED( item ); }
    virtual void removeItems() {}

    const QgsComposition* composition() const {return mComposition;}
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

    /**Like the above, but with a rectangle for multiline text*/
    void drawText( QPainter* p, const QRectF& rect, const QString& text, const QFont& font, Qt::AlignmentFlag halignment = Qt::AlignLeft, Qt::AlignmentFlag valignment = Qt::AlignTop ) const;

    /**Returns the font width in millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE*/
    double textWidthMillimeters( const QFont& font, const QString& text ) const;

    /**Returns the font height of a character in millimeters
      @note this method was added in version 1.7*/
    double fontHeightCharacterMM( const QFont& font, const QChar& c ) const;

    /**Returns the font ascent in Millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE*/
    double fontAscentMillimeters( const QFont& font ) const;

    /**Returns the font ascent in Millimeters (considers upscaling and downscaling with FONT_WORKAROUND_SCALE*/
    double fontDescentMillimeters( const QFont& font ) const;

    /**Calculates font to from point size to pixel size*/
    double pixelFontSize( double pointSize ) const;

    /**Returns a font where size is in pixel and font size is upscaled with FONT_WORKAROUND_SCALE*/
    QFont scaledFontPixelSize( const QFont& font ) const;

    /**Locks / unlocks the item position for mouse drags
    @note this method was added in version 1.2*/
    void setPositionLock( bool lock ) {mItemPositionLocked = lock;}

    /**Returns position lock for mouse drags (true means locked)
    @note this method was added in version 1.2*/
    bool positionLock() const {return mItemPositionLocked;}

    /**Update mouse cursor at (item) position
    @note this method was added in version 1.2*/
    void updateCursor( const QPointF& itemPos );

    double rotation() const {return mRotation;}

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

  public slots:
    virtual void setRotation( double r );
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

    /**True if item position  and size cannot be changed with mouse move
    @note: this member was added in version 1.2*/
    bool mItemPositionLocked;

    /**Backup to restore item appearance if no view scale factor is available*/
    mutable double mLastValidViewScaleFactor;

    /**Item rotation in degrees, clockwise*/
    double mRotation;

    /**Composition blend mode for item*/
    QPainter::CompositionMode mBlendMode;
    bool mEffectsEnabled;
    QgsComposerEffect *mEffect;

    /**Item transparency*/
    int mTransparency;

    /**The item's position mode
    @note: this member was added in version 2.0*/
    ItemPositionMode mLastUsedPositionMode;

    //event handlers
    virtual void mouseMoveEvent( QGraphicsSceneMouseEvent * event );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent * event );
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event );

    virtual void hoverMoveEvent( QGraphicsSceneHoverEvent * event );

    /**Finds out the appropriate cursor for the current mouse position in the widget (e.g. move in the middle, resize at border)*/
    Qt::CursorShape cursorForPosition( const QPointF& itemCoordPos );

    /**Finds out which mouse move action to choose depending on the cursor position inside the widget*/
    QgsComposerItem::MouseMoveAction mouseMoveActionForPosition( const QPointF& itemCoordPos );

    /**Changes the rectangle of an item depending on current mouse action (resize or move)
     @param currentPosition current position of mouse cursor
     @param mouseMoveStartPos cursor position at the start of the current mouse action
     @param originalItem Item position at the start of the mouse action
     @param dx x-Change of mouse cursor
     @param dy y-Change of mouse cursor
     @param changeItem Item to change size (can be the same as originalItem or a differen one)
    */
    void changeItemRectangle( const QPointF& currentPosition, const QPointF& mouseMoveStartPos, const QGraphicsRectItem* originalItem, double dx, double dy, QGraphicsRectItem* changeItem );

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
    bool imageSizeConsideringRotation( double& width, double& height ) const;
    /**Calculates corner point after rotation and scaling*/
    bool cornerPointOnRotatedAndScaledRect( double& x, double& y, double width, double height ) const;

    /**Calculates width / height of the bounding box of a rotated rectangle (mRotation)*/
    void sizeChangedByRotation( double& width, double& height );
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
    /**Is emitted on rotation change to notify north arrow pictures*/
    void rotationChanged( double newRotation );
    /**Used e.g. by the item widgets to update the gui elements*/
    void itemChanged();
    /**Emitted if the rectangle changes*/
    void sizeChanged();
  private:
    // id (not unique)
    QString mId;
    // name (unique)
    QString mUuid;

    void init( bool manageZValue );
};

#endif
