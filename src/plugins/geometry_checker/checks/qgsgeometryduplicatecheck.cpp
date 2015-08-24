/***************************************************************************
 *  qgsgeometryduplicatecheck.cpp                                          *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#include "qgsgeometryengine.h"
#include "qgsgeometryduplicatecheck.h"
#include "qgsspatialindex.h"
#include "qgsgeometry.h"
#include "../utils/qgsfeaturepool.h"

void QgsGeometryDuplicateCheck::collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &messages, QAtomicInt* progressCounter , const QgsFeatureIds &ids ) const
{
  const QgsFeatureIds& featureIds = ids.isEmpty() ? mFeaturePool->getFeatureIds() : ids;
  foreach ( const QgsFeatureId& featureid, featureIds )
  {
    if ( progressCounter ) progressCounter->fetchAndAddRelaxed( 1 );
    QgsFeature feature;
    if ( !mFeaturePool->get( featureid, feature ) )
    {
      continue;
    }
    QgsGeometryEngine* geomEngine = QgsGeomUtils::createGeomEngine( feature.geometry()->geometry(), QgsGeometryCheckPrecision::precision() );

    QList<QgsFeatureId> duplicates;
    QgsFeatureIds ids = mFeaturePool->getIntersects( feature.geometry()->geometry()->boundingBox() );
    foreach ( const QgsFeatureId& id, ids )
    {
      // > : only report overlaps once
      if ( id >= featureid )
      {
        continue;
      }
      QgsFeature testFeature;
      if ( !mFeaturePool->get( id, testFeature ) )
      {
        continue;
      }
      QString errMsg;
      QgsAbstractGeometryV2* diffGeom = geomEngine->symDifference( *testFeature.geometry()->geometry(), &errMsg );
      if ( diffGeom && diffGeom->area() < QgsGeometryCheckPrecision::tolerance() )
      {
        duplicates.append( id );
      }
      else if ( !diffGeom )
      {
        messages.append( tr( "Duplicate check between features %1 and %2: %3" ).arg( feature.id() ).arg( testFeature.id() ).arg( errMsg ) );
      }
      delete diffGeom;
    }
    if ( !duplicates.isEmpty() )
    {
      qSort( duplicates );
      errors.append( new QgsGeometryDuplicateCheckError( this, featureid, feature.geometry()->geometry()->centroid(), duplicates ) );
    }
    delete geomEngine;
  }
}

void QgsGeometryDuplicateCheck::fixError( QgsGeometryCheckError* error, int method, int /*mergeAttributeIndex*/, Changes &changes ) const
{
  QgsFeature feature;
  if ( !mFeaturePool->get( error->featureId(), feature ) )
  {
    error->setObsolete();
    return;
  }

  if ( method == NoChange )
  {
    error->setFixed( method );
  }
  else if ( method == RemoveDuplicates )
  {
    QgsGeometryEngine* geomEngine = QgsGeomUtils::createGeomEngine( feature.geometry()->geometry(), QgsGeometryCheckPrecision::precision() );

    QgsGeometryDuplicateCheckError* duplicateError = static_cast<QgsGeometryDuplicateCheckError*>( error );
    foreach ( const QgsFeatureId& id, duplicateError->duplicates() )
    {
      QgsFeature testFeature;
      if ( !mFeaturePool->get( id, testFeature ) )
      {
        continue;
      }
      QgsAbstractGeometryV2* diffGeom = geomEngine->symDifference( *testFeature.geometry()->geometry() );
      if ( diffGeom && diffGeom->area() < QgsGeometryCheckPrecision::tolerance() )
      {
        mFeaturePool->deleteFeature( testFeature );
        changes[id].append( Change( ChangeFeature, ChangeRemoved ) );
      }

      delete diffGeom;
    }
    delete geomEngine;
    error->setFixed( method );
  }
  else
  {
    error->setFixFailed( tr( "Unknown method" ) );
  }
}

const QStringList& QgsGeometryDuplicateCheck::getResolutionMethods() const
{
  static QStringList methods = QStringList()
                               << tr( "No action" )
                               << tr( "Remove duplicates" );
  return methods;
}
