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
#include "qgsfloatinginputwidget.h"
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
#include <QMenu>
#include <QMouseEvent>
#include <qmath.h>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

QgsMapToolDrawShape::QgsMapToolDrawShape( QgsMapCanvas *canvas, bool isArea, State *initialState )
    : QgsMapTool( canvas )
    , mIsArea( isArea )
    , mMultipart( false )
    , mInputWidget( 0 )
    , mStateStack( initialState )
    , mSnapPoints( false )
    , mShowInput( false )
    , mResetOnDeactivate( true )
    , mIgnoreNextMoveEvent( false )
    , mEditContext( 0 )
    , mParentTool( 0 )
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
  mRubberBand->setIconSize( 10 );
  mRubberBand->setIconOutlineWidth( 2 );
  mRubberBand->setIconOutlineColor( mRubberBand->outlineColor() );
  mRubberBand->setIconFillColor( Qt::white );
  mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );

  // Shapes typically won't survive projection changes without distortion (circle, square, etc)
  connect( canvas, SIGNAL( destinationCrsChanged() ), this, SLOT( reset() ) );

  connect( &mStateStack, SIGNAL( canUndoChanged( bool ) ), this, SIGNAL( canUndo( bool ) ) );
  connect( &mStateStack, SIGNAL( canRedoChanged( bool ) ), this, SIGNAL( canRedo( bool ) ) );
  connect( &mStateStack, SIGNAL( stateChanged() ), this, SLOT( update() ) );
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
    mInputWidget = new QgsFloatingInputWidget( canvas() );
    initInputWidget();
    mInputWidget->show();
  }
}

void QgsMapToolDrawShape::deactivate()
{
  if ( state()->status == StatusEditingReady || state()->status == StatusEditingMoving )
  {
    emit finished();
  }
  QgsMapTool::deactivate();
  if ( mResetOnDeactivate )
    reset();
  delete mInputWidget;
  mInputWidget = 0;
}

void QgsMapToolDrawShape::editGeometry( const QgsAbstractGeometryV2 *geometry, const QgsCoordinateReferenceSystem &sourceCrs )
{
  reset();
  addGeometry( geometry, sourceCrs );
  State* newState = cloneState();
  newState->status = StatusEditingReady;
  mStateStack.clear( newState );
  setShowInputWidget( false );
}

void QgsMapToolDrawShape::setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits , QgsGeometryRubberBand::AngleUnit angleUnits )
{
  mRubberBand->setMeasurementMode( measurementMode, displayUnits, angleUnits );
  emit geometryChanged();
}

void QgsMapToolDrawShape::update()
{
  QList<QgsVertexId> hiddenNodes;
  QgsAbstractGeometryV2* geom = createGeometry( mCanvas->mapSettings().destinationCrs(), &hiddenNodes );
  mRubberBand->setGeometry( geom, hiddenNodes );
  emit geometryChanged();
}

void QgsMapToolDrawShape::updateStyle( int outlineWidth, const QColor &outlineColor, const QColor &fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle )
{
  mRubberBand->setOutlineWidth( outlineWidth );
  mRubberBand->setOutlineColor( outlineColor );
  mRubberBand->setFillColor( fillColor );
  mRubberBand->setLineStyle( lineStyle );
  mRubberBand->setBrushStyle( brushStyle );
  mRubberBand->update();
}

void QgsMapToolDrawShape::reset()
{
  mRubberBand->setGeometry( 0 );
  mStateStack.clear( emptyState() );
  emit geometryChanged();
  emit cleared();
}

void QgsMapToolDrawShape::canvasPressEvent( QMouseEvent* e )
{
  if ( state()->status == StatusEditingReady )
  {
    mEditContext = getEditContext( transformPoint( e->pos() ) );
    if ( mEditContext )
    {
      if ( e->button() == Qt::RightButton )
      {
        QMenu menu;
        addContextMenuActions( mEditContext, menu );
        if ( !menu.isEmpty() )
        {
          menu.addSeparator();
        }
        QAction* deleteAction = menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ) );
        QAction* clickedAction = menu.exec( canvas()->mapToGlobal( e->pos() ) );
        if ( !clickedAction )
        {
          // pass
        }
        else if ( clickedAction == deleteAction )
        {
          deleteShape();
        }
        else
        {
          executeContextMenuAction( mEditContext, clickedAction->data().toInt(), transformPoint( e->pos() ) );
        }
      }
      else
      {
        mStateStack.updateState( cloneState() );
        mutableState()->status = StatusEditingMoving;
        mLastEditPos = transformPoint( e->pos() );
      }
    }
    return;
  }
  if ( state()->status == StatusFinished )
  {
    reset();
  }
  if ( state()->status == StatusReady && e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
    return;
  }
  buttonEvent( transformPoint( e->pos() ), true, e->button() );

  if ( mShowInput )
  {
    updateInputWidget( transformPoint( e->pos() ) );
  }
  if ( state()->status == StatusFinished )
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
  if ( state()->status == StatusEditingMoving && mEditContext )
  {
    QgsPoint newPos = transformPoint( e->pos() );
    edit( mEditContext, newPos, newPos - mLastEditPos );
    update();
    mLastEditPos = newPos;
  }
  else if ( state()->status == StatusDrawing )
  {
    moveEvent( transformPoint( e->pos() ) );
  }
  if ( mShowInput )
  {
    updateInputWidget( transformPoint( e->pos() ) );
    mInputWidget->move( e->x(), e->y() + 20 );
  }
}

void QgsMapToolDrawShape::canvasReleaseEvent( QMouseEvent* e )
{
  if ( state()->status == StatusEditingMoving )
  {
    delete mEditContext;
    mEditContext = 0;
    mutableState()->status = StatusEditingReady;
  }
  else if ( state()->status != StatusEditingReady && state()->status != StatusFinished )
  {
    buttonEvent( transformPoint( e->pos() ), false, e->button() );

    if ( mShowInput )
    {
      updateInputWidget( transformPoint( e->pos() ) );
    }
    if ( state()->status == StatusFinished )
    {
      emit finished();
    }
  }
  else if ( state()->status == StatusEditingReady )
  {
    canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
  }
}

void QgsMapToolDrawShape::keyPressEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( state()->status == StatusReady || state()->status == StatusEditingReady )
      canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
    else if ( state()->status != StatusEditingMoving )
      reset();
  }
  else if ( e->key() == Qt::Key_Delete && state()->status == StatusEditingReady )
  {
    State* state = emptyState();
    state->status = StatusEditingReady;
    mStateStack.updateState( state );
    canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
  }
  else if ( e->key() == Qt::Key_Z && e->modifiers() == Qt::CTRL )
  {
    undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::CTRL )
  {
    redo();
  }
}

void QgsMapToolDrawShape::deleteShape()
{
  State* state = emptyState();
  state->status = StatusEditingReady;
  mStateStack.updateState( state );
  canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
}

void QgsMapToolDrawShape::acceptInput()
{
  if ( state()->status == StatusFinished )
  {
    reset();
  }
  inputAccepted();
  if ( state()->status == StatusFinished )
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
}

void QgsMapToolDrawShape::moveMouseToPos( const QgsPoint& geoPos )
{
  // Ignore the move event emitted by re-positioning the mouse cursor:
  // The widget mouse coordinates (stored in a integer QPoint) loses precision,
  // and mapping it back to map coordinates in the mouseMove event handler
  // results in a position different from geoPos, and hence the user-input
  // may get altered
  mIgnoreNextMoveEvent = true;

  mInputWidget->adjustCursorAndExtent( geoPos );

  if ( state()->status == StatusDrawing )
  {
    moveEvent( geoPos );
    update();
  }
  updateInputWidget( geoPos );
}

bool QgsMapToolDrawShape::pointInPolygon( const QgsPoint& p, const QList<QgsPoint> &poly )
{
  bool contains = false;
  for ( int n = poly.size(), i = 0, j = n - 1; i < n; j = i++ )
  {
    const QgsPoint& pi = poly[i];
    const QgsPoint& pj = poly[j];
    if (
      (( pi.y() > p.y() ) != ( pj.y() > p.y() ) ) &&
      ( p.x() < ( pj.x() - pi.x() ) * ( p.y() - pi.y() ) / ( pj.y() - pi.y() ) + pi.x() )
    )
    {
      contains = !contains;
    }
  }
  return contains;
}

QgsPoint QgsMapToolDrawShape::projPointOnSegment( const QgsPoint &p, const QgsPoint &s1, const QgsPoint &s2 )
{
  double nx = s2.y() - s1.y();
  double ny = -( s2.x() - s1.x() );
  double t = ( p.x() * ny - p.y() * nx - s1.x() * ny + s1.y() * nx ) / (( s2.x() - s1.x() ) * ny - ( s2.y() - s1.y() ) * nx );
  return t < 0. ? s1 : t > 1. ? s2 : QgsPoint( s1.x() + ( s2.x() - s1.x() ) * t, s1.y() + ( s2.y() - s1.y() ) * t );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawPoint::QgsMapToolDrawPoint( QgsMapCanvas *canvas )
    : QgsMapToolDrawShape( canvas, false, emptyState() )
{
  mRubberBand->setIconType( QgsGeometryRubberBand::ICON_CIRCLE );
}

QgsMapToolDrawShape::State* QgsMapToolDrawPoint::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  return newState;
}

void QgsMapToolDrawPoint::buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button )
{
  if ( !press && button == Qt::LeftButton )
  {
    State* newState = cloneState();
    newState->points.append( QList<QgsPoint>() << pos );
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

QgsAbstractGeometryV2* QgsMapToolDrawPoint::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPointV2();
  foreach ( const QList<QgsPoint>& part, state()->points )
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

void QgsMapToolDrawPoint::doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t )
{
  State* newState = cloneState();
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    if ( newState->points.isEmpty() || !newState->points.back().isEmpty() )
    {
      newState->points.append( QList<QgsPoint>() );
    }
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      for ( int iVtx = 0, nVtx = geometry->vertexCount( iPart, iRing ); iVtx < nVtx; ++iVtx )
      {
        QgsPointV2 vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, iVtx ) );
        newState->points.back().append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
      }
    }
  }
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void QgsMapToolDrawPoint::initInputWidget()
{
  mXEdit = new QgsFloatingInputWidgetField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new QgsFloatingInputWidgetField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "y:", mYEdit );
}

void QgsMapToolDrawPoint::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void QgsMapToolDrawPoint::inputAccepted()
{
  State* newState = cloneState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  newState->points.append( QList<QgsPoint>() << QgsPoint( x, y ) );
  mInputWidget->setFocusedInputField( mXEdit );
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void QgsMapToolDrawPoint::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
}

QgsMapToolDrawShape::EditContext* QgsMapToolDrawPoint::getEditContext( const QgsPoint& pos ) const
{
  int closestIndex = -1;
  double closestDistance = std::numeric_limits<double>::max();
  for ( int i = 0, n = state()->points.size(); i < n; ++i )
  {
    double distance = state()->points[i].front().sqrDist( pos );
    if ( distance < closestDistance )
    {
      closestDistance = distance;
      closestIndex = i;
    }
  }
  if ( closestIndex == -1 )
  {
    return 0;
  }
  QPoint p1 = toCanvasCoordinates( pos );
  QPoint p2 = toCanvasCoordinates( state()->points[closestIndex].front() );
  if ( qAbs(( p2 - p1 ).manhattanLength() ) > 10 )
  {
    return 0;
  }
  EditContext* context = new EditContext;
  context->index = closestIndex;
  return context;
}

void QgsMapToolDrawPoint::edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector &/*delta*/ )
{
  EditContext* ctx = static_cast<EditContext*>( context );
  mutableState()->points[ctx->index].front() = pos;
}

void QgsMapToolDrawPoint::setPart( int part, const QgsPoint& p )
{
  State* newState = cloneState();
  newState->points[part].front() = p;
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawPolyLine::QgsMapToolDrawPolyLine( QgsMapCanvas *canvas, bool closed, bool geodesic )
    : QgsMapToolDrawShape( canvas, closed, emptyState() ), mGeodesic( geodesic )
{
  if ( geodesic )
  {
    mDa.setEllipsoid( "WGS84" );
    mDa.setEllipsoidalMode( true );
    mDa.setSourceCrs( QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
  }
}

QgsMapToolDrawShape::State* QgsMapToolDrawPolyLine::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  newState->points.append( QList<QgsPoint>() );
  return newState;
}

void QgsMapToolDrawPolyLine::buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton )
  {
    State* newState = cloneState();
    if ( newState->points.back().isEmpty() )
    {
      newState->points.back().append( pos );
    }
    newState->points.back().append( pos );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( !press && button == Qt::RightButton )
  {
    if (( mIsArea && state()->points.back().size() >= 3 ) || state()->points.back().size() >= 2 )
    {
      State* newState = cloneState();
      // If last two points are very close, discard last point
      // (To avoid confusion if user left-clicks to set point and right-clicks to terminate)
      int nPoints = newState->points.back().size();
      QPoint p1 = toCanvasCoordinates( newState->points.back()[nPoints - 2] );
      QPoint p2 = toCanvasCoordinates( newState->points.back()[nPoints - 1] );
      if ( qAbs(( p2  - p1 ).manhattanLength() ) < 10 )
      {
        newState->points.back().pop_back();
      }
      if ( mMultipart )
      {
        newState->points.append( QList<QgsPoint>() );
        newState->status = StatusReady;
      }
      else
      {
        newState->status = StatusFinished;
      }
      mStateStack.updateState( newState );
    }
  }
}

void QgsMapToolDrawPolyLine::moveEvent( const QgsPoint &pos )
{
  mutableState()->points.back().back() = pos;
  update();
}

QgsAbstractGeometryV2* QgsMapToolDrawPolyLine::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> *hiddenNodes ) const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = mIsArea ? static_cast<QgsGeometryCollectionV2*>( new QgsMultiPolygonV2() ) : static_cast<QgsGeometryCollectionV2*>( new QgsMultiLineStringV2() );
  for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
  {
    const QList<QgsPoint>& part = state()->points[iPart];
    QgsLineStringV2* ring = new QgsLineStringV2();
    if ( mGeodesic )
    {
      int nPoints = part.size();
      if ( nPoints < 2 )
      {
        continue;
      }
      const QgsCoordinateTransform* t1 = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), "EPSG:4326" );
      const QgsCoordinateTransform* t2 = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", targetCrs.authid() );
      QList<QgsPoint> wgsPoints;

      GeographicLib::Geodesic geod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() );

      foreach ( const QgsPoint& point, part )
      {
        wgsPoints.append( t1->transform( point ) );
      }

      double sdist = 500000; // 500km segments
      for ( int i = 0; i < nPoints - 1; ++i )
      {
        int ringSize = ring->vertexCount();
        GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[i].y(), wgsPoints[i].x(), wgsPoints[i+1].y(), wgsPoints[i+1].x() );
        double dist = line.Distance();
        int nIntervals = qMax( 1, int( std::ceil( dist / sdist ) ) );
        for ( int j = 0; j < nIntervals; ++j )
        {
          double lat, lon;
          line.Position( j * sdist, lat, lon );
          ring->addVertex( QgsPointV2( t2->transform( QgsPoint( lon, lat ) ) ) );
          if ( hiddenNodes && j != 0 )
          {
            hiddenNodes->append( QgsVertexId( iPart, 0, ringSize + j ) );
          }
        }
        if ( !mIsArea && i == nPoints - 2 )
        {
          double lat, lon;
          line.Position( dist, lat, lon );
          ring->addVertex( QgsPointV2( t2->transform( QgsPoint( lon, lat ) ) ) );
        }
      }
      if ( mIsArea && !part.isEmpty() )
      {
        GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[nPoints -1].y(), wgsPoints[nPoints -1].x(), wgsPoints[0].y(), wgsPoints[0].x() );
        double dist = line.Distance();
        int nIntervals = qMax( 1, int( std::ceil( dist / sdist ) ) );
        int ringSize = ring->vertexCount();
        for ( int j = 0; j < nIntervals; ++j )
        {
          double lat, lon;
          line.Position( j * sdist, lat, lon );
          ring->addVertex( QgsPointV2( t2->transform( QgsPoint( lon, lat ) ) ) );
          if ( hiddenNodes && j != 0 )
          {
            hiddenNodes->append( QgsVertexId( iPart, 0, ringSize + j ) );
          }
        }
        ring->addVertex( ring->vertexAt( QgsVertexId( 0, 0, 0 ) ) );
      }
    }
    else
    {
      foreach ( const QgsPoint& point, part )
      {
        ring->addVertex( QgsPointV2( t->transform( point ) ) );
      }
      if ( mIsArea && !part.isEmpty() )
      {
        ring->addVertex( QgsPointV2( t->transform( part.front() ) ) );
      }
    }

    if ( mIsArea )
    {
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
  State* newState = cloneState();
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    if ( !newState->points.back().isEmpty() )
    {
      newState->points.append( QList<QgsPoint>() );
    }
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      for ( int iVtx = 0, nVtx = geometry->vertexCount( iPart, iRing ); iVtx < nVtx; ++iVtx )
      {
        QgsPointV2 vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, iVtx ) );
        newState->points.back().append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
      }
      // Don't add closing vertex
      if ( mIsArea && !newState->points.back().isEmpty() )
      {
        newState->points.back().removeLast();
      }
    }
  }
  if ( mMultipart )
  {
    newState->points.append( QList<QgsPoint>() );
    newState->status = StatusReady;
  }
  else
  {
    newState->status = StatusFinished;
  }
  mStateStack.updateState( newState );
}

void QgsMapToolDrawPolyLine::initInputWidget()
{
  mXEdit = new QgsFloatingInputWidgetField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new QgsFloatingInputWidgetField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "y:", mYEdit );
}

void QgsMapToolDrawPolyLine::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void QgsMapToolDrawPolyLine::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->points.back().append( QgsPoint( x, y ) );
    newState->points.back().append( QgsPoint( x, y ) );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    // At least two points and enter pressed twice -> finish
    int n = newState->points.back().size();
    if ( n > 1 && newState->points.back()[n - 2] == newState->points.back()[n - 1] )
    {
      mInputWidget->setFocusedInputField( mXEdit );
      newState->points.back().removeLast();
      newState->points.append( QList<QgsPoint>() );
      newState->status = mMultipart ? StatusReady : StatusFinished;
      mStateStack.updateState( newState, true );
      return;
    }
    newState->points.back().back() = QgsPoint( x, y );
    newState->points.back().append( QgsPoint( x, y ) );
    mStateStack.updateState( newState );
  }
}

void QgsMapToolDrawPolyLine::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
}

QgsMapToolDrawShape::EditContext* QgsMapToolDrawPolyLine::getEditContext( const QgsPoint& pos ) const
{
  int closestPart = -1;
  int closestNode = -1;
  double closestDistance = std::numeric_limits<double>::max();
  for ( int i = 0, n = state()->points.size(); i < n; ++i )
  {
    for ( int j = 0, m = state()->points[i].size(); j < m; ++j )
    {
      double distance = state()->points[i][j].sqrDist( pos );
      if ( distance < closestDistance )
      {
        closestDistance = distance;
        closestPart = i;
        closestNode = j;
      }
    }
  }
  // Check if a node was picked
  if ( closestNode != -1 )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( state()->points[closestPart][closestNode] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->node = closestNode;
      return context;
    }
  }
  // Check whether entire geometry was picked
  if ( mIsArea )
  {
    for ( int i = 0, n = state()->points.size(); i < n; ++i )
    {
      if ( pointInPolygon( pos, state()->points[i] ) )
      {
        EditContext* context = new EditContext;
        context->part = closestPart;
        context->node = -1;
        return context;
      }
    }
  }
  else
  {
    QPoint p1 = toCanvasCoordinates( pos );
    for ( int i = 0, n = state()->points.size(); i < n; ++i )
    {
      const QList<QgsPoint>& poly = state()->points[i];
      for ( int m = poly.size(), j = m - 1, k = 0; k < m - 1; j = k++ )
      {
        QgsPoint pProj = projPointOnSegment( pos, poly[j], poly[k] );
        QPoint p2 = toCanvasCoordinates( pProj );
        if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
        {
          EditContext* context = new EditContext;
          context->part = closestPart;
          context->node = -1;
          return context;
        }
      }
    }
  }
  return 0;
}

void QgsMapToolDrawPolyLine::edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta )
{
  EditContext* ctx = static_cast<EditContext*>( context );
  if ( ctx->node != -1 )
  {
    mutableState()->points[ctx->part][ctx->node] = pos;
  }
  else
  {
    QList<QgsPoint>& poly = mutableState()->points[ctx->part];
    for ( int i = 0, n = poly.size(); i < n; ++i )
    {
      poly[i] += delta;
    }
  }
}

void QgsMapToolDrawPolyLine::addContextMenuActions( const QgsMapToolDrawShape::EditContext* context, QMenu& menu ) const
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( ctx->node != -1 )
  {
    QAction* deleteNodeAction = menu.addAction( QIcon( ":/images/themes/default/mActionDeleteVertex.png" ), tr( "Delete node" ) );
    deleteNodeAction->setData( DeleteNode );
    deleteNodeAction->setEnabled( state()->points[ctx->part].length() >= 3 + mIsArea );
    if ( mIsArea || ( ctx->node == 0 || ctx->node == state()->points[ctx->part].length() - 1 ) )
    {
      QAction* continueAction = menu.addAction( QIcon( ":/images/themes/default/mActionMoveVertex.png" ), tr( "Continue drawing" ) );
      continueAction->setData( ContinueGeometry );
    }
  }
  else
  {
    QAction* addNodeAction = menu.addAction( QIcon( ":/images/themes/default/mActionAddVertex.png" ), tr( "Add node" ) );
    addNodeAction->setData( AddNode );
  }
}

void QgsMapToolDrawPolyLine::executeContextMenuAction( const QgsMapToolDrawShape::EditContext *context, int action, const QgsPoint& pos )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( action == DeleteNode )
  {
    State* newstate = cloneState();
    newstate->points[ctx->part].removeAt( ctx->node );
    mStateStack.updateState( newstate );
  }
  else if ( action == AddNode )
  {
    // Find closest segment
    int closestIdx = -1;
    double closestDist = std::numeric_limits<double>::max();
    State* newstate = cloneState();
    const QList<QgsPoint>& poly = newstate->points[ctx->part];
    for ( int m = poly.size(), j = m - 1, k = 0; k < m - 1; j = k++ )
    {
      QgsPoint pProj = projPointOnSegment( pos, poly[j], poly[k] );
      double dist = pProj.sqrDist( pos );
      if ( dist < closestDist )
      {
        closestIdx = k;
        closestDist = dist;
      }
    }
    if ( closestIdx != -1 )
    {
      newstate->points[ctx->part].insert( closestIdx, pos );
      mStateStack.updateState( newstate );
    }
  }
  else if ( action == ContinueGeometry )
  {
    State* newstate = cloneState();
    if ( !mIsArea )
    {
      // Reverse points if continuing from start
      if ( ctx->node == 0 )
      {
        QList<QgsPoint> reversed;
        foreach ( const QgsPoint& p, newstate->points[ctx->part] )
        {
          reversed.prepend( p );
        }
        newstate->points[ctx->part] = reversed;
      }
    }
    else
    {
      // Rearrange points so that picked point is last
      QList<QgsPoint> rearranged;
      int n = newstate->points[ctx->part].size();
      for ( int i = ( ctx->node + 1 ) % n; i  != ctx->node; i = ( i + 1 ) % n )
      {
        rearranged.append( newstate->points[ctx->part][i] );
      }
      rearranged.append( newstate->points[ctx->part][ctx->node] );
      newstate->points[ctx->part] = rearranged;
    }
    newstate->points[ctx->part].append( newstate->points[ctx->part].last() );
    newstate->status = StatusDrawing;
    mStateStack.updateState( newstate );
  }
}

void QgsMapToolDrawPolyLine::setPart( int part, const QList<QgsPoint>& p )
{
  State* newState = cloneState();
  newState->points[part] = p;
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawRectangle::QgsMapToolDrawRectangle( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true, emptyState() ) {}

QgsMapToolDrawShape::State* QgsMapToolDrawRectangle::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  return newState;
}

void QgsMapToolDrawRectangle::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton && state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->p1.append( pos );
    newState->p2.append( pos );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( !press && state()->status == StatusDrawing && state()->p1.back() != pos )
  {
    State* newState = cloneState();
    newState->p2.back() = pos;
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void QgsMapToolDrawRectangle::moveEvent( const QgsPoint &pos )
{
  mutableState()->p2.back() = pos;
  update();
}

QgsAbstractGeometryV2* QgsMapToolDrawRectangle::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2;
  for ( int i = 0, n = state()->p1.size(); i < n; ++i )
  {
    const QgsPoint& p1 = state()->p1[i];
    const QgsPoint& p2 = state()->p2[i];
    QgsLineStringV2* ring = new QgsLineStringV2;
    ring->addVertex( QgsPointV2( t->transform( p1 ) ) );
    ring->addVertex( QgsPointV2( t->transform( p2.x(), p1.y() ) ) );
    ring->addVertex( QgsPointV2( t->transform( p2 ) ) );
    ring->addVertex( QgsPointV2( t->transform( p1.x(), p2.y() ) ) );
    ring->addVertex( QgsPointV2( t->transform( p1 ) ) );
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
  State* newState = cloneState();
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      if ( geometry->vertexCount( iPart, iRing ) == 5 )
      {
        QgsPointV2 vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, 0 ) );
        newState->p1.append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
        vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, 2 ) );
        newState->p2.append( t.transform( QgsPoint( vertex.x(), vertex.y() ) ) );
      }
    }
  }
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void QgsMapToolDrawRectangle::initInputWidget()
{
  mXEdit = new QgsFloatingInputWidgetField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new QgsFloatingInputWidgetField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( inputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "y:", mYEdit );
}

void QgsMapToolDrawRectangle::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void QgsMapToolDrawRectangle::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  mInputWidget->setFocusedInputField( mXEdit );
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->p1.append( QgsPoint( x, y ) );
    newState->p2.append( QgsPoint( x, y ) );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    newState->p2.back() = QgsPoint( x, y );
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void QgsMapToolDrawRectangle::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
}


QgsMapToolDrawShape::EditContext* QgsMapToolDrawRectangle::getEditContext( const QgsPoint& pos ) const
{
  int closestPart = -1;
  int closestPoint = -1;
  double closestDistance = std::numeric_limits<double>::max();
  QList< QList<QgsPoint> > points;
  for ( int i = 0, n = state()->p1.size(); i < n; ++i )
  {
    points.append( QList<QgsPoint>()
                   << state()->p1[i]
                   << QgsPoint( state()->p2[i].x(), state()->p1[i].y() )
                   << state()->p2[i]
                   << QgsPoint( state()->p1[i].x(), state()->p2[i].y() ) );
    for ( int j = 0, m = points[i].size(); j < m; ++j )
    {
      double dist = points[i][j].sqrDist( pos );
      if ( dist < closestDistance )
      {
        closestDistance = dist;
        closestPart = i;
        closestPoint = j;
      }
    }
  }
  // Check if a node was picked
  if ( closestPart != -1 )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( points[closestPart][closestPoint] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->point = closestPoint;
      return context;
    }
  }
  // Check whether entire geometry was picked
  for ( int i = 0, n = points.size(); i < n; ++i )
  {
    if ( pointInPolygon( pos, points[i] ) )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->point = -1;
      return context;
    }
  }
  return 0;
}

void QgsMapToolDrawRectangle::edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta )
{
  EditContext* ctx = static_cast<EditContext*>( context );
  if ( ctx->point == 0 )
  {
    mutableState()->p1[ctx->part] = pos;
  }
  else if ( ctx->point == 1 )
  {
    mutableState()->p2[ctx->part].setX( pos.x() );
    mutableState()->p1[ctx->part].setY( pos.y() );
  }
  else if ( ctx->point == 2 )
  {
    mutableState()->p2[ctx->part] = pos;
  }
  else if ( ctx->point == 3 )
  {
    mutableState()->p1[ctx->part].setX( pos.x() );
    mutableState()->p2[ctx->part].setY( pos.y() );
  }
  else if ( ctx->point == -1 )
  {
    mutableState()->p1[ctx->part] += delta;
    mutableState()->p2[ctx->part] += delta;
  }
}

void QgsMapToolDrawRectangle::setPart( int part, const QgsPoint& p1, const QgsPoint& p2 )
{
  State* newState = cloneState();
  newState->p1[part] = p1;
  newState->p2[part] = p2;
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

class GeodesicCircleMeasurer : public QgsGeometryRubberBand::Measurer
{
  public:
    GeodesicCircleMeasurer( QgsMapToolDrawCircle* tool ) : mTool( tool ) {}
    QList<Measurement> measure( QgsGeometryRubberBand::MeasurementMode measurementMode, int part, const QgsAbstractGeometryV2* /*geometry*/, QList<double>& partMeasurements )
    {
      QList<Measurement> measurements;
      if ( measurementMode == QgsGeometryRubberBand::MEASURE_CIRCLE )
      {
        const QgsPoint& c = mTool->state()->centers[mTool->mPartMap[part]];
        const QgsPoint& p = mTool->state()->ringPos[mTool->mPartMap[part]];
        const QgsCoordinateTransform* t1 = QgsCoordinateTransformCache::instance()->transform( mTool->canvas()->mapSettings().destinationCrs().authid(), "EPSG:4326" );
        double radius = mTool->mDa.measureLine( t1->transform( c ), t1->transform( p ) );

        double area = radius * radius * M_PI;
        partMeasurements.append( area );

        Measurement areaMeasurement = {Measurement::Area, "", area};
        measurements.append( areaMeasurement );
        Measurement radiusMeasurement = {Measurement::Length, QApplication::translate( "GeodesicCircleMeasurer", "Radius" ), radius};
        measurements.append( radiusMeasurement );
      }
      return measurements;
    }
  private:
    QgsMapToolDrawCircle* mTool;
};

QgsMapToolDrawCircle::QgsMapToolDrawCircle( QgsMapCanvas* canvas , bool geodesic )
    : QgsMapToolDrawShape( canvas, true, emptyState() ), mGeodesic( geodesic )
{
  if ( geodesic )
  {
    mDa.setEllipsoid( "WGS84" );
    mDa.setEllipsoidalMode( true );
    mDa.setSourceCrs( QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
    mRubberBand->setMeasurer( new GeodesicCircleMeasurer( this ) );
  }
}

QgsMapToolDrawShape::State* QgsMapToolDrawCircle::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  return newState;
}

void QgsMapToolDrawCircle::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton && state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->centers.append( pos );
    newState->ringPos.append( pos );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( !press && state()->status == StatusDrawing && state()->centers.back() != pos )
  {
    State* newState = cloneState();
    newState->ringPos.back() = pos;
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void QgsMapToolDrawCircle::moveEvent( const QgsPoint &pos )
{
  mutableState()->ringPos.back() = pos;
  update();
}

void QgsMapToolDrawCircle::getPart( int part, QgsPoint &center, double &radius ) const
{
  center = state()->centers[part];
  radius = qSqrt( state()->centers[part].sqrDist( state()->ringPos[part] ) );
}

QgsAbstractGeometryV2* QgsMapToolDrawCircle::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  mPartMap.clear();
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    // 1 deg segmentized circle around center
    if ( mGeodesic )
    {
      const QgsCoordinateTransform* t1 = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), "EPSG:4326" );
      const QgsCoordinateTransform* t2 = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", targetCrs.authid() );
      const QgsPoint& center = state()->centers[i];
      const QgsPoint& ringPos = state()->ringPos[i];
      QgsPoint p1 = t1->transform( center.x(), center.y() );
      QgsPoint p2 = t1->transform( ringPos.x(), ringPos.y() );
      double clampLatitude = targetCrs.authid() == "EPSG:3857" ? 85 : 90;
      if ( p2.y() > 90 )
      {
        p2.setY( 90. - ( p2.y() - 90. ) );
      }
      if ( p2.y() < -90 )
      {
        p2.setY( -90. - ( p2.y() + 90. ) );
      }
      double radius = mDa.measureLine( p1, p2 );
      QList<QgsPoint> wgsPoints;
      for ( int a = 0; a < 360; ++a )
      {
        wgsPoints.append( mDa.computeDestination( p1, radius, a ) );
      }
      // Check if area would cross north or south pole
      // -> Check if destination point at bearing 0 / 180 with given radius would flip longitude
      // -> If crosses north/south pole, add points at lat 90 resp. -90 between points with max resp. min latitude
      QgsPoint pn = mDa.computeDestination( p1, radius, 0 );
      QgsPoint ps = mDa.computeDestination( p1, radius, 180 );
      int shift = 0;
      int nPoints = wgsPoints.size();
      if ( qFuzzyCompare( qAbs( pn.x() - p1.x() ), 180 ) )   // crosses north pole
      {
        wgsPoints[nPoints-1].setX( p1.x() - 179.999 );
        wgsPoints[1].setX( p1.x() + 179.999 );
        wgsPoints.append( QgsPoint( p1.x() - 179.999, clampLatitude ) );
        wgsPoints[0] = QgsPoint( p1.x() + 179.999, clampLatitude );
        wgsPoints.prepend( QgsPoint( p1.x(), clampLatitude ) ); // Needed to ensure first point does not overflow in longitude below
        wgsPoints.append( QgsPoint( p1.x(), clampLatitude ) ); // Needed to ensure last point does not overflow in longitude below
        shift = 3;
      }
      if ( qFuzzyCompare( qAbs( ps.x() - p1.x() ), 180 ) )   // crosses south pole
      {
        wgsPoints[181 + shift].setX( p1.x() - 179.999 );
        wgsPoints[179 + shift].setX( p1.x() + 179.999 );
        wgsPoints[180 + shift] = QgsPoint( p1.x() - 179.999, -clampLatitude );
        wgsPoints.insert( 180 + shift, QgsPoint( p1.x() + 179.999, -clampLatitude ) );
      }
      // Check if area overflows in longitude
      // 0: left-overflow, 1: center, 2: right-overflow
      QList<QgsPoint> poly[3];
      int current = 1;
      poly[1].append( wgsPoints[0] ); // First point is always in center region
      nPoints = wgsPoints.size();
      for ( int j = 1; j < nPoints; ++j )
      {
        const QgsPoint& p = wgsPoints[j];
        if ( p.x() > 180. )
        {
          // Right-overflow
          if ( current == 1 )
          {
            poly[1].append( QgsPoint( 180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            poly[2].append( QgsPoint( -180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            current = 2;
          }
          poly[2].append( QgsPoint( p.x() - 360., p.y() ) );
        }
        else if ( p.x() < -180 )
        {
          // Left-overflow
          if ( current == 1 )
          {
            poly[1].append( QgsPoint( -180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            poly[0].append( QgsPoint( 180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            current = 0;
          }
          poly[0].append( QgsPoint( p.x() + 360., p.y() ) );
        }
        else
        {
          // No overflow
          if ( current == 0 )
          {
            poly[0].append( QgsPoint( 180, 0.5 * ( poly[0].back().y() + p.y() ) ) );
            poly[1].append( QgsPoint( -180, 0.5 * ( poly[0].back().y() + p.y() ) ) );
            current = 1;
          }
          else if ( current == 2 )
          {
            poly[2].append( QgsPoint( -180, 0.5 * ( poly[2].back().y() + p.y() ) ) );
            poly[1].append( QgsPoint( 180, 0.5 * ( poly[2].back().y() + p.y() ) ) );
            current = 1;
          }
          poly[1].append( p );
        }
      }
      for ( int j = 0; j < 3; ++j )
      {
        if ( !poly[j].isEmpty() )
        {
          QList<QgsPointV2> points;
          for ( int k = 0, n = poly[j].size(); k < n; ++k )
          {
            poly[j][k].setY( qMin( clampLatitude, qMax( -clampLatitude, poly[j][k].y() ) ) );
            try
            {
              points.append( QgsPointV2( t2->transform( poly[j][k] ) ) );
            }
            catch ( ... )
              {}
          }
          QgsLineStringV2* ring = new QgsLineStringV2();
          ring->setPoints( points );
          QgsPolygonV2* poly = new QgsPolygonV2();
          poly->setExteriorRing( ring );
          multiGeom->addGeometry( poly );
          mPartMap.append( i );
        }
      }
    }
    else
    {
      const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
      const QgsPoint& center = state()->centers[i];
      const QgsPoint& ringPos = state()->ringPos[i];
      double radius = qSqrt( center.sqrDist( ringPos ) );
      QgsCircularStringV2* ring = new QgsCircularStringV2();
      ring->setPoints( QList<QgsPointV2>()
                       << QgsPointV2( t->transform( center.x() + radius, center.y() ) )
                       << QgsPointV2( t->transform( center.x(), center.y() ) )
                       << QgsPointV2( t->transform( center.x() + radius, center.y() ) ) );
      QgsCurvePolygonV2* poly = new QgsCurvePolygonV2();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
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

void QgsMapToolDrawCircle::doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t )
{
  State* newState = cloneState();
  if ( dynamic_cast<const QgsGeometryCollectionV2*>( geometry ) )
  {
    const QgsGeometryCollectionV2* geomCollection = static_cast<const QgsGeometryCollectionV2*>( geometry );
    for ( int i = 0, n = geomCollection->numGeometries(); i < n; ++i )
    {
      QgsRectangle bb = geomCollection->geometryN( i )->boundingBox();
      QgsPoint c = t.transform( bb.center() );
      QgsPoint r = t.transform( QgsPoint( bb.xMaximum(), bb.center().y() ) );
      newState->centers.append( c );
      newState->ringPos.append( r );
    }
  }
  else
  {
    QgsRectangle bb = geometry->boundingBox();
    QgsPoint c = t.transform( bb.center() );
    QgsPoint r = t.transform( QgsPoint( bb.xMaximum(), bb.center().y() ) );
    newState->centers.append( c );
    newState->ringPos.append( r );
  }
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void QgsMapToolDrawCircle::initInputWidget()
{
  mXEdit = new QgsFloatingInputWidgetField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new QgsFloatingInputWidgetField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "y:", mYEdit );
  QDoubleValidator* validator = new QDoubleValidator();
  validator->setBottom( 0 );
  mREdit = new QgsFloatingInputWidgetField( validator );
  connect( mREdit, SIGNAL( inputChanged() ), this, SLOT( radiusInputChanged() ) );
  connect( mREdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "r:", mREdit );
}

void QgsMapToolDrawCircle::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  if ( state()->status == StatusReady )
  {
    mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
    mREdit->setText( "0" );
  }
  else if ( state()->status == StatusDrawing )
  {
    mREdit->setText( QString::number( qSqrt( state()->centers.last().sqrDist( state()->ringPos.last() ) ), 'f', isDegrees ? 4 : 0 ) );
  }
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void QgsMapToolDrawCircle::inputAccepted()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  mInputWidget->setFocusedInputField( mXEdit );
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->centers.append( QgsPoint( x, y ) );
    newState->ringPos.append( QgsPoint( x + r, y ) );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    mREdit->setText( "0" );
    newState->centers.back() = QgsPoint( x, y );
    newState->ringPos.back() = QgsPoint( x + r, y );
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void QgsMapToolDrawCircle::centerInputChanged()
{
  State* state = mutableState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->ringPos.append( QgsPoint( x + r, y ) );
    state->status = StatusDrawing;
  }
  else
  {
    state->centers.back() = QgsPoint( x, y );
  }
  update();
  moveMouseToPos( QgsPoint( x + r, y ) );
}

void QgsMapToolDrawCircle::radiusInputChanged()
{
  State* state = mutableState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->ringPos.append( QgsPoint( x + r, y ) );
    state->status = StatusDrawing;
  }
  else
  {
    state->ringPos.back() = QgsPoint( x + r, y );
  }
  update();
  moveMouseToPos( QgsPoint( x + r, y ) );
}

QgsMapToolDrawShape::EditContext* QgsMapToolDrawCircle::getEditContext( const QgsPoint& pos ) const
{
  int closestPart = -1;
  double closestDistance = std::numeric_limits<double>::max();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    double dist = state()->ringPos[i].sqrDist( pos );
    if ( dist < closestDistance )
    {
      closestDistance = dist;
      closestPart = i;
    }
  }
  // Check if a node was picked
  if ( closestPart != -1 )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( state()->ringPos[closestPart] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->point = 0;
      return context;
    }
  }
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    double radiusSqr = state()->centers[i].sqrDist( state()->ringPos[i] );
    if ( pos.sqrDist( state()->centers[i] ) <= radiusSqr )
    {
      EditContext* context = new EditContext;
      context->part = i;
      context->point = -1;
      return context;
    }
  }
  return 0;
}

void QgsMapToolDrawCircle::edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta )
{
  EditContext* ctx = static_cast<EditContext*>( context );
  if ( ctx->point == 0 )
  {
    mutableState()->ringPos[ctx->part] = pos;
  }
  else if ( ctx->point == -1 )
  {
    mutableState()->centers[ctx->part] += delta;
    mutableState()->ringPos[ctx->part] += delta;
  }
}

void QgsMapToolDrawCircle::setPart( int part, const QgsPoint& center, double radius )
{
  State* newState = cloneState();
  newState->centers[part] = center;
  newState->ringPos[part] = QgsPoint( center.x() + radius, center.y() );
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolDrawCircularSector::QgsMapToolDrawCircularSector( QgsMapCanvas* canvas )
    : QgsMapToolDrawShape( canvas, true, emptyState() ) {}

QgsMapToolDrawShape::State* QgsMapToolDrawCircularSector::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  newState->sectorStatus = HaveNothing;
  return newState;
}

void QgsMapToolDrawCircularSector::buttonEvent( const QgsPoint &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton )
  {
    if ( state()->sectorStatus == HaveNothing )
    {
      State* newState = cloneState();
      newState->centers.append( pos );
      newState->radii.append( 0 );
      newState->startAngles.append( 0 );
      newState->stopAngles.append( 0 );
      newState->sectorStatus = HaveCenter;
      newState->status = StatusDrawing;
      mStateStack.updateState( newState );
    }
    else if ( state()->sectorStatus == HaveCenter )
    {
      State* newState = cloneState();
      newState->sectorStatus = HaveRadius;
      newState->status = StatusDrawing;
      mStateStack.updateState( newState );
    }
    else if ( state()->sectorStatus == HaveRadius )
    {
      State* newState = cloneState();
      newState->sectorStatus = HaveNothing;
      newState->status = mMultipart ? StatusReady : StatusFinished;
      mStateStack.updateState( newState );
    }
  }
}

void QgsMapToolDrawCircularSector::moveEvent( const QgsPoint &pos )
{
  State* state = mutableState();
  if ( state->sectorStatus == HaveCenter )
  {
    state->radii.back() = qSqrt( pos.sqrDist( state->centers.back() ) );
    state->startAngles.back() = state->stopAngles.back() = qAtan2( pos.y() - state->centers.back().y(), pos.x() - state->centers.back().x() );
  }
  else if ( state->sectorStatus == HaveRadius )
  {
    state->stopAngles.back() = qAtan2( pos.y() - state->centers.back().y(), pos.x() - state->centers.back().x() );
    if ( state->stopAngles.back() <= state->startAngles.back() )
    {
      state->stopAngles.back() += 2 * M_PI;
    }

    // Snap to full circle if within 5px
    const QgsPoint& center = state->centers.back();
    const double& radius = state->radii.back();
    const double& startAngle = state->startAngles.back();
    const double& stopAngle = state->stopAngles.back();
    QgsPoint pStart( center.x() + radius * qCos( startAngle ),
                     center.y() + radius * qSin( startAngle ) );
    QgsPoint pEnd( center.x() + radius * qCos( stopAngle ),
                   center.y() + radius * qSin( stopAngle ) );
    QPoint diff = toCanvasCoordinates( pEnd ) - toCanvasCoordinates( pStart );
    if (( diff.x() * diff.x() + diff.y() * diff.y() ) < 25 )
    {
      state->stopAngles.back() = state->startAngles.back() + 2 * M_PI;
    }
  }
  update();
}

QgsAbstractGeometryV2* QgsMapToolDrawCircularSector::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), targetCrs.authid() );
  QgsGeometryCollectionV2* multiGeom = new QgsMultiPolygonV2;
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    const QgsPoint& center = state()->centers[i];
    const double& radius = state()->radii[i];
    const double& startAngle = state()->startAngles[i];
    const double& stopAngle = state()->stopAngles[i];
    QgsPoint pStart, pMid, pEnd;
    if ( stopAngle == startAngle + 2 * M_PI )
    {
      pStart = pEnd = QgsPoint( center.x() + radius * qCos( stopAngle ),
                                center.y() + radius * qSin( stopAngle ) );
      pMid = center;
    }
    else
    {
      double alphaMid = 0.5 * ( startAngle + stopAngle - 2 * M_PI );
      pStart = QgsPoint( center.x() + radius * qCos( startAngle ),
                         center.y() + radius * qSin( startAngle ) );
      pMid = QgsPoint( center.x() + radius * qCos( alphaMid ),
                       center.y() + radius * qSin( alphaMid ) );
      pEnd = QgsPoint( center.x() + radius * qCos( stopAngle - 2 * M_PI ),
                       center.y() + radius * qSin( stopAngle - 2 * M_PI ) );
    }
    QgsCompoundCurveV2* exterior = new QgsCompoundCurveV2();
    if ( startAngle != stopAngle )
    {
      QgsCircularStringV2* arc = new QgsCircularStringV2();
      arc->setPoints( QList<QgsPointV2>() << t->transform( pStart ) << t->transform( pMid ) << t->transform( pEnd ) );
      exterior->addCurve( arc );
    }
    if ( startAngle != stopAngle + 2 * M_PI )
    {
      QgsLineStringV2* line = new QgsLineStringV2();
      line->setPoints( QList<QgsPointV2>() << t->transform( pEnd ) << t->transform( center ) << t->transform( pStart ) );
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
  /* Not yet implemented */
}

void QgsMapToolDrawCircularSector::initInputWidget()
{
  mXEdit = new QgsFloatingInputWidgetField();
  connect( mXEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mXEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new QgsFloatingInputWidgetField();
  connect( mYEdit, SIGNAL( inputChanged() ), this, SLOT( centerInputChanged() ) );
  connect( mYEdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "y:", mYEdit );
  QDoubleValidator* validator = new QDoubleValidator();
  validator->setBottom( 0 );
  mREdit = new QgsFloatingInputWidgetField( validator );
  connect( mREdit, SIGNAL( inputChanged() ), this, SLOT( arcInputChanged() ) );
  connect( mREdit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( "r:", mREdit );
  mA1Edit = new QgsFloatingInputWidgetField();
  connect( mA1Edit, SIGNAL( inputChanged() ), this, SLOT( arcInputChanged() ) );
  connect( mA1Edit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( QString( QChar( 0x03B1 ) ) + "1:", mA1Edit );
  mA2Edit = new QgsFloatingInputWidgetField();
  connect( mA2Edit, SIGNAL( inputChanged() ), this, SLOT( arcInputChanged() ) );
  connect( mA2Edit, SIGNAL( inputConfirmed() ), this, SLOT( acceptInput() ) );
  mInputWidget->addInputField( QString( QChar( 0x03B1 ) ) + "2:", mA2Edit );
}

void QgsMapToolDrawCircularSector::updateInputWidget( const QgsPoint& mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
  if ( state()->sectorStatus == HaveNothing )
  {
    mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
    mREdit->setText( "0" );
    mA1Edit->setText( "0" );
    mA2Edit->setText( "0" );
  }
  else if ( state()->sectorStatus == HaveCenter )
  {
    mREdit->setText( QString::number( state()->radii.last(), 'f', isDegrees ? 4 : 0 ) );
  }
  else if ( state()->sectorStatus == HaveRadius )
  {
    double startAngle = 2.5 * M_PI - state()->startAngles.last();
    if ( startAngle > 2 * M_PI )
      startAngle -= 2 * M_PI;
    else if ( startAngle < 0 )
      startAngle += 2 * M_PI;
    mA1Edit->setText( QString::number( startAngle / M_PI * 180., 'f', 1 ) );
    double stopAngle = 2.5 * M_PI - state()->stopAngles.last();
    if ( stopAngle > 2 * M_PI )
      stopAngle -= 2 * M_PI;
    else if ( stopAngle < 0 )
      stopAngle += 2 * M_PI;
    mA2Edit->setText( QString::number( stopAngle / M_PI * 180., 'f', 1 ) );
  }
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void QgsMapToolDrawCircularSector::inputAccepted()
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
  mInputWidget->setFocusedInputField( mXEdit );
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->centers.append( QgsPoint( x, y ) );
    newState->radii.append( r );
    if ( r > 0 )
    {
      newState->startAngles.append( a1 );
      newState->stopAngles.append( a2 );
      newState->sectorStatus = HaveRadius;
    }
    else
    {
      newState->startAngles.append( 0 );
      newState->stopAngles.append( 0 );
      newState->sectorStatus = HaveCenter;
    }
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    newState->centers.back() = QgsPoint( x, y );
    newState->radii.back() = r;
    if ( r > 0 )
    {
      newState->startAngles.back() = a1;
      newState->stopAngles.back() = a2;
      newState->sectorStatus = HaveRadius;
    }
    else
    {
      newState->startAngles.back() = 0;
      newState->stopAngles.back() = 0;
      newState->sectorStatus = HaveCenter;
    }
    if ( newState->sectorStatus == HaveRadius )
    {
      mREdit->setText( "0" );
      mA1Edit->setText( "0" );
      mA2Edit->setText( "0" );
      newState->status = mMultipart ? StatusReady : StatusFinished;
    }
    mStateStack.updateState( newState );
  }
}

void QgsMapToolDrawCircularSector::centerInputChanged()
{
  State* state = mutableState();
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
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->radii.append( r );
    if ( r > 0 )
    {
      state->startAngles.append( a1 );
      state->stopAngles.append( a2 );
      state->sectorStatus = HaveRadius;
    }
    else
    {
      state->startAngles.append( 0 );
      state->stopAngles.append( 0 );
      state->sectorStatus = HaveCenter;
    }
    state->status = StatusDrawing;
  }
  state->centers.back() = QgsPoint( x, y );
  update();
  moveMouseToPos( QgsPoint( x + r * qCos( state->stopAngles.back() ), y + r * qSin( state->stopAngles.back() ) ) );
}

void QgsMapToolDrawCircularSector::arcInputChanged()
{
  State* state = mutableState();
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
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->radii.append( r );
    if ( r > 0 )
    {
      state->startAngles.append( a1 );
      state->stopAngles.append( a2 );
      state->sectorStatus = HaveRadius;
    }
    else
    {
      state->startAngles.append( 0 );
      state->stopAngles.append( 0 );
      state->sectorStatus = HaveCenter;
    }
    state->status = StatusDrawing;
  }
  state->radii.back() = r;
  if ( r > 0 )
  {
    state->startAngles.back() = a1;
    state->stopAngles.back() = a2;
    state->sectorStatus = HaveRadius;
  }
  else
  {
    state->startAngles.back() = 0;
    state->stopAngles.back() = 0;
    state->sectorStatus = HaveCenter;
  }
  update();
  moveMouseToPos( QgsPoint( x + r * qCos( state->stopAngles.back() ), y + r * qSin( state->stopAngles.back() ) ) );
}

QgsMapToolDrawShape::EditContext* QgsMapToolDrawCircularSector::getEditContext( const QgsPoint& /*pos*/ ) const
{
  /* Currently not implemented */
  return 0;
}

void QgsMapToolDrawCircularSector::edit( EditContext* /*context*/, const QgsPoint& /*pos*/, const QgsVector& /*delta*/ )
{
  /* Currently not implemented */
}

void QgsMapToolDrawCircularSector::setPart( int part, const QgsPoint& center, double radius, double startAngle, double stopAngle )
{
  State* newState = cloneState();
  newState->centers[part] = center;
  newState->radii[part] = radius;
  newState->startAngles[part] = startAngle;
  newState->stopAngles[part] = stopAngle;
  mStateStack.updateState( newState );
}
