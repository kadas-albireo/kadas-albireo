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
#include <QBitmap>
#include <QCursor>
#include <QMouseEvent>


QgsMapToolPan::QgsMapToolPan( QgsMapCanvas* canvas )
    : QgsMapTool( canvas )
    , mDragging( false )
    , mZoomRubberBand( 0 )
{
  mToolName = tr( "Pan" );
  // set cursor
  QBitmap panBmp = QBitmap::fromData( QSize( 16, 16 ), pan_bits );
  QBitmap panBmpMask = QBitmap::fromData( QSize( 16, 16 ), pan_mask_bits );
  mCursor = QCursor( panBmp, panBmpMask, 5, 5 );
}

void QgsMapToolPan::canvasPressEvent( QMouseEvent * e )
{
  if (( e->buttons() & Qt::LeftButton ) && ( e->modifiers() & Qt::ShiftModifier ) != 0 )
  {
    mZoomRubberBand = new QgsRubberBand( mCanvas, QGis::Polygon );
    mZoomRubberBand->setColor( QColor( 0, 0, 255, 63 ) );
    mZoomRect.setTopLeft( e->pos() );
    mZoomRect.setBottomRight( e->pos() );
    mZoomRubberBand->setToCanvasRectangle( mZoomRect );
    mZoomRubberBand->show();
  }
}

void QgsMapToolPan::canvasMoveEvent( QMouseEvent * e )
{
  if (( e->buttons() & Qt::LeftButton ) )
  {
    if ( mZoomRubberBand )
    {
      mZoomRect.setBottomRight( e->pos() );
      mZoomRubberBand->setToCanvasRectangle( mZoomRect );
    }
    else
    {
      mDragging = true;
      mCanvas->panAction( e );
    }
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
    else // add pan to mouse cursor
    {
      // transform the mouse pos to map coordinates
      QgsPoint center = mCanvas->getCoordinateTransform()->toMapPoint( e->x(), e->y() );
      mCanvas->setCenter( center );
      mCanvas->refresh();
    }
  }
}
