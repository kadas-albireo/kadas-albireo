class QgsGlobeFeatureUtils
{
%TypeHeaderCode
#include <qgsglobefeatureutils.h>
%End
  public:
    static osg::Vec3d pointFromQgsPoint( const QgsPoint& pt );

    static Polygon* polygonFromQgsPolygon( const QgsPolygon& poly );
/*
    static Geometry* geometryFromQgsGeometry( const QgsGeometry& geom );
*/
    static osgEarth::Features::Feature* featureFromQgsFeature( const QgsFields& fields, const QgsFeature& feat );
};

