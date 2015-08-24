/***************************************************************************
 *  qgsgeometryduplicatecheck.h                                            *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#ifndef QGS_GEOMETRY_DUPLICATE_CHECK_H
#define QGS_GEOMETRY_DUPLICATE_CHECK_H

#include "qgsgeometrycheck.h"

class QgsGeometryDuplicateCheckError : public QgsGeometryCheckError
{
  public:
    QgsGeometryDuplicateCheckError( const QgsGeometryCheck* check,
                                    const QgsFeatureId& featureId,
                                    const QgsPointV2& errorLocation,
                                    const QList<QgsFeatureId>& duplicates )
        : QgsGeometryCheckError( check, featureId, errorLocation, QgsVertexId(), duplicatesString( duplicates ) ), mDuplicates( duplicates ) { }
    const QList<QgsFeatureId>& duplicates() const { return mDuplicates; }

    bool isEqual( QgsGeometryCheckError* other ) const
    {
      return other->check() == check() &&
             other->featureId() == featureId() &&
             // static_cast: since other->checker() == checker is only true if the types are actually the same
             static_cast<QgsGeometryDuplicateCheckError*>( other )->duplicates() == duplicates();
    }

  private:
    QList<QgsFeatureId> mDuplicates;

    static inline QString duplicatesString( const QList<QgsFeatureId>& duplicates )
    {
      QStringList str;
      foreach ( QgsFeatureId id, duplicates )
      {
        str.append( QString::number( id ) );
      }
      return str.join( ", " );
    }
};

class QgsGeometryDuplicateCheck : public QgsGeometryCheck
{
    Q_OBJECT

  public:
    QgsGeometryDuplicateCheck( QgsFeaturePool* featurePool )
        : QgsGeometryCheck( FeatureCheck, featurePool ) {}
    void collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter = 0, const QgsFeatureIds& ids = QgsFeatureIds() ) const;
    void fixError( QgsGeometryCheckError* error, int method, int mergeAttributeIndex, Changes& changes ) const;
    const QStringList& getResolutionMethods() const;
    QString errorDescription() const { return tr( "Duplicate" ); }
    QString errorName() const { return "QgsGeometryDuplicateCheck"; }

  private:
    enum ResolutionMethod { NoChange, RemoveDuplicates };
};

#endif // QGS_GEOMETRY_DUPLICATE_CHECK_H
