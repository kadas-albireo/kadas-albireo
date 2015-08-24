/***************************************************************************
 *  qgsgeometrytypecheck.h                                                 *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#ifndef QGS_GEOMETRY_TYPE_CHECK_H
#define QGS_GEOMETRY_TYPE_CHECK_H

#include "qgsgeometrycheck.h"

class QgsGeometryTypeCheckError : public QgsGeometryCheckError
{
  public:
    QgsGeometryTypeCheckError( const QgsGeometryCheck* check,
                               const QgsFeatureId& featureId,
                               const QgsPointV2& errorLocation,
                               QgsWKBTypes::Type flatType )
        : QgsGeometryCheckError( check, featureId, errorLocation )
    {
      mTypeName = QgsWKBTypes::displayString( flatType );
    }

    bool isEqual( QgsGeometryCheckError* other ) const
    {
      return QgsGeometryCheckError::isEqual( other ) &&
             mTypeName == static_cast<QgsGeometryTypeCheckError*>( other )->mTypeName;
    }

    virtual QString description() const { return QString( "%1 (%2)" ).arg( mCheck->errorDescription(), mTypeName ); }

  private:
    QString mTypeName;
};

class QgsGeometryTypeCheck : public QgsGeometryCheck
{
    Q_OBJECT

  public:
    QgsGeometryTypeCheck( QgsFeaturePool* featurePool, int allowedTypes )
        : QgsGeometryCheck( FeatureCheck, featurePool ), mAllowedTypes( allowedTypes ) {}
    void collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter = 0, const QgsFeatureIds& ids = QgsFeatureIds() ) const;
    void fixError( QgsGeometryCheckError* error, int method, int mergeAttributeIndex, Changes& changes ) const;
    const QStringList& getResolutionMethods() const;
    QString errorDescription() const { return tr( "Geometry type" ); }
    QString errorName() const { return "QgsGeometryTypeCheck"; }
  private:
    enum ResolutionMethod { Convert, Delete, NoChange };
    int mAllowedTypes;
};

#endif // QGS_GEOMETRY_TYPE_CHECK_H
