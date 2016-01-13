/***************************************************************************
    qgsmaptooldrawshape.cpp  -  Generic map tool for drawing shapes
    ---------------------------------------------------------------
  begin                : Nov 26, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptooldrawshape.h"
#include "qgscircularstringv2.h"
#include "qgscompoundcurvev2.h"
#include "qgslinestringv2.h"
#include "qgsmultilinestringv2.h"
#include "qgsmultipolygonv2.h"
#include "qgsmultipointv2.h"
#include "qgspolygonv2.h"
#include "qgsrubberband.h"
#include "qgsdistancearea.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgssnappingutils.h"

#include <QMouseEvent>
#include <qmath.h>

QgsMapToolDrawShape::QgsMapToolDrawShape( QgsMapCanvas *canvas, bool isArea )
    : QgsMapTool( canvas ), mState( StateReady ), mIsArea( isArea ), mMultipart( false ), mSnapPoints( false )
{
  setCursor( Qt::ArrowCursor );

  QSettings settings;
  int red = settings.value( "/qgis/default_measure_color_red", 255 ).toInt();
  int green = settings.value( "/qgis/default_measure_color_green", 0 ).toInt();
  int blue = settings.value( "/qgis/default_measure_color_blue", 0 ).toInt();

  mRubberBand = new QgsGeometryRubberBand( canvas, isArea ? QGis::Polygon : QGis::Line );
  mRubberBand->setFillColor( QColor( red, green, blue, 63 ) );
  mRubberBand->setOutlineColor( QColor( red, green, blue, 255 ) );
  mRubberBand->setOutlineWidth( 3 );

  setShowNodes( false );
}

QgsMapToolDrawShape::~QgsMapToolDrawShape()
{
  delete mRubberBand;
}

void QgsMapToolDrawShape::setShowNodes( bool showNodes )
{
  if ( showNodes )
  {
    mRubberBand->setIconType( showNodes ? QgsGeometryRubberBand::ICON_CIRCLE : QgsGeometryRubberBand::ICON_NONE );
    mRubberBand->setIconSize( 10 );
    mRubberBand->setIconFillColor( Qt::white );
  }
}

void QgsMapToolDrawShape::setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits , QgsGeometryRubberBand::AngleUnit angleUnits )
{
  mRubberBand->setMeasurementMode( measurementMode, displayUnits, angleUnits );
  emit geometryChanged();
}

void QgsMapToolDrawShape::update()
{
  mRubberBand->setGeometry( createGeometry( mCanvas->mapSettings().destinationCrs() ) );
  emit geometryChanged();
}

void QgsMapToolDrawShape::reset()
{
  clear();
  mRubberBand->setGeometry( 0 );
  mState = StateReady;
  emit geometryChanged();
}

void QgsMapToolDrawShape::canvasPressEvent( QMouseEvent* e )
{
  if ( mState == StateFinished )
  {
    reset();
  }
  mState = buttonEvent( transformPoint( e->pos() ), true, e->button() );
  update();
  if ( mState == StateFinished )
  {
    emit finished();
  }
}

void QgsMapToolDrawShape::canvasMoveEvent( QMouseEvent* e )
{
  if ( mState == StateDrawing )
  {
    moveEvent( transformPoint( e->pos() ) );
    update();
  }
}

void QgsMapToolDrawShape::canvasReleaseEvent( QMouseEvent* e )
{
  if ( mState != StateFinished )
  {
    mState = buttonEvent( transformPoint( e->pos() ), false, e->button() );
    update();
    if ( mState == StateFinished )
    {
      emit finished();
    }
  }
}

QgsPoint QgsMapToolDrawShape::transformPoint( const QPoint& p )
{
  if ( !mSnapPoints )
  {
    return toMapCoordinates( p );
  }
  QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToMap( p );
  return m.isValid() ? m.point() : toMapCoordinates( p );
}

void QgsMapToolDrawShape::addGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateReferenceSystem& sourceCrs )
{
  if ( !mMultipart )
  {
    reset();
  }
  doAddGeometry( geometry, QgsCoordinateTransform( sourceCrs, canvas()->mapSettings().destinationCrs() ) );
  mState = mMultipart ? StateReady : StateFinished;
  update();
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawPoint::QgsMapToolDrawPoint( QgsMapCanvas *canvas )
    : QgsMapToolDrawShape( canvas, false )
{
  mRubberBand->setIconType( QgsGeometryRubberBand::ICON_CIRCLE );
}

QgsMapToolDrawShape::State QgsMapToolDrawPoint::buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton /*button*/ )
{
  if ( !press )
  {
    mPoints.append( QList<QgsPoint>() << pos );
    return mMultipart ? StateReady : StateFinished;
  }
  return mState;
}

void QgsMapToolDrawPoint::moveEvent( const QgsPoint& /*pos*/ )
{
}

QgsAbstractGeometryV2* QgsMapToolDrawPoint::createGeometry( const QgsCoordinateReferenceSystem &targetCrs ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPointV2();
  foreach ( const QList<QgsPoint>& part, mPoints )
  {
    if ( part.size() > 0 )
    {
      QgsPoint p = t.transform( part.front() );
      multiGeom->addGeometry( new QgsPointV2( p.x(), p.y() ) );
    }
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometryV2* geom = multiGeom->takeGeometry( 0 );
    delete multiGeom;
    return geom;
  }
}

void QgsMapToolDrawPoint::clear()
{
  mPoints.clear();
}

void QgsMapToolDrawPoint::doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t )
{
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    if ( !mPoints.back().isEmpty() )
    {
      mPoints.append( QList<QgsPoint>() );
    }
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      for ( int iVtx = 0, nVtx = geometry->vertexCount( iPart, iRing ); iVtx < nVtx; ++iVtx )
      {
        QgsPointV2 vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, iVtx ) );
        mPoints.back().append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawPolyLine::QgsMapToolDrawPolyLine( QgsMapCanvas *canvas, bool closed )
    : QgsMapToolDrawShape( canvas, closed )
{
  mPoints.append( QList<QgsPoint>() );
}

QgsMapToolDrawShape::State QgsMapToolDrawPolyLine::buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton )
  {
    if ( mPoints.back().isEmpty() )
    {
      mPoints.back().append( pos );
    }
    mPoints.back().append( pos );
    return StateDrawing;
  }
  else if ( !press && button == Qt::RightButton )
  {
    if ( mMultipart )
    {
      mPoints.append( QList<QgsPoint>() );
      return StateReady;
    }
    else
    {
      return StateFinished;
    }
  }
  return mState;
}

void QgsMapToolDrawPolyLine::moveEvent( const QgsPoint& pos )
{
  mPoints.back().back() = pos;
}

QgsAbstractGeometryV2* QgsMapToolDrawPolyLine::createGeometry( const QgsCoordinateReferenceSystem &targetCrs ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs );
  QgsGeometryCollectionV2* multiGeom = mIsArea ? static_cast<QgsGeometryCollectionV2*>( new QgsMultiPolygonV2() ) : static_cast<QgsGeometryCollectionV2*>( new QgsMultiLineStringV2() );
  foreach ( const QList<QgsPoint>& part, mPoints )
  {
    QgsLineStringV2* ring = new QgsLineStringV2();
    foreach ( const QgsPoint& point, part )
    {
      ring->addVertex( QgsPointV2( t.transform( point ) ) );
    }
    if ( mIsArea )
    {
      if ( !part.isEmpty() ) ring->addVertex( QgsPointV2( t.transform( part.front() ) ) );
      QgsPolygonV2* poly = new QgsPolygonV2;
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
    else
    {
      multiGeom->addGeometry( ring );
    }
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometryV2* geom = multiGeom->takeGeometry( 0 );
    delete multiGeom;
    return geom;
  }
}

void QgsMapToolDrawPolyLine::doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t )
{
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    if ( !mPoints.back().isEmpty() )
    {
      mPoints.append( QList<QgsPoint>() );
    }
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      for ( int iVtx = 0, nVtx = geometry->vertexCount( iPart, iRing ); iVtx < nVtx; ++iVtx )
      {
        QgsPointV2 vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, iVtx ) );
        mPoints.back().append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
      }
    }
  }
  mPoints.append( QList<QgsPoint>() );
}

void QgsMapToolDrawPolyLine::clear()
{
  mPoints.clear();
  mPoints.append( QList<QgsPoint>() );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawRectangle::QgsMapToolDrawRectangle( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true ) {}

QgsMapToolDrawShape::State QgsMapToolDrawRectangle::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton /*button*/ )
{
  if ( press )
  {
    mP1.append( pos );
    mP2.append( pos );
    return StateDrawing;
  }
  else
  {
    return mMultipart ? StateReady : StateFinished;
  }
  return mState;
}

void QgsMapToolDrawRectangle::moveEvent( const QgsPoint &pos )
{
  mP2.last() = pos;
}

QgsAbstractGeometryV2* QgsMapToolDrawRectangle::createGeometry( const QgsCoordinateReferenceSystem &targetCrs ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2;
  for ( int i = 0, n = mP1.size(); i < n; ++i )
  {
    QgsLineStringV2* ring = new QgsLineStringV2;
    ring->addVertex( QgsPointV2( t.transform( mP1[i] ) ) );
    ring->addVertex( QgsPointV2( t.transform( mP2[i].x(), mP1[i].y() ) ) );
    ring->addVertex( QgsPointV2( t.transform( mP2[i] ) ) );
    ring->addVertex( QgsPointV2( t.transform( mP1[i].x(), mP2[i].y() ) ) );
    ring->addVertex( QgsPointV2( t.transform( mP1[i] ) ) );
    QgsPolygonV2* poly = new QgsPolygonV2;
    poly->setExteriorRing( ring );
    multiGeom->addGeometry( poly );
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometryV2* geom = multiGeom->takeGeometry( 0 );
    delete multiGeom;
    return geom;
  }
}

void QgsMapToolDrawRectangle::doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t )
{
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      if ( geometry->vertexCount( iPart, iRing ) == 5 )
      {
        QgsPointV2 vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, 0 ) );
        mP1.append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
        vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, 2 ) );
        mP2.append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
      }
    }
  }
}

void QgsMapToolDrawRectangle::clear()
{
  mP1.clear();
  mP2.clear();
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawCircle::QgsMapToolDrawCircle( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true ) {}

QgsMapToolDrawShape::State QgsMapToolDrawCircle::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton /*button*/ )
{
  if ( press )
  {
    mCenters.append( pos );
    mRadii.append( 0. );
    return StateDrawing;
  }
  else
  {
    return mMultipart ? StateReady : StateFinished;
  }
  return mState;
}

void QgsMapToolDrawCircle::moveEvent( const QgsPoint &pos )
{
  mRadii.last() = qSqrt( mCenters.last().sqrDist( pos ) );
}

QgsAbstractGeometryV2* QgsMapToolDrawCircle::createGeometry( const QgsCoordinateReferenceSystem &targetCrs ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2();
  for ( int i = 0, n = mCenters.size(); i < n; ++i )
  {
    QgsCircularStringV2* ring = new QgsCircularStringV2;
    ring->setPoints( QList<QgsPointV2>()
                     << QgsPointV2( t.transform( mCenters[i].x() + mRadii[i], mCenters[i].y() ) )
                     << QgsPointV2( t.transform( mCenters[i].x(), mCenters[i].y() ) )
                     << QgsPointV2( t.transform( mCenters[i].x() + mRadii[i], mCenters[i].y() ) ) );
    QgsCurvePolygonV2* poly = new QgsCurvePolygonV2();
    poly->setExteriorRing( ring );
    multiGeom->addGeometry( poly );
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometryV2* geom = multiGeom->takeGeometry( 0 );
    delete multiGeom;
    return geom;
  }
}

void QgsMapToolDrawCircle::doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t )
{
  if ( dynamic_cast<const QgsGeometryCollectionV2*>( geometry ) )
  {
    const QgsGeometryCollectionV2* geomCollection = static_cast<const QgsGeometryCollectionV2*>( geometry );
    for ( int i = 0, n = geomCollection->numGeometries(); i < n; ++i )
    {
      QgsRectangle bb = geomCollection->geometryN( i )->boundingBox();
      QgsPoint c = t.transform( bb.center() );
      QgsPoint r = t.transform( QgsPoint( bb.xMinimum(), bb.center().y() ) );
      mCenters.append( c );
      mRadii.append( qSqrt( c.sqrDist( r ) ) );
    }
  }
  else
  {
    QgsRectangle bb = geometry->boundingBox();
    QgsPoint c = t.transform( bb.center() );
    QgsPoint r = t.transform( QgsPoint( bb.xMinimum(), bb.center().y() ) );
    mCenters.append( c );
    mRadii.append( qSqrt( c.sqrDist( r ) ) );
  }
}

void QgsMapToolDrawCircle::clear()
{
  mCenters.clear();
  mRadii.clear();
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawCircularSector::QgsMapToolDrawCircularSector( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true ), mSectorStage( HaveNothing ) {}

QgsMapToolDrawShape::State QgsMapToolDrawCircularSector::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton /*button*/ )
{
  if ( press )
  {
    if ( mSectorStage == HaveNothing )
    {
      mCenters.append( pos );
      mRadii.append( 0 );
      mStartAngles.append( 0 );
      mStopAngles.append( 0 );
      mSectorStage = HaveCenter;
      return StateDrawing;
    }
    else if ( mSectorStage == HaveCenter )
    {
      mSectorStage = HaveArc;
      return StateDrawing;
    }
    else if ( mSectorStage == HaveArc )
    {
      mSectorStage = HaveNothing;
      return mMultipart ? StateReady : StateFinished;
    }
  }
  return mState;
}

void QgsMapToolDrawCircularSector::moveEvent( const QgsPoint &pos )
{
  if ( mSectorStage == HaveCenter )
  {
    mRadii.back() = qSqrt( pos.sqrDist( mCenters.back() ) );
    mStartAngles.back() = mStopAngles.back() = qAtan2( pos.y() - mCenters.back().y(), pos.x() - mCenters.back().x() );
  }
  else if ( mSectorStage == HaveArc )
  {
    mStopAngles.back() = qAtan2( pos.y() - mCenters.back().y(), pos.x() - mCenters.back().x() );
    if ( mStopAngles.back() < mStartAngles.back() )
    {
      mStopAngles.back() += 2 * M_PI;
    }
  }
}

QgsAbstractGeometryV2* QgsMapToolDrawCircularSector::createGeometry( const QgsCoordinateReferenceSystem &targetCrs ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2;
  for ( int i = 0, n = mCenters.size(); i < n; ++i )
  {
    double alphaMid = 0.5 * ( mStartAngles[i] + mStopAngles[i] );
    QgsPoint pStart( mCenters[i].x() + mRadii[i] * qCos( mStartAngles[i] ),
                     mCenters[i].y() + mRadii[i] * qSin( mStartAngles[i] ) );
    QgsPoint pMid( mCenters[i].x() + mRadii[i] * qCos( alphaMid ),
                   mCenters[i].y() + mRadii[i] * qSin( alphaMid ) );
    QgsPoint pEnd( mCenters[i].x() + mRadii[i] * qCos( mStopAngles[i] ),
                   mCenters[i].y() + mRadii[i] * qSin( mStopAngles[i] ) );
    QgsCompoundCurveV2* exterior = new QgsCompoundCurveV2();
    if ( mStartAngles[i] != mStopAngles[i] )
    {
      QgsCircularStringV2* arc = new QgsCircularStringV2();
      arc->setPoints( QList<QgsPointV2>() << t.transform( pStart ) << t.transform( pMid ) << t.transform( pEnd ) );
      exterior->addCurve( arc );
    }
    QgsLineStringV2* line = new QgsLineStringV2();
    line->setPoints( QList<QgsPointV2>() << t.transform( pEnd ) << t.transform( mCenters[i] ) << t.transform( pStart ) );
    exterior->addCurve( line );
    QgsCurvePolygonV2* poly = new QgsCurvePolygonV2;
    poly->setExteriorRing( exterior );
    multiGeom->addGeometry( poly );
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometryV2* geom = multiGeom->takeGeometry( 0 );
    delete multiGeom;
    return geom;
  }
}

void QgsMapToolDrawCircularSector::doAddGeometry( const QgsAbstractGeometryV2* /*geometry*/, const QgsCoordinateTransform& /*t*/ )
{
  // Not yet implemented
}

void QgsMapToolDrawCircularSector::clear()
{
  mCenters.clear();
  mRadii.clear();
  mStartAngles.clear();
  mStopAngles.clear();
}
