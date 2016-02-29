#ifndef QGSGLOBEFEATURESOURCE_H
#define QGSGLOBEFEATURESOURCE_H

#include <osgEarthFeatures/FeatureSource>
#include <QObject>

#include "qgsglobefeatureoptions.h"
#include "qgsfeature.h"

class QgsGlobeFeatureSource : public QObject, public osgEarth::Features::FeatureSource
{
    Q_OBJECT
  public:
    QgsGlobeFeatureSource( const QgsGlobeFeatureOptions& options = osgEarth::Features::ConfigOptions() );

    osgEarth::Features::FeatureCursor* createFeatureCursor( const osgEarth::Symbology::Query& query = osgEarth::Symbology::Query() ) override;

    int getFeatureCount() const override;
    osgEarth::Features::Feature* getFeature( osgEarth::Features::FeatureID fid ) override;
    osgEarth::Features::Geometry::Type getGeometryType() const override;

    QgsVectorLayer* layer() const { return mLayer; }

    const char* className() const override { return "QGISFeatureSource"; }
    const char* libraryName() const override { return "QGIS"; }

    void initialize( const osgDB::Options* dbOptions );

  protected:
    const osgEarth::Features::FeatureProfile* createFeatureProfile() override { return mProfile; }
    const osgEarth::Features::FeatureSchema& getSchema() const override { return mSchema; }

    ~QgsGlobeFeatureSource() {}

  private:
    QgsGlobeFeatureOptions mOptions;
    QgsVectorLayer* mLayer;
    osgEarth::Features::FeatureProfile* mProfile;
    osgEarth::Features::FeatureSchema mSchema;
    typedef std::map<osgEarth::Features::FeatureID, osg::observer_ptr<osgEarth::Features::Feature> > FeatureMap_t;
    FeatureMap_t mFeatures;

  private slots:
    void attributeValueChanged( const QgsFeatureId&featureId, int idx, const QVariant &value );
    void geometryChanged( const QgsFeatureId&featureId, QgsGeometry&geometry );
};

#endif // QGSGLOBEFEATURESOURCE_H
