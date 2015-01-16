/***************************************************************************
    qgsmapcanvasitem.h  - base class for map canvas items
    ----------------------
    begin                : February 2006
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

#ifndef QGSMAPCANVASITEM_H
#define QGSMAPCANVASITEM_H

#include <QGraphicsItem>
#include "qgsrectangle.h"

class QgsMapCanvas;
class QgsRenderContext;
class QPainter;

/** \ingroup gui
 * An abstract class for items that can be placed on the
 * map canvas.
 */
class GUI_EXPORT QgsMapCanvasItem : public QGraphicsItem
{
  protected:

    //! protected constructor: cannot be constructed directly
    QgsMapCanvasItem( QgsMapCanvas* mapCanvas );

    virtual ~QgsMapCanvasItem();

    //! function to be implemented by derived classes
    virtual void paint( QPainter * painter ) = 0;

    //! paint function called by map canvas
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;

    //! schedules map canvas for repaint
    void updateCanvas();

    /**Sets render context parameters
    @param p painter for rendering
    @param context out: configured context
    @return true in case of success */
    bool setRenderContextVariables( QPainter* p, QgsRenderContext& context ) const;

  public:

    //! called on changed extent or resize event to update position of the item
    virtual void updatePosition();

    //! default implementation for canvas items
    virtual QRectF boundingRect() const override;

    //! sets current offset, to be called from QgsMapCanvas
    //! @deprecated since v2.4 - not called by QgsMapCanvas anymore
    Q_DECL_DEPRECATED void setPanningOffset( const QPoint& point );

    //! returns canvas item rectangle in map units
    QgsRectangle rect() const;

    //! sets canvas item rectangle in map units
    void setRect( const QgsRectangle& r, bool resetRotation = true );

    //! transformation from screen coordinates to map coordinates
    QgsPoint toMapCoordinates( const QPoint& point ) const;

    //! transformation from map coordinates to screen coordinates
    QPointF toCanvasCoordinates( const QgsPoint& point ) const;

  protected:

    //! pointer to map canvas
    QgsMapCanvas* mMapCanvas;

    //! cached canvas item rectangle in map coordinates
    //! encodes position (xmin,ymax) and size (width/height)
    //! used to re-position and re-size the item on zoom/pan
    //! while waiting for the renderer to complete.
    //!
    //! NOTE: does not include rotation information, so cannot
    //!       be used to correctly present pre-rendered map
    //!       on rotation change
    QgsRectangle mRect;

    double mRectRotation;

    //! offset from normal position due current panning operation,
    //! used when converting map coordinates to move map canvas items
    //! @deprecated since v2.4
    QPoint mPanningOffset;

    //! cached size of the item (to return in boundingRect())
    QSizeF mItemSize;

};


#endif
