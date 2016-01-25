/***************************************************************************
    qgsmaptoolpan.h  -  map tool for panning in map canvas
    ---------------------
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

#ifndef QGSMAPTOOLPAN_H
#define QGSMAPTOOLPAN_H

#include "qgsmaptool.h"

class QgsFeature;
class QgsLabelPosition;
class QgsMapCanvas;
class QgsRubberBand;
class QgsVectorLayer;
class QPinchGesture;


/** \ingroup gui
 * A map tool for panning the map.
 * @see QgsMapTool
 */
class GUI_EXPORT QgsMapToolPan : public QgsMapTool
{
    Q_OBJECT
  public:
    //! constructor
    QgsMapToolPan( QgsMapCanvas* canvas );

    ~QgsMapToolPan();

    void activate() override;
    void deactivate() override;

    //! Overridden mouse double click event
    virtual void canvasDoubleClickEvent( QMouseEvent *e ) override;

    //! Overridden mouse press event
    virtual void canvasPressEvent( QMouseEvent * e ) override;

    //! Overridden mouse move event
    virtual void canvasMoveEvent( QMouseEvent * e ) override;

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QMouseEvent * e ) override;

    //! Overridden key press event
    virtual void keyPressEvent( QKeyEvent *e ) override;

    virtual bool isTransient() override { return true; }

    bool gestureEvent( QGestureEvent *event ) override;

  signals:
    void contextMenuRequested( QPoint screenPos, QgsPoint mapPos );
    void featurePicked( const QgsFeature& feature, QgsVectorLayer* layer );
    void labelPicked( const QgsLabelPosition& labelPos );

  private:

    //! Flag to indicate a map canvas drag operation is taking place
    bool mDragging;

    //! Flag to indicate a pinch gesture is taking place
    bool mPinching;

    //! Zoom are rubberband for shift+select mode
    QgsRubberBand* mZoomRubberBand;

    //! Stores zoom rect for shift+select mode
    QRect mZoomRect;

    //!Flag to indicate whether mouseRelease is a click (i.e. no moves inbetween)
    bool mPickClick;

    //! Current annotation move action
    int mAnnotationMoveAction;

    //! Previous mouse position during move
    QPointF mMouseMoveLastXY;

    //! The pan cursor
    QCursor mPanCursor;

    void pinchTriggered( QPinchGesture *gesture );

};

#endif
