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

#include "qgsmaptoolpan.h"
#include "qgsmapcanvas.h"
#include "qgscursors.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"
#include "qgstextannotationitem.h"
#include <QBitmap>
#include <QCursor>
#include <QMouseEvent>
#include <QPinchGesture>


QgsMapToolPan::QgsMapToolPan( QgsMapCanvas* canvas )
    : QgsMapTool( canvas )
    , mDragging( false )
    , mPinching( false )
    , mZoomRubberBand( 0 )
    , mAnnotationPickClick( false )
    , mAnnotationMoveAction( QgsAnnotationItem::NoAction )
{
  mToolName = tr( "Pan" );
  mPanCursor = QCursor( QBitmap::fromData( QSize( 16, 16 ), pan_bits ),  QBitmap::fromData( QSize( 16, 16 ), pan_mask_bits ), 5, 5 );
  mCursor = mPanCursor;
}

QgsMapToolPan::~QgsMapToolPan()
{
  mCanvas->ungrabGesture( Qt::PinchGesture );
}

void QgsMapToolPan::activate()
{
  mCanvas->grabGesture( Qt::PinchGesture );
  QgsMapTool::activate();
}

void QgsMapToolPan::deactivate()
{
  mCanvas->ungrabGesture( Qt::PinchGesture );
  QgsMapTool::deactivate();
}

void QgsMapToolPan::canvasDoubleClickEvent( QMouseEvent *e )
{
  QgsAnnotationItem* selItem = mCanvas->selectedAnnotationItem();
  if ( selItem && selItem == mCanvas->annotationItemAtPos( e->pos() ) )
  {
    if ( mAnnotationMoveAction != QgsAnnotationItem::NoAction )
    {
      mAnnotationMoveAction = QgsAnnotationItem::NoAction;
    }
    selItem->showItemEditor();
  }
}

void QgsMapToolPan::canvasPressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if (( e->modifiers() & Qt::ShiftModifier ) != 0 )
    {
      mZoomRubberBand = new QgsRubberBand( mCanvas, QGis::Polygon );
      mZoomRubberBand->setColor( QColor( 0, 0, 255, 63 ) );
      mZoomRect.setTopLeft( e->pos() );
      mZoomRect.setBottomRight( e->pos() );
      mZoomRubberBand->setToCanvasRectangle( mZoomRect );
      mZoomRubberBand->show();
    }
    else
    {
      mAnnotationPickClick = true;
      mMouseMoveLastXY = e->pos();

      QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
      if ( e->button() == Qt::LeftButton && selectedItem && selectedItem == mCanvas->annotationItemAtPos( e->pos() ) )
      {
        mAnnotationMoveAction = selectedItem->moveActionForPosition( e->posF() );
      }
    }
  }
  else if ( e->button() == Qt::RightButton )
  {
    // First, try to show menu for annotation items
    QgsAnnotationItem* selItem = canvas()->selectedAnnotationItem();
    if ( selItem && selItem == canvas()->annotationItemAtPos( e->pos() ) )
    {
      selItem->showContextMenu( canvas()->mapToGlobal( e->pos() ) );
    }
    // Otherwise, request application context menu
    else
    {
      emit contextMenuRequested( mCanvas->mapToGlobal( e->pos() ), toMapCoordinates( e->pos() ) );
    }
  }
}

void QgsMapToolPan::canvasMoveEvent( QMouseEvent * e )
{
  QgsAnnotationItem* selAnnotationItem = mCanvas->selectedAnnotationItem();
  mAnnotationPickClick = false;

  if (( e->buttons() & Qt::LeftButton ) )
  {
    if ( mZoomRubberBand )
    {
      mZoomRect.setBottomRight( e->pos() );
      mZoomRubberBand->setToCanvasRectangle( mZoomRect );
    }
    else if ( selAnnotationItem && mAnnotationMoveAction != QgsAnnotationItem::NoAction )
    {
      selAnnotationItem->handleMoveAction( mAnnotationMoveAction, e->posF(), mMouseMoveLastXY );
      mMouseMoveLastXY = e->pos();
    }
    else
    {
      mDragging = true;
      mCanvas->panAction( e );
    }
  }

  if ( selAnnotationItem )
  {
    QgsAnnotationItem::MouseMoveAction moveAction = selAnnotationItem->moveActionForPosition( e -> pos() );
    if ( moveAction != QgsAnnotationItem::NoAction )
      setCursor( QCursor( selAnnotationItem->cursorShapeForAction( moveAction ) ) );
    else
      setCursor( mPanCursor );
  }
  else
  {
    setCursor( mPanCursor );
  }
}

void QgsMapToolPan::canvasReleaseEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( mZoomRubberBand )
    {
      delete mZoomRubberBand;
      mZoomRubberBand = 0;

      // set center and zoom
      QSize zoomRectSize = mZoomRect.normalized().size();
      QSize canvasSize = mCanvas->mapSettings().outputSize();
      double sfx = ( double )zoomRectSize.width() / canvasSize.width();
      double sfy = ( double )zoomRectSize.height() / canvasSize.height();
      mCanvas->setCenter( mCanvas->getCoordinateTransform()->toMapCoordinates( mZoomRect.center() ) );
      mCanvas->zoomByFactor( qMax( sfx, sfy ) );
    }
    else if ( mDragging )
    {
      mCanvas->panActionEnd( e->pos() );
      mDragging = false;
    }
    else if ( mAnnotationPickClick )
    {
      QgsAnnotationItem* annotationItem = mCanvas->annotationItemAtPos( e->pos() );
      QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
      if ( selectedItem )
      {
        selectedItem->setSelected( false );
      }
      if ( annotationItem )
      {
        annotationItem->setSelected( true );
        QgsAnnotationItem::MouseMoveAction moveAction = annotationItem->moveActionForPosition( e -> pos() );
        if ( moveAction != QgsAnnotationItem::NoAction )
          setCursor( QCursor( annotationItem->cursorShapeForAction( moveAction ) ) );
      }
      mAnnotationMoveAction = QgsAnnotationItem::NoAction;
    }
    mAnnotationMoveAction = QgsAnnotationItem::NoAction;
  }
}

void QgsMapToolPan::keyPressEvent( QKeyEvent *e )
{
  switch ( e->key() )
  {
    case Qt::Key_T:
      if ( e->modifiers() == Qt::ControlModifier )
      {
        foreach ( QGraphicsItem* item, mCanvas->items() )
        {
          if ( dynamic_cast<QgsTextAnnotationItem*>( item ) )
          {
            item->setVisible( !item->isVisible() );
          }
        }
        e->ignore();
        break;
      }
      // Fall-through

    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    {
      QgsAnnotationItem* selAnnotationItem = mCanvas->selectedAnnotationItem();
      if ( selAnnotationItem )
      {
        delete selAnnotationItem;
        e->ignore();
        break;
      }
    }
    default:
      break;
  }
  // Fall-through
}

bool QgsMapToolPan::gestureEvent( QGestureEvent *event )
{
  qDebug() << "gesture " << event;
  if ( QGesture *gesture = event->gesture( Qt::PinchGesture ) )
  {
    mPinching = true;
    pinchTriggered( static_cast<QPinchGesture *>( gesture ) );
  }
  return true;
}


void QgsMapToolPan::pinchTriggered( QPinchGesture *gesture )
{
  if ( gesture->state() == Qt::GestureFinished )
  {
    //a very small totalScaleFactor indicates a two finger tap (pinch gesture without pinching)
    if ( 0.98 < gesture->totalScaleFactor()  && gesture->totalScaleFactor() < 1.02 )
    {
      mCanvas->zoomOut();
    }
    else
    {
      //Transfor global coordinates to widget coordinates
      QPoint pos = gesture->centerPoint().toPoint();
      pos = mCanvas->mapFromGlobal( pos );
      // transform the mouse pos to map coordinates
      QgsPoint center  = mCanvas->getCoordinateTransform()->toMapPoint( pos.x(), pos.y() );
      QgsRectangle r = mCanvas->extent();
      r.scale( 1 / gesture->totalScaleFactor(), &center );
      mCanvas->setExtent( r );
      mCanvas->refresh();
    }
    mPinching = false;
  }
}
