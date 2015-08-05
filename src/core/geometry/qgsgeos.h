/***************************************************************************
                        qgsgeos.h
  -------------------------------------------------------------------
Date                 : 22 Sept 2014
Copyright            : (C) 2014 by Marco Hugentobler
email                : marco.hugentobler at sourcepole dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGEOS_H
#define QGSGEOS_H

#include "qgsgeometryengine.h"
#include "qgspointv2.h"
#include "geosextra/geos_c_extra.h"
#include <geos_c.h>

class QgsLineStringV2;
class QgsPolygonV2;

/**Does vector analysis using the geos library and handles import, export, exception handling*/
class CORE_EXPORT QgsGeos: public QgsGeometryEngine
{
  public:
    QgsGeos( const QgsAbstractGeometryV2* geometry, int precision = 7 );
    ~QgsGeos();

    /**Removes caches*/
    void geometryChanged() override;
    void prepareGeometry() override;

    QgsAbstractGeometryV2* intersection( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* difference( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* combine( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* combine( const QList< const QgsAbstractGeometryV2* >, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* symDifference( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* buffer( double distance, int segments, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* buffer( double distance, int segments, int endCapStyle, int joinStyle, double mitreLimit ) const override;
    QgsAbstractGeometryV2* simplify( double tolerance, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* interpolate( double distance, QString* errorMsg = 0 ) const override;
    bool centroid( QgsPointV2& pt, QString* errorMsg = 0 ) const override;
    bool pointOnSurface( QgsPointV2& pt, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* convexHull( QString* errorMsg = 0 ) const override;
    double distance( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool intersects( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool touches( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool crosses( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool within( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool overlaps( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool contains( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool disjoint( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    double area( QString* errorMsg = 0 ) const override;
    double length( QString* errorMsg = 0 ) const override;
    bool isValid( QString* errorMsg = 0 ) const override;
    bool isEqual( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const override;
    bool isEmpty( QString* errorMsg = 0 ) const override;

    /**Splits this geometry according to a given line.
    @param splitLine the line that splits the geometry
    @param[out] newGeometries list of new geometries that have been created with the split
    @param topological true if topological editing is enabled
    @param[out] topologyTestPoints points that need to be tested for topological completeness in the dataset
    @return 0 in case of success, 1 if geometry has not been split, error else*/
    int splitGeometry( const QgsLineStringV2& splitLine,
                       QList<QgsAbstractGeometryV2*>& newGeometries,
                       bool topological,
                       QList<QgsPointV2> &topologyTestPoints, QString* errorMsg = 0 ) const override;

    QgsAbstractGeometryV2* offsetCurve( double distance, int segments, int joinStyle, double mitreLimit, QString* errorMsg = 0 ) const override;
    QgsAbstractGeometryV2* reshapeGeometry( const QgsLineStringV2& reshapeWithLine, int* errorCode, QString* errorMsg = 0 ) const;

    static QgsAbstractGeometryV2* fromGeos( const GEOSGeometry* geos );
    static QgsPolygonV2* fromGeosPolygon( const GEOSGeometry* geos );
    static GEOSGeometry* asGeos( const QgsAbstractGeometryV2* geom );
    static QgsPointV2 coordSeqPoint( const GEOSCoordSequence* cs, int i, bool hasZ, bool hasM );

    static GEOSContextHandle_t getGEOSHandler();

  private:
    mutable GEOSGeometry* mGeos;
    const GEOSPreparedGeometry* mGeosPrepared;
    //precision reducer
    GEOSPrecisionModel* mPrecisionModel;
    GEOSGeometryPrecisionReducer* mPrecisionReducer;

    enum Overlay
    {
      INTERSECTION,
      DIFFERENCE,
      UNION,
      SYMDIFFERENCE
    };

    enum Relation
    {
      INTERSECTS,
      TOUCHES,
      CROSSES,
      WITHIN,
      OVERLAPS,
      CONTAINS,
      DISJOINT
    };

    //geos util functions
    void cacheGeos() const;
    QgsAbstractGeometryV2* overlay( const QgsAbstractGeometryV2& geom, Overlay op, QString* errorMsg = 0 ) const;
    bool relation( const QgsAbstractGeometryV2& geom, Relation r, QString* errorMsg = 0 ) const;
    static GEOSCoordSequence* createCoordinateSequence( const QgsCurveV2* curve );
    static QgsLineStringV2* sequenceToLinestring( const GEOSGeometry* geos, bool hasZ, bool hasM );
    static int numberOfGeometries( GEOSGeometry* g );
    static GEOSGeometry* nodeGeometries( const GEOSGeometry *splitLine, const GEOSGeometry *geom );
    int mergeGeometriesMultiTypeSplit( QVector<GEOSGeometry*>& splitResult ) const;
    static GEOSGeometry* createGeosCollection( int typeId, const QVector<GEOSGeometry*>& geoms );

    static GEOSGeometry* createGeosPoint( const QgsAbstractGeometryV2* point, int coordDims );
    static GEOSGeometry* createGeosLinestring( const QgsAbstractGeometryV2* curve );
    static GEOSGeometry* createGeosPolygon( const QgsAbstractGeometryV2* poly );

    //utils for geometry split
    int topologicalTestPointsSplit( const GEOSGeometry* splitLine, QList<QgsPointV2>& testPoints, QString* errorMsg = 0 ) const;
    GEOSGeometry* linePointDifference( GEOSGeometry* GEOSsplitPoint ) const;
    int splitLinearGeometry( GEOSGeometry* splitLine, QList<QgsAbstractGeometryV2*>& newGeometries ) const;
    int splitPolygonGeometry( GEOSGeometry* splitLine, QList<QgsAbstractGeometryV2*>& newGeometries ) const;

    //utils for reshape
    static GEOSGeometry* reshapeLine( const GEOSGeometry* line, const GEOSGeometry* reshapeLineGeos );
    static GEOSGeometry* reshapePolygon( const GEOSGeometry* polygon, const GEOSGeometry* reshapeLineGeos );
    static int lineContainedInLine( const GEOSGeometry* line1, const GEOSGeometry* line2 );
    static int pointContainedInLine( const GEOSGeometry* point, const GEOSGeometry* line );
    static int geomDigits( const GEOSGeometry* geom );
};

#endif // QGSGEOS_H
