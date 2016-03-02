/***************************************************************************
                              qgsannotationitem.h
                              ------------------------
  begin                : February 9, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSANNOTATIONITEM_H
#define QGSANNOTATIONITEM_H

#include "qgsmapcanvasitem.h"
#include "qgscoordinatereferencesystem.h"

class QDomDocument;
class QDomElement;
class QDialog;
class QgsVectorLayer;
class QgsMarkerSymbolV2;

#define QGS_ANNOTATION_ITEM(classname, xmlname) \
  static QgsAnnotationItem* classname##Factory(QgsMapCanvas* mapCanvas){ return new classname(mapCanvas); } \
  static AnnotationItemRegisterer* registerAnnotationItem(){ static AnnotationItemRegisterer reg(xmlname, classname##Factory); return &reg; } \
  static AnnotationItemRegisterer* sAnnotationItemRegistryItem;

#define REGISTER_QGS_ANNOTATION_ITEM(classname) classname::AnnotationItemRegisterer* classname::sAnnotationItemRegistryItem = classname::registerAnnotationItem();


/**An annotation item can be either placed either on screen corrdinates or on map coordinates.
  It may reference a feature and displays that associatiation with a balloon like appearance*/
class GUI_EXPORT QgsAnnotationItem: public QObject, public QgsMapCanvasItem
{
    Q_OBJECT
  public:
    enum FeatureFlags
    {
      ItemAllFeatures = 0,
      ItemIsNotResizeable = 1,
      ItemHasNoFrame = 2,
      ItemHasNoMarker = 4,
      ItemIsNotEditable = 8
    };

    enum MouseMoveAction
    {
      NoAction,
      MoveMapPosition,
      MoveFramePosition,
      ResizeFrameUp,
      ResizeFrameDown,
      ResizeFrameLeft,
      ResizeFrameRight,
      ResizeFrameLeftUp,
      ResizeFrameRightUp,
      ResizeFrameLeftDown,
      ResizeFrameRightDown,
      NumMouseMoveActions
    };

    typedef QgsAnnotationItem*( *AnnotationItemFactory_t )( QgsMapCanvas* );
    typedef QPair<QString, AnnotationItemFactory_t> AnnotationRegistryItem;
    static const QList<AnnotationRegistryItem>& registeredAnnotations() { return sRegisteredAnnotations; }

    QgsAnnotationItem( QgsMapCanvas* mapCanvas );
    virtual ~QgsAnnotationItem();

    virtual QgsAnnotationItem* clone( QgsMapCanvas* /*canvas*/ ) = 0;

    void updatePosition() override;

    QRectF boundingRect() const override;

    virtual QSizeF minimumFrameSize() const;

    /**Returns the mouse move behaviour for a given position
      @param pos the position in scene coordinates*/
    virtual int moveActionForPosition( const QPointF& pos ) const;
    /**Handles the mouse move action*/
    virtual void handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos );
    /**Returns suitable cursor shape for mouse move action*/
    virtual Qt::CursorShape cursorShapeForAction( int moveAction ) const;

    //setters and getters
    void setMapPositionFixed( bool fixed );
    bool mapPositionFixed() const { return mMapPositionFixed; }

    virtual void setMapPosition( const QgsPoint& pos , const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() );
    const QgsPoint& mapPosition() const { return mMapPosition; }
    const QgsPoint& mapGeoPos() const { return mGeoPos; }
    const QgsCoordinateReferenceSystem& mapGeoPosCrs() const { return mGeoPosCrs; }


    void setOffsetFromReferencePoint( const QPointF& offset );
    QPointF offsetFromReferencePoint() const { return mOffsetFromReferencePoint; }

    /**Set symbol that is drawn on map position. Takes ownership*/
    void setMarkerSymbol( QgsMarkerSymbolV2* symbol );
    const QgsMarkerSymbolV2* markerSymbol() const {return mMarkerSymbol;}

    void setFrameSize( const QSizeF& size );
    QSizeF frameSize() const { return mFrameSize; }

    void setFrameBorderWidth( double w ) { mFrameBorderWidth = w; }
    double frameBorderWidth() const { return mFrameBorderWidth; }

    void setFrameColor( const QColor& c ) { mFrameColor = c; }
    QColor frameColor() const { return mFrameColor; }

    void setFrameBackgroundColor( const QColor& c ) { mFrameBackgroundColor = c; }
    QColor frameBackgroundColor() const { return mFrameBackgroundColor; }

    virtual void writeXML( QDomDocument& doc ) const = 0;
    virtual void readXML( const QDomDocument& doc, const QDomElement& itemElem ) = 0;

    void _writeXML( QDomDocument& doc, QDomElement& itemElem ) const;
    void _readXML( const QDomDocument& doc, const QDomElement& annotationElem );

    void setItemFlags( int flags ) { mFlags = flags; }
    int itemFlags() const { return mFlags; }

    void showItemEditor();
    virtual void showContextMenu( const QPoint& screenPos );
    virtual bool hitTest( const QPoint& /*screenPos*/ ) const { return true; }

  signals:
    void itemUpdated( QgsAnnotationItem* item );

  protected:
    QgsAnnotationItem( QgsMapCanvas* canvas, QgsAnnotationItem* source );

    static QList< QPair<QString, AnnotationItemFactory_t> > sRegisteredAnnotations;

    struct AnnotationItemRegisterer
    {
      AnnotationItemRegisterer( const QString& name, AnnotationItemFactory_t factory )
      { sRegisteredAnnotations.append( qMakePair( name, factory ) ); }
    };

    /**Flags specifying the features of the item*/
    int mFlags;

    /**True: the item stays at the same map position, False: the item stays on same screen position*/
    bool mMapPositionFixed;

    /**Map position (in case mMapPositionFixed is true)*/
    QgsPoint mMapPosition;

    /** The georeferenced position of the item. mMapPosition is this position always transformed to the
     * current destination crs */
    QgsPoint mGeoPos;
    /**Coordinate reference system of position (in case mMapPositionFixed is true)*/
    QgsCoordinateReferenceSystem mGeoPosCrs;

    /**Describes the shift of the item content box to the reference point*/
    QPointF mOffsetFromReferencePoint;

    /**Bounding rect (including item frame and balloon)*/
    QRectF mBoundingRect;

    /**Point symbol that is to be drawn at the map reference location*/
    QgsMarkerSymbolV2* mMarkerSymbol;

    /**Size of the frame (without balloon)*/
    QSizeF mFrameSize;
    /**Width of the frame*/
    double mFrameBorderWidth;
    /**Frame / balloon color*/
    QColor mFrameColor;
    QColor mFrameBackgroundColor;

    /**Segment number where the connection to the map point is attached. -1 if no balloon needed (e.g. if point is contained in frame)*/
    int mBalloonSegment;
    /**First segment point for drawing the connection (ccw direction)*/
    QPointF mBalloonSegmentPoint1;
    /**Second segment point for drawing the balloon connection (ccw direction)*/
    QPointF mBalloonSegmentPoint2;

    void updateBoundingRect();
    /**Check where to attach the balloon connection between frame and map point*/
    void updateBalloon();

    void drawFrame( QPainter* p );
    void drawMarkerSymbol( QPainter* p );
    void drawSelectionBoxes( QPainter* p );
    /**Returns frame width in painter units*/
    //double scaledFrameWidth( QPainter* p) const;
    /**Gets the frame line (0 is the top line, 1 right, 2 bottom, 3 left)*/
    QLineF segment( int index );
    /**Returns a point on the line from startPoint to directionPoint that is a certain distance away from the starting point*/
    QPointF pointOnLineWithDistance( const QPointF& startPoint, const QPointF& directionPoint, double distance ) const;
    /**Returns the symbol size scaled in (mapcanvas) pixels. Used for the counding rect calculation*/
    double scaledSymbolSize() const;

  private:
    virtual void _showItemEditor() = 0;
    void notifyItemUpdated();

  private slots:
    void syncGeoPos();
};

#endif // QGSANNOTATIONITEM_H
