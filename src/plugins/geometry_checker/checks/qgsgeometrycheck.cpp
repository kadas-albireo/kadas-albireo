/***************************************************************************
 *  qgsgeometrycheck.h                                                     *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#include "qgsgeometrycollectionv2.h"
#include "qgscurvepolygonv2.h"
#include "qgsgeometrycheck.h"
#include "../utils/qgsfeaturepool.h"

QgsGeometryCheckPrecision::QgsGeometryCheckPrecision()
{
  mPrecision = 10;
  mReducedPrecision = 6;
}

QgsGeometryCheckPrecision* QgsGeometryCheckPrecision::get()
{
  static QgsGeometryCheckPrecision instance;
  return &instance;
}

void QgsGeometryCheckPrecision::setPrecision( int tolerance )
{
  get()->mPrecision = tolerance;
  get()->mReducedPrecision = tolerance / 2;
}

int QgsGeometryCheckPrecision::precision()
{
  return get()->mPrecision;
}

int QgsGeometryCheckPrecision::reducedPrecision()
{
  return get()->mReducedPrecision;
}

double QgsGeometryCheckPrecision::tolerance()
{
  return qPow( 10, -get()->mPrecision );
}

double QgsGeometryCheckPrecision::reducedTolerance()
{
  return qPow( 10, -get()->mReducedPrecision );
}

QgsGeometryCheckError::QgsGeometryCheckError( const QgsGeometryCheck* check,
    const QgsFeatureId& featureId,
    const QgsPointV2& errorLocation,
    const QgsVertexId& vidx,
    const QVariant& value , ValueType valueType )
    : mCheck( check ),
    mFeatureId( featureId ),
    mErrorLocation( errorLocation ),
    mVidx( vidx ),
    mValue( value ),
    mValueType( valueType ),
    mStatus( StatusPending )
{}

QgsGeometryCheckError::~QgsGeometryCheckError()
{
}

QgsAbstractGeometryV2 *QgsGeometryCheckError::geometry()
{
  QgsFeature f;
  if ( mCheck->getFeaturePool()->get( featureId(), f ) && f.geometry() )
    return f.geometry()->geometry()->clone();
  return 0;
}

bool QgsGeometryCheckError::handleChanges( const QgsGeometryCheck::Changes& changes )
{
  if ( status() == StatusObsolete )
  {
    return false;
  }

  foreach ( const QgsGeometryCheck::Change& change, changes.value( featureId() ) )
  {
    if ( change.what == QgsGeometryCheck::ChangeFeature )
    {
      if ( change.type == QgsGeometryCheck::ChangeRemoved )
      {
        return false;
      }
      else if ( change.type == QgsGeometryCheck::ChangeChanged )
      {
        // If the check is checking the feature at geometry nodes level, the
        // error almost certainly invalid after a geometry change. In the other
        // cases, it might likely still be valid.
        return mCheck->getCheckType() == QgsGeometryCheck::FeatureNodeCheck ? false : true;
      }
    }
    else if ( change.what == QgsGeometryCheck::ChangePart )
    {
      if ( change.type == QgsGeometryCheck::ChangeChanged || mVidx.part == change.vidx.part )
      {
        return false;
      }
      else if ( mVidx.part > change.vidx.part )
      {
        mVidx.part += change.type == QgsGeometryCheck::ChangeAdded ? 1 : -1;
      }
    }
    else if ( change.what == QgsGeometryCheck::ChangeRing )
    {
      if ( mVidx.partEqual( change.vidx ) )
      {
        if ( change.type == QgsGeometryCheck::ChangeChanged || mVidx.ring == change.vidx.ring )
        {
          return false;
        }
        else if ( mVidx.ring > change.vidx.ring )
        {
          mVidx.ring += change.type == QgsGeometryCheck::ChangeAdded ? 1 : -1;
        }
      }
    }
    else if ( change.what == QgsGeometryCheck::ChangeNode )
    {
      if ( mVidx.ringEqual( change.vidx ) )
      {
        if ( mVidx.vertex == change.vidx.vertex )
        {
          return false;
        }
        else if ( mVidx.vertex > change.vidx.vertex )
        {
          mVidx.vertex += change.type == QgsGeometryCheck::ChangeAdded ? 1 : -1;
        }
      }
    }
  }
  return true;
}

void QgsGeometryCheck::replaceFeatureGeometryPart( QgsFeature& feature, int partIdx, QgsAbstractGeometryV2* newPartGeom, Changes& changes ) const
{
  QgsAbstractGeometryV2* geom = feature.geometry()->geometry();
  if ( dynamic_cast<QgsGeometryCollectionV2*>( geom ) )
  {
    QgsGeometryCollectionV2* GeomCollection = static_cast<QgsGeometryCollectionV2*>( geom );
    GeomCollection->removeGeometry( partIdx );
    GeomCollection->addGeometry( newPartGeom );
    changes[feature.id()].append( Change( ChangeFeature, ChangeRemoved, QgsVertexId( partIdx ) ) );
    changes[feature.id()].append( Change( ChangeFeature, ChangeAdded, QgsVertexId( GeomCollection->partCount() - 1 ) ) );
  }
  else
  {
    feature.setGeometry( new QgsGeometry( newPartGeom ) );
    changes[feature.id()].append( Change( ChangeFeature, ChangeChanged ) );
  }
  mFeaturePool->updateFeature( feature );
}

void QgsGeometryCheck::deleteFeatureGeometryPart( QgsFeature &feature, int partIdx, Changes &changes ) const
{
  QgsAbstractGeometryV2* geom = feature.geometry()->geometry();
  if ( dynamic_cast<QgsGeometryCollectionV2*>( geom ) )
  {
    static_cast<QgsGeometryCollectionV2*>( geom )->removeGeometry( partIdx );
    if ( static_cast<QgsGeometryCollectionV2*>( geom )->numGeometries() == 0 )
    {
      mFeaturePool->deleteFeature( feature );
      changes[feature.id()].append( Change( ChangeFeature, ChangeRemoved ) );
    }
    else
    {
      mFeaturePool->updateFeature( feature );
      changes[feature.id()].append( Change( ChangePart, ChangeRemoved, QgsVertexId( partIdx ) ) );
    }
  }
  else
  {
    mFeaturePool->deleteFeature( feature );
    changes[feature.id()].append( Change( ChangeFeature, ChangeRemoved ) );
  }
}

void QgsGeometryCheck::deleteFeatureGeometryRing( QgsFeature &feature, int partIdx, int ringIdx, Changes &changes ) const
{
  QgsAbstractGeometryV2* partGeom = QgsGeomUtils::getGeomPart( feature.geometry()->geometry(), partIdx );
  if ( dynamic_cast<QgsCurvePolygonV2*>( partGeom ) )
  {
    // If we delete the exterior ring of a polygon, it makes no sense to keep the interiors
    if ( ringIdx == 0 )
    {
      deleteFeatureGeometryPart( feature, partIdx, changes );
    }
    else
    {
      static_cast<QgsCurvePolygonV2*>( partGeom )->removeInteriorRing( ringIdx - 1 );
      mFeaturePool->updateFeature( feature );
      changes[feature.id()].append( Change( ChangeRing, ChangeRemoved, QgsVertexId( partIdx, ringIdx ) ) );
    }
  }
  // Other geometry types do not have rings, remove the entire part
  else
  {
    deleteFeatureGeometryPart( feature, partIdx, changes );
  }
}
