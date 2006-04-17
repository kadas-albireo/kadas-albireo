/***************************************************************************
                              qgsscalecalculator.h    
                 Calculates scale based on map extent and units
                              -------------------
  begin                : May 18, 2004
  copyright            : (C) 2004 by Gary E.Sherman
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

#include <assert.h>
#include <math.h>
#include <qstring.h>
#include "qgslogger.h"
#include "qgsrect.h"
#include "qgsscalecalculator.h"

QgsScaleCalculator::QgsScaleCalculator(int dpi, QGis::units mapUnits) 
: mDpi(dpi), mMapUnits(mapUnits)
{}

QgsScaleCalculator::~QgsScaleCalculator()
{}

void QgsScaleCalculator::setDpi(int dpi)
{
  mDpi = dpi;
}

void QgsScaleCalculator::setMapUnits(QGis::units mapUnits)
{
  mMapUnits = mapUnits;
}

double QgsScaleCalculator::calculate(QgsRect &mapExtent, int canvasWidth)
{
  double conversionFactor; 
  double delta;
  // calculation is based on the map units and extent, the dpi of the
  // users display, and the canvas width
  switch(mMapUnits)
  {
    case QGis::METERS:
      // convert meters to inches
      conversionFactor = 39.3700787;
      delta = mapExtent.xMax() - mapExtent.xMin();
      break;
    case QGis::FEET:
      conversionFactor = 12.0;
      delta = mapExtent.xMax() - mapExtent.xMin();
      break;
    case QGis::DEGREES:
      // degrees require conversion to meters first
      conversionFactor = 39.3700787;
      delta = calculateGeographicDistance(mapExtent);
      break;
    default:
      assert("bad map units");
      break; 
  }
  QgsDebugMsg("Using conversionFactor of " + QString::number(conversionFactor));
  double scale = (delta * conversionFactor)/(canvasWidth/mDpi);
  return scale;
}


double  QgsScaleCalculator::calculateGeographicDistance(QgsRect &mapExtent)
{
  // need to calculate the x distance in meters 
  // We'll use the middle latitude for the calculation
  // Note this is an approximation (although very close) but calculating scale
  // for geographic data over large extents is quasi-meaningless

  // The distance between two points on a sphere can be estimated
  // using the Haversine formula. This gives the shortest distance
  // between two points on the sphere. However, what we're after is
  // the distance from the left of the given extent and the right of
  // it. This is not necessarily the shortest distance between two
  // points on a sphere.
  //
  // The code below uses the Haversine formula, but with some changes
  // to cope with the above problem, and also to deal with the extent
  // possibly extending beyond +/-180 degrees: 
  //
  // - Use the Halversine formula to calculate the distance from -90 to
  //   +90 degrees at the mean latitude.
  // - Scale this distance by the number of degrees between
  //   mapExtent.xMin() and mapExtent.xMax();
  // - For a slight improvemnt, allow for the ellipsoid shape of earth.


  // For a longitude change of 180 degrees
  double lat = (mapExtent.yMax() + mapExtent.yMin()) * 0.5;
  const static double rads = (4.0 * atan(1.0))/180.0;
  double a = pow(cos(lat * rads), 2);
  double c = 2.0 * atan2(sqrt(a), sqrt(1.0-a));
  const static double ra = 6378000; // [m]
  const static double rb = 6357000; // [m]
  // The eccentricity. This comes from sqrt(1.0 - rb*rb/(ra*ra));
  const static double e = 0.0810820288;
  double radius = ra * (1.0 - e*e) / 
    pow(1.0 - e*e*sin(lat*rads)*sin(lat*rads), 1.5);
  double meters = (mapExtent.xMax() - mapExtent.xMin()) / 180.0 * radius * c;

  QgsDebugMsg("Distance across map extent (m): " + QString::number(meters));

  return meters;
}
