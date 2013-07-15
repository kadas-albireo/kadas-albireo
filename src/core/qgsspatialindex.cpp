/***************************************************************************
    qgsspatialindex.cpp  - wrapper class for spatial index library
    ----------------------
    begin                : December 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsspatialindex.h"

#include "qgsgeometry.h"
#include "qgsfeature.h"
#include "qgsrectangle.h"
#include "qgslogger.h"

#include "SpatialIndex.h"

using namespace SpatialIndex;


// custom visitor that adds found features to list
class QgisVisitor : public SpatialIndex::IVisitor
{
  public:
    QgisVisitor( QList<QgsFeatureId> & list )
        : mList( list ) {}

    void visitNode( const INode& n )
    { Q_UNUSED( n ); }

    void visitData( const IData& d )
    {
      mList.append( d.getIdentifier() );
    }

    void visitData( std::vector<const IData*>& v )
    { Q_UNUSED( v ); }

  private:
    QList<QgsFeatureId>& mList;
};


QgsSpatialIndex::QgsSpatialIndex()
{
  // for now only memory manager
  mStorageManager = StorageManager::createNewMemoryStorageManager();

  // create buffer

  unsigned int capacity = 10;
  bool writeThrough = false;
  mStorage = StorageManager::createNewRandomEvictionsBuffer( *mStorageManager, capacity, writeThrough );

  // R-Tree parameters
  double fillFactor = 0.7;
  unsigned long indexCapacity = 10;
  unsigned long leafCapacity = 10;
  unsigned long dimension = 2;
  RTree::RTreeVariant variant = RTree::RV_RSTAR;

  // create R-tree
  SpatialIndex::id_type indexId;
  mRTree = RTree::createNewRTree( *mStorage, fillFactor, indexCapacity,
                                  leafCapacity, dimension, variant, indexId );
}

QgsSpatialIndex:: ~QgsSpatialIndex()
{
  delete mRTree;
  delete mStorage;
  delete mStorageManager;
}

Region QgsSpatialIndex::rectToRegion( QgsRectangle rect )
{
  double pt1[2], pt2[2];
  pt1[0] = rect.xMinimum();
  pt1[1] = rect.yMinimum();
  pt2[0] = rect.xMaximum();
  pt2[1] = rect.yMaximum();
  return Region( pt1, pt2, 2 );
}

bool QgsSpatialIndex::featureInfo( QgsFeature& f, SpatialIndex::Region& r, QgsFeatureId &id )
{
  QgsGeometry *g = f.geometry();
  if ( !g )
    return false;

  id = f.id();
  r = rectToRegion( g->boundingBox() );
  return true;
}

bool QgsSpatialIndex::insertFeature( QgsFeature& f )
{
  Region r;
  QgsFeatureId id;
  if ( !featureInfo( f, r, id ) )
    return false;

  // TODO: handle possible exceptions correctly
  try
  {
    mRTree->insertData( 0, 0, r, FID_TO_NUMBER( id ) );
    return true;
  }
  catch ( Tools::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Tools::Exception caught: " ).arg( e.what().c_str() ) );
  }
  catch ( const std::exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "std::exception caught: " ).arg( e.what() ) );
  }
  catch ( ... )
  {
    QgsDebugMsg( "unknown spatial index exception caught" );
  }

  return false;
}

bool QgsSpatialIndex::deleteFeature( QgsFeature& f )
{
  Region r;
  QgsFeatureId id;
  if ( !featureInfo( f, r, id ) )
    return false;

  // TODO: handle exceptions
  return mRTree->deleteData( r, FID_TO_NUMBER( id ) );
}

QList<QgsFeatureId> QgsSpatialIndex::intersects( QgsRectangle rect )
{
  QList<QgsFeatureId> list;
  QgisVisitor visitor( list );

  Region r = rectToRegion( rect );

  mRTree->intersectsWithQuery( r, visitor );

  return list;
}

QList<QgsFeatureId> QgsSpatialIndex::nearestNeighbor( QgsPoint point, int neighbors )
{
  QList<QgsFeatureId> list;
  QgisVisitor visitor( list );

  double pt[2];
  pt[0] = point.x();
  pt[1] = point.y();
  Point p( pt, 2 );

  mRTree->nearestNeighborQuery( neighbors, p, visitor );

  return list;
}
