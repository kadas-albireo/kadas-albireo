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
#include <QPainter>

/*!
  \class QgsRubberBand
  \brief The QgsRubberBand class provides a transparent overlay widget
  for tracking the mouse while drawing polylines or polygons.
*/
QgsRubberBand::QgsRubberBand(QgsMapCanvas* mapCanvas, bool isPolygon)
: QgsMapCanvasItem(mapCanvas), mIsPolygon(isPolygon)
{
  mPoints.push_back(QgsPoint());
  setColor(QColor(Qt::lightGray));
}

QgsRubberBand::~QgsRubberBand()
{}

/*!
  Set the outline and fill color.
*/
void QgsRubberBand::setColor(const QColor & color)
{
  mPen.setColor(color);
  QColor fillColor(color.red(), color.green(), color.blue(), 63);
  mBrush.setColor(fillColor);
  mBrush.setStyle(Qt::SolidPattern);
}

/*!
  Set the outline width.
*/
void QgsRubberBand::setWidth(int width)
{
  mPen.setWidth(width);
}

/*!
  Remove all points from the shape being created.
*/
void QgsRubberBand::reset(bool isPolygon)
{
  mPoints.resize(1); // addPoint assumes an initial allocated point
  mIsPolygon = isPolygon;
  updateRect();
  updateCanvas();
}

/*!
  Add a point to the shape being created.
*/
void QgsRubberBand::addPoint(const QgsPoint & p, bool update /* = true */)
{
  mPoints[mPoints.size()-1] = p; // Current mouse position becomes added point
  mPoints.push_back(p); // Allocate new point to continue tracking current mouse position
  if (update)
  {
    updateRect();
    updateCanvas();
  }
}

/*!
  Update the line between the last added point and the mouse position.
*/
void QgsRubberBand::movePoint(const QgsPoint & p)
{
  mPoints[mPoints.size()-1] = p; // Update current mouse position
  updateRect();
  updateCanvas();
}

void QgsRubberBand::movePoint(int index, const QgsPoint& p)
{
  mPoints[index] = p;
  updateRect();
  updateCanvas();
}

/*!
  Draw the shape in response to an update event.
*/
void QgsRubberBand::drawShape(QPainter & p)
{
  if (mPoints.size() > 1)
  {
    QPolygon pts;
    int i;
    for (i = 0; i < mPoints.size(); i++)
      pts.append(toCanvasCoords(mPoints[i]));
    
    p.setPen(mPen);
    p.setBrush(mBrush);
    if (mIsPolygon)
    {
      p.drawPolygon(pts);
    }
    else
    {
      p.drawPolyline(pts);
    }
  }
}

void QgsRubberBand::updateRect()
{
  if (mPoints.size() > 0)
  {
    QgsRect r(mPoints[0], mPoints[0]);
    int i;
    for (i = 1; i < mPoints.size(); i++)
      r.combineExtentWith(mPoints[i].x(), mPoints[i].y());
    setRect(r);
  }
  else
  {
    // set empty rect
    setRect(QgsRect());
  }
  
  setVisible(mPoints.size() > 1);
}

