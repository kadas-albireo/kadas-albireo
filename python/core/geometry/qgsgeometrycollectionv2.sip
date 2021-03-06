class QgsGeometryCollectionV2: public QgsAbstractGeometryV2
{
%TypeHeaderCode
#include <qgsgeometrycollectionv2.h>
%End

  public:
    QgsGeometryCollectionV2();
    QgsGeometryCollectionV2( const QgsGeometryCollectionV2& c );
    //QgsGeometryCollectionV2& operator=( const QgsGeometryCollectionV2& c );
    virtual ~QgsGeometryCollectionV2();

    virtual QgsGeometryCollectionV2* clone() const;

    int numGeometries() const;
    //const QgsAbstractGeometryV2* geometryN( int n ) const;
    QgsAbstractGeometryV2* geometryN( int n );

    //methods inherited from QgsAbstractGeometry
    virtual int dimension() const;
    virtual QString geometryType() const;
    virtual void clear();

    /**Adds a geometry and takes ownership. Returns true in case of success*/
    virtual bool addGeometry( QgsAbstractGeometryV2* g /Transfer/ );

    /** Removes a geometry from the collection.
     * @param nr index of geometry to remove
     * @returns true if removal was successful.
     */
    virtual bool removeGeometry( int nr );

    /** Removes and returns a geometry from the collection.
     * @param nr index of geometry to remove
     * @returns The geometry, if removal was successful, otherwise 0.
     */
    virtual QgsAbstractGeometryV2* takeGeometry( int nr ) /TransferBack/;

    virtual void transform( const QgsCoordinateTransform& ct, QgsCoordinateTransform::TransformDirection d = QgsCoordinateTransform::ForwardTransform );
    void transform( const QTransform& t );
    //virtual void clip( const QgsRectangle& rect );
    virtual void draw( QPainter& p ) const;

    bool fromWkb( const unsigned char * wkb );
    int wkbSize() const;
    unsigned char* asWkb( int& binarySize ) const;
    QString asWkt( int precision = 17 ) const;
    QDomElement asGML2( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const;
    QDomElement asGML3( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const;
    QString asJSON( int precision = 17 ) const;

    virtual QgsRectangle calculateBoundingBox() const;

    virtual void coordinateSequence( QList< QList< QList< QgsPointV2 > > >& coord /Out/ ) const;
    virtual double closestSegment( const QgsPointV2& pt, QgsPointV2& segmentPt,  QgsVertexId& vertexAfter, bool* leftOf, double epsilon ) const;
    bool nextVertex( QgsVertexId& id, QgsPointV2& vertex ) const;

    //low-level editing
    virtual bool insertVertex( const QgsVertexId& position, const QgsPointV2& vertex );
    virtual bool moveVertex( const QgsVertexId& position, const QgsPointV2& newPos );
    virtual bool deleteVertex( const QgsVertexId& position );

    virtual double length() const;
    virtual double area() const;

    bool hasCurvedSegments() const;

    virtual int vertexCount(int part = 0, int ring = 0) const;
    virtual int ringCount(int part = 0) const;
    virtual int partCount() const;
    virtual QgsPointV2 vertexAt( const QgsVertexId& id ) const;

    /** Returns approximate rotation angle for a vertex. Usually average angle between adjacent segments.
        @return rotation in radians, clockwise from north*/
    double vertexAngle( const QgsVertexId& vertex ) const;

  protected:

    /** Returns whether child type names are omitted from Wkt representations of the collection
     * @note added in QGIS 2.12
     */
    virtual bool wktOmitChildType() const;
    virtual bool addZValue( double zValue = 0 );
    virtual bool addMValue( double mValue = 0 );
};
