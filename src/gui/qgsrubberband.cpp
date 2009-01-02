/***************************************************************************
    qgsrubberband.cpp - Rubberband widget for drawing multilines and polygons
     --------------------------------------
    Date                 : 07-Jan-2006
    Copyright            : (C) 2006 by Tom Elwertowski
    Email                : telwertowski at users dot sourceforge dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsrubberband.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsvectorlayer.h"
#include <QPainter>

/*!
  \class QgsRubberBand
  \brief The QgsRubberBand class provides a transparent overlay widget
  for tracking the mouse while drawing polylines or polygons.
*/
QgsRubberBand::QgsRubberBand( QgsMapCanvas* mapCanvas, bool isPolygon )
    : QgsMapCanvasItem( mapCanvas ), mIsPolygon( isPolygon ), mTranslationOffsetX( 0.0 ), mTranslationOffsetY( 0.0 )
{
  reset( isPolygon );
  setColor( QColor( Qt::lightGray ) );
}

QgsRubberBand::QgsRubberBand(): QgsMapCanvasItem( 0 )
{}

QgsRubberBand::~QgsRubberBand()
{}

/*!
  Set the outline and fill color.
  */
void QgsRubberBand::setColor( const QColor & color )
{
  mPen.setColor( color );
  QColor fillColor( color.red(), color.green(), color.blue(), 63 );
  mBrush.setColor( fillColor );
  mBrush.setStyle( Qt::SolidPattern );
}

/*!
  Set the outline width.
  */
void QgsRubberBand::setWidth( int width )
{
  mPen.setWidth( width );
}

/*!
  Remove all points from the shape being created.
  */
void QgsRubberBand::reset( bool isPolygon )
{

  mPoints.clear();
  QList< QgsPoint > initial_list;
  mPoints.push_back( initial_list );
  mIsPolygon = isPolygon;
  updateRect();
  update();
}

/*!
  Add a point to the shape being created.
  */
void QgsRubberBand::addPoint( const QgsPoint & p, bool do_update /* = true */, int geometryIndex )
{
  if ( mPoints.size() < ( geometryIndex + 1 ) )
  {
    return;
  }

  //we need to set two points at the begin of the ruber band for operations that move the last point
  if ( mPoints[geometryIndex].size() == 0 )
  {
    mPoints[geometryIndex].push_back( p );
  }
  mPoints[geometryIndex].push_back( p );

  if ( do_update )
  {
    updateRect();
    update();
  }
}

void QgsRubberBand::removeLastPoint( int geometryIndex )
{
  if ( mPoints.size() < ( geometryIndex + 1 ) )
  {
    return;
  }

  if ( mPoints[geometryIndex].size() > 0 )
  {
    mPoints[geometryIndex].pop_back();
  }

  updateRect();
  update();
}

/*!
  Update the line between the last added point and the mouse position.
  */
void QgsRubberBand::movePoint( const QgsPoint & p, int geometryIndex )
{
  if ( mPoints.size() < ( geometryIndex + 1 ) )
  {
    return;
  }

  if ( mPoints.at( geometryIndex ).size() < 1 )
  {
    return;
  }

  mPoints[geometryIndex][mPoints.at( geometryIndex ).size() - 1] = p;

  updateRect();
  update();
}

void QgsRubberBand::movePoint( int index, const QgsPoint& p, int geometryIndex )
{
  if ( mPoints.size() < ( geometryIndex + 1 ) )
  {
    return;
  }

  if ( mPoints.at( geometryIndex ).size() < index )
  {
    return;
  }

  mPoints[geometryIndex][index] = p;

  updateRect();
  update();
}

void QgsRubberBand::setToGeometry( QgsGeometry* geom, QgsVectorLayer* layer )
{
  if ( !geom )
  {
    return;
  }

  //maprender object of canvas
  QgsMapRenderer* mr = mMapCanvas->mapRenderer();
  if ( !mr )
  {
    return;
  }

  reset( mIsPolygon );

  switch ( geom->wkbType() )
  {

    case QGis::WKBPoint:
    case QGis::WKBPoint25D:
    {
      mIsPolygon = true;
      double d = mMapCanvas->extent().width() * 0.005;
      QgsPoint pt;
      if ( layer )
      {
        pt = mr->layerToMapCoordinates( layer, geom->asPoint() );
      }
      else
      {
        pt = geom->asPoint();
      }
      addPoint( QgsPoint( pt.x() - d, pt.y() - d ), false );
      addPoint( QgsPoint( pt.x() + d, pt.y() - d ), false );
      addPoint( QgsPoint( pt.x() + d, pt.y() + d ), false );
      addPoint( QgsPoint( pt.x() - d, pt.y() + d ), false );
    }
    break;

    case QGis::WKBMultiPoint:
    case QGis::WKBMultiPoint25D:
    {
      mIsPolygon = true;
      double d = mMapCanvas->extent().width() * 0.005;
      QgsMultiPoint mpt = geom->asMultiPoint();
      for ( int i = 0; i < mpt.size(); ++i )
      {
        QgsPoint pt = mpt[i];
        if ( layer )
        {
          addPoint( mr->layerToMapCoordinates( layer, QgsPoint( pt.x() - d, pt.y() - d ) ), false );
          addPoint( mr->layerToMapCoordinates( layer, QgsPoint( pt.x() + d, pt.y() - d ) ), false );
          addPoint( mr->layerToMapCoordinates( layer, QgsPoint( pt.x() + d, pt.y() + d ) ), false );
          addPoint( mr->layerToMapCoordinates( layer, QgsPoint( pt.x() - d, pt.y() + d ) ), false );
        }
        else
        {
          addPoint( QgsPoint( pt.x() - d, pt.y() - d ), false );
          addPoint( QgsPoint( pt.x() + d, pt.y() - d ), false );
          addPoint( QgsPoint( pt.x() + d, pt.y() + d ), false );
          addPoint( QgsPoint( pt.x() - d, pt.y() + d ), false );
        }
      }
    }
    break;

    case QGis::WKBLineString:
    case QGis::WKBLineString25D:
    {
      mIsPolygon = false;
      QgsPolyline line = geom->asPolyline();
      for ( int i = 0; i < line.count(); i++ )
      {
        if ( layer )
        {
          addPoint( mr->layerToMapCoordinates( layer, line[i] ), false );
        }
        else
        {
          addPoint( line[i], false );
        }
      }
    }
    break;

    case QGis::WKBMultiLineString:
    case QGis::WKBMultiLineString25D:
    {
      mIsPolygon = false;
      mPoints.clear();

      QgsMultiPolyline mline = geom->asMultiPolyline();
      for ( int i = 0; i < mline.size(); ++i )
      {
        QList<QgsPoint> newList;
        mPoints.push_back( newList );
        QgsPolyline line = mline[i];
        for ( int j = 0; j < line.size(); ++j )
        {
          if ( layer )
          {
            addPoint( mr->layerToMapCoordinates( layer, line[j] ), false, i );
          }
          else
          {
            addPoint( line[j], false, i );
          }
        }
      }
    }
    break;

    case QGis::WKBPolygon:
    case QGis::WKBPolygon25D:
    {
      mIsPolygon = true;
      QgsPolygon poly = geom->asPolygon();
      QgsPolyline line = poly[0];
      for ( int i = 0; i < line.count(); i++ )
      {
        if ( layer )
        {
          addPoint( mr->layerToMapCoordinates( layer, line[i] ), false );
        }
        else
        {
          addPoint( line[i], false );
        }
      }
    }
    break;

    case QGis::WKBMultiPolygon:
    case QGis::WKBMultiPolygon25D:
    {
      mIsPolygon = true;
      mPoints.clear();

      QgsMultiPolygon multipoly = geom->asMultiPolygon();
      for ( int i = 0; i < multipoly.size(); ++i )
      {
        QList<QgsPoint> newList;
        mPoints.push_back( newList );
        QgsPolygon poly = multipoly[i];
        QgsPolyline line = poly[0];
        for ( int j = 0; j < line.count(); ++j )
        {
          if ( layer )
          {
            addPoint( mr->layerToMapCoordinates( layer, line[j] ), false, i );
          }
          else
          {
            addPoint( line[j], false, i );
          }
        }
      }
    }
    break;

    case QGis::WKBUnknown:
    default:
      return;
  }
  updateRect();
  update();
}

/*!
  Draw the shape in response to an update event.
  */
void QgsRubberBand::paint( QPainter* p )
{
  QList<QgsPoint> currentList;
  if ( mPoints.size() > 0 )
  {
    p->setPen( mPen );
    p->setBrush( mBrush );

    for ( int i = 0; i < mPoints.size(); ++i )
    {
      QPolygonF pts;
      QList<QgsPoint>::const_iterator it = mPoints.at( i ).constBegin();
      for ( ; it != mPoints.at( i ).constEnd(); ++it )
      {
        //QgsDebugMsg("Drawing rubberband vertex: " + QString::number(it->x() + mTranslationOffsetX) + "//" + QString::number(it->y() + mTranslationOffsetY));
        pts.append( toCanvasCoordinates( QgsPoint( it->x() + mTranslationOffsetX, it->y() + mTranslationOffsetY ) ) - pos() );
      }

      if ( mIsPolygon )
      {
        p->drawPolygon( pts );
      }
      else
      {
        p->drawPolyline( pts );
      }
    }
  }
}

void QgsRubberBand::updateRect()
{
  if ( mPoints.size() > 0 )
  {
    //initial point
    QList<QgsPoint>::const_iterator it = mPoints.at( 0 ).constBegin();
    if ( it == mPoints.at( 0 ).constEnd() )
    {
      return;
    }
    QgsRectangle r( it->x() + mTranslationOffsetX, it->y() + mTranslationOffsetY,
                    it->x() + mTranslationOffsetX, it->y() + mTranslationOffsetY );

    for ( int i = 0; i < mPoints.size(); ++i )
    {
      QList<QgsPoint>::const_iterator it = mPoints.at( i ).constBegin();
      for ( ; it != mPoints.at( i ).constEnd(); ++it )
      {
        r.combineExtentWith( it->x() + mTranslationOffsetX, it->y() + mTranslationOffsetY );
        //QgsDebugMsg("Combining extent with: " + QString::number(it->x()) + "//" + QString::number(it->y()));
      }
      //QgsDebugMsg("r: " + r.toString());
    }
    setRect( r );
  }
  else
  {
    setRect( QgsRectangle() );
  }
  setVisible( mPoints.size() > 0 );
}

void QgsRubberBand::setTranslationOffset( double dx, double dy )
{
  mTranslationOffsetX = dx;
  mTranslationOffsetY = dy;
  updateRect();
}

int QgsRubberBand::numberOfVertices() const
{
  int count = 0;
  QList<QList<QgsPoint> >::const_iterator it = mPoints.constBegin();
  for ( ;it != mPoints.constEnd(); ++it )
  {
    QList<QgsPoint>::const_iterator iter = it->constBegin();
    for ( ;iter != it->constEnd(); ++iter )
    {
      ++count;
    }
  }
  return count;
}

const QgsPoint *QgsRubberBand::getPoint( int i, int j ) const
{
  if ( i < mPoints.size() && j < mPoints[i].size() )
    return &mPoints[i][j];
  else
    return 0;
}
