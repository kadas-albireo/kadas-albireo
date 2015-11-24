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
#include "qgscompoundcurvev2.h"
#include "qgslinestringv2.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"

#include <QMouseEvent>

void QgsMapToolFilter::PolyData::updateRubberband( QgsRubberBand* rubberband )
{
  rubberband->reset( true );
  for ( int i = 0, n = points.size(); i < n; ++i )
  {
    rubberband->addPoint( points[i], i == n - 1 );
  }
}

void QgsMapToolFilter::RectData::updateRubberband( QgsRubberBand* rubberband )
{
  rubberband->reset( true );
  rubberband->addPoint( p1, false );
  rubberband->addPoint( QgsPoint( p2.x(), p1.y() ), false );
  rubberband->addPoint( p2, false );
  rubberband->addPoint( QgsPoint( p1.x(), p2.y() ), true );
}

void QgsMapToolFilter::CircleData::updateRubberband( QgsRubberBand* rubberband )
{
  QgsCircularStringV2* exterior = new QgsCircularStringV2();
  exterior->setPoints(
    QList<QgsPointV2>() << QgsPoint( center.x() + radius, center.y() )
    << center
    << QgsPoint( center.x() + radius, center.y() ) );
  QgsCurvePolygonV2 geom;
  geom.setExteriorRing( exterior );
  QgsGeometry g( geom.segmentize() );
  rubberband->setToGeometry( &g, 0 );
}

void QgsMapToolFilter::CircularSectorData::updateRubberband( QgsRubberBand *rubberband )
{
  if ( stage == CircularSectorData::HaveCenter )
  {
    rubberband->reset();
    QgsPoint point( center.x() + radius * qCos( startAngle ), center.y() + radius * qSin( startAngle ) );
    rubberband->addPoint( center );
    rubberband->addPoint( point );
  }
  else if ( stage == CircularSectorData::HaveArc )
  {
    double alphaMid = 0.5 * ( startAngle + stopAngle );
    QgsPoint pStart( center.x() + radius * qCos( startAngle ),
                     center.y() + radius * qSin( startAngle ) );
    QgsPoint pMid( center.x() + radius * qCos( alphaMid ),
                   center.y() + radius * qSin( alphaMid ) );
    QgsPoint pEnd( center.x() + radius * qCos( stopAngle ),
                   center.y() + radius * qSin( stopAngle ) );
    QgsCircularStringV2* arc = new QgsCircularStringV2();
    arc->setPoints( QList<QgsPointV2>() << pStart << pMid << pEnd );
    QgsCompoundCurveV2* exterior = new QgsCompoundCurveV2();
    exterior->addCurve( arc );
    QgsLineStringV2* line = new QgsLineStringV2();
    line->setPoints( QList<QgsPointV2>() << pEnd << center << pStart );
    exterior->addCurve( line );
    QgsCurvePolygonV2 geom;
    geom.setExteriorRing( exterior );
    QgsGeometry g( geom.segmentize() );
    rubberband->setToGeometry( &g, 0 );
  }
}


QgsMapToolFilter::QgsMapToolFilter( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand, FilterData* filterData )
    : QgsMapTool( canvas ), mMode( mode ), mRubberBand( rubberBand ), mCapturing( false ), mOwnFilterData( filterData == 0 ), mFilterData( filterData )
{
  if ( !filterData )
  {
    switch ( mode )
    {
      case Poly:
        mFilterData = new PolyData; break;
      case Rect:
        mFilterData = new RectData; break;
      case Circle:
        mFilterData = new CircleData; break;
      case CircularSector:
        mFilterData = new CircularSectorData; break;
    }
  }
  setCursor( Qt::CrossCursor );
}

QgsMapToolFilter::~QgsMapToolFilter()
{
  if ( mOwnFilterData ) delete mFilterData;
}

void QgsMapToolFilter::canvasPressEvent( QMouseEvent * e )
{
  QgsPoint mousePos = toMapCoordinates( e->pos() );
  if ( mCapturing && mMode == Poly )
  {
    if ( e->button() == Qt::RightButton )
    {
      mCapturing = false;
      mCanvas->unsetMapTool( this );
    }
    else
    {
      static_cast<PolyData*>( mFilterData )->points.append( mousePos );
      static_cast<PolyData*>( mFilterData )->updateRubberband( mRubberBand );
    }
  }
  else if ( e->button() == Qt::LeftButton )
  {
    mCapturing = true;
    if ( mMode == Poly )
    {
      static_cast<PolyData*>( mFilterData )->points.append( mousePos );
      static_cast<PolyData*>( mFilterData )->points.append( mousePos );
    }
    else if ( mMode == Rect )
    {
      static_cast<RectData*>( mFilterData )->p1 = mousePos;
    }
    else if ( mMode == Circle )
    {
      static_cast<CircleData*>( mFilterData )->center = mousePos;
    }
    else if ( mMode == CircularSector )
    {
      CircularSectorData* filterData = static_cast<CircularSectorData*>( mFilterData );
      if ( filterData->stage == CircularSectorData::Empty )
      {
        filterData->stage = CircularSectorData::HaveCenter;
        filterData->center = mousePos;
      }
      else if ( filterData->stage == CircularSectorData::HaveCenter )
      {
        filterData->stage = CircularSectorData::HaveArc;
      }
      else if ( filterData->stage == CircularSectorData::HaveArc )
      {
        mCapturing = false;
        mCanvas->unsetMapTool( this );
      }
    }
  }
}

void QgsMapToolFilter::canvasMoveEvent( QMouseEvent * e )
{
  QgsPoint mousePos = toMapCoordinates( e->pos() );
  if ( mCapturing )
  {
    if ( mMode == Poly )
    {
      static_cast<PolyData*>( mFilterData )->points.last() = mousePos;
      static_cast<PolyData*>( mFilterData )->updateRubberband( mRubberBand );
    }
    else if ( mMode == Rect )
    {
      static_cast<RectData*>( mFilterData )->p2 = mousePos;
      static_cast<RectData*>( mFilterData )->updateRubberband( mRubberBand );
    }
    else if ( mMode == Circle )
    {
      CircleData* circleData = static_cast<CircleData*>( mFilterData );
      circleData->radius = qSqrt( mousePos.sqrDist( circleData->center ) );
      circleData->updateRubberband( mRubberBand );
    }
    else if ( mMode == CircularSector )
    {
      CircularSectorData* csectorData = static_cast<CircularSectorData*>( mFilterData );
      if ( csectorData->stage == CircularSectorData::HaveCenter )
      {
        csectorData->radius = qSqrt( mousePos.sqrDist( csectorData->center ) );
        csectorData->startAngle = qAtan2( mousePos.y() - csectorData->center.y(), mousePos.x() - csectorData->center.x() );
        csectorData->updateRubberband( mRubberBand );
      }
      else if ( csectorData->stage == CircularSectorData::HaveArc )
      {
        csectorData->stopAngle = qAtan2( mousePos.y() - csectorData->center.y(), mousePos.x() - csectorData->center.x() );
        if ( csectorData->stopAngle > csectorData->startAngle + M_PI )
        {
          csectorData->stopAngle -= 2 * M_PI;
        }
        else if ( csectorData->stopAngle < csectorData->startAngle - M_PI )
        {
          csectorData->stopAngle += 2 * M_PI;
        }
        csectorData->updateRubberband( mRubberBand );
      }
    }
  }
}

void QgsMapToolFilter::canvasReleaseEvent( QMouseEvent * /*e*/ )
{
  if ( mMode != Poly && mMode != CircularSector )
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
