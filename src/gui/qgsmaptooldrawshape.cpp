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
#include "qgsmaptooldrawshape_p.h"
#include "qgscircularstringv2.h"
#include "qgscompoundcurvev2.h"
#include "qgscrscache.h"
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

#include <QDoubleValidator>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <qmath.h>

QgsMapToolDrawShape::QgsMapToolDrawShape( QgsMapCanvas *canvas, bool isArea )
    : QgsMapTool( canvas ), mState( StateReady ), mIsArea( isArea ), mMultipart( false ), mSnapPoints( false ), mShowInput( false ), mResetOnDeactivate( true ), mIgnoreNextMoveEvent( false ), mInputWidget( 0 )
{
  setCursor( Qt::CrossCursor );

  QSettings settings;
  int red = settings.value( "/qgis/default_measure_color_red", 255 ).toInt();
  int green = settings.value( "/qgis/default_measure_color_green", 0 ).toInt();
  int blue = settings.value( "/qgis/default_measure_color_blue", 0 ).toInt();

  mRubberBand = new QgsGeometryRubberBand( canvas, isArea ? QGis::Polygon : QGis::Line );
  mRubberBand->setFillColor( QColor( red, green, blue, 63 ) );
  mRubberBand->setOutlineColor( QColor( red, green, blue, 255 ) );
  mRubberBand->setOutlineWidth( 3 );

  setShowNodes( false );

  // Shapes typically won't survive projection changes without distortion (circle, square, etc)
  connect( canvas, SIGNAL( destinationCrsChanged() ), this, SLOT( reset() ) );
}

QgsMapToolDrawShape::~QgsMapToolDrawShape()
{
  delete mRubberBand.data();
}

void QgsMapToolDrawShape::activate()
{
  QgsMapTool::activate();
  if ( mShowInput )
  {
    mInputWidget = new QgsMapToolDrawShapeInputWidget( canvas() );
    initInputWidget();
    // Initially out of sight
    mInputWidget->move( -1000, -1000 );
    mInputWidget->show();
  }
}

void QgsMapToolDrawShape::deactivate()
{
  QgsMapTool::deactivate();
  if ( mResetOnDeactivate )
    reset();
  delete mInputWidget;
  mInputWidget = 0;
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
  emit cleared();
}

void QgsMapToolDrawShape::canvasPressEvent( QMouseEvent* e )
{
  if ( mState == StateFinished )
  {
    reset();
  }
  if ( mState == StateReady && e->button() == Qt::RightButton && canvas()->mapTool() == this )
  {
    canvas()->unsetMapTool( this ); // unset
    return;
  }
  mState = buttonEvent( transformPoint( e->pos() ), true, e->button() );
  if ( mShowInput )
  {
    updateInputWidget( transformPoint( e->pos() ) );
  }
  update();
  if ( mState == StateFinished )
  {
    emit finished();
  }
}

void QgsMapToolDrawShape::canvasMoveEvent( QMouseEvent* e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }

  if ( mState == StateDrawing )
  {
    moveEvent( transformPoint( e->pos() ) );
    update();
  }
  if ( mShowInput )
  {
    updateInputWidget( transformPoint( e->pos() ) );
    mInputWidget->move( e->x(), e->y() + 20 );
  }
}

void QgsMapToolDrawShape::canvasReleaseEvent( QMouseEvent* e )
{
  if ( mState != StateFinished )
  {
    mState = buttonEvent( transformPoint( e->pos() ), false, e->button() );
    if ( mShowInput )
    {
      updateInputWidget( transformPoint( e->pos() ) );
    }
    update();
    if ( mState == StateFinished )
    {
      emit finished();
    }
  }
}

void QgsMapToolDrawShape::keyReleaseEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mState == StateReady )
      canvas()->unsetMapTool( this ); // unset
    else
      reset();
  }
}

void QgsMapToolDrawShape::acceptInput()
{
  if ( mState == StateFinished )
  {
    reset();
  }
  mState = inputAccepted();
  update();
  if ( mState == StateFinished )
  {
    emit finished();
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
  doAddGeometry( geometry, *QgsCoordinateTransformCache::instance()->transform( sourceCrs.authid(), canvas()->mapSettings().destinationCrs().authid() ) );
  mState = mMultipart ? StateReady : StateFinished;
  update();
}

void QgsMapToolDrawShape::moveMouseToPos( const QgsPoint& geoPos )
{
  // If position is not within visible extent, center map there
  if ( !mCanvas->mapSettings().visibleExtent().contains( geoPos ) )
  {
    QgsRectangle rect = mCanvas->mapSettings().visibleExtent();
    rect = QgsRectangle( geoPos.x() - 0.5 * rect.width(), geoPos.y() - 0.5 * rect.height(), geoPos.x() + 0.5 * rect.width(), geoPos.y() + 0.5 * rect.height() );
    mCanvas->setExtent( rect );
    mCanvas->refresh();
  }
  // Then, move cursor to corresponding screen position and simulate move event
  QPoint p = toCanvasCoordinates( geoPos );
  // Ignore the move event emitted by re-positioning the mouse cursor:
  // The widget mouse coordinates (stored in a integer QPoint) loses precision,
  // and mapping it back to map coordinates in the mouseMove event handler
  // results in a position different from geoPos, and hence the user-input
  // may get altered
  mIgnoreNextMoveEvent = true;
  QCursor::setPos( mCanvas->mapToGlobal( p ) );
  if ( mState == StateDrawing )
  {
    moveEvent( geoPos );
    update();
  }
  if ( mShowInput )
  {
    updateInputWidget( geoPos );
    mInputWidget->move( p.x(), p.y() + 20 );
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawPoint::QgsMapToolDrawPoint( QgsMapCanvas *canvas )
    : QgsMapToolDrawShape( canvas, false )
{
  mRubberBand->setIconType( QgsGeometryRubberBand::ICON_CIRCLE );
}

QgsMapToolDrawShape::State QgsMapToolDrawPoint::buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button )
{
  if ( !press && button == Qt::LeftButton )
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
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPointV2();
  foreach ( const QList<QgsPoint>& part, mPoints )
  {
    if ( part.size() > 0 )
    {
      QgsPoint p = t->transform( part.front() );
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

void QgsMapToolDrawPoint::initInputWidget()
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( mInputWidget->layout() );
  gridLayout->addWidget( new QLabel( "x=" ), 0, 0, 1, 1 );
  mXEdit = new QgsMapToolDrawShapeInputField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mXEdit, 0, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "y=" ), 1, 0, 1, 1 );
  mYEdit = new QgsMapToolDrawShapeInputField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mYEdit, 1, 1, 1, 1 );
  mInputWidget->addFocusChild( mXEdit );
  mInputWidget->addFocusChild( mYEdit );
  mInputWidget->setFocusChild( mXEdit );
}

void QgsMapToolDrawPoint::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusChild() )
    mInputWidget->focusChild()->selectAll();
}

QgsMapToolDrawShape::State QgsMapToolDrawPoint::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  mPoints.append( QList<QgsPoint>() << QgsPoint( x, y ) );
  mInputWidget->setFocusChild( mXEdit );
  return mMultipart ? StateReady : StateFinished;
}

void QgsMapToolDrawPoint::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
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
      if ( !mIsArea || mPoints.back().size() > 2 )
      {
        mPoints.append( QList<QgsPoint>() );
        return StateReady;
      }
    }
    else if ( !mIsArea || mPoints.back().size() > 2 )
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
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = mIsArea ? static_cast<QgsGeometryCollectionV2*>( new QgsMultiPolygonV2() ) : static_cast<QgsGeometryCollectionV2*>( new QgsMultiLineStringV2() );
  foreach ( const QList<QgsPoint>& part, mPoints )
  {
    QgsLineStringV2* ring = new QgsLineStringV2();
    foreach ( const QgsPoint& point, part )
    {
      ring->addVertex( QgsPointV2( t->transform( point ) ) );
    }
    if ( mIsArea )
    {
      if ( !part.isEmpty() ) ring->addVertex( QgsPointV2( t->transform( part.front() ) ) );
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

void QgsMapToolDrawPolyLine::initInputWidget()
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( mInputWidget->layout() );
  gridLayout->addWidget( new QLabel( "x=" ), 0, 0, 1, 1 );
  mXEdit = new QgsMapToolDrawShapeInputField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mXEdit, 0, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "y=" ), 1, 0, 1, 1 );
  mYEdit = new QgsMapToolDrawShapeInputField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mYEdit, 1, 1, 1, 1 );
  mInputWidget->addFocusChild( mXEdit );
  mInputWidget->addFocusChild( mYEdit );
  mInputWidget->setFocusChild( mXEdit );
}

void QgsMapToolDrawPolyLine::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusChild() )
    mInputWidget->focusChild()->selectAll();
}

QgsMapToolDrawShape::State QgsMapToolDrawPolyLine::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  if ( mState == StateReady )
  {
    mPoints.back().append( QgsPoint( x, y ) );
    mPoints.back().append( QgsPoint( x, y ) );
    return StateDrawing;
  }
  else if ( mState == StateDrawing )
  {
    // At least two points and enter pressed twice -> finish
    int n = mPoints.back().size();
    if ( n > 1 && mPoints.back()[n - 2] == mPoints.back()[n - 1] )
    {
      mInputWidget->setFocusChild( mXEdit );
      mPoints.back().removeLast();
      mPoints.append( QList<QgsPoint>() );
      return mMultipart ? StateReady : StateFinished;
    }
    mPoints.back().back() = QgsPoint( x, y );
    mPoints.back().append( QgsPoint( x, y ) );
  }
  return mState;
}

void QgsMapToolDrawPolyLine::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawRectangle::QgsMapToolDrawRectangle( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true ) {}

QgsMapToolDrawShape::State QgsMapToolDrawRectangle::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton && mState == StateReady )
  {
    mP1.append( pos );
    mP2.append( pos );
    return StateDrawing;
  }
  else if ( !press && mState == StateDrawing && mP1.back() != mP2.back() )
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
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2;
  for ( int i = 0, n = mP1.size(); i < n; ++i )
  {
    QgsLineStringV2* ring = new QgsLineStringV2;
    ring->addVertex( QgsPointV2( t->transform( mP1[i] ) ) );
    ring->addVertex( QgsPointV2( t->transform( mP2[i].x(), mP1[i].y() ) ) );
    ring->addVertex( QgsPointV2( t->transform( mP2[i] ) ) );
    ring->addVertex( QgsPointV2( t->transform( mP1[i].x(), mP2[i].y() ) ) );
    ring->addVertex( QgsPointV2( t->transform( mP1[i] ) ) );
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

void QgsMapToolDrawRectangle::initInputWidget()
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( mInputWidget->layout() );
  gridLayout->addWidget( new QLabel( "x=" ), 0, 0, 1, 1 );
  mXEdit = new QgsMapToolDrawShapeInputField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mXEdit, 0, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "y=" ), 1, 0, 1, 1 );
  mYEdit = new QgsMapToolDrawShapeInputField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mYEdit, 1, 1, 1, 1 );
  mInputWidget->addFocusChild( mXEdit );
  mInputWidget->addFocusChild( mYEdit );
  mInputWidget->setFocusChild( mXEdit );
}

void QgsMapToolDrawRectangle::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusChild() )
    mInputWidget->focusChild()->selectAll();
}

QgsMapToolDrawShape::State QgsMapToolDrawRectangle::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  mInputWidget->setFocusChild( mXEdit );
  if ( mState == StateReady )
  {
    mP1.append( QgsPoint( x, y ) );
    mP2.append( QgsPoint( x, y ) );
    return StateDrawing;
  }
  else if ( mState == StateDrawing )
  {
    mP2.back() = QgsPoint( x, y );
    return mMultipart ? StateReady : StateFinished;
  }
  return mState;
}

void QgsMapToolDrawRectangle::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawCircle::QgsMapToolDrawCircle( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true ) {}

QgsMapToolDrawShape::State QgsMapToolDrawCircle::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton && mState == StateReady )
  {
    mCenters.append( pos );
    mRadii.append( 0. );
    return StateDrawing;
  }
  else if ( !press && mState == StateDrawing && mRadii.back() > 0. )
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
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2();
  for ( int i = 0, n = mCenters.size(); i < n; ++i )
  {
    QgsCircularStringV2* ring = new QgsCircularStringV2;
    ring->setPoints( QList<QgsPointV2>()
                     << QgsPointV2( t->transform( mCenters[i].x() + mRadii[i], mCenters[i].y() ) )
                     << QgsPointV2( t->transform( mCenters[i].x(), mCenters[i].y() ) )
                     << QgsPointV2( t->transform( mCenters[i].x() + mRadii[i], mCenters[i].y() ) ) );
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

void QgsMapToolDrawCircle::initInputWidget()
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( mInputWidget->layout() );
  gridLayout->addWidget( new QLabel( "x=" ), 0, 0, 1, 1 );
  mXEdit = new QgsMapToolDrawShapeInputField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mXEdit, 0, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "y=" ), 1, 0, 1, 1 );
  mYEdit = new QgsMapToolDrawShapeInputField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mYEdit, 1, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "r=" ), 2, 0, 1, 1 );
  QDoubleValidator* validator = new QDoubleValidator();
  validator->setBottom( 0 );
  mREdit = new QgsMapToolDrawShapeInputField( validator );
  connect( mREdit, SIGNAL( inputChanged() ), this, SLOT( radiusInputChanged() ) );
  connect( mREdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mREdit, 2, 1, 1, 1 );
  mInputWidget->addFocusChild( mXEdit );
  mInputWidget->addFocusChild( mYEdit );
  mInputWidget->addFocusChild( mREdit );
  mInputWidget->setFocusChild( mXEdit );
}

void QgsMapToolDrawCircle::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  if ( mState == StateReady )
  {
    mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
    mREdit->setText( "0" );
  }
  else if ( mState == StateDrawing )
  {
    mREdit->setText( QString::number( mRadii.last(), 'f', isDegrees ? 4 : 0 ) );
  }
  if ( mInputWidget->focusChild() )
    mInputWidget->focusChild()->selectAll();
}

QgsMapToolDrawShape::State QgsMapToolDrawCircle::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  mInputWidget->setFocusChild( mXEdit );
  if ( mState == StateReady )
  {
    mCenters.append( QgsPoint( x, y ) );
    mRadii.append( r );
    return StateDrawing;
  }
  else if ( mState == StateDrawing )
  {
    mREdit->setText( "0" );
    mCenters.back() = QgsPoint( x, y );
    mRadii.back() = r;
    return mMultipart ? StateReady : StateFinished;
  }
  return mState;
}

void QgsMapToolDrawCircle::centerInputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  if ( mState == StateReady )
  {
    mCenters.append( QgsPoint( x, y ) );
    mRadii.append( r );
    mState = StateDrawing;
  }
  mCenters.back() = QgsPoint( x, y );
  moveMouseToPos( QgsPoint( x + r, y ) );
}

void QgsMapToolDrawCircle::radiusInputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  if ( mState == StateReady )
  {
    mCenters.append( QgsPoint( x, y ) );
    mRadii.append( r );
    mState = StateDrawing;
  }
  mRadii.back() = r;
  moveMouseToPos( QgsPoint( x + r, y ) );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawCircularSector::QgsMapToolDrawCircularSector( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true ), mSectorStage( HaveNothing ) {}

QgsMapToolDrawShape::State QgsMapToolDrawCircularSector::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton )
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
      mSectorStage = HaveRadius;
      return StateDrawing;
    }
    else if ( mSectorStage == HaveRadius )
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
  else if ( mSectorStage == HaveRadius )
  {
    mStopAngles.back() = qAtan2( pos.y() - mCenters.back().y(), pos.x() - mCenters.back().x() );
    if ( mStopAngles.back() <= mStartAngles.back() )
    {
      mStopAngles.back() += 2 * M_PI;
    }

    // Snap to full circle if within 5px
    QgsPoint pStart( mCenters.back().x() + mRadii.back() * qCos( mStartAngles.back() ),
                     mCenters.back().y() + mRadii.back() * qSin( mStartAngles.back() ) );
    QgsPoint pEnd( mCenters.back().x() + mRadii.back() * qCos( mStopAngles.back() ),
                   mCenters.back().y() + mRadii.back() * qSin( mStopAngles.back() ) );
    QPoint diff = toCanvasCoordinates( pEnd ) - toCanvasCoordinates( pStart );
    if (( diff.x() * diff.x() + diff.y() * diff.y() ) < 25 )
    {
      mStopAngles.back() = mStartAngles.back() + 2 * M_PI;
    }
  }
}

QgsAbstractGeometryV2* QgsMapToolDrawCircularSector::createGeometry( const QgsCoordinateReferenceSystem &targetCrs ) const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2;
  for ( int i = 0, n = mCenters.size(); i < n; ++i )
  {
    QgsPoint pStart, pMid, pEnd;
    if ( mStopAngles[i] == mStartAngles[i] + 2 * M_PI )
    {
      pStart = pEnd = QgsPoint( mCenters[i].x() + mRadii[i] * qCos( mStopAngles[i] ),
                                mCenters[i].y() + mRadii[i] * qSin( mStopAngles[i] ) );
      pMid = mCenters[i];
    }
    else
    {
      double alphaMid = 0.5 * ( mStartAngles[i] + mStopAngles[i] - 2 * M_PI );
      pStart = QgsPoint( mCenters[i].x() + mRadii[i] * qCos( mStartAngles[i] ),
                         mCenters[i].y() + mRadii[i] * qSin( mStartAngles[i] ) );
      pMid = QgsPoint( mCenters[i].x() + mRadii[i] * qCos( alphaMid ),
                       mCenters[i].y() + mRadii[i] * qSin( alphaMid ) );
      pEnd = QgsPoint( mCenters[i].x() + mRadii[i] * qCos( mStopAngles[i] - 2 * M_PI ),
                       mCenters[i].y() + mRadii[i] * qSin( mStopAngles[i] - 2 * M_PI ) );
    }
    QgsCompoundCurveV2* exterior = new QgsCompoundCurveV2();
    if ( mStartAngles[i] != mStopAngles[i] )
    {
      QgsCircularStringV2* arc = new QgsCircularStringV2();
      arc->setPoints( QList<QgsPointV2>() << t->transform( pStart ) << t->transform( pMid ) << t->transform( pEnd ) );
      exterior->addCurve( arc );
    }
    if ( mStopAngles[i] != mStartAngles[i] + 2 * M_PI )
    {
      QgsLineStringV2* line = new QgsLineStringV2();
      line->setPoints( QList<QgsPointV2>() << t->transform( pEnd ) << t->transform( mCenters[i] ) << t->transform( pStart ) );
      exterior->addCurve( line );
    }
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
  mSectorStage = HaveNothing;
}

void QgsMapToolDrawCircularSector::initInputWidget()
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( mInputWidget->layout() );
  gridLayout->addWidget( new QLabel( "x=" ), 0, 0, 1, 1 );
  mXEdit = new QgsMapToolDrawShapeInputField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mXEdit, 0, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "y=" ), 1, 0, 1, 1 );
  mYEdit = new QgsMapToolDrawShapeInputField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mYEdit, 1, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( "r=" ), 2, 0, 1, 1 );
  QDoubleValidator* validator = new QDoubleValidator();
  validator->setBottom( 0 );
  mREdit = new QgsMapToolDrawShapeInputField( validator );
  connect( mREdit, SIGNAL( inputChanged() ), this, SLOT( arcInputChanged() ) );
  connect( mREdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mREdit, 2, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( QString( QChar( 0x03B1 ) ) + "1=" ), 3, 0, 1, 1 );
  mA1Edit = new QgsMapToolDrawShapeInputField();
  connect( mA1Edit, SIGNAL( inputChanged() ), this, SLOT( arcInputChanged() ) );
  connect( mA1Edit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mA1Edit, 3, 1, 1, 1 );
  gridLayout->addWidget( new QLabel( QString( QChar( 0x03B1 ) ) + "2=" ), 4, 0, 1, 1 );
  mA2Edit = new QgsMapToolDrawShapeInputField();
  connect( mA2Edit, SIGNAL( inputChanged() ), this, SLOT( arcInputChanged() ) );
  connect( mA2Edit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  gridLayout->addWidget( mA2Edit, 4, 1, 1, 1 );
  mInputWidget->addFocusChild( mXEdit );
  mInputWidget->addFocusChild( mYEdit );
  mInputWidget->addFocusChild( mREdit );
  mInputWidget->addFocusChild( mA1Edit );
  mInputWidget->addFocusChild( mA2Edit );
  mInputWidget->setFocusChild( mXEdit );
}

void QgsMapToolDrawCircularSector::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  if ( mSectorStage == HaveNothing )
  {
    mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
    mREdit->setText( "0" );
    mA1Edit->setText( "0" );
    mA2Edit->setText( "0" );
  }
  else if ( mSectorStage == HaveCenter )
  {
    mREdit->setText( QString::number( mRadii.last(), 'f', isDegrees ? 4 : 0 ) );
  }
  else if ( mSectorStage == HaveRadius )
  {
    double startAngle = 2.5 * M_PI - mStartAngles.last();
    if ( startAngle > 2 * M_PI )
      startAngle -= 2 * M_PI;
    else if ( startAngle < 0 )
      startAngle += 2 * M_PI;
    mA1Edit->setText( QString::number( startAngle / M_PI * 180., 'f', 1 ) );
    double stopAngle = 2.5 * M_PI - mStopAngles.last();
    if ( stopAngle > 2 * M_PI )
      stopAngle -= 2 * M_PI;
    else if ( stopAngle < 0 )
      stopAngle += 2 * M_PI;
    mA2Edit->setText( QString::number( stopAngle / M_PI * 180., 'f', 1 ) );
  }
  if ( mInputWidget->focusChild() )
    mInputWidget->focusChild()->selectAll();
}

QgsMapToolDrawShape::State QgsMapToolDrawCircularSector::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  double a1 = 2.5 * M_PI - mA1Edit->text().toDouble() / 180. * M_PI;
  double a2 = 2.5 * M_PI - mA2Edit->text().toDouble() / 180. * M_PI;
  while ( a1 < 0 )
    a1 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a1 -= 2 * M_PI;
  while ( a2 < 0 )
    a2 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a2 -= 2 * M_PI;
  if ( a2 <= a1 )
    a2 += 2 * M_PI;
  mInputWidget->setFocusChild( mXEdit );
  if ( mState == StateReady )
  {
    mCenters.append( QgsPoint( x, y ) );
    mRadii.append( r );
    if ( r > 0 )
    {
      mStartAngles.append( a1 );
      mStopAngles.append( a2 );
      mSectorStage = HaveRadius;
    }
    else
    {
      mStartAngles.append( 0 );
      mStopAngles.append( 0 );
      mSectorStage = HaveCenter;
    }
    return StateDrawing;
  }
  else if ( mState == StateDrawing )
  {
    mCenters.back() = QgsPoint( x, y );
    mRadii.back() = r;
    if ( r > 0 )
    {
      mStartAngles.back() = a1;
      mStopAngles.back() = a2;
      mSectorStage = HaveRadius;
    }
    else
    {
      mStartAngles.back() = 0;
      mStopAngles.back() = 0;
      mSectorStage = HaveCenter;
    }
    if ( mSectorStage == HaveRadius )
    {
      mREdit->setText( "0" );
      mA1Edit->setText( "0" );
      mA2Edit->setText( "0" );
      return mMultipart ? StateReady : StateFinished;
    }
  }
  return mState;
}

void QgsMapToolDrawCircularSector::centerInputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  double a1 = 2.5 * M_PI - mA1Edit->text().toDouble() / 180. * M_PI;
  double a2 = 2.5 * M_PI - mA2Edit->text().toDouble() / 180. * M_PI;
  while ( a1 < 0 )
    a1 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a1 -= 2 * M_PI;
  while ( a2 < 0 )
    a2 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a2 -= 2 * M_PI;
  if ( a2 <= a1 )
    a2 += 2 * M_PI;
  if ( mState == StateReady )
  {
    mCenters.append( QgsPoint( x, y ) );
    mRadii.append( r );
    if ( r > 0 )
    {
      mStartAngles.append( a1 );
      mStopAngles.append( a2 );
      mSectorStage = HaveRadius;
    }
    else
    {
      mStartAngles.append( 0 );
      mStopAngles.append( 0 );
      mSectorStage = HaveCenter;
    }
    mState = StateDrawing;
  }
  mCenters.back() = QgsPoint( x, y );
  moveMouseToPos( QgsPoint( x + r * qCos( mStopAngles.back() ), y + r * qSin( mStopAngles.back() ) ) );
}

void QgsMapToolDrawCircularSector::arcInputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  double a1 = 2.5 * M_PI - mA1Edit->text().toDouble() / 180. * M_PI;
  double a2 = 2.5 * M_PI - mA2Edit->text().toDouble() / 180. * M_PI;
  while ( a1 < 0 )
    a1 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a1 -= 2 * M_PI;
  while ( a2 < 0 )
    a2 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a2 -= 2 * M_PI;
  if ( a2 <= a1 )
    a2 += 2 * M_PI;
  if ( mState == StateReady )
  {
    mCenters.append( QgsPoint( x, y ) );
    mRadii.append( r );
    if ( r > 0 )
    {
      mStartAngles.append( a1 );
      mStopAngles.append( a2 );
      mSectorStage = HaveRadius;
    }
    else
    {
      mStartAngles.append( 0 );
      mStopAngles.append( 0 );
      mSectorStage = HaveCenter;
    }
    mState = StateDrawing;
  }
  mRadii.back() = r;
  if ( r > 0 )
  {
    mStartAngles.back() = a1;
    mStopAngles.back() = a2;
    mSectorStage = HaveRadius;
  }
  else
  {
    mStartAngles.back() = 0;
    mStopAngles.back() = 0;
    mSectorStage = HaveCenter;
  }
  moveMouseToPos( QgsPoint( x + r * qCos( mStopAngles.back() ), y + r * qSin( mStopAngles.back() ) ) );
}
