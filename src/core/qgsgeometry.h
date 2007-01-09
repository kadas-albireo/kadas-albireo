/***************************************************************************
  qgsgeometry.h - Geometry (stored as Open Geospatial Consortium WKB)
  -------------------------------------------------------------------
Date                 : 02 May 2005
Copyright            : (C) 2005 by Brendan Morley
email                : morb at ozemail dot com dot au
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSGEOMETRY_H
#define QGSGEOMETRY_H

#include <QString>
#include <QVector>

#include "qgis.h"

#include <geos.h>
#if GEOS_VERSION_MAJOR < 3
#define GEOS_GEOM geos
#define GEOS_IO geos
#define GEOS_UTIL geos
#else
#define GEOS_GEOM geos::geom
#define GEOS_IO geos::io
#define GEOS_UTIL geos::util
#endif

#include "qgspoint.h"

namespace geos
{
  class CoordinateSequence;
  class Geometry;
  class GeometryFactory;
  class Polygon;
}


/** polyline is represented as a vector of points */
typedef QVector<QgsPoint> QgsPolyline;

/** polygon: first item of the list is outer ring, inner rings (if any) start from second item */
typedef QVector<QgsPolyline> QgsPolygon;
    
/** a collection of QgsPoints that share a common collection of attributes */
typedef QVector<QgsPoint> QgsMultiPoint;

/** a collection of QgsPolylines that share a common collection of attributes */
typedef QVector<QgsPolyline> QgsMultiPolyline;

/** a collection of QgsPolygons that share a common collection of attributes */
typedef QVector<QgsPolygon> QgsMultiPolygon;

class QgsGeometryVertexIndex;
class QgsPoint;
class QgsRect;

/** 
 * Represents a geometry with input and output in formats specified by 
 * (at least) the Open Geospatial Consortium (WKB / WKT), and containing
 * various functions for geoprocessing of the geometry.
 *
 * The geometry is represented internally by the OGC WKB format, though
 * perhaps this will be migrated to GEOS geometry in future.
 *
 * @author Brendan Morley
 */

class CORE_EXPORT QgsGeometry {

  public:
  

    //! Constructor
    QgsGeometry();
    
    /** copy constructor will prompt a deep copy of the object */
    QgsGeometry( QgsGeometry const & );
    
    /** assignments will prompt a deep copy of the object */
    QgsGeometry & operator=( QgsGeometry const & rhs );

    //! Destructor
    ~QgsGeometry();

    /** static method that creates geometry from WKT */
    static QgsGeometry* fromWkt(QString wkt);
   
    /** 
       Set the geometry, feeding in the buffer containing OGC Well-Known Binary and the buffer's length.
       This class will take ownership of the buffer.
    */
    void setWkbAndOwnership(unsigned char * wkb, size_t length);
    
    /** 
       Returns the buffer containing this geometry in WKB format.
       You may wish to use in conjunction with wkbSize().
    */
    unsigned char * wkbBuffer() const;
    
    /** 
       Returns the size of the WKB in wkbBuffer().
    */
    size_t wkbSize() const;
    
    /** 
       Returns the QString containing this geometry in WKT format.
    */
    QString const& wkt() const; 
    
    /** Returns type of wkb (point / linestring / polygon etc.) */
    QGis::WKBTYPE wkbType() const;
    
    /** Returns type of the vector */
    QGis::VectorType vectorType() const;
    
    /** Returns true if wkb of the geometry is of WKBMulti* type */
    bool isMultipart() const;

    /**
       Set the geometry, feeding in a geometry in GEOS format.
    */
    void setGeos(GEOS_GEOM::Geometry* geos);
    
    double distance(QgsGeometry& geom);

    /**
       Returns the vertex closest to the given point 
       (and also vertex index, squared distance and indexes of the vertices before/after)
    */
    QgsPoint closestVertex(const QgsPoint& point, QgsGeometryVertexIndex& atVertex, int& beforeVertex, int& afterVertex, double& sqrDist) const;


    /**
       Returns the indexes of the vertices before and after the given vertex index.

       This function takes into account the following factors:

       1. If the given vertex index is at the end of a linestring,
          the adjacent index will be -1 (for "no adjacent vertex")
       2. If the given vertex index is at the end of a linear ring
          (such as in a polygon), the adjacent index will take into
          account the first vertex is equal to the last vertex (and will
          skip equal vertex positions).
    */
    void adjacentVerticies(const QgsGeometryVertexIndex& atVertex, int& beforeVertex, int& afterVertex) const;


    /** Insert a new vertex before the given vertex index,
     *  ring and item (first number is index 0)
     *  If the requested vertex number (beforeVertex.back()) is greater
     *  than the last actual vertex on the requested ring and item,
     *  it is assumed that the vertex is to be appended instead of inserted.
     *  Returns FALSE if atVertex does not correspond to a valid vertex
     *  on this geometry (including if this geometry is a Point).
     *  It is up to the caller to distinguish between
     *  these error conditions.  (Or maybe we add another method to this
     *  object to help make the distinction?)
     */
    bool insertVertexBefore(double x, double y, QgsGeometryVertexIndex beforeVertex);

    /** Moves the vertex at the given position number,
     *  ring and item (first number is index 0)
     *  to the given coordinates.
     *  Returns FALSE if atVertex does not correspond to a valid vertex
     *  on this geometry
     */
    bool moveVertexAt(double x, double y, QgsGeometryVertexIndex atVertex);

    /** Deletes the vertex at the given position number,
     *  ring and item (first number is index 0)
     *  Returns FALSE if atVertex does not correspond to a valid vertex
     *  on this geometry (including if this geometry is a Point),
     *  or if the number of remaining verticies in the linestring
     *  would be less than two.
     *  It is up to the caller to distinguish between
     *  these error conditions.  (Or maybe we add another method to this
     *  object to help make the distinction?)
     */
    bool deleteVertexAt(QgsGeometryVertexIndex atVertex);

    /**
     *  Modifies x and y to indicate the location of
     *  the vertex at the given position number,
     *  ring and item (first number is index 0)
     *  to the given coordinates
     *
     *  Returns TRUE if atVertex is a valid position on
     *  the geometry, otherwise FALSE.
     *
     *  If FALSE, x and y are not modified.
     */
    bool vertexAt(double &x, double &y, QgsGeometryVertexIndex atVertex) const;

    /**
        Returns the squared cartesian distance between the given point
        to the given vertex index (vertex at the given position number,
        ring and item (first number is index 0))

     */
    double sqrDistToVertexAt(QgsPoint& point,
                             QgsGeometryVertexIndex& atVertex) const;

    /**
        Returns, in atVertex, the closest vertex in this geometry to the given point.
        The squared cartesian distance is also returned in sqrDist.
     */
    QgsPoint closestVertexWithContext(QgsPoint& point,
                                      QgsGeometryVertexIndex& atVertex,
                                      double& sqrDist);

    /**
        Returns, in beforeVertex, the closest segment in this geometry to the given point.
        The squared cartesian distance is also returned in sqrDist.
     */
    QgsPoint closestSegmentWithContext(QgsPoint& point,
                                       QgsGeometryVertexIndex& beforeVertex,
                                       double& sqrDist);

    /**Returns the bounding box of this feature*/
    QgsRect boundingBox() const;

    /** Test for intersection with a rectangle (uses GEOS) */
    bool intersects(const QgsRect& r) const;

    /**Also tests for intersection, but uses direct geos export of QgsGeometry instead wkb export and geos wkb import. Therefore this method is faster and could replace QgsGeometry::intersects in the future*/
    bool fast_intersects(const QgsRect& r) const;

    /** Test for containment of a point (uses GEOS) */
    bool contains(QgsPoint* p) const;

    /**Creates a geos geometry from this features geometry. Note, that the returned object needs to be deleted*/
    GEOS_GEOM::Geometry* geosGeometry() const;


    /* Accessor functions for getting geometry data */
    
    /** return contents of the geometry as a point
        if wkbType is WKBPoint, otherwise returns [0,0] */
    QgsPoint asPoint();
    
    /** return contents of the geometry as a polyline
        if wkbType is WKBLineString, otherwise an empty list */
    QgsPolyline asPolyline();
    
    /** return contents of the geometry as a polygon
        if wkbType is WKBPolygon, otherwise an empty list */
    QgsPolygon asPolygon();
    
    /** return contents of the geometry as a polygon
        if wkbType is WKBPolygon, otherwise an empty list */
    QgsMultiPoint asMultiPoint();
    
    /** return contents of the geometry as a polygon
        if wkbType is WKBPolygon, otherwise an empty list */
    QgsMultiPolyline asMultiPolyline();
    
    /** return contents of the geometry as a polygon
        if wkbType is WKBPolygon, otherwise an empty list */
    QgsMultiPolygon asMultiPolygon();

  private:

    // Private static members

    //! This is used to create new GEOS variables.
    static GEOS_GEOM::GeometryFactory* geosGeometryFactory;


    // Private variables

    // All of these are mutable since there may be on-the-fly
    // conversions between WKB, GEOS and WKT;
    // However the intent is the const functions do not
    // semantically change the value that this object represents.

    /** pointer to geometry in binary WKB format
        This is the class' native implementation
     */
    mutable unsigned char * mGeometry;

    /** size of geometry */
    mutable size_t mGeometrySize;

    /** cached WKT version of this geometry */
    mutable QString mWkt;

    /** cached GEOS version of this geometry */
    mutable GEOS_GEOM::Geometry* mGeos;

    /** If the geometry has been set since the last conversion to WKB **/
    mutable bool mDirtyWkb;

    /** If the geometry has been set since the last conversion to WKT **/
    mutable bool mDirtyWkt;

    /** If the geometry has been set  since the last conversion to GEOS **/
    mutable bool mDirtyGeos;


    // Private functions

    /** Squared distance from point to the given line segment 
     *  TODO: Perhaps move this to QgsPoint
     */
    double distanceSquaredPointToSegment(QgsPoint& point,
                                         double *x1, double *y1,
                                         double *x2, double *y2,
                                         QgsPoint& minDistPoint);

    /**Exports the current WKB to mWkt
     @return true in case of success and false else*/
    bool exportToWkt(unsigned char * geom) const;
    bool exportToWkt() const;

    /** Converts from the WKB geometry to the GEOS geometry.  (Experimental) 
        @return   true in case of success and false else
     */
    bool exportWkbToGeos() const;

    /** Converts from the GEOS geometry to the WKB geometry.  (Experimental) 
        @return   true in case of success and false else
     */
    bool exportGeosToWkb() const;



    /** Insert a new vertex before the given vertex index (first number is index 0)
     *  in the given GEOS Coordinate Sequence.
     *  If the requested vertex number is greater
     *  than the last actual vertex,
     *  it is assumed that the vertex is to be appended instead of inserted.
     *  @param old_sequence   The sequence to update (The caller remains the owner).
     *  @param new_sequence   The updated sequence (The caller becomes the owner if the function returns TRUE).
     *  Returns FALSE if beforeVertex does not correspond to a valid vertex number
     *  on the Coordinate Sequence.
     */
    bool insertVertexBefore(double x, double y,
                            int beforeVertex,
                            const GEOS_GEOM::CoordinateSequence*  old_sequence,
                                  GEOS_GEOM::CoordinateSequence** new_sequence);

    /** return point from wkb */
    QgsPoint asPoint(unsigned char*& ptr, bool hasZValue);
    
    /** return polyline from wkb */
    QgsPolyline asPolyline(unsigned char*& ptr, bool hasZValue);
    
    /** return polygon from wkb */
    QgsPolygon asPolygon(unsigned char*& ptr, bool hasZValue);

}; // class QgsGeometry

#endif
