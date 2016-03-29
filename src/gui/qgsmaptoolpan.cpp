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

#include "qgsfeaturepicker.h"
#include "qgspallabeling.h"
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


QgsMapToolPan::QgsMapToolPan( QgsMapCanvas* canvas , bool allowItemInteraction )
    : QgsMapTool( canvas )
    , mAllowItemInteraction( allowItemInteraction )
    , mDragging( false )
    , mPinching( false )
    , mZoomRubberBand( 0 )
    , mPickClick( false )
    , mAnnotationMoveAction( QgsAnnotationItem::NoAction )
{
  mToolName = tr( "Pan" );
  setCursor( QCursor( Qt::ArrowCursor ) );
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
  if ( mAllowItemInteraction )
  {
    QgsAnnotationItem* selItem = mCanvas->selectedAnnotationItem();
    if ( selItem && selItem == mCanvas->annotationItemAtPos( e->pos() ) )
    {
      mAnnotationMoveAction = QgsAnnotationItem::NoAction;
      selItem->showItemEditor();
    }
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
    else if ( mAllowItemInteraction )
    {
      mPickClick = true;
      mMouseMoveLastXY = e->pos();

      QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
      if ( e->button() == Qt::LeftButton && selectedItem && selectedItem == mCanvas->annotationItemAtPos( e->pos() ) )
      {
        mAnnotationMoveAction = selectedItem->moveActionForPosition( e->posF() );
      }
    }
  }
  else if ( e->button() == Qt::RightButton && mAllowItemInteraction )
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
  QgsAnnotationItem* selAnnotationItem = mAllowItemInteraction ? mCanvas->selectedAnnotationItem() : 0;
  mPickClick = false;

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
      mCanvas->setCursor( QCursor( Qt::ClosedHandCursor ) );
    }
  }
  else if ( selAnnotationItem )
  {
    int moveAction = selAnnotationItem->moveActionForPosition( e -> pos() );
    if ( moveAction != QgsAnnotationItem::NoAction )
      mCanvas->setCursor( QCursor( selAnnotationItem->cursorShapeForAction( moveAction ) ) );
    else
      mCanvas->setCursor( mCursor );
  }
  else
  {
    mCanvas->setCursor( mCursor );
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
      mCanvas->setCursor( mCursor );
    }
    else if ( mAllowItemInteraction && mPickClick )
    {
      QgsAnnotationItem* annotationItem = mCanvas->annotationItemAtPos( e->pos() );
      QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
      if ( selectedItem )
      {
        selectedItem->setSelected( false );
      }
      // Handle pick on annotation item if any
      if ( annotationItem )
      {
        annotationItem->setSelected( true );
        int moveAction = annotationItem->moveActionForPosition( e -> pos() );
        if ( moveAction != QgsAnnotationItem::NoAction )
          mCanvas->setCursor( QCursor( annotationItem->cursorShapeForAction( moveAction ) ) );
      }
      // Otherwise pick label / feature
      else
      {
        const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
        QList<QgsLabelPosition> labelPositions = labelingResults ? labelingResults->labelsAtPosition( toMapCoordinates( e->pos() ) ) : QList<QgsLabelPosition>();
        if ( !labelPositions.isEmpty() )
        {
          emit labelPicked( labelPositions.first() );
        }
        else
        {

          QgsFeaturePicker::PickResult result = QgsFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), QGis::AnyGeometry );
          if ( result.layer )
          {
            emit featurePicked( result.layer, result.feature, result.otherResult );
          }
        }
      }
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
      QgsAnnotationItem* selAnnotationItem = mAllowItemInteraction ? mCanvas->selectedAnnotationItem() : 0;
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
