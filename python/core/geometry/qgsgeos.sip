class QgsGeos: public QgsGeometryEngine
{
%TypeHeaderCode
#include <qgsgeos.h>
%End

  public:
    QgsGeos( const QgsAbstractGeometryV2* geometry );
    ~QgsGeos();

    /**Removes caches*/
    void geometryChanged();
    void prepareGeometry();

    QgsAbstractGeometryV2* intersection( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* difference( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* combine( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const ;
    QgsAbstractGeometryV2* combine( const QList< const QgsAbstractGeometryV2* >, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* symDifference( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* buffer( double distance, int segments, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* simplify( double tolerance, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* interpolate( double distance, QString* errorMsg = 0 ) const;
    bool centroid( QgsPointV2& pt, QString* errorMsg = 0 ) const;
    bool pointOnSurface( QgsPointV2& pt, QString* errorMsg = 0 ) const;
    QgsAbstractGeometryV2* convexHull( QString* errorMsg ) const;
    double distance( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool intersects( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool touches( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool crosses( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool within( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool overlaps( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool contains( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool disjoint( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    double area( QString* errorMsg ) const;
    double length( QString* errorMsg ) const;
    bool isValid( QString* errorMsg ) const;
    bool isEqual( const QgsAbstractGeometryV2& geom, QString* errorMsg = 0 ) const;
    bool isEmpty( QString* errorMsg = 0 ) const;

    QgsAbstractGeometryV2* offsetCurve( double distance, int segments, int joinStyle, double mitreLimit ) const;
    QgsAbstractGeometryV2* reshapeGeometry( const QgsLineStringV2& reshapeWithLine, int* errorCode ) const;
};
