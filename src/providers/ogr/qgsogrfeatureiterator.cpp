/***************************************************************************
    qgsogrfeatureiterator.cpp
    ---------------------
    begin                : Juli 2012
    copyright            : (C) 2012 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsogrfeatureiterator.h"

#include "qgsogrprovider.h"
#include "qgsogrgeometrysimplifier.h"

#include "qgsapplication.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"

#include <QTextCodec>
#include <QFile>

// using from provider:
// - setRelevantFields(), mRelevantFieldsForNextFeature
// - ogrLayer
// - mFetchFeaturesWithoutGeom
// - mAttributeFields
// - mEncoding


QgsOgrFeatureIterator::QgsOgrFeatureIterator( QgsOgrFeatureSource* source, bool ownSource, const QgsFeatureRequest& request )
    : QgsAbstractFeatureIteratorFromSource<QgsOgrFeatureSource>( source, ownSource, request )
    , ogrLayer( 0 )
    , mSubsetStringSet( false )
    , mGeometrySimplifier( NULL )
{
  mFeatureFetched = false;

  mConn = QgsOgrConnPool::instance()->acquireConnection( mSource->mDataSource );

  if ( mSource->mLayerName.isNull() )
  {
    ogrLayer = OGR_DS_GetLayer( mConn->ds, mSource->mLayerIndex );
  }
  else
  {
    ogrLayer = OGR_DS_GetLayerByName( mConn->ds, TO8( mSource->mLayerName ) );
  }

  if ( !mSource->mSubsetString.isEmpty() )
  {
    ogrLayer = QgsOgrUtils::setSubsetString( ogrLayer, mConn->ds, mSource->mEncoding, mSource->mSubsetString );
    mSubsetStringSet = true;
  }

  mFetchGeometry = ( mRequest.filterType() == QgsFeatureRequest::FilterRect ) || !( mRequest.flags() & QgsFeatureRequest::NoGeometry );
  QgsAttributeList attrs = ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes ) ? mRequest.subsetOfAttributes() : mSource->mFields.allAttributesList();

  // make sure we fetch just relevant fields
  // unless it's a VRT data source filtered by geometry as we don't know which
  // attributes make up the geometry and OGR won't fetch them to evaluate the
  // filter if we choose to ignore them (fixes #11223)
  if (( mSource->mDriverName != "VRT" && mSource->mDriverName != "OGR_VRT" ) || mRequest.filterType() != QgsFeatureRequest::FilterRect )
  {
    QgsOgrUtils::setRelevantFields( ogrLayer, mSource->mFields.count(), mFetchGeometry, attrs );
  }

  // spatial query to select features
  if ( mRequest.filterType() == QgsFeatureRequest::FilterRect )
  {
    OGRGeometryH filter = 0;
    QString wktExtent = QString( "POLYGON((%1))" ).arg( mRequest.filterRect().asPolygon() );
    QByteArray ba = wktExtent.toAscii();
    const char *wktText = ba;

    OGR_G_CreateFromWkt(( char ** )&wktText, NULL, &filter );
    QgsDebugMsg( "Setting spatial filter using " + wktExtent );
    OGR_L_SetSpatialFilter( ogrLayer, filter );
    OGR_G_DestroyGeometry( filter );
  }
  else
  {
    OGR_L_SetSpatialFilter( ogrLayer, 0 );
  }

  //start with first feature
  rewind();
}

QgsOgrFeatureIterator::~QgsOgrFeatureIterator()
{
  delete mGeometrySimplifier;
  mGeometrySimplifier = NULL;

  close();
}

bool QgsOgrFeatureIterator::prepareSimplification( const QgsSimplifyMethod& simplifyMethod )
{
  delete mGeometrySimplifier;
  mGeometrySimplifier = NULL;

  // setup simplification of OGR-geometries fetched
  if ( !( mRequest.flags() & QgsFeatureRequest::NoGeometry ) && simplifyMethod.methodType() != QgsSimplifyMethod::NoSimplification && !simplifyMethod.forceLocalOptimization() )
  {
    QgsSimplifyMethod::MethodType methodType = simplifyMethod.methodType();
    Q_UNUSED( methodType );

#if defined(GDAL_VERSION_NUM) && defined(GDAL_COMPUTE_VERSION)
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(1,11,0)
    if ( methodType == QgsSimplifyMethod::OptimizeForRendering )
    {
      int simplifyFlags = QgsMapToPixelSimplifier::SimplifyGeometry | QgsMapToPixelSimplifier::SimplifyEnvelope;
      mGeometrySimplifier = new QgsOgrMapToPixelSimplifier( simplifyFlags, simplifyMethod.tolerance() );
      return true;
    }
#endif
#endif
#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_NUM >= 1900
    if ( methodType == QgsSimplifyMethod::PreserveTopology )
    {
      mGeometrySimplifier = new QgsOgrTopologyPreservingSimplifier( simplifyMethod.tolerance() );
      return true;
    }
#endif

    QgsDebugMsg( QString( "Simplification method type (%1) is not recognised by OgrFeatureIterator class" ).arg( methodType ) );
  }
  return QgsAbstractFeatureIterator::prepareSimplification( simplifyMethod );
}

bool QgsOgrFeatureIterator::providerCanSimplify( QgsSimplifyMethod::MethodType methodType ) const
{
#if defined(GDAL_VERSION_NUM) && defined(GDAL_COMPUTE_VERSION)
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(1,11,0)
  if ( methodType == QgsSimplifyMethod::OptimizeForRendering )
  {
    return true;
  }
#endif
#endif
#if defined(GDAL_VERSION_NUM) && GDAL_VERSION_NUM >= 1900
  if ( methodType == QgsSimplifyMethod::PreserveTopology )
  {
    return true;
  }
#endif

  return false;
}

bool QgsOgrFeatureIterator::fetchFeature( QgsFeature& feature )
{
  feature.setValid( false );

  if ( mClosed )
    return false;

  if ( mRequest.filterType() == QgsFeatureRequest::FilterFid )
  {
    OGRFeatureH fet = OGR_L_GetFeature( ogrLayer, FID_TO_NUMBER( mRequest.filterFid() ) );
    if ( !fet )
    {
      close();
      return false;
    }

    if ( readFeature( fet, feature ) )
      OGR_F_Destroy( fet );

    feature.setValid( true );
    close(); // the feature has been read: we have finished here
    return true;
  }

  OGRFeatureH fet;

  while (( fet = OGR_L_GetNextFeature( ogrLayer ) ) )
  {
    if ( !readFeature( fet, feature ) )
      continue;

    // we have a feature, end this cycle
    feature.setValid( true );
    OGR_F_Destroy( fet );
    return true;

  } // while

  close();
  return false;
}


bool QgsOgrFeatureIterator::rewind()
{
  if ( mClosed )
    return false;

  OGR_L_ResetReading( ogrLayer );

  return true;
}


bool QgsOgrFeatureIterator::close()
{
  if ( mClosed )
    return false;

  iteratorClosed();

  if ( mSubsetStringSet )
  {
    OGR_DS_ReleaseResultSet( mConn->ds, ogrLayer );
  }

  QgsOgrConnPool::instance()->releaseConnection( mConn );
  mConn = 0;

  mClosed = true;
  return true;
}


void QgsOgrFeatureIterator::getFeatureAttribute( OGRFeatureH ogrFet, QgsFeature & f, int attindex )
{
  OGRFieldDefnH fldDef = OGR_F_GetFieldDefnRef( ogrFet, attindex );

  if ( ! fldDef )
  {
    QgsDebugMsg( "ogrFet->GetFieldDefnRef(attindex) returns NULL" );
    return;
  }

  QVariant value;

  if ( OGR_F_IsFieldSet( ogrFet, attindex ) )
  {
    switch ( mSource->mFields[attindex].type() )
    {
      case QVariant::String: value = QVariant( mSource->mEncoding->toUnicode( OGR_F_GetFieldAsString( ogrFet, attindex ) ) ); break;
      case QVariant::Int: value = QVariant( OGR_F_GetFieldAsInteger( ogrFet, attindex ) ); break;
      case QVariant::Double: value = QVariant( OGR_F_GetFieldAsDouble( ogrFet, attindex ) ); break;
      case QVariant::Date:
      case QVariant::DateTime:
      {
        int year, month, day, hour, minute, second, tzf;

        OGR_F_GetFieldAsDateTime( ogrFet, attindex, &year, &month, &day, &hour, &minute, &second, &tzf );
        if ( mSource->mFields[attindex].type() == QVariant::Date )
          value = QDate( year, month, day );
        else
          value = QDateTime( QDate( year, month, day ), QTime( hour, minute, second ) );
      }
      break;
      default:
        assert( 0 && "unsupported field type" );
    }
  }
  else
  {
    value = QVariant( QString::null );
  }

  f.setAttribute( attindex, value );
}


bool QgsOgrFeatureIterator::readFeature( OGRFeatureH fet, QgsFeature& feature )
{
  feature.setFeatureId( OGR_F_GetFID( fet ) );
  feature.initAttributes( mSource->mFields.count() );
  feature.setFields( &mSource->mFields ); // allow name-based attribute lookups

  bool useIntersect = mRequest.flags() & QgsFeatureRequest::ExactIntersect;
  bool geometryTypeFilter = mSource->mOgrGeometryTypeFilter != wkbUnknown;
  if ( mFetchGeometry || useIntersect || geometryTypeFilter )
  {
    OGRGeometryH geom = OGR_F_GetGeometryRef( fet );

    if ( geom )
    {
      if ( mGeometrySimplifier )
        mGeometrySimplifier->simplifyGeometry( geom );

      // get the wkb representation
      int memorySize = OGR_G_WkbSize( geom );
      unsigned char *wkb = new unsigned char[memorySize];
      OGR_G_ExportToWkb( geom, ( OGRwkbByteOrder ) QgsApplication::endian(), wkb );

      // Read original geometry type
      uint32_t origGeomType;
      memcpy( &origGeomType, wkb + 1, sizeof( uint32_t ) );
      bool hasZ = ( origGeomType >= 1000 && origGeomType < 2000 ) || ( origGeomType >= 3000 && origGeomType < 4000 );
      bool hasM = ( origGeomType >= 2000 && origGeomType < 3000 ) || ( origGeomType >= 3000 && origGeomType < 4000 );

      // Triangles and TINs are not supported, convert triangles to polygons and TINs to multipolygons
      if ( origGeomType % 1000 == 16 ) // is TIN, TINZ, TINM or TINZM
      {
        // TIN has the same wkb layout as a multipolygon, just need to overwrite the geom types...
        int nDims = 2 + hasZ + hasM;
        uint32_t newMultiType = static_cast<uint32_t>( QgsWKBTypes::zmType( QgsWKBTypes::MultiPolygon, hasZ, hasM ) );
        uint32_t newSingleType = static_cast<uint32_t>( QgsWKBTypes::zmType( QgsWKBTypes::Polygon, hasZ, hasM ) );
        unsigned char *wkbptr = wkb;

        // Endianness
        wkbptr += 1;

        // Overwrite geom type
        memcpy( wkbptr, &newMultiType, sizeof( uint32_t ) );
        wkbptr += 4;

        // Geom count
        uint32_t numGeoms;
        memcpy( &numGeoms, wkb + 5, sizeof( uint32_t ) );
        wkbptr += 4;

        // For each part, overwrite the geometry type to polygon (Z|M)
        for ( uint32_t i = 0; i < numGeoms; ++i )
        {
          // Endianness
          wkbptr += 1;

          // Overwrite geom type
          memcpy( wkbptr, &newSingleType, sizeof( uint32_t ) );
          wkbptr += sizeof( uint32_t );

          // skip coordinates
          uint32_t nRings;
          memcpy( &nRings, wkbptr, sizeof( uint32_t ) );
          wkbptr += sizeof( uint32_t );

          for ( uint32_t j = 0; j < nRings; ++j )
          {
            uint32_t nPoints;
            memcpy( &nPoints, wkbptr, sizeof( uint32_t ) );
            wkbptr += sizeof( uint32_t ) + sizeof( double ) * nDims * nPoints;
          }
        }
      }
      else if ( origGeomType % 1000 == 17 ) // Triangle, TriangleZ, TriangleM or TriangleZM
      {
        // Triangle has the same wkb layout as a triangle, just need to overwrite the geom type...
        uint32_t newType = static_cast<uint32_t>( QgsWKBTypes::zmType( QgsWKBTypes::Polygon, hasZ, hasM ) );
        // Overwrite geom type
        memcpy( wkb + 1, &newType, sizeof( uint32_t ) );
      }
      else if ( origGeomType % 1000 == 15 ) // PolyhedralSurface, PolyhedralSurfaceZ, PolyhedralSurfaceM or PolyhedralSurfaceZM
      {
        // PolyhedralSurface has the same wkb layout as a MultiPolygon, just need to overwrite the geom type...
        uint32_t newType = static_cast<uint32_t>( QgsWKBTypes::zmType( QgsWKBTypes::MultiPolygon, hasZ, hasM ) );
        // Overwrite geom type
        memcpy( wkb + 1, &newType, sizeof( uint32_t ) );
      }

      QgsGeometry* geometry = feature.geometry();
      if ( !geometry ) feature.setGeometryAndOwnership( wkb, memorySize ); else geometry->fromWkb( wkb, memorySize );
    }
    else
      feature.setGeometry( 0 );

    if (( useIntersect && ( !feature.geometry() || !feature.geometry()->intersects( mRequest.filterRect() ) ) )
        || ( geometryTypeFilter && ( !feature.geometry() || QgsOgrProvider::ogrWkbSingleFlatten(( OGRwkbGeometryType )feature.geometry()->wkbType() ) != mSource->mOgrGeometryTypeFilter ) ) )
    {
      OGR_F_Destroy( fet );
      return false;
    }
  }

  if ( !mFetchGeometry )
  {
    feature.setGeometry( 0 );
  }

  // fetch attributes
  if ( mRequest.flags() & QgsFeatureRequest::SubsetOfAttributes )
  {
    const QgsAttributeList& attrs = mRequest.subsetOfAttributes();
    for ( QgsAttributeList::const_iterator it = attrs.begin(); it != attrs.end(); ++it )
    {
      getFeatureAttribute( fet, feature, *it );
    }
  }
  else
  {
    // all attributes
    for ( int idx = 0; idx < mSource->mFields.count(); ++idx )
    {
      getFeatureAttribute( fet, feature, idx );
    }
  }

  return true;
}


QgsOgrFeatureSource::QgsOgrFeatureSource( const QgsOgrProvider* p )
{
  mDataSource = p->dataSourceUri();
  mLayerName = p->layerName();
  mLayerIndex = p->layerIndex();
  mSubsetString = p->mSubsetString;
  mEncoding = p->mEncoding; // no copying - this is a borrowed pointer from Qt
  mFields = p->mAttributeFields;
  mDriverName = p->ogrDriverName;
  mOgrGeometryTypeFilter = wkbFlatten( p->mOgrGeometryTypeFilter );
  QgsOgrConnPool::instance()->ref( mDataSource );
}

QgsOgrFeatureSource::~QgsOgrFeatureSource()
{
  QgsOgrConnPool::instance()->unref( mDataSource );
}

QgsFeatureIterator QgsOgrFeatureSource::getFeatures( const QgsFeatureRequest& request )
{
  return QgsFeatureIterator( new QgsOgrFeatureIterator( this, false, request ) );
}
