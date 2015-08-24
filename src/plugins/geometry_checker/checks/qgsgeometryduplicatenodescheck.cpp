/***************************************************************************
 *  qgsgeometryduplicatenodescheck.cpp                                     *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#include "qgsgeometryduplicatenodescheck.h"
#include "qgsgeometryutils.h"
#include "../utils/qgsfeaturepool.h"

void QgsGeometryDuplicateNodesCheck::collectErrors( QList<QgsGeometryCheckError*>& errors, QStringList &/*messages*/, QAtomicInt* progressCounter , const QgsFeatureIds &ids ) const
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

    QgsAbstractGeometryV2* geom = feature.geometry()->geometry();
    for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
    {
      for ( int iRing = 0, nRings = geom->ringCount( iPart ); iRing < nRings; ++iRing )
      {
        int nVerts = QgsGeomUtils::polyLineSize( geom, iPart, iRing );
        if ( nVerts < 2 )
          continue;
        for ( int iVert = nVerts - 1, jVert = 0; jVert < nVerts; iVert = jVert++ )
        {
          QgsPointV2 pi = geom->vertexAt( QgsVertexId( iPart, iRing, iVert ) );
          QgsPointV2 pj = geom->vertexAt( QgsVertexId( iPart, iRing, jVert ) );
          QgsDebugMsg( QString( "(%1,%2) (%3,%4) d = %5, tol = %6" ).arg( pi.x(), 0, 'f', 6 ).arg( pi.y(), 0, 'f', 6 ).arg( pj.x(), 0, 'f', 6 ).arg( pj.y(), 0, 'f', 6 ).arg( QgsGeometryUtils::sqrDistance2D( pi, pj ) ).arg( QgsGeometryCheckPrecision::tolerance(), 0, 'f', 16 ) );
          if ( QgsGeometryUtils::sqrDistance2D( pi, pj ) < QgsGeometryCheckPrecision::tolerance() * QgsGeometryCheckPrecision::tolerance() )
          {
            QgsDebugMsg( "Duplicate" );
            errors.append( new QgsGeometryCheckError( this, featureid, pj, QgsVertexId( iPart, iRing, jVert ) ) );
          }
        }
      }
    }
  }
}

void QgsGeometryDuplicateNodesCheck::fixError( QgsGeometryCheckError* error, int method, int /*mergeAttributeIndex*/, Changes &changes ) const
{
  QgsFeature feature;
  if ( !mFeaturePool->get( error->featureId(), feature ) )
  {
    error->setObsolete();
    return;
  }
  QgsAbstractGeometryV2* geom = feature.geometry()->geometry();
  const QgsVertexId& vidx = error->vidx();

  // Check if point still exists
  if ( !vidx.isValid( geom ) )
  {
    error->setObsolete();
    return;
  }

  // Check if error still applies
  int nVerts = QgsGeomUtils::polyLineSize( geom, vidx.part, vidx.ring );
  QgsPointV2 pi = geom->vertexAt( QgsVertexId( vidx.part, vidx.ring, ( vidx.vertex + nVerts - 1 ) % nVerts ) );
  QgsPointV2 pj = geom->vertexAt( error->vidx() );
  if ( QgsGeometryUtils::sqrDistance2D( pi, pj ) >= QgsGeometryCheckPrecision::tolerance() * QgsGeometryCheckPrecision::tolerance() )
  {
    error->setObsolete();
    return;
  }

  // Fix error
  if ( method == NoChange )
  {
    error->setFixed( method );
  }
  else if ( method == RemoveDuplicates )
  {
    geom->deleteVertex( error->vidx() );
    if ( QgsGeomUtils::polyLineSize( geom, vidx.part, vidx.ring ) < 3 )
    {
      error->setFixFailed( tr( "Resulting geometry is degenerate" ) );
    }
    else
    {
      mFeaturePool->updateFeature( feature );
      error->setFixed( method );
      changes[error->featureId()].append( Change( ChangeNode, ChangeRemoved, error->vidx() ) );
    }
  }
  else
  {
    error->setFixFailed( tr( "Unknown method" ) );
  }
}

const QStringList& QgsGeometryDuplicateNodesCheck::getResolutionMethods() const
{
  static QStringList methods = QStringList() << tr( "Delete duplicate node" ) << tr( "No action" );
  return methods;
}
