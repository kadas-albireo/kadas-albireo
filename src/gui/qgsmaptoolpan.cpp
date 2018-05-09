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
#include "qgsmaptooldeleteitems.h"
#include "qgsrubberband.h"
#include <QBitmap>
#include <QCursor>
#include <QMouseEvent>
#include <QPinchGesture>


QgsMapToolPan::QgsMapToolPan( QgsMapCanvas* canvas , bool allowItemInteraction )
    : QgsMapTool( canvas )
    , mAllowItemInteraction( allowItemInteraction )
    , mDragging( false )
    , mPinching( false )
    , mExtentRubberBand( 0 )
    , mPickClick( false )
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

void QgsMapToolPan::canvasPressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( e->modifiers() == Qt::ShiftModifier || e->modifiers() == Qt::ControlModifier )
    {
      mExtentRubberBand = new QgsRubberBand( mCanvas, QGis::Polygon );
      if ( e->modifiers() == Qt::ShiftModifier )
        mExtentRubberBand->setColor( QColor( 0, 0, 255, 63 ) );
      else if ( e->modifiers() == Qt::ControlModifier )
      {
        QSettings settings;
        int red = settings.value( "/Qgis/default_measure_color_red", 255 ).toInt();
        int green = settings.value( "/Qgis/default_measure_color_green", 0 ).toInt();
        int blue = settings.value( "/Qgis/default_measure_color_blue", 0 ).toInt();
        mExtentRubberBand->setFillColor( QColor( red, green, blue, 63 ) );
        mExtentRubberBand->setBorderColor( QColor( red, green, blue, 255 ) );
        mExtentRubberBand->setWidth( 3 );
      }
      mExtentRect.setTopLeft( e->pos() );
      mExtentRect.setBottomRight( e->pos() );
      mExtentRubberBand->setToCanvasRectangle( mExtentRect );
      mExtentRubberBand->show();
    }
    else if ( mAllowItemInteraction )
    {
      mPickClick = true;
    }
  }
  else if ( e->button() == Qt::RightButton && mAllowItemInteraction )
  {
    emit contextMenuRequested( e->pos(), toMapCoordinates( e->pos() ) );
  }
}

void QgsMapToolPan::canvasMoveEvent( QMouseEvent * e )
{
  mPickClick = false;

  if (( e->buttons() & Qt::LeftButton ) )
  {
    if ( mExtentRubberBand )
    {
      mExtentRect.setBottomRight( e->pos() );
      mExtentRubberBand->setToCanvasRectangle( mExtentRect );
    }
    else
    {
      mDragging = true;
      mCanvas->panAction( e );
      mCanvas->setCursor( QCursor( Qt::ClosedHandCursor ) );
    }
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
    if ( mExtentRubberBand )
    {
      if ( e->modifiers() == Qt::ShiftModifier )
      {
        // set center and zoom
        QSize zoomRectSize = mExtentRect.normalized().size();
        QSize canvasSize = mCanvas->mapSettings().outputSize();
        double sfx = ( double )zoomRectSize.width() / canvasSize.width();
        double sfy = ( double )zoomRectSize.height() / canvasSize.height();
        double scaleFactor = qMax( sfx, sfy );

        QgsPoint c = mCanvas->getCoordinateTransform()->toMapCoordinates( mExtentRect.center() );
        QgsRectangle oe = mCanvas->mapSettings().extent();
        QgsRectangle e(
          c.x() - oe.width() / 2.0, c.y() - oe.height() / 2.0,
          c.x() + oe.width() / 2.0, c.y() + oe.height() / 2.0
        );
        e.scale( scaleFactor, &c );
        mCanvas->setExtent( e, true );
        mCanvas->refresh();
      }
      else if ( e->modifiers() == Qt::ControlModifier )
      {
        QgsMapToolDeleteItems( canvas() ).deleteItems( mExtentRubberBand->rect(), canvas()->mapSettings().destinationCrs() );
      }

      delete mExtentRubberBand;
      mExtentRubberBand = 0;
    }
    else if ( mDragging )
    {
      mCanvas->panActionEnd( e->pos() );
      mDragging = false;
      mCanvas->setCursor( mCursor );
    }
    else if ( mAllowItemInteraction && mPickClick )
    {
      QgsFeaturePicker::PickResult result = QgsFeaturePicker::pick( mCanvas, e->pos(), toMapCoordinates( e->pos() ), QGis::AnyGeometry );
      if ( !result.isEmpty() )
      {
        emit itemPicked( result );
      }
    }
  }
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
