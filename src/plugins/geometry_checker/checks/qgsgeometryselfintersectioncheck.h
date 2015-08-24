/***************************************************************************
 *  qgsgeometryselfintersectioncheck.h                                     *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#ifndef QGS_GEOMETRY_SELFINTERSECTION_CHECK_H
#define QGS_GEOMETRY_SELFINTERSECTION_CHECK_H

#include "qgsgeometryutils.h"
#include "qgsgeometrycheck.h"

class QgsGeometrySelfIntersectionCheckError : public QgsGeometryCheckError
{
  public:
    QgsGeometrySelfIntersectionCheckError( const QgsGeometryCheck* check,
                                           const QgsFeatureId& featureId,
                                           const QgsPointV2& errorLocation,
                                           const QgsVertexId& vidx,
                                           const QgsGeometryUtils::SelfIntersection& inter )
        : QgsGeometryCheckError( check, featureId, errorLocation, vidx ), mInter( inter ) { }
    const QgsGeometryUtils::SelfIntersection& intersection() const { return mInter; }
    bool isEqual( QgsGeometryCheckError* other ) const;
    bool handleChanges( const QgsGeometryCheck::Changes& changes );
    void update( const QgsGeometrySelfIntersectionCheckError* other )
    {
      QgsGeometryCheckError::update( other );
      // Static cast since this should only get called if isEqual == true
      const QgsGeometrySelfIntersectionCheckError* err = static_cast<const QgsGeometrySelfIntersectionCheckError*>( other );
      mInter.point = err->mInter.point;
    }

  private:
    QgsGeometryUtils::SelfIntersection mInter;
};

class QgsGeometrySelfIntersectionCheck : public QgsGeometryCheck
{
    Q_OBJECT

  public:
    QgsGeometrySelfIntersectionCheck( QgsFeaturePool* featurePool )
        : QgsGeometryCheck( FeatureNodeCheck, featurePool ) {}
    void collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter = 0, const QgsFeatureIds& ids = QgsFeatureIds() ) const;
    void fixError( QgsGeometryCheckError* error, int method, int mergeAttributeIndex, Changes& changes ) const;
    const QStringList& getResolutionMethods() const;
    QString errorDescription() const { return tr( "Self intersection" ); }
    QString errorName() const { return "QgsGeometrySelfIntersectionCheck"; }
  private:
    enum ResolutionMethod { ToMultiObject, ToSingleObjects, NoChange };
};

#endif // QGS_GEOMETRY_SELFINTERSECTION_CHECK_H
