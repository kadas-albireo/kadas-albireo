/***************************************************************************
                         qgsdxpaintengine.cpp
                         --------------------
    begin                : November 2013
    copyright            : (C) 2013 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdxfpaintengine.h"
#include "qgsdxfexport.h"
#include "qgsdxfpaintdevice.h"
#include "qgslogger.h"

QgsDxfPaintEngine::QgsDxfPaintEngine( const QgsDxfPaintDevice* dxfDevice, QgsDxfExport* dxf )
    : QPaintEngine( QPaintEngine::AllFeatures /*QPaintEngine::PainterPaths | QPaintEngine::PaintOutsidePaintEvent*/ )
    , mPaintDevice( dxfDevice ), mDxf( dxf )
{

}

QgsDxfPaintEngine::~QgsDxfPaintEngine()
{

}

bool QgsDxfPaintEngine::begin( QPaintDevice* pdev )
{
  Q_UNUSED( pdev );
  return true;
}

bool QgsDxfPaintEngine::end()
{
  return true;
}

QPaintEngine::Type QgsDxfPaintEngine::type() const
{
  return QPaintEngine::User;
}

void QgsDxfPaintEngine::drawPixmap( const QRectF& r, const QPixmap& pm, const QRectF& sr )
{
  Q_UNUSED( r ); Q_UNUSED( pm ); Q_UNUSED( sr );
}

void QgsDxfPaintEngine::updateState( const QPaintEngineState& state )
{
  if ( state.state() & QPaintEngine::DirtyTransform )
  {
    mTransform = state.transform();
  }
  if ( state.state() & QPaintEngine::DirtyPen )
  {
    mPen = state.pen();
  }
  if ( state.state() & QPaintEngine::DirtyBrush )
  {
    mBrush = state.brush();
  }
}

void QgsDxfPaintEngine::drawPolygon( const QPointF* points, int pointCount, PolygonDrawMode mode )
{
  Q_UNUSED( mode );
  if ( !mDxf || !mPaintDevice )
  {
    return;
  }

  QgsPolygon polygon( 1 );
  polygon[0].resize( pointCount );

  QgsPolyline &polyline = polygon[0];
  for ( int i = 0; i < pointCount; ++i )
  {
    polyline[i] = toDxfCoordinates( points[i] );
  }

  mDxf->writePolygon( polygon, mLayer, "SOLID", currentColor() );
}

void QgsDxfPaintEngine::drawRects( const QRectF* rects, int rectCount )
{
  if ( !mDxf || !mPaintDevice || !rects )
  {
    return;
  }

  for ( int i = 0; i < rectCount; ++i )
  {
    double left = rects[i].left();
    double right = rects[i].right();
    double top = rects[i].top();
    double bottom = rects[i].bottom();
    QgsPoint pt1 = toDxfCoordinates( QPointF( left, bottom ) );
    QgsPoint pt2 = toDxfCoordinates( QPointF( right, bottom ) );
    QgsPoint pt3 = toDxfCoordinates( QPointF( left, top ) );
    QgsPoint pt4 = toDxfCoordinates( QPointF( right, top ) );
    mDxf->writeSolid( mLayer, currentColor(), pt1, pt2, pt3, pt4 );
  }
}

void QgsDxfPaintEngine::drawPath( const QPainterPath& path )
{
  int pathLength = path.elementCount();
  for ( int i = 0; i < pathLength; ++i )
  {
    const QPainterPath::Element& pathElem = path.elementAt( i );
    if ( pathElem.type == QPainterPath::MoveToElement )
    {
      moveTo( pathElem.x, pathElem.y );
    }
    else if ( pathElem.type == QPainterPath::LineToElement )
    {
      lineTo( pathElem.x, pathElem.y );
    }
    else if ( pathElem.type == QPainterPath::CurveToElement )
    {
      curveTo( pathElem.x, pathElem.y );
    }
    else if ( pathElem.type == QPainterPath::CurveToDataElement )
    {
      mCurrentCurve.append( QPointF( pathElem.x, pathElem.y ) );
    }
  }
  endCurve();
  endPolygon();
}

void QgsDxfPaintEngine::moveTo( double dx, double dy )
{
  endCurve();
  endPolygon();
  mCurrentPolygon.append( QPointF( dx, dy ) );
}

void QgsDxfPaintEngine::lineTo( double dx, double dy )
{
  endCurve();
  mCurrentPolygon.append( QPointF( dx, dy ) );
}

void QgsDxfPaintEngine::curveTo( double dx, double dy )
{
  endCurve();
  if ( mCurrentPolygon.size() > 0 )
  {
    mCurrentCurve.append( mCurrentPolygon.last() );
  }
  mCurrentCurve.append( QPointF( dx, dy ) );
}

void QgsDxfPaintEngine::endPolygon()
{
  if ( mCurrentPolygon.size() > 1 )
  {
    drawPolygon( mCurrentPolygon.constData(), mCurrentPolygon.size(), QPaintEngine::OddEvenMode );
  }
  mCurrentPolygon.clear();
}

void QgsDxfPaintEngine::endCurve()
{
  if ( mCurrentCurve.size() < 1 )
  {
    return;
  }
  if ( mCurrentPolygon.size() < 1 )
  {
    mCurrentCurve.clear();
    return;
  }
  //mCurrentCurve.prepend( mCurrentPolygon.last() );

  if ( mCurrentCurve.size() >= 3 )
  {
    double t = 0.05;
    for ( int i = 1; i < 20; ++i ) //approximate curve with 20 segments
    {
      mCurrentPolygon.append( bezierPoint( mCurrentCurve, t ) );
      t += 0.05;
    }
  }
  else if ( mCurrentCurve.size() == 2 )
  {
    mCurrentPolygon.append( mCurrentCurve.at( 1 ) );
  }
  mCurrentCurve.clear();
}

void QgsDxfPaintEngine::drawLines( const QLineF* lines, int lineCount )
{
  if ( !mDxf || !mPaintDevice || !lines )
  {
    return;
  }

  for ( int i = 0; i < lineCount; ++i )
  {
    QgsPoint pt1 = toDxfCoordinates( lines[i].p1() );
    QgsPoint pt2 = toDxfCoordinates( lines[i].p2() );
    mDxf->writeLine( pt1, pt2, mLayer, "CONTINUOUS", currentColor(), currentWidth() );
  }
}

QgsPoint QgsDxfPaintEngine::toDxfCoordinates( const QPointF& pt ) const
{
  if ( !mPaintDevice || !mDxf )
  {
    return QgsPoint( pt.x(), pt.y() );
  }

  QPointF dxfPt = mPaintDevice->dxfCoordinates( mTransform.map( pt ) ) + mShift;
  return QgsPoint( dxfPt.x(), dxfPt.y() );
}

QColor QgsDxfPaintEngine::currentColor() const
{
  if ( !mDxf )
  {
    return QColor();
  }

  QColor c = mPen.color();
  if ( mPen.style() == Qt::NoPen )
  {
    c = mBrush.color();
  }
  return c;
}

double QgsDxfPaintEngine::currentWidth() const
{
  if ( !mPaintDevice )
  {
    return 1;
  }

  return mPen.widthF() * mPaintDevice->widthScaleFactor();
}

QPointF QgsDxfPaintEngine::bezierPoint( const QList<QPointF>& controlPolygon, double t )
{
  double x = 0;
  double y = 0;
  int cPolySize = controlPolygon.size();
  double bPoly  = 0;

  QList<QPointF>::const_iterator it = controlPolygon.constBegin();
  int i = 0;
  for ( ; it != controlPolygon.constEnd(); ++it )
  {
    bPoly = bernsteinPoly( cPolySize - 1, i, t );
    x += ( it->x() * bPoly );
    y += ( it->y() * bPoly );
    ++i;
  }

  return QPointF( x, y );
}

double QgsDxfPaintEngine::bernsteinPoly( int n, int i, double t )
{
  if ( i < 0 )
  {
    return 0;
  }

  return lower( n, i )*power( t, i )*power(( 1 - t ), ( n - i ) );
}

int QgsDxfPaintEngine::lower( int n, int i )
{
  if ( i >= 0 && i <= n )
  {
    return faculty( n ) / ( faculty( i )*faculty( n - i ) );
  }
  else
  {
    return 0;
  }
}

double QgsDxfPaintEngine::power( double a, int b )
{
  if ( b == 0 )
  {
    return 1;
  }
  double tmp = a;
  for ( int i = 2; i <= qAbs(( double )b ); i++ )
  {
    a *= tmp;
  }
  if ( b > 0 )
  {
    return a;
  }
  else
  {
    return 1.0 / a;
  }
}

int QgsDxfPaintEngine::faculty( int n )
{
  if ( n < 0 )//Is faculty also defined for negative integers?
  {
    return 0;
  }
  int i;
  int result = n;

  if ( n == 0 || n == 1 )
  {
    return 1;
  }//faculty of 0 is 1!

  for ( i = n - 1; i >= 2; i-- )
  {
    result *= i;
  }
  return result;
}
