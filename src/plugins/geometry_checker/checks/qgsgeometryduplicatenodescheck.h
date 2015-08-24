/***************************************************************************
 *  qgsgeometryduplicatenodescheck.h                                       *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#ifndef QGS_GEOMETRY_DUPLICATENODES_CHECK_H
#define QGS_GEOMETRY_DUPLICATENODES_CHECK_H

#include "qgsgeometrycheck.h"

class QgsGeometryDuplicateNodesCheck : public QgsGeometryCheck
{
    Q_OBJECT

  public:
    QgsGeometryDuplicateNodesCheck( QgsFeaturePool* featurePool )
        : QgsGeometryCheck( FeatureNodeCheck, featurePool ) {}
    void collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter = 0, const QgsFeatureIds& ids = QgsFeatureIds() ) const;
    void fixError( QgsGeometryCheckError* error, int method, int mergeAttributeIndex, Changes& changes ) const;
    const QStringList& getResolutionMethods() const;
    QString errorDescription() const { return tr( "Duplicate node" ); }
    QString errorName() const { return "QgsGeometryDuplicateNodesCheck"; }

  private:
    enum ResolutionMethod { RemoveDuplicates, NoChange };
};

#endif // QGS_GEOMETRY_DUPLICATENODES_CHECK_H
