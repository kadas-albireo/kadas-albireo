/***************************************************************************
                          qgspoint.h  -  description
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

#ifndef QGSPOINT_H
#define QGSPOINT_H

#include <iostream>

#include <QString>

/** \ingroup core
 * A class to represent a point geometry.
 * Currently no Z axis / 2.5D support is implemented.
 */
class CORE_EXPORT QgsPoint
{
  public:
    /// Default constructor
    QgsPoint()
    {}

    /*! Create a point from another point */
    QgsPoint( const QgsPoint& p );

    /*! Create a point from x,y coordinates
     * @param x x coordinate
     * @param y y coordinate
     */
    QgsPoint( double x, double y )
        : m_x( x ), m_y( y )
    {}

    ~QgsPoint()
    {}

    /*! Sets the x value of the point
     * @param x x coordinate
     */
    void setX( double x )
    {
      m_x = x;
    }

    /*! Sets the y value of the point
     * @param y y coordinate
     */
    void setY( double y )
    {
      m_y = y;
    }

    /*! Sets the x and y value of the point */
    void set( double x, double y )
    {
      m_x = x;
      m_y = y;
    }

    /*! Get the x value of the point
     * @return x coordinate
     */
    double x() const
    {
      return m_x;
    }

    /*! Get the y value of the point
     * @return y coordinate
     */
    double y() const
    {
      return m_y;
    }

    //! String representation of the point (x,y)
    QString toString() const;

    //! As above but with precision for string representaiton of a point
    QString toString( int thePrecision ) const;

    /*! Return the well known text representation for the point.
     * The wkt is created without an SRID.
     * @return Well known text in the form POINT(x y)
     */
    QString wellKnownText() const;

    /**Returns the squared distance between this point and x,y*/
    double sqrDist( double x, double y ) const;

    /**Returns the squared distance between this and other point*/
    double sqrDist( const QgsPoint& other ) const;

    //! equality operator
    bool operator==( const QgsPoint &other );

    //! Inequality operator
    bool operator!=( const QgsPoint &other ) const;

    //! Assignment
    QgsPoint & operator=( const QgsPoint &other );

    //! Multiply x and y by the given value
    void multiply( const double& scalar );

    //! Test if this point is on the segment defined by points a, b
    //! @return 0 if this point is not on the open ray through a and b,
    //! 1 if point is on open ray a, 2 if point is within line segment,
    //! 3 if point is on open ray b.
    int onSegment( const QgsPoint& a, const QgsPoint& b ) const;

  private:

    //! x coordinate
    double m_x;

    //! y coordinate
    double m_y;


}; // class QgsPOint


inline bool operator==( const QgsPoint &p1, const QgsPoint &p2 )
{
  if (( p1.x() == p2.x() ) && ( p1.y() == p2.y() ) )
    { return true; }
  else
    { return false; }
}

inline std::ostream& operator << ( std::ostream& os, const QgsPoint &p )
{
  // Use Local8Bit for printouts
  os << p.toString().toLocal8Bit().data();
  return os;
}


#endif //QGSPOINT_H

