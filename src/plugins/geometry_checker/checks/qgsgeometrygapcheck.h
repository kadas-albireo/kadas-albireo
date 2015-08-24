/***************************************************************************
 *  qgsgeometrygapcheck.h                                                  *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#ifndef QGS_GEOMETRY_GAP_CHECK_H
#define QGS_GEOMETRY_GAP_CHECK_H

#include "qgsgeometrycheck.h"


class QgsGeometryGapCheckError : public QgsGeometryCheckError
{
  public:
    QgsGeometryGapCheckError( const QgsGeometryCheck* check,
                              QgsAbstractGeometryV2* geometry,
                              const QgsFeatureIds& neighbors,
                              double area,
                              const QgsRectangle& gapAreaBBox )
        : QgsGeometryCheckError( check, FEATUREID_NULL, geometry->centroid(), QgsVertexId(), area, ValueArea )
        , mNeighbors( neighbors )
        , mGapAreaBBox( gapAreaBBox )
    {
      mGeometry = geometry;
    }
    ~QgsGeometryGapCheckError()
    {
      delete mGeometry;
    }

    QgsAbstractGeometryV2* geometry() { return mGeometry->clone(); }
    const QgsFeatureIds& neighbors() const { return mNeighbors; }

    bool isEqual( QgsGeometryCheckError* other ) const
    {
      QgsGeometryGapCheckError* err = dynamic_cast<QgsGeometryGapCheckError*>( other );
      return err && QgsGeomUtils::pointsFuzzyEqual( err->location(), location(), QgsGeometryCheckPrecision::reducedTolerance() ) && err->neighbors() == neighbors();
    }

    bool closeMatch( QgsGeometryCheckError *other ) const
    {
      QgsGeometryGapCheckError* err = dynamic_cast<QgsGeometryGapCheckError*>( other );
      return err && err->neighbors() == neighbors();
    }

    void update( const QgsGeometryCheckError* other )
    {
      QgsGeometryCheckError::update( other );
      // Static cast since this should only get called if isEqual == true
      const QgsGeometryGapCheckError* err = static_cast<const QgsGeometryGapCheckError*>( other );
      delete mGeometry;
      mGeometry = err->mGeometry->clone();
      mNeighbors = err->mNeighbors;
      mGapAreaBBox = err->mGapAreaBBox;
    }

    bool handleChanges( const QgsGeometryCheck::Changes& /*changes*/ )
    {
      return true;
    }

    QgsRectangle affectedAreaBBox() const
    {
      return mGapAreaBBox;
    }

  private:
    QgsFeatureIds mNeighbors;
    QgsRectangle mGapAreaBBox;
    QgsAbstractGeometryV2* mGeometry;
};

class QgsGeometryGapCheck : public QgsGeometryCheck
{
    Q_OBJECT

  public:
    QgsGeometryGapCheck( QgsFeaturePool* featurePool, double threshold )
        : QgsGeometryCheck( LayerCheck, featurePool ), mThreshold( threshold ) {}
    void collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter = 0, const QgsFeatureIds& ids = QgsFeatureIds() ) const;
    void fixError( QgsGeometryCheckError* error, int method, int mergeAttributeIndex, Changes& changes ) const;
    const QStringList& getResolutionMethods() const;
    QString errorDescription() const { return tr( "Gap" ); }
    QString errorName() const { return "QgsGeometryGapCheck"; }

  private:
    enum ResolutionMethod { MergeLongestEdge, NoChange };

    double mThreshold;

    bool mergeWithNeighbor( QgsGeometryGapCheckError *err, Changes &changes , QString &errMsg ) const;
};

#endif // QGS_GEOMETRY_GAP_CHECK_H
