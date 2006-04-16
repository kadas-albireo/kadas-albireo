/***************************************************************************
                          qgsrect.cpp  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */

#include <iostream>
#include <algorithm>
#include <cmath>
#ifdef WIN32
#include <limits>
#endif
#include <QString>
#include <QTextStream> 

#include "qgspoint.h"
#include "qgsrect.h"

QgsRect::QgsRect(double newxmin, double newymin, double newxmax, double newymax)
    : xmin(newxmin), ymin(newymin), xmax(newxmax), ymax(newymax)
{
  normalize();
}

QgsRect::QgsRect(QgsPoint const & p1, QgsPoint const & p2)
{
  set(p1, p2);
}

QgsRect::QgsRect(const QgsRect &r)
{
  xmin = r.xMin();
  ymin = r.yMin();
  xmax = r.xMax();
  ymax = r.yMax();
}

void QgsRect::set(const QgsPoint& p1, const QgsPoint& p2)
{
  xmin = p1.x();
  xmax = p2.x();
  ymin = p1.y();
  ymax = p2.y();
  normalize();
}

void QgsRect::set(double xmin_, double ymin_, double xmax_, double ymax_)
{
  xmin = xmin_;
  ymin = ymin_;
  xmax = xmax_;
  ymax = ymax_;
  normalize();
}

void QgsRect::normalize()
{
  if (xmin > xmax)
  {
      std::swap(xmin, xmax);
  }
  if (ymin > ymax)
  {
      std::swap(ymin, ymax);
  }
} // QgsRect::normalize()


void QgsRect::setMinimal()
{
  xmin = std::numeric_limits<double>::max();
  ymin = std::numeric_limits<double>::max();
  xmax =-std::numeric_limits<double>::max();
  ymax =-std::numeric_limits<double>::max();
}

void QgsRect::scale(double scaleFactor, const QgsPoint * cp)
{
  // scale from the center
  double centerX, centerY;
  if (cp)
  {
      centerX = cp->x();
      centerY = cp->y();
  } else
  {
      centerX = xmin + width() / 2;
      centerY = ymin + height() / 2;
  }
  double newWidth = width() * scaleFactor;
  double newHeight = height() * scaleFactor;
  xmin = centerX - newWidth / 2.0;
  xmax = centerX + newWidth / 2.0;
  ymin = centerY - newHeight / 2.0;
  ymax = centerY + newHeight / 2.0;
}

void QgsRect::expand(double scaleFactor, const QgsPoint * cp)
{
  // scale from the center
  double centerX, centerY;
  if (cp)
  {
      centerX = cp->x();
      centerY = cp->y();
  } else
  {
      centerX = xmin + width() / 2;
      centerY = ymin + height() / 2;
  }

  double newWidth = width() * scaleFactor;
  double newHeight = height() * scaleFactor;
  xmin = centerX - newWidth;
  xmax = centerX + newWidth;
  ymin = centerY - newHeight;
  ymax = centerY + newHeight;
}

QgsRect QgsRect::intersect(QgsRect * rect)
{
  QgsRect intersection = QgsRect();
  
  intersection.setXmin(xmin > rect->xMin()?xmin:rect->xMin());
  intersection.setXmax(xmax < rect->xMax()?xmax:rect->xMax());
  intersection.setYmin(ymin > rect->yMin()?ymin:rect->yMin());
  intersection.setYmax(ymax < rect->yMax()?ymax:rect->yMax());
  return intersection;
}


void QgsRect::combineExtentWith(QgsRect * rect)
{
 
  xmin = ( (xmin < rect->xMin())? xmin : rect->xMin() );
  xmax = ( (xmax > rect->xMax())? xmax : rect->xMax() );

  ymin = ( (ymin < rect->yMin())? ymin : rect->yMin() );
  ymax = ( (ymax > rect->yMax())? ymax : rect->yMax() );

}

void QgsRect::combineExtentWith(double x, double y)
{
 
  xmin = ( (xmin < x)? xmin : x );
  xmax = ( (xmax > x)? xmax : x );

  ymin = ( (ymin < y)? ymin : y );
  ymax = ( (ymax > y)? ymax : y );

}

bool QgsRect::isEmpty() const
{
  if (xmax <= xmin || ymax <= ymin)
  {
      return TRUE;
  } else
  {
      return FALSE;
  }
}

QString QgsRect::asWKTCoords() const
{
  QString rep = 
    QString::number(xmin,'f',16) + " " +
    QString::number(ymin,'f',16) + ", " +
    QString::number(xmax,'f',16) + " " +
    QString::number(ymax,'f',16);

    return rep;
}

// Return a string representation of the rectangle with automatic or high precision
QString QgsRect::stringRep(bool automaticPrecision) const
{
  if (automaticPrecision)
  {
    int precision = 0;
    if ( (width() < 1 || height() < 1) && (width() > 0 && height() > 0) )
    {
      precision = static_cast<int>( ceil( -1.0*log10(std::min(width(), height())) ) ) + 1;
      // sanity check
      if (precision > 20)
	precision = 20;
    }
    return stringRep(precision);
  }
  else
    return stringRep(16);
}

// overloaded version of above fn to allow precision to be set
// Return a string representation of the rectangle with high precision
QString QgsRect::stringRep(int thePrecision) const
{
  
  QString rep = QString::number(xmin,'f',thePrecision) + 
                QString(",") +
                QString::number(ymin,'f',thePrecision) +
                QString(" : ") +
                QString::number(xmax,'f',thePrecision) +
                QString(",") +
                QString::number(ymax,'f',thePrecision) ;
#ifdef QGISDEBUG
  //std::cout << "Extents : " << rep.toLocal8Bit().data() << std::endl;
#endif
  return rep;
}


// Return the rectangle as a set of polygon coordinates
QString QgsRect::asPolygon() const
{
//   QString rep = tmp.sprintf("%16f %16f,%16f %16f,%16f %16f,%16f %16f,%16f %16f",
//     xmin, ymin, xmin, ymax, xmax, ymax, xmax, ymin, xmin, ymin);
   QString rep;

   QTextOStream foo( &rep );

   foo.precision(8);
   foo.setf(QTextStream::fixed);
   // NOTE: a polygon isn't a polygon unless its closed. In the case of 
   //       a rectangle, that means 5 points (last == first)
   foo <<  xmin << " " <<  ymin << ", " 
       <<  xmin << " " <<  ymax << ", " 
       <<  xmax << " " <<  ymax << ", " 
       <<  xmax << " " <<  ymin << ", "
       <<  xmin << " " <<  ymin;

   return rep;

} // QgsRect::asPolygon() const


bool QgsRect::operator==(const QgsRect & r1) const
{
  return (r1.xMax() == xMax() && 
          r1.xMin() == xMin() && 
          r1.yMax() == yMax() && 
          r1.yMin() == yMin());
}


bool QgsRect::operator!=(const QgsRect & r1) const
{
  return ( ! operator==(r1));
}


QgsRect & QgsRect::operator=(const QgsRect & r)
{
  if (&r != this)
  {
      xmax = r.xMax();
      xmin = r.xMin();
      ymax = r.yMax();
      ymin = r.yMin();
    }

  return *this;
}


void QgsRect::unionRect(const QgsRect& r)
{
  if (r.xMin() < xMin()) setXmin(r.xMin());
  if (r.xMax() > xMax()) setXmax(r.xMax());
  if (r.yMin() < yMin()) setYmin(r.yMin());
  if (r.yMax() > yMax()) setYmax(r.yMax());
}
