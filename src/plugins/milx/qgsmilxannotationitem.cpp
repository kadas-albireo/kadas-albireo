/***************************************************************************
 *  qgsmilxannotationitem.cpp                                              *
 *  -------------------------                                              *
 *  begin                : Oct 01, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmilxannotationitem.h"
#include "qgsmilxlayer.h"
#include "qgsbillboardregistry.h"
#include "qgsdistancearea.h"
#include "qgscrscache.h"
#include "MilXClient.hpp"
#include "qgsmapcanvas.h"
#include "qgscoordinatetransform.h"

#include <QPainter>
#include <QMenu>
#include <QVector2D>

REGISTER_QGS_ANNOTATION_ITEM( QgsMilXAnnotationItem )

QgsMilXAnnotationItem::QgsMilXAnnotationItem( QgsMapCanvas* canvas )
    : QgsAnnotationItem( canvas ), mFinalized( false )
{
  mOffsetFromReferencePoint = QPoint( 0, 0 );
  connect( canvas, SIGNAL( extentsChanged() ), this, SLOT( updateSymbol() ) );
}

QgsMilXAnnotationItem::QgsMilXAnnotationItem( QgsMapCanvas* canvas, QgsMilXAnnotationItem* source )
    : QgsAnnotationItem( canvas, source )
{
  setSymbolXml( source->mSymbolXml, source->mSymbolMilitaryName );
  mAdditionalPoints = source->mAdditionalPoints;
  mRenderOffset = source->mRenderOffset;
  mControlPoints = source->mControlPoints;
  mAttributes = source->mAttributes;
  mAttributePoints = source->mAttributePoints;
  mFinalized = source->mFinalized;
  updateSymbol( false );
  connect( canvas, SIGNAL( extentsChanged() ), this, SLOT( updateSymbol() ) );
}

QgsMilXAnnotationItem::~QgsMilXAnnotationItem()
{
  QgsBillBoardRegistry::instance()->removeItem( this );
}

void QgsMilXAnnotationItem::fromMilxItem( QgsMilXItem* item )
{
  const QgsCoordinateReferenceSystem& canvasCrs = mMapCanvas->mapSettings().destinationCrs();
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", canvasCrs.authid() );
  setMapPosition( crst->transform( item->points().front() ), canvasCrs );
  for ( int i = 1, n = item->points().size(); i < n; ++i )
  {
    mAdditionalPoints.append( crst->transform( item->points()[i] ) );
  }
  mControlPoints = item->controlPoints();
  mAttributes = item->attributes();
  setSymbolXml( item->mssString(), item->militaryName() );
  mFinalized = true;
  mOffsetFromReferencePoint = item->userOffset();
  updateSymbol( true );
}

QgsMilXItem* QgsMilXAnnotationItem::toMilxItem()
{
  Q_ASSERT( mFinalized );
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" );
  QList<QgsPoint> points;
  points.append( crst->transform( mGeoPos ) );
  foreach ( const QgsPoint& p, mAdditionalPoints )
  {
    points.append( crst->transform( p ) );
  }
  QgsMilXItem* item = new QgsMilXItem();
  item->initialize( mSymbolXml, mSymbolMilitaryName, points, mControlPoints, mAttributes, mOffsetFromReferencePoint.toPoint() - mRenderOffset );
  return item;
}

void QgsMilXAnnotationItem::setSymbolXml( const QString &symbolXml, const QString& symbolMilitaryName )
{
  mSymbolXml = symbolXml;
  mSymbolMilitaryName = symbolMilitaryName;
  setToolTip( mSymbolMilitaryName );
}

void QgsMilXAnnotationItem::paint( QPainter* painter )
{
  if ( !painter )
  {
    return;
  }
  if ( isMultiPoint() )
  {
    setItemFlags( QgsAnnotationItem::ItemIsNotResizeable | QgsAnnotationItem::ItemHasNoMarker | QgsAnnotationItem::ItemHasNoFrame );
  }
  else
  {
    setItemFlags( QgsAnnotationItem::ItemIsNotResizeable | QgsAnnotationItem::ItemHasNoFrame );
  }

  if ( mGraphic.isNull() )
  {
    updateSymbol( false );
  }

  // Draw line from visual reference point to actual refrence point
  painter->setPen( Qt::black );
  if ( !isMultiPoint() )
  {
    painter->drawLine( QPoint( 0, 0 ), mOffsetFromReferencePoint.toPoint() - mRenderOffset );
  }

  painter->drawPixmap( mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y(), mGraphic );

  if ( isSelected() )
  {
    drawMarkerSymbol( painter );
    drawSelectionBoxes( painter );

    if ( !mAdditionalPoints.isEmpty() )
    {
      painter->save();
      painter->setPen( QPen( Qt::black, 1 ) );
      QList<QPoint> pts = screenPoints();
      for ( int i = 0, n = pts.size(); i < n; ++i )
      {
        painter->setBrush( mControlPoints.contains( i ) ? Qt::red : Qt::yellow );
        painter->drawEllipse( pts[i] - pos(), 4, 4 );
      }
      painter->restore();
    }
    if ( !mAttributePoints.isEmpty() )
    {
      painter->save();
      painter->setPen( QPen( Qt::black, 1 ) );
      painter->setBrush( Qt::red );
      QList< QPair<int, QPoint> > apts = screenAttributePoints();
      for ( int i = 0, n = apts.size(); i < n; ++i )
      {
        painter->drawEllipse( apts[i].second - pos(), 4, 4 );
      }
      painter->restore();
    }
  }
}

int QgsMilXAnnotationItem::moveActionForPosition( const QPointF& pos ) const
{
  if ( isMultiPoint() )
  {
    // Frist, check attribute points
    QList< QPair<int, QPoint> > apts = screenAttributePoints();
    for ( int i = 0, n = apts.size(); i < n; ++i )
    {
      if ( qAbs( pos.x() - apts[i].second.x() ) < 10 && qAbs( pos.y() - apts[i].second.y() ) < 10 )
      {
        return NumMouseMoveActions + 1 + mAdditionalPoints.size() + i;
      }
    }
    QList<QPoint> pts = screenPoints();
    // Then, control points
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( mControlPoints.contains( i ) && qAbs( pos.x() - pts[i].x() ) < 10 && qAbs( pos.y() - pts[i].y() ) < 10 )
      {
        return NumMouseMoveActions + i;
      }
    }
    // Last, regular points
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( qAbs( pos.x() - pts[i].x() ) < 10 && qAbs( pos.y() - pts[i].y() ) < 10 )
      {
        return NumMouseMoveActions + i;
      }
    }
  }
  return QgsAnnotationItem::moveActionForPosition( pos );
}

void QgsMilXAnnotationItem::handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos )
{
  if ( moveAction >= NumMouseMoveActions )
  {
    int idx = moveAction - NumMouseMoveActions;
    if ( idx < 1 + mAdditionalPoints.size() )
    {
      movePoint( idx, newPos.toPoint() );
    }
    else if ( idx < 1 + mAdditionalPoints.size() + mAttributes.size() )
    {
      moveAttributePoint( mAttributes[idx - 1 - mAdditionalPoints.size()].first, newPos.toPoint() );
    }
  }
  else
  {
    QgsAnnotationItem::handleMoveAction( moveAction, newPos, oldPos );
  }
}

Qt::CursorShape QgsMilXAnnotationItem::cursorShapeForAction( int moveAction ) const
{
  if ( moveAction >= NumMouseMoveActions )
  {
    return Qt::ArrowCursor;
  }
  return QgsAnnotationItem::cursorShapeForAction( moveAction );
}

void QgsMilXAnnotationItem::_showItemEditor()
{
  QString symbolId;
  QString symbolMilitaryName;
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, screenAttributes(), mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::editSymbol( mMapCanvas->sceneRect().toRect(), symbol, symbolId, symbolMilitaryName, result ) )
  {
    setSymbolXml( symbolId, symbolMilitaryName );
    setGraphic( result, true );
  }
}

void QgsMilXAnnotationItem::setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs )
{
  if ( mGeoPosCrs.isValid() )
  {
    const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), crs.authid() );
    QgsPoint pOldCrs = t->transform( pos, QgsCoordinateTransform::ReverseTransform );
    QgsPoint delta( pOldCrs.x() - mGeoPos.x(), pOldCrs.y() - mGeoPos.y() );
    QgsAnnotationItem::setMapPosition( pos, crs );

    for ( int i = 0, n = mAdditionalPoints.size(); i < n; ++i )
    {
      mAdditionalPoints[i] = t->transform( QgsPoint( mAdditionalPoints[i].x() + delta.x(), mAdditionalPoints[i].y() + delta.y() ) );
    }
    for ( int i = 0, n = mAttributePoints.size(); i < n; ++i )
    {
      mAttributePoints[i].second = t->transform( QgsPoint( mAttributePoints[i].second.x() + delta.x(), mAttributePoints[i].second.y() + delta.y() ) );
    }
  }
  else
  {
    QgsAnnotationItem::setMapPosition( pos, crs );
  }
  if ( isMultiPoint() )
  {
    updateSymbol( false );
  }
  else
  {
    QgsBillBoardRegistry::instance()->addItem( this, mGraphic.toImage(), QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" )->transform( mGeoPos ) );
  }
}

void QgsMilXAnnotationItem::appendPoint( const QPoint& newPoint )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, screenAttributes(), mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::appendPoint( mMapCanvas->sceneRect().toRect(), symbol, newPoint, result ) )
  {
    setGraphic( result, true );
  }
}

void QgsMilXAnnotationItem::movePoint( int index, const QPoint& newPos )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, screenAttributes(), mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::movePoint( mMapCanvas->sceneRect().toRect(), symbol, index, newPos, result ) )
  {
    setGraphic( result, true );
  }
}

void QgsMilXAnnotationItem::moveAttributePoint( int attr, const QPoint& newPos )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, screenAttributes(), mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::moveAttributePoint( mMapCanvas->sceneRect().toRect(), symbol, attr, newPos, result ) )
  {
    setGraphic( result, true );
  }
}

QList<QPoint> QgsMilXAnnotationItem::screenPoints() const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), mMapCanvas->mapSettings().destinationCrs().authid() );
  QList<QPoint> points;
  points.append( toCanvasCoordinates( t->transform( mGeoPos ) ).toPoint() );
  foreach ( const QgsPoint& geoPoint, mAdditionalPoints )
  {
    points.append( toCanvasCoordinates( t->transform( geoPoint ) ).toPoint() );
  }
  return points;
}

double QgsMilXAnnotationItem::metersToPixels() const
{
  QgsDistanceArea da;
  da.setSourceCrs( mGeoPosCrs );
  da.setEllipsoid( "WGS84" );
  da.setEllipsoidalMode( true );
  QGis::UnitType measureUnit = mGeoPosCrs.mapUnits();
  double delta = mGeoPosCrs.mapUnits() == QGis::Degrees ? 0.001 : 1000;
  QgsPoint otherPoint( mGeoPos.x() + delta, mGeoPos.y() );
  QPointF screenPoint = toCanvasCoordinates( mGeoPos );
  QPointF otherScreenPoint = toCanvasCoordinates( otherPoint );
  double ellipsoidDist = da.measureLine( mGeoPos, otherPoint );
  da.convertMeasurement( ellipsoidDist, measureUnit, QGis::Meters, false );
  double screenDist = QVector2D( screenPoint - otherScreenPoint ).length();
  return screenDist / ellipsoidDist;
}

QList< QPair<int, double> > QgsMilXAnnotationItem::screenAttributes() const
{
  QList< QPair<int, double> > screenAttribs;
  if ( !mAttributes.isEmpty() )
  {
    double m2p = metersToPixels();
    typedef QPair<int, double> AttribPt_t;
    foreach ( const AttribPt_t& attrib, mAttributes )
    {
      double value = attrib.second;
      if ( attrib.first != MilXClient::AttributeAttutide )
      {
        value = value * m2p;
      }
      screenAttribs.append( qMakePair( attrib.first, value ) );
    }
  }
  return screenAttribs;
}

QList<QPair<int, QPoint> > QgsMilXAnnotationItem::screenAttributePoints() const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), mMapCanvas->mapSettings().destinationCrs().authid() );
  QList<QPair<int, QPoint> > points;
  typedef QPair<int, QgsPoint> AttribPoint_t;
  foreach ( const AttribPoint_t& attribPoint, mAttributePoints )
  {
    points.append( qMakePair( attribPoint.first, toCanvasCoordinates( t->transform( attribPoint.second ) ).toPoint() ) );
  }
  return points;
}

int QgsMilXAnnotationItem::absolutePointIdx( int regularIdx ) const
{
  int counter = 0;
  for ( int i = 0, n = 1 + mAdditionalPoints.size(); i < n; ++i )
  {
    counter += !mControlPoints.contains( i );
    if ( counter == regularIdx + 1 )
    {
      return i;
    }
  }
  return 0; // Should not happen
}

void QgsMilXAnnotationItem::setGraphic( MilXClient::NPointSymbolGraphic &result, bool updatePoints )
{
  mGraphic = QPixmap::fromImage( result.graphic );
  if ( !isMultiPoint() )
  {
    QgsBillBoardRegistry::instance()->addItem( this, result.graphic, QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" )->transform( mGeoPos ) );
  }
  setFrameSize( mGraphic.size() );
  mOffsetFromReferencePoint += result.offset - mRenderOffset;
  mRenderOffset = result.offset;
  if ( updatePoints )
  {
    const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mMapCanvas->mapSettings().destinationCrs().authid(), mGeoPosCrs.authid() );
    mMapPosition = mGeoPos = t->transform( toMapCoordinates( result.adjustedPoints[0] ) );
    setPos( result.adjustedPoints[0] );
    mAdditionalPoints.clear();
    for ( int i = 1, n = result.adjustedPoints.size(); i < n; ++i )
    {
      mAdditionalPoints.append( t->transform( toMapCoordinates( result.adjustedPoints[i] ) ) );
    }
    mControlPoints = result.controlPoints;
    mAttributes.clear();
    if ( !result.attributes.isEmpty() )
    {
      double m2p = metersToPixels();
      typedef QPair<int, double> AttribPt_t;
      foreach ( const AttribPt_t& attrib, result.attributes )
      {
        double value = attrib.second;
        if ( attrib.first != MilXClient::AttributeAttutide )
        {
          value = value / m2p;
        }
        mAttributes.append( qMakePair( attrib.first, value ) );
      }
    }
    mAttributePoints.clear();
    for ( int i = 0, n = result.attributePoints.size(); i < n; ++i )
    {
      mAttributePoints.append( qMakePair( result.attributePoints[i].first, t->transform( toMapCoordinates( result.attributePoints[i].second ) ) ) );
    }
  }
  updateBoundingRect();
  update();
}

void QgsMilXAnnotationItem::finalize()
{
  mFinalized = true;
  updateSymbol( true );
}

void QgsMilXAnnotationItem::showContextMenu( const QPoint &screenPos )
{
  QPoint canvasPos = mMapCanvas->mapFromGlobal( screenPos );
  QMenu menu;
  menu.addAction( mSymbolMilitaryName )->setEnabled( false );
  menu.addSeparator();
  QList<QPoint> pts = screenPoints();
  QAction* actionAddPoint = 0;
  QAction* actionRemovePoint = 0;
  QAction* actionClearOffset = 0;
  if ( isMultiPoint() )
  {
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( qAbs( canvasPos.x() - pts[i].x() ) < 7 && qAbs( canvasPos.y() - pts[i].y() ) < 7 )
      {
        actionRemovePoint = menu.addAction( tr( "Remove node" ) );
        actionRemovePoint->setData( i );
        MilXClient::NPointSymbol symbol( mSymbolXml, pts, mControlPoints, screenAttributes(), mFinalized, true );
        bool canDelete = false;
        if ( !MilXClient::canDeletePoint( symbol, i, canDelete ) || !canDelete )
        {
          actionRemovePoint->setEnabled( false );
        }
        break;
      }
    }
    if ( !actionRemovePoint )
    {
      typedef QPair<int, QPoint> AttribPoint_t;
      foreach ( const AttribPoint_t& attribPoint, screenAttributePoints() )
      {
        if ( qAbs( canvasPos.x() - attribPoint.second.x() ) < 7 && qAbs( canvasPos.y() - attribPoint.second.y() ) < 7 )
        {
          actionRemovePoint = menu.addAction( tr( "Remove node" ) );
          actionRemovePoint->setEnabled( false );
          break;
        }
      }
    }
    if ( !actionRemovePoint )
    {
      actionAddPoint = menu.addAction( tr( "Add node" ) );
    }
  }
  else
  {
    actionClearOffset = menu.addAction( tr( "Reset offset" ) );
  }
  menu.addSeparator();
  QAction* actionEdit = menu.addAction( tr( "Edit symbol" ) );
  QAction* actionRemove = menu.addAction( tr( "Remove symbol" ) );
  QAction* clickedAction = menu.exec( screenPos );
  if ( !clickedAction )
  {
    return;
  }
  if ( clickedAction == actionAddPoint )
  {
    MilXClient::NPointSymbol symbol( mSymbolXml, pts, mControlPoints, screenAttributes(), mFinalized, true );
    MilXClient::NPointSymbolGraphic result;
    if ( MilXClient::insertPoint( mMapCanvas->sceneRect().toRect(), symbol, canvasPos, result ) )
    {
      setGraphic( result, true );
    }
  }
  else if ( clickedAction == actionRemovePoint )
  {
    int index = actionRemovePoint->data().toInt();
    MilXClient::NPointSymbol symbol( mSymbolXml, pts, mControlPoints, screenAttributes(), mFinalized, true );
    MilXClient::NPointSymbolGraphic result;
    if ( MilXClient::deletePoint( mMapCanvas->sceneRect().toRect(), symbol, index, result ) )
    {
      setGraphic( result, true );
    }
  }
  else if ( clickedAction == actionClearOffset )
  {
    setOffsetFromReferencePoint( mRenderOffset );
  }
  else if ( clickedAction == actionEdit )
  {
    _showItemEditor();
  }
  else if ( clickedAction == actionRemove )
  {
    deleteLater();
  }
}

bool QgsMilXAnnotationItem::hitTest( const QPoint& screenPos ) const
{
  if ( !isMultiPoint() )
  {
    return true;
  }
  // First, check against attribute points, controlpoints and regular points
  QList< QPair<int, QPoint> > screenAttrPoints = screenAttributePoints();
  typedef QPair<int, QPoint> Attrib_t;
  foreach ( const Attrib_t& attrib, screenAttrPoints )
  {
    if (( screenPos - attrib.second ).manhattanLength() < 15 )
    {
      return true;
    }
  }

  QList<QPoint> screenPts = screenPoints();
  foreach ( const QPoint& screenPt, screenPts )
  {
    if (( screenPos - screenPt ).manhattanLength() < 15 )
    {
      return true;
    }
  }
  // Do the full hit test
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPts, mControlPoints, screenAttributes(), mFinalized, true );
  bool hitTestResult = false;
  MilXClient::hitTest( symbol, screenPos, hitTestResult );
  return hitTestResult;
}

void QgsMilXAnnotationItem::updateSymbol( bool updatePoints )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, screenAttributes(), mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::updateSymbol( mMapCanvas->sceneRect().toRect(), symbol, result, updatePoints ) )
  {
    setGraphic( result, updatePoints );
  }
}
