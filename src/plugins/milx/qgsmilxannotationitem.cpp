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
#include "qgscrscache.h"
#include "MilXClient.hpp"
#include "qgsmapcanvas.h"
#include "qgscoordinatetransform.h"

#include <QPainter>
#include <QMenu>

REGISTER_QGS_ANNOTATION_ITEM( QgsMilXAnnotationItem )

QgsMilXAnnotationItem::QgsMilXAnnotationItem( QgsMapCanvas* canvas )
    : QgsAnnotationItem( canvas ), mIsMultiPoint( false ), mFinalized( false )
{
  mOffsetFromReferencePoint = QPoint( 0, 0 );
  connect( canvas, SIGNAL( extentsChanged() ), this, SLOT( updateSymbol() ) );
}

QgsMilXAnnotationItem::QgsMilXAnnotationItem( QgsMapCanvas* canvas, QgsMilXAnnotationItem* source )
    : QgsAnnotationItem( canvas, source )
{
  setSymbolXml( source->mSymbolXml, source->mSymbolMilitaryName, source->mIsMultiPoint );
  mAdditionalPoints = source->mAdditionalPoints;
  mRenderOffset = source->mRenderOffset;
  mControlPoints = source->mControlPoints;
  mFinalized = source->mFinalized;
  updateSymbol( false );
  connect( canvas, SIGNAL( extentsChanged() ), this, SLOT( updateSymbol() ) );
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
  setSymbolXml( item->mssString(), item->militaryName(), item->isMultiPoint() );
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
  item->initialize( mSymbolXml, mSymbolMilitaryName, points, mControlPoints, mOffsetFromReferencePoint.toPoint() - mRenderOffset );
  return item;
}

void QgsMilXAnnotationItem::setSymbolXml( const QString &symbolXml, const QString& symbolMilitaryName, bool isMultiPoint )
{
  mIsMultiPoint = isMultiPoint;
  mSymbolXml = symbolXml;
  mSymbolMilitaryName = symbolMilitaryName;
  setToolTip( mSymbolMilitaryName );

  if ( mIsMultiPoint )
  {
    setItemFlags( QgsAnnotationItem::ItemIsNotResizeable | QgsAnnotationItem::ItemHasNoMarker | QgsAnnotationItem::ItemHasNoFrame );
  }
  else
  {
    setItemFlags( QgsAnnotationItem::ItemIsNotResizeable | QgsAnnotationItem::ItemHasNoFrame );
  }
}

void QgsMilXAnnotationItem::paint( QPainter* painter )
{
  if ( !painter )
  {
    return;
  }

  if ( mGraphic.isNull() )
  {
    updateSymbol( false );
  }

  // Draw line from visual reference point to actual refrence point
  painter->setPen( Qt::black );
  if ( !mIsMultiPoint )
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
  }
}

int QgsMilXAnnotationItem::moveActionForPosition( const QPointF& pos ) const
{
  if ( mIsMultiPoint )
  {
    QList<QPoint> pts = screenPoints();
    // Priority to control points
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( mControlPoints.contains( i ) && qAbs( pos.x() - pts[i].x() ) < 7 && qAbs( pos.y() - pts[i].y() ) < 7 )
      {
        return NumMouseMoveActions + i;
      }
    }
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( qAbs( pos.x() - pts[i].x() ) < 7 && qAbs( pos.y() - pts[i].y() ) < 7 )
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
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::editSymbol( mMapCanvas->sceneRect().toRect(), symbol, symbolId, symbolMilitaryName, result ) )
  {
    setSymbolXml( symbolId, symbolMilitaryName, result.adjustedPoints.size() > 1 );
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
  }
  else
  {
    QgsAnnotationItem::setMapPosition( pos, crs );
  }
  if ( mIsMultiPoint )
  {
    updateSymbol( false );
  }
}

void QgsMilXAnnotationItem::appendPoint( const QPoint& newPoint )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::appendPoint( mMapCanvas->sceneRect().toRect(), symbol, newPoint, result ) )
  {
    setGraphic( result, true );
  }
}

void QgsMilXAnnotationItem::movePoint( int index, const QPoint& newPos )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::movePoint( mMapCanvas->sceneRect().toRect(), symbol, index, newPos, result ) )
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
  }
  updateBoundingRect();
  update();
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
  if ( mIsMultiPoint )
  {
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( qAbs( canvasPos.x() - pts[i].x() ) < 7 && qAbs( canvasPos.y() - pts[i].y() ) < 7 )
      {
        actionRemovePoint = menu.addAction( tr( "Remove node" ) );
        actionRemovePoint->setData( i );
        MilXClient::NPointSymbol symbol( mSymbolXml, pts, mControlPoints, mFinalized, true );
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
      actionAddPoint = menu.addAction( tr( "Add node" ) );
    }
  }
  else
  {
    actionClearOffset = menu.addAction( tr( "Reset offset" ) );
  }
  menu.addSeparator();
  QAction* actionEdit = menu.addAction( "Edit symbol" );
  QAction* actionRemove = menu.addAction( "Remove symbol" );
  QAction* clickedAction = menu.exec( screenPos );
  if ( !clickedAction )
  {
    return;
  }
  if ( clickedAction == actionAddPoint )
  {
    MilXClient::NPointSymbol symbol( mSymbolXml, pts, mControlPoints, mFinalized, true );
    MilXClient::NPointSymbolGraphic result;
    if ( MilXClient::insertPoint( mMapCanvas->sceneRect().toRect(), symbol, canvasPos, result ) )
    {
      setGraphic( result, true );
    }
  }
  else if ( clickedAction == actionRemovePoint )
  {
    int index = actionRemovePoint->data().toInt();
    MilXClient::NPointSymbol symbol( mSymbolXml, pts, mControlPoints, mFinalized, true );
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
  if ( !mIsMultiPoint )
  {
    return true;
  }
  QList<QPoint> screenPts = screenPoints();
  // First, check against pos/control points
  foreach ( const QPoint& screenPt, screenPts )
  {
    if (( screenPos - screenPt ).manhattanLength() < 15 )
    {
      return true;
    }
  }
  // Do the full hit test
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPts, mControlPoints, mFinalized, true );
  bool hitTestResult = false;
  MilXClient::hitTest( symbol, screenPos, hitTestResult );
  return hitTestResult;
}

void QgsMilXAnnotationItem::updateSymbol( bool updatePoints )
{
  MilXClient::NPointSymbol symbol( mSymbolXml, screenPoints(), mControlPoints, mFinalized, true );
  MilXClient::NPointSymbolGraphic result;
  if ( MilXClient::updateSymbol( mMapCanvas->sceneRect().toRect(), symbol, result, updatePoints ) )
  {
    setGraphic( result, updatePoints );
  }
}
