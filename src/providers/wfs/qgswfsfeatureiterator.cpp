/***************************************************************************
    qgswfsfeatureiterator.cpp
    ---------------------
    begin                : Januar 2013
    copyright            : (C) 2013 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgswfsfeatureiterator.h"
#include "qgsspatialindex.h"
#include "qgswfsprovider.h"
#include "qgsmessagelog.h"
#include "qgsgeometry.h"

QgsWFSFeatureIterator::QgsWFSFeatureIterator( QgsWFSProvider* provider, const QgsFeatureRequest& request )
    : QgsAbstractFeatureIterator( request )
    , mProvider( provider )
{
  //select ids
  //get iterator
  if ( !mProvider )
  {
    return;
  }

  mProvider->mActiveIterators << this;

  switch ( request.filterType() )
  {
    case QgsFeatureRequest::FilterRect:
      if ( mProvider->mSpatialIndex )
      {
        mSelectedFeatures = mProvider->mSpatialIndex->intersects( request.filterRect() );
      }
      break;
    case QgsFeatureRequest::FilterFid:
      mSelectedFeatures.push_back( request.filterFid() );
      break;
    case QgsFeatureRequest::FilterNone:
      mSelectedFeatures = mProvider->mFeatures.keys();
    default: //QgsFeatureRequest::FilterNone
      mSelectedFeatures = mProvider->mFeatures.keys();
  }

  mFeatureIterator = mSelectedFeatures.constBegin();
}

QgsWFSFeatureIterator::~QgsWFSFeatureIterator()
{
  close();
}

bool QgsWFSFeatureIterator::fetchFeature( QgsFeature& f )
{
  if ( !mProvider )
  {
    return false;
  }

  if ( mFeatureIterator == mSelectedFeatures.constEnd() )
  {
    return false;
  }

  QgsFeature *fet = 0;

  for ( ;; )
  {
    QMap<QgsFeatureId, QgsFeature* >::iterator it = mProvider->mFeatures.find( *mFeatureIterator );
    if ( it == mProvider->mFeatures.end() )
      return false;

    fet = it.value();
    if (( mRequest.flags() & QgsFeatureRequest::ExactIntersect ) == 0 )
      break;

    if ( fet->geometry() && fet->geometry()->intersects( mRequest.filterRect() ) )
      break;

    ++mFeatureIterator;
  }


  mProvider->copyFeature( fet, f, !( mRequest.flags() & QgsFeatureRequest::NoGeometry ) );
  ++mFeatureIterator;
  return true;
}

bool QgsWFSFeatureIterator::rewind()
{
  if ( !mProvider )
  {
    return false;
  }

  mFeatureIterator = mSelectedFeatures.constBegin();

  return true;
}

bool QgsWFSFeatureIterator::close()
{
  if ( !mProvider )
    return false;

  mProvider->mActiveIterators.remove( this );

  mProvider = 0;
  return true;
}
