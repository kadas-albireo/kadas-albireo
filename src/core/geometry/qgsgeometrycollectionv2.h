/***************************************************************************
                        qgsgeometrycollectionv2.h
  -------------------------------------------------------------------
Date                 : 28 Oct 2014
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

#ifndef QGSGEOMETRYCOLLECTIONV2_H
#define QGSGEOMETRYCOLLECTIONV2_H

#include "qgsabstractgeometryv2.h"
#include "qgspointv2.h"
#include <QVector>

/**\ingroup core
 * \class QgsGeometryCollectionV2
 * \brief Geometry collection
 * \note added in QGIS 2.10
 */
class CORE_EXPORT QgsGeometryCollectionV2: public QgsAbstractGeometryV2
{
  public:
    QgsGeometryCollectionV2();
    QgsGeometryCollectionV2( const QgsGeometryCollectionV2& c );
    QgsGeometryCollectionV2& operator=( const QgsGeometryCollectionV2& c );
    virtual ~QgsGeometryCollectionV2();

    virtual QgsGeometryCollectionV2* clone() const override;

    /** Returns the number of geometries within the collection.
     */
    int numGeometries() const;

    /** Returns a const reference to a geometry from within the collection.
     * @param n index of geometry to return
     */
    const QgsAbstractGeometryV2* geometryN( int n ) const;

    /** Returns a geometry from within the collection.
     * @param n index of geometry to return
     */
    QgsAbstractGeometryV2* geometryN( int n );

    //methods inherited from QgsAbstractGeometry
    virtual int dimension() const override;
    virtual QString geometryType() const override { return "GeometryCollection"; }
    virtual void clear() override;

    /**Adds a geometry and takes ownership. Returns true in case of success.*/
    virtual bool addGeometry( QgsAbstractGeometryV2* g );

    /** Inserts a geometry before a specified index and takes ownership. Returns true in case of success.
    * @param index position to insert geometry before
    */
    virtual bool insertGeometry( QgsAbstractGeometryV2* g, int index );

    /** Removes a geometry from the collection.
     * @param nr index of geometry to remove
     * @returns true if removal was successful.
     */
    virtual bool removeGeometry( int nr );

    /** Transforms the geometry using a coordinate transform
     * @param ct coordinate transform
       @param d transformation direction
     */
    virtual void transform( const QgsCoordinateTransform& ct, QgsCoordinateTransform::TransformDirection d = QgsCoordinateTransform::ForwardTransform ) override;
    void transform( const QTransform& t ) override;
    virtual void draw( QPainter& p ) const override;

    bool fromWkb( const unsigned char * wkb ) override;
    virtual bool fromWkt( const QString& wkt ) override;
    int wkbSize() const override;
    unsigned char* asWkb( int& binarySize ) const override;
    QString asWkt( int precision = 17 ) const override;
    QDomElement asGML2( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const override;
    QDomElement asGML3( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const override;
    QString asJSON( int precision = 17 ) const override;
    /**Exports geometry as a KML fragment*/
    QString asKML( int precision = 17 ) const override;

    virtual QgsRectangle calculateBoundingBox() const override;

    virtual void coordinateSequence( QList< QList< QList< QgsPointV2 > > >& coord ) const override;
    virtual double closestSegment( const QgsPointV2& pt, QgsPointV2& segmentPt,  QgsVertexId& vertexAfter, bool* leftOf, double epsilon ) const override;
    bool nextVertex( QgsVertexId& id, QgsPointV2& vertex ) const override;

    //low-level editing
    virtual bool insertVertex( const QgsVertexId& position, const QgsPointV2& vertex ) override;
    virtual bool moveVertex( const QgsVertexId& position, const QgsPointV2& newPos ) override;
    virtual bool deleteVertex( const QgsVertexId& position ) override;

    virtual double length() const override;
    virtual double area() const override;

    bool hasCurvedSegments() const override;

    /**Returns a geometry without curves. Caller takes ownership*/
    QgsAbstractGeometryV2* segmentize() const override;

    virtual int vertexCount( int part = 0, int ring = 0 ) const override { return mGeometries[part]->vertexCount( 0, ring ); }
    virtual int ringCount( int part = 0 ) const override { return mGeometries[part]->ringCount(); }
    virtual int partCount() const override { return mGeometries.size(); }
    virtual QgsPointV2 vertexAt( const QgsVertexId& id ) const override { return mGeometries[id.part]->vertexAt( id ); }

    /** Returns approximate rotation angle for a vertex. Usually average angle between adjacent segments.
        @return rotation in radians, clockwise from north*/
    double vertexAngle( const QgsVertexId& vertex ) const override;

    virtual bool addZValue( double zValue = 0 ) override;
    virtual bool addMValue( double mValue = 0 ) override;

  protected:
    QVector< QgsAbstractGeometryV2* > mGeometries;

    /** Returns whether child type names are omitted from Wkt representations of the collection
     * @note added in QGIS 2.12
     */
    virtual bool wktOmitChildType() const { return false; }

    /** Reads a collection from a WKT string.
     */
    bool fromCollectionWkt( const QString &wkt, const QList<QgsAbstractGeometryV2*>& subtypes, const QString& defaultChildWkbType = QString() );

};

#endif // QGSGEOMETRYCOLLECTIONV2_H
