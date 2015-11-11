/***************************************************************************
    qgsmaptoolfilter.cpp  -  generic map tool for choosing a filter area
    ----------------------
    begin                : November 2015
    copyright            : (C) 2006 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptoolfilter.h"
#include "qgsrubberband.h"
#include "qgscircularstringv2.h"
#include "qgscurvepolygonv2.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"

#include <QMouseEvent>

QgsMapToolFilter::QgsMapToolFilter( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand )
    : QgsMapTool( canvas ), mMode( mode ), mRubberBand( rubberBand ), mCapturing( false )
{
  setCursor( Qt::CrossCursor );
}

void QgsMapToolFilter::canvasPressEvent( QMouseEvent * e )
{
  if ( mCapturing && mMode == Poly )
  {
    if ( e->button() == Qt::RightButton )
    {
      mCapturing = false;
      mCanvas->unsetMapTool( this );
    }
    else
    {
      mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
    }
  }
  else if ( e->button() == Qt::LeftButton )
  {
    mPressPos = e->pos();
    mCapturing = true;
    if ( mMode == Poly )
    {
      mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
      mRubberBand->addPoint( toMapCoordinates( e->pos() ) );
    }
  }
}

void QgsMapToolFilter::canvasMoveEvent( QMouseEvent * e )
{
  if ( mCapturing )
  {
    if ( mMode == Circle )
    {
      QPoint diff = mPressPos - e->pos();
      double r = qSqrt( diff.x() * diff.x() + diff.y() + diff.y() );
      QgsCircularStringV2* exterior = new QgsCircularStringV2();
      exterior->setPoints(
        QList<QgsPointV2>() << toMapCoordinates( QPoint( mPressPos.x() + r, mPressPos.y() ) )
        << toMapCoordinates( mPressPos )
        << toMapCoordinates( QPoint( mPressPos.x() + r, mPressPos.y() ) ) );
      QgsCurvePolygonV2 geom;
      geom.setExteriorRing( exterior );
      QgsGeometry g( geom.segmentize() );
      mRubberBand->setToGeometry( &g, 0 );
    }
    else if ( mMode == Rect )
    {
      mRubberBand->setToCanvasRectangle( QRect( mPressPos, e->pos() ).normalized() );
    }
    else if ( mMode == Poly )
    {
      mRubberBand->movePoint( mRubberBand->partSize( 0 ) - 1, toMapCoordinates( e->pos() ) );
    }
  }
}

void QgsMapToolFilter::canvasReleaseEvent( QMouseEvent * /*e*/ )
{
  if ( mMode != Poly )
  {
    mCapturing = false;
    mCanvas->unsetMapTool( this );
  }
}

void QgsMapToolFilter::deactivate()
{
  // Deactivated while capture, clear rubberband
  if ( mCapturing )
    mRubberBand->reset( QGis::Polygon );
  QgsMapTool::deactivate();
}
