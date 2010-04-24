/***************************************************************************
                          qgsspatialquery.cpp
                             -------------------
    begin                : Dec 29, 2009
    copyright            : (C) 2009 by Diego Moreira And Luiz Motta
    email                : moreira.geo at gmail.com And motta.luiz at gmail.com

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id: $ */

#include <QMessageBox>

#include <qgsvectordataprovider.h>
#include <qgsfeature.h>

#include "qgsgeometrycoordinatetransform.h"
#include "qgsspatialquery.h"

QgsSpatialQuery::QgsSpatialQuery( MngProgressBar *pb )
{
  mPb = pb;
  mUseTargetSelection = mUseReferenceSelection = false;

} // QgsSpatialQuery::QgsSpatialQuery(MngProgressBar *pb)

QgsSpatialQuery::~QgsSpatialQuery()
{
  delete mReaderFeaturesTarget;

} // QgsSpatialQuery::~QgsSpatialQuery()

void QgsSpatialQuery::setSelectedFeaturesTarget( bool useSelected )
{
  mUseTargetSelection = useSelected;

} // void QgsSpatialQuery::setSelectedFeaturesTarget(bool useSelected)

void QgsSpatialQuery::setSelectedFeaturesReference( bool useSelected )
{
  mUseReferenceSelection = useSelected;

} // void QgsSpatialQuery::setSelectedFeaturesReference(bool useSelected)

void QgsSpatialQuery::runQuery( QSet<int> & qsetIndexResult, int relation, QgsVectorLayer* lyrTarget, QgsVectorLayer* lyrReference )
{
  setQuery( lyrTarget, lyrReference );

  // Create Spatial index for Reference - Set mIndexReference
  mPb->setFormat( "Processing 1/2 - %p%" );
  int totalStep = mUseReferenceSelection
                  ? mLayerReference->selectedFeatureCount()
                  : ( int )( mLayerReference->featureCount() );
  mPb->init( 1, totalStep );
  setSpatialIndexReference(); // Need set mLayerReference before

  // Make Query
  mPb->setFormat( "Processing 2/2 - %p%" );
  totalStep = mUseTargetSelection
              ? mLayerTarget->selectedFeatureCount()
              : ( int )( mLayerTarget->featureCount() );
  mPb->init( 1, totalStep );

  execQuery( qsetIndexResult, relation );

} // QSet<int> QgsSpatialQuery::runQuery( int relation)

QMap<QString, int>* QgsSpatialQuery::getTypesOperations( QgsVectorLayer* lyrTarget, QgsVectorLayer* lyrReference )
{
  /* Relations from OGC document (obtain in February 2010)
     06-103r3_Candidate_Implementation_Specification_for_Geographic_Information_-_Simple_feature_access_-_Part_1_Common_Architecture_v1.2.0.pdf

     (P)oint, (L)ine and (A)rea
     Target Geometry(P,L,A) / Reference Geometry (P,L,A)
     dim -> Dimension of geometry
     Relations:
      1) Intersects and Disjoint: All
      2) Touches: P/L P/A L/L L/A A/A
         dimReference  > dimTarget OR dimReference = dimTarget if dimReference > 0
      3) Crosses: P/L P/A L/L L/A
         dimReference  > dimTarget OR dimReference = dimTarget if dimReference = 1
      4) Within: P/L P/A L/A A/A
         dimReference  > dimTarget OR dimReference = dimTarget if dimReference = 2
      5) Equals: P/P L/L A/A
         dimReference = dimTarget
      6) Overlaps: P/P L/L A/A
         dimReference = dimTarget
      7) Contains: L/P A/P A/L A/A
         dimReference  <  dimTarget OR dimReference = dimTarget if dimReference = 2
  */

  QMap<QString, int> * operations = new QMap<QString, int>;
  operations->insert( QObject::tr( "Intersects" ), Intersects );
  operations->insert( QObject::tr( "Disjoint" ), Disjoint );

  short int dimTarget = 0, dimReference = 0;
  dimTarget = dimensionGeometry( lyrTarget->geometryType() );
  dimReference = dimensionGeometry( lyrReference->geometryType() );

  if ( dimReference > dimTarget )
  {
    operations->insert( QObject::tr( "Touches" ), Touches );
    operations->insert( QObject::tr( "Crosses" ), Crosses );
    operations->insert( QObject::tr( "Within" ), Within );
  }
  else if ( dimReference < dimTarget )
  {
    operations->insert( QObject::tr( "Contains" ), Contains );
  }
  else // dimReference == dimTarget
  {
    operations->insert( QObject::tr( "Equals" ), Equals );
    operations->insert( QObject::tr( "Overlaps" ), Overlaps );
    switch ( dimReference )
    {
      case 0:
        break;
      case 1:
        operations->insert( QObject::tr( "Touches" ), Touches );
        operations->insert( QObject::tr( "Crosses" ), Crosses );
        break;
      case 2:
        operations->insert( QObject::tr( "Touches" ), Touches );
        operations->insert( QObject::tr( "Within" ), Within );
        operations->insert( QObject::tr( "Contains" ), Contains );
    }
  }
  return operations;

} // QMap<QString, int>* QgsSpatialQuery::getTypesOperators(QgsVectorLayer* lyrTarget, QgsVectorLayer* lyrReference)

short int QgsSpatialQuery::dimensionGeometry( QGis::GeometryType geomType )
{
  int dimGeom = 0;
  switch ( geomType )
  {
    case QGis::Point:
      dimGeom = 0;
      break;
    case QGis::Line:
      dimGeom = 1;
      break;
    case QGis::Polygon:
      dimGeom = 2;
      break;
    default:
      Q_ASSERT( 0 );
      break;
  }
  return dimGeom;

} // int QgsSpatialQuery::dimensionGeometry(QGis::GeometryType geomType)

void QgsSpatialQuery::setQuery( QgsVectorLayer *layerTarget, QgsVectorLayer *layerReference )
{
  mLayerTarget = layerTarget;
  mReaderFeaturesTarget = new QgsReaderFeatures( mLayerTarget, mUseTargetSelection );
  mLayerReference = layerReference;

} // void QgsSpatialQuery::setQuery (QgsVectorLayer *layerTarget, QgsVectorLayer *layerReference)

bool QgsSpatialQuery::hasValidGeometry( QgsFeature &feature )
{
  if ( ! feature.isValid() )
  {
    return false;
  }

  QgsGeometry *geom = feature.geometry();

  if ( NULL == geom )
  {
    return false;
  }

  GEOSGeometry *geomGeos = geom->asGeos();

  if ( GEOSisEmpty( geomGeos ) || 1 !=  GEOSisValid( geomGeos ) )
  {
    return false;
  }

  return true;

} // bool QgsSpatialQuery::hasValidGeometry(QgsFeature &feature)

void QgsSpatialQuery::setSpatialIndexReference()
{
  QgsReaderFeatures * readerFeaturesReference = new QgsReaderFeatures( mLayerReference, mUseReferenceSelection );
  QgsFeature feature;
  int step = 1;
  while ( true )
  {
    if ( ! readerFeaturesReference->nextFeature( feature ) )
    {
      break;
    }
    mPb->step( step++ );

    if ( ! hasValidGeometry( feature ) )
    {
      continue;
    }

    mIndexReference.insertFeature( feature );
  }
  delete readerFeaturesReference;

} // void QgsSpatialQuery::setSpatialIndexReference()

void QgsSpatialQuery::execQuery( QSet<int> & qsetIndexResult, int relation )
{
  // Set GEOS function
  char( *operation )( const GEOSGeometry *, const GEOSGeometry* );
  switch ( relation )
  {
    case Disjoint:
      operation = &GEOSDisjoint;
      break;
    case Equals:
      operation = &GEOSEquals;
      break;
    case Touches:
      operation = &GEOSTouches;
      break;
    case Overlaps:
      operation = &GEOSOverlaps;
      break;
    case Within:
      operation = &GEOSWithin;
      break;
    case Contains:
      operation = &GEOSContains;
      break;
    case Crosses:
      operation = &GEOSCrosses;
      break;
    case Intersects:
      operation = &GEOSIntersects;
      break;
  }

  // Transform referencer Target = Reference
  QgsGeometryCoordinateTransform *coordinateTransform = new QgsGeometryCoordinateTransform();
  coordinateTransform->setCoordinateTransform( mLayerTarget, mLayerReference );

  // Set function for populate result
  void ( QgsSpatialQuery::* funcPopulateIndexResult )
  ( QSet<int> &, int, QgsGeometry *,
    char( * )( const GEOSGeometry *, const GEOSGeometry * ) );
  funcPopulateIndexResult = ( relation == Disjoint )
                            ? &QgsSpatialQuery::populateIndexResultDisjoint
                            : &QgsSpatialQuery::populateIndexResult;

  QgsFeature featureTarget;
  QgsGeometry * geomTarget;
  int step = 1;
  while ( true )
  {
    if ( ! mReaderFeaturesTarget->nextFeature( featureTarget ) ) break;

    mPb->step( step++ );

    if ( ! hasValidGeometry( featureTarget ) )
    {
      continue;
    }

    geomTarget = featureTarget.geometry();
    coordinateTransform->transform( geomTarget );

    ( this->*funcPopulateIndexResult )( qsetIndexResult, featureTarget.id(), geomTarget, operation );
  }
  delete coordinateTransform;

} // QSet<int> QgsSpatialQuery::execQuery( QSet<int> & qsetIndexResult, int relation)

void QgsSpatialQuery::populateIndexResult(
  QSet<int> &qsetIndexResult, int idTarget, QgsGeometry * geomTarget,
  char( *operation )( const GEOSGeometry *, const GEOSGeometry * ) )
{
  QList<int> listIdReference;
  listIdReference = mIndexReference.intersects( geomTarget->boundingBox() );
  if ( listIdReference.count() == 0 )
  {
    return;
  }
  GEOSGeometry * geosTarget = geomTarget->asGeos();
  QgsFeature featureReference;
  QgsGeometry * geomReference;
  QList<int>::iterator iterIdReference = listIdReference.begin();
  for ( ; iterIdReference != listIdReference.end(); iterIdReference++ )
  {
    mLayerReference->featureAtId( *iterIdReference, featureReference );
    geomReference = featureReference.geometry();
    if (( *operation )( geosTarget, geomReference->asGeos() ) == 1 )
    {
      qsetIndexResult.insert( idTarget );
      break;
    }
  }

} // void QgsSpatialQuery::populateIndexResult(...

void QgsSpatialQuery::populateIndexResultDisjoint(
  QSet<int> &qsetIndexResult, int idTarget, QgsGeometry * geomTarget,
  char( *operation )( const GEOSGeometry *, const GEOSGeometry * ) )
{
  QList<int> listIdReference;
  listIdReference = mIndexReference.intersects( geomTarget->boundingBox() );
  if ( listIdReference.count() == 0 )
  {
    qsetIndexResult.insert( idTarget );
    return;
  }
  GEOSGeometry * geosTarget = geomTarget->asGeos();
  QgsFeature featureReference;
  QgsGeometry * geomReference;
  QList<int>::iterator iterIdReference = listIdReference.begin();
  bool addIndex = true;
  for ( ; iterIdReference != listIdReference.end(); iterIdReference++ )
  {
    mLayerReference->featureAtId( *iterIdReference, featureReference );
    geomReference = featureReference.geometry();
    if (( *operation )( geosTarget, geomReference->asGeos() ) == 0 )
    {
      addIndex = false;
      break;
    }
  }
  if ( addIndex )
  {
    qsetIndexResult.insert( idTarget );
  }

} // void QgsSpatialQuery::populateIndexResultDisjoint( ...

