/***************************************************************************
                          qgsmaptopixel.h  -  description
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
#ifndef QGSMAPTOPIXEL
#define QGSMAPTOPIXEL

#include "qgspoint.h"
#include <vector>

#include <cassert>

class QgsPoint;
class QPoint;

/** \ingroup core
  * Perform transforms between map coordinates and device coordinates.
  *
  * This class can convert device coordinates to map coordinates and vice versa.
  */
class CORE_EXPORT QgsMapToPixel
{
  public:
    /* Constructor
    * @param mapUnitsPerPixel Map units per pixel
    * @param ymax Maximum y value of the map canvas
    * @param ymin Minimum y value of the map canvas
    * @param xmin Minimum x value of the map canvas
    */
    QgsMapToPixel( double mapUnitsPerPixel = 0, double ymax = 0, double ymin = 0,
                   double xmin = 0 );
    //! destructor
    ~QgsMapToPixel();
    /*! Transform the point from map (world) coordinates to device coordinates
    * @param p Point to transform
    * @return QgsPoint in device coordinates
    */
    QgsPoint transform( const QgsPoint& p ) const;
    void transform( QgsPoint* p ) const;
    /*! Transform the point specified by x,y from map (world)
     * coordinates to device coordinates
     * @param x x cordinate o point to transform
     * @param y y coordinate of point to transform
     * @return QgsPoint in device coordinates
    */
    QgsPoint transform( double x, double y ) const;
    /*! Tranform device coordinates to map (world)  coordinates
    * @param x x coordinate of point to be converted to map cooordinates
    * @param y y coordinate of point to be converted to map cooordinates
    * @return QgsPoint in map coordinates
    */

    /* Transform device coordinates to map coordinates. Modifies the
       given coordinates in place. Intended as a fast way to do the
       transform. */
    void transformInPlace( double& x, double& y ) const;

    /* Transform device coordinates to map coordinates. Modifies the
       given coordinates in place. Intended as a fast way to do the
       transform.
       @note not available in python bindings
     */
    void transformInPlace( QVector<double>& x, QVector<double>& y ) const;

#ifdef ANDROID
    void transformInPlace( float& x, float& y ) const;
    void transformInPlace( QVector<float>& x, QVector<float>& y ) const;
#endif

    QgsPoint toMapCoordinates( int x, int y ) const;

    /*! Transform device coordinates to map (world) coordinates
      @note: this method was added in version 1.6*/
    QgsPoint toMapCoordinatesF( double x, double y ) const;

    /*! Tranform device coordinates to map (world)  coordinates
     * @param p Point to be converted to map cooordinates
     * @return QgsPoint in map coorndiates
     */
    QgsPoint toMapCoordinates( QPoint p ) const;

    QgsPoint toMapPoint( double x, double y ) const;
    /*! Set map units per pixel
    * @param mapUnitsPerPixel Map units per pixel
    */
    void setMapUnitsPerPixel( double mapUnitsPerPixel );

    //! Return current map units per pixel
    double mapUnitsPerPixel() const;

    //! Set maximum y value
    void setYMaximum( double ymax );
    //! Set minimum y value
    void setYMinimum( double ymin );
    //! set minimum x value
    void setXMinimum( double xmin );
    /*! Set parameters for use in tranfsorming coordinates
    * @param mapUnitsPerPixel Map units per pixel
    * @param xmin Minimum x value
    * @param ymin Minimum y value
    * @param ymax Maximum y value
    */
    void setParameters( double mapUnitsPerPixel, double xmin, double ymin, double ymax );
    //! String representation of the parameters used in the transform
    QString showParameters();

  private:
    double mMapUnitsPerPixel;
    double yMax;
    double yMin;
    double xMin;
    double xMax;

};


#endif // QGSMAPTOPIXEL
