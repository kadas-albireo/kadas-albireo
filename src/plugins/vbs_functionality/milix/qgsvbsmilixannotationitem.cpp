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

REGISTER_QGS_ANNOTATION_ITEM( QgsVBSMilixAnnotationItem )

QgsVBSMilixAnnotationItem::QgsVBSMilixAnnotationItem( QgsMapCanvas* canvas )
    : QgsAnnotationItem( canvas )
{
  setItemFlags( QgsAnnotationItem::ItemIsNotResizeable |
                QgsAnnotationItem::ItemHasNoFrame |
                QgsAnnotationItem::ItemHasNoMarker );
}

void QgsVBSMilixAnnotationItem::setSymbolXml( const QString &symbolXml )
{
  mSymbolXml = symbolXml;
  renderPixmap();
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
  foreach ( const QgsPoint& p, mGeoPoints )
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
  QDomNodeList pointElems = itemElem.elementsByTagName( "Point" );
  mGeoPoints.clear();
  for ( int i = 0, n = pointElems.count(); i < n; ++i )
  {
    QDomElement pointElem = pointElems.at( i ).toElement();
    mGeoPoints.append( QgsPoint( pointElem.attribute( "x" ).toDouble(), pointElem.attribute( "y" ).toDouble() ) );
  }

  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }
  renderPixmap();
}

void QgsVBSMilixAnnotationItem::paint( QPainter* painter )
{
  if ( !painter )
  {
    return;
  }

  drawFrame( painter );

  painter->drawPixmap( mOffsetFromReferencePoint.x() + 2, mOffsetFromReferencePoint.y() + 2, mPixmap );

  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}

void QgsVBSMilixAnnotationItem::_showItemEditor()
{
  QString outXml;
  if ( VBSMilixClient::editSymbol( mSymbolXml, outXml ) )
  {
    mSymbolXml = outXml;
    renderPixmap();
  }
}

void QgsVBSMilixAnnotationItem::setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs )
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), crs.authid() );
  QgsPoint pOldCrs = t->transform( pos, QgsCoordinateTransform::ReverseTransform );
  QgsPoint delta( pOldCrs.x() - mGeoPos.x(), pOldCrs.y() - mGeoPos.y() );
  QgsAnnotationItem::setMapPosition( pos, crs );

  for ( int i = 0, n = mGeoPoints.size(); i < n; ++i )
  {
    mGeoPoints[i] = t->transform( QgsPoint( mGeoPoints[i].x() + delta.x(), mGeoPoints[i].y() + delta.y() ) );
  }
}

void QgsVBSMilixAnnotationItem::addPoint( const QgsPoint& p )
{
  if ( mMapCanvas->mapSettings().destinationCrs().authid() != mGeoPosCrs.authid() )
  {
    mGeoPoints.append( QgsCoordinateTransform( mMapCanvas->mapSettings().destinationCrs(), mGeoPosCrs ).transform( p ) );
  }
  else
  {
    mGeoPoints.append( p );
  }
  renderPixmap();
}

void QgsVBSMilixAnnotationItem::modifyPoint( int index, const QgsPoint& p )
{
  if ( mMapCanvas->mapSettings().destinationCrs().authid() != mGeoPosCrs.authid() )
  {
    mGeoPoints[index] = QgsCoordinateTransform( mMapCanvas->mapSettings().destinationCrs(), mGeoPosCrs ).transform( p );
  }
  else
  {
    mGeoPoints[index] = p;
  }
  renderPixmap();
}

void QgsVBSMilixAnnotationItem::renderPixmap()
{
  QPixmap pixmap;
  QPoint offset;
  if ( VBSMilixClient::getNPointSymbol( mSymbolXml, points(), mMapCanvas->sceneRect().toRect(), pixmap, offset ) )
  {
    updatePixmap( pixmap, offset );
  }
}

QList<QPoint> QgsVBSMilixAnnotationItem::points() const
{
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), mMapCanvas->mapSettings().destinationCrs().authid() );
  QList<QPoint> points;
  points.append( toCanvasCoordinates( t->transform( mGeoPos ) ).toPoint() );
  foreach ( const QgsPoint& geoPoint, mGeoPoints )
  {
    points.append( toCanvasCoordinates( t->transform( geoPoint ) ).toPoint() );
  }
  return points;
}

void QgsVBSMilixAnnotationItem::updatePixmap( const QPixmap& pixmap, const QPoint &offset )
{
  mPixmap = pixmap;
  setFrameSize( mPixmap.size() );
  setOffsetFromReferencePoint( offset );
  update();
}
