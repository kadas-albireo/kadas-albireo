/***************************************************************************
                              qgsvbsmilixannotationitem.cpp
                              -----------------------------
  begin                : Oct 01, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbsmilixannotationitem.h"
#include "qgscrscache.h"
#include "Client/VBSMilixClient.hpp"
#include "qgsmapcanvas.h"
#include "qgscoordinatetransform.h"

#include <QPainter>
#include <QMenu>

REGISTER_QGS_ANNOTATION_ITEM( QgsVBSMilixAnnotationItem )

QgsVBSMilixAnnotationItem::QgsVBSMilixAnnotationItem( QgsMapCanvas* canvas )
    : QgsAnnotationItem( canvas ), mIsMultiPoint( false ), mFinalized( false )
{
  mOffsetFromReferencePoint = QPoint( 0, 0 );
}

QgsVBSMilixAnnotationItem::QgsVBSMilixAnnotationItem( QgsMapCanvas* canvas, QgsVBSMilixAnnotationItem* source )
    : QgsAnnotationItem( canvas, source )
{
  mSymbolXml = source->mSymbolXml;
  mGraphic = source->mGraphic;
  mAdditionalPoints = source->mAdditionalPoints;
  mRenderOffset = source->mRenderOffset;
  mControlPoints = source->mControlPoints;
  mIsMultiPoint = source->mIsMultiPoint;
  mFinalized = source->mFinalized;
}

void QgsVBSMilixAnnotationItem::setSymbolXml( const QString &symbolXml, const QString& symbolMilitaryName, bool isMultiPoint )
{
  mIsMultiPoint = isMultiPoint;
  mSymbolXml = symbolXml;
  mSymbolMilitaryName = symbolMilitaryName;

  if ( mIsMultiPoint )
  {
    setItemFlags( QgsAnnotationItem::ItemIsNotResizeable | QgsAnnotationItem::ItemHasNoMarker | QgsAnnotationItem::ItemHasNoFrame );
  }
  else
  {
    setItemFlags( QgsAnnotationItem::ItemIsNotResizeable | QgsAnnotationItem::ItemHasNoFrame );
  }
}

void QgsVBSMilixAnnotationItem::writeXML( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( documentElem.isNull() )
  {
    return;
  }

  QDomElement milixAnnotationElem = doc.createElement( "MilixAnnotationItem" );
  milixAnnotationElem.setAttribute( "symbolXml", mSymbolXml );
  milixAnnotationElem.setAttribute( "isMultiPoint", mIsMultiPoint );
  foreach ( const QgsPoint& p, mAdditionalPoints )
  {
    QDomElement pointElem = doc.createElement( "Point" );
    pointElem.setAttribute( "x", p.x() );
    pointElem.setAttribute( "y", p.y() );
    milixAnnotationElem.appendChild( pointElem );
  }
  _writeXML( doc, milixAnnotationElem );
  documentElem.appendChild( milixAnnotationElem );
}

void QgsVBSMilixAnnotationItem::readXML( const QDomDocument& doc, const QDomElement& itemElem )
{
  mSymbolXml = itemElem.attribute( "symbolXml" );
  mIsMultiPoint = itemElem.attribute( "isMultiPoint" ).toInt();
  QDomNodeList pointElems = itemElem.elementsByTagName( "Point" );
  mAdditionalPoints.clear();
  for ( int i = 0, n = pointElems.count(); i < n; ++i )
  {
    QDomElement pointElem = pointElems.at( i ).toElement();
    mAdditionalPoints.append( QgsPoint( pointElem.attribute( "x" ).toDouble(), pointElem.attribute( "y" ).toDouble() ) );
  }

  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }
  updateSymbol( true );
}

void QgsVBSMilixAnnotationItem::paint( QPainter* painter )
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
    painter->drawLine( QPoint( 0, 0 ), mOffsetFromReferencePoint + QPoint( 0.5 * mGraphic.width(), 0.5 * mGraphic.height() ) );
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
      QList<QPoint> pts = points();
      for ( int i = 0, n = pts.size(); i < n; ++i )
      {
        painter->setBrush( mControlPoints.contains( i ) ? Qt::red : Qt::yellow );
        painter->drawEllipse( pts[i] - pos(), 4, 4 );
      }
      painter->restore();
    }
  }
}

int QgsVBSMilixAnnotationItem::moveActionForPosition( const QPointF& pos ) const
{
  if ( mIsMultiPoint )
  {
    QList<QPoint> pts = points();
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

void QgsVBSMilixAnnotationItem::handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos )
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

Qt::CursorShape QgsVBSMilixAnnotationItem::cursorShapeForAction( int moveAction ) const
{
  if ( moveAction >= NumMouseMoveActions )
  {
    return Qt::ArrowCursor;
  }
  return QgsAnnotationItem::cursorShapeForAction( moveAction );
}

void QgsVBSMilixAnnotationItem::_showItemEditor()
{
  QString symbolId;
  QString symbolMilitaryName;
  VBSMilixClient::NPointSymbol symbol( mSymbolXml, points(), controlPoints(), mFinalized );
  VBSMilixClient::NPointSymbolGraphic result;
  if ( VBSMilixClient::editSymbol( mMapCanvas->sceneRect().toRect(), symbol, symbolId, symbolMilitaryName, result ) )
  {
    setSymbolXml( symbolId, symbolMilitaryName, result.adjustedPoints.size() > 1 );
    setGraphic( result, true );
  }
}

void QgsVBSMilixAnnotationItem::setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs )
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

void QgsVBSMilixAnnotationItem::appendPoint( const QPoint& newPoint )
{
  VBSMilixClient::NPointSymbol symbol( mSymbolXml, points(), controlPoints(), mFinalized );
  VBSMilixClient::NPointSymbolGraphic result;
  if ( VBSMilixClient::appendPoint( mMapCanvas->sceneRect().toRect(), symbol, newPoint, result ) )
  {
    setGraphic( result, true );
  }
}

void QgsVBSMilixAnnotationItem::movePoint( int index, const QPoint& newPos )
{
  VBSMilixClient::NPointSymbol symbol( mSymbolXml, points(), controlPoints(), mFinalized );
  VBSMilixClient::NPointSymbolGraphic result;
  if ( VBSMilixClient::movePoint( mMapCanvas->sceneRect().toRect(), symbol, index, newPos, result ) )
  {
    setGraphic( result, true );
  }
}

QList<QPoint> QgsVBSMilixAnnotationItem::points() const
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

int QgsVBSMilixAnnotationItem::absolutePointIdx( int regularIdx ) const
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

void QgsVBSMilixAnnotationItem::setGraphic( VBSMilixClient::NPointSymbolGraphic &result, bool updatePoints )
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

void QgsVBSMilixAnnotationItem::showContextMenu( const QPoint &screenPos )
{
  QPoint canvasPos = mMapCanvas->mapFromGlobal( screenPos );
  QMenu menu;
  QList<QPoint> pts = points();
  QAction* actionAddPoint = 0;
  QAction* actionRemovePoint = 0;
  if ( mIsMultiPoint )
  {
    for ( int i = 0, n = pts.size(); i < n; ++i )
    {
      if ( qAbs( canvasPos.x() - pts[i].x() ) < 7 && qAbs( canvasPos.y() - pts[i].y() ) < 7 )
      {
        actionRemovePoint = menu.addAction( tr( "Remove node" ) );
        actionRemovePoint->setData( i );
        VBSMilixClient::NPointSymbol symbol( mSymbolXml, pts, controlPoints(), mFinalized );
        bool canDelete = false;
        if ( !VBSMilixClient::canDeletePoint( symbol, i, canDelete ) || !canDelete )
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
    menu.addSeparator();
  }
  QAction* actionEdit = menu.addAction( "Edit symbol" );
  QAction* actionRemove = menu.addAction( "Remove symbol" );
  QAction* clickedAction = menu.exec( screenPos );
  if ( !clickedAction )
  {
    return;
  }
  if ( clickedAction == actionAddPoint )
  {
    VBSMilixClient::NPointSymbol symbol( mSymbolXml, pts, controlPoints(), mFinalized );
    VBSMilixClient::NPointSymbolGraphic result;
    if ( VBSMilixClient::insertPoint( mMapCanvas->sceneRect().toRect(), symbol, canvasPos, result ) )
    {
      setGraphic( result, true );
    }
  }
  else if ( clickedAction == actionRemovePoint )
  {
    int index = actionRemovePoint->data().toInt();
    VBSMilixClient::NPointSymbol symbol( mSymbolXml, pts, controlPoints(), mFinalized );
    VBSMilixClient::NPointSymbolGraphic result;
    if ( VBSMilixClient::deletePoint( mMapCanvas->sceneRect().toRect(), symbol, index, result ) )
    {
      setGraphic( result, true );
    }
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

void QgsVBSMilixAnnotationItem::updateSymbol( bool updatePoints )
{
  VBSMilixClient::NPointSymbol symbol( mSymbolXml, points(), controlPoints(), mFinalized );
  VBSMilixClient::NPointSymbolGraphic result;
  if ( VBSMilixClient::updateSymbol( mMapCanvas->sceneRect().toRect(), symbol, result, updatePoints ) )
  {
    setGraphic( result, updatePoints );
  }
}

void QgsVBSMilixAnnotationItem::writeMilx( QDomDocument& doc, QDomElement& graphicListEl ) const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" );

  QDomElement graphicEl = doc.createElement( "MilXGraphic" );
  graphicListEl.appendChild( graphicEl );

  QDomElement stringXmlEl = doc.createElement( "MssStringXML" );
  stringXmlEl.appendChild( doc.createTextNode( mSymbolXml ) );
  graphicEl.appendChild( stringXmlEl );

  QDomElement nameEl = doc.createElement( "Name" );
  nameEl.appendChild( doc.createTextNode( mSymbolMilitaryName ) );
  graphicEl.appendChild( nameEl );

  QDomElement pointListEl = doc.createElement( "PointList" );
  graphicEl.appendChild( pointListEl );

  QDomElement p0El = doc.createElement( "Point" );
  pointListEl.appendChild( p0El );

  QgsPoint p0WGS = t->transform( mGeoPos );
  QDomElement p0XEl = doc.createElement( "X" );
  p0XEl.appendChild( doc.createTextNode( QString::number( p0WGS.x(), 'f', 6 ) ) );
  p0El.appendChild( p0XEl );
  QDomElement p0YEl = doc.createElement( "Y" );
  p0YEl.appendChild( doc.createTextNode( QString::number( p0WGS.y(), 'f', 6 ) ) );
  p0El.appendChild( p0YEl );

  foreach ( const QgsPoint& p, mAdditionalPoints )
  {
    QDomElement pEl = doc.createElement( "Point" );
    pointListEl.appendChild( pEl );

    QgsPoint pWGS = t->transform( p );
    QDomElement pXEl = doc.createElement( "X" );
    pXEl.appendChild( doc.createTextNode( QString::number( pWGS.x(), 'f', 6 ) ) );
    pEl.appendChild( pXEl );
    QDomElement pYEl = doc.createElement( "Y" );
    pYEl.appendChild( doc.createTextNode( QString::number( pWGS.y(), 'f', 6 ) ) );
    pEl.appendChild( pYEl );
  }

  QDomElement offsetEl = doc.createElement( "Offset" );
  graphicEl.appendChild( offsetEl );

  QPointF rawOffset = mOffsetFromReferencePoint - mRenderOffset;
  QDomElement factorXEl = doc.createElement( "FactorX" );
  factorXEl.appendChild( doc.createTextNode( QString::number( rawOffset.x() / VBSMilixClient::SymbolSize ) ) );
  offsetEl.appendChild( factorXEl );

  QDomElement factorYEl = doc.createElement( "FactorY" );
  factorYEl.appendChild( doc.createTextNode( QString::number( rawOffset.y() / VBSMilixClient::SymbolSize ) ) );
  offsetEl.appendChild( factorYEl );
}

void QgsVBSMilixAnnotationItem::readMilx( const QDomElement& graphicEl, const QgsCoordinateTransform* crst, int symbolSize )
{
  QString symbolId = graphicEl.firstChildElement( "MssStringXML" ).text();
  QString militaryName = graphicEl.firstChildElement( "Name" ).text();

  QList<QgsPoint> points;
  QDomNodeList pointEls = graphicEl.firstChildElement( "PointList" ).elementsByTagName( "Point" );
  for ( int iPoint = 0, nPoints = pointEls.count(); iPoint < nPoints; ++iPoint )
  {
    QDomElement pointEl = pointEls.at( iPoint ).toElement();
    double x = pointEl.firstChildElement( "X" ).text().toDouble();
    double y = pointEl.firstChildElement( "Y" ).text().toDouble();
    points.append( crst->transform( QgsPoint( x, y ) ) );
  }
  mMapPosition = mGeoPos = points.front();
  mGeoPosCrs = crst->destCRS();
  points.removeFirst();
  mAdditionalPoints = points;

  double offsetX = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorX" ).text().toDouble() * symbolSize;
  double offsetY = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorY" ).text().toDouble() * symbolSize;
  mOffsetFromReferencePoint = QPointF( offsetX, offsetY );
  setSymbolXml( symbolId, militaryName, points.size() > 1 );
  updateSymbol( true );
}
