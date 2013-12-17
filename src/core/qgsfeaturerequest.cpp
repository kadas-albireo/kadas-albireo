/***************************************************************************
    qgsfeaturerequest.cpp
    ---------------------
    begin                : Mai 2012
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
#include "qgsfeaturerequest.h"

#include "qgsfield.h"
#include "qgsgeometry.h"

#include <QStringList>

QgsFeatureRequest::QgsFeatureRequest()
    : mFilter( FilterNone )
    , mFilterExpression( 0 )
    , mFlags( 0 )
    , mMapCoordTransform( NULL )
    , mMapToPixel( NULL )
    , mMapToPixelTol( QGis::DEFAULT_MAPTOPIXEL_THRESHOLD )
{
}

QgsFeatureRequest::QgsFeatureRequest( QgsFeatureId fid )
    : mFilter( FilterFid )
    , mFilterFid( fid )
    , mFilterExpression( 0 )
    , mFlags( 0 )
    , mMapCoordTransform( NULL )
    , mMapToPixel( NULL )
    , mMapToPixelTol( QGis::DEFAULT_MAPTOPIXEL_THRESHOLD )
{
}

QgsFeatureRequest::QgsFeatureRequest( const QgsRectangle& rect )
    : mFilter( FilterRect )
    , mFilterRect( rect )
    , mFilterExpression( 0 )
    , mFlags( 0 )
    , mMapCoordTransform( NULL )
    , mMapToPixel( NULL )
    , mMapToPixelTol( QGis::DEFAULT_MAPTOPIXEL_THRESHOLD )
{
}

QgsFeatureRequest::QgsFeatureRequest( const QgsExpression& expr )
    : mFilter( FilterExpression )
    , mFilterExpression( new QgsExpression( expr.expression() ) )
    , mFlags( 0 )
    , mMapCoordTransform( NULL )
    , mMapToPixel( NULL )
    , mMapToPixelTol( QGis::DEFAULT_MAPTOPIXEL_THRESHOLD )
{
}

QgsFeatureRequest::QgsFeatureRequest( const QgsFeatureRequest &rh )
{
  operator=( rh );
}

QgsFeatureRequest& QgsFeatureRequest::operator=( const QgsFeatureRequest & rh )
{
  mFlags = rh.mFlags;
  mFilter = rh.mFilter;
  mFilterRect = rh.mFilterRect;
  mFilterFid = rh.mFilterFid;
  mFilterFids = rh.mFilterFids;
  if ( rh.mFilterExpression )
  {
    mFilterExpression = new QgsExpression( rh.mFilterExpression->expression() );
  }
  else
  {
    mFilterExpression = 0;
  }
  mAttrs = rh.mAttrs;
  mMapCoordTransform = rh.mMapCoordTransform;
  mMapToPixel = rh.mMapToPixel;
  mMapToPixelTol = rh.mMapToPixelTol;
  return *this;
}

QgsFeatureRequest::~QgsFeatureRequest()
{
  delete mFilterExpression;
}

QgsFeatureRequest& QgsFeatureRequest::setFilterRect( const QgsRectangle& rect )
{
  mFilter = FilterRect;
  mFilterRect = rect;
  return *this;
}

QgsFeatureRequest& QgsFeatureRequest::setFilterFid( QgsFeatureId fid )
{
  mFilter = FilterFid;
  mFilterFid = fid;
  return *this;
}

QgsFeatureRequest&QgsFeatureRequest::setFilterFids( QgsFeatureIds fids )
{
  mFilter = FilterFids;
  mFilterFids = fids;
  return *this;
}

QgsFeatureRequest& QgsFeatureRequest::setFilterExpression( const QString& expression )
{
  mFilter = FilterExpression;
  delete mFilterExpression;
  mFilterExpression = new QgsExpression( expression );
  return *this;
}

QgsFeatureRequest& QgsFeatureRequest::setFlags( QgsFeatureRequest::Flags flags )
{
  mFlags = flags;
  return *this;
}

QgsFeatureRequest& QgsFeatureRequest::setSubsetOfAttributes( const QgsAttributeList& attrs )
{
  mFlags |= SubsetOfAttributes;
  mAttrs = attrs;
  return *this;
}


QgsFeatureRequest& QgsFeatureRequest::setSubsetOfAttributes( const QStringList& attrNames, const QgsFields& fields )
{
  mFlags |= SubsetOfAttributes;
  mAttrs.clear();

  for ( int idx = 0; idx < fields.count(); ++idx )
  {
    if ( attrNames.contains( fields[idx].name() ) )
      mAttrs.append( idx );
  }

  return *this;
}

bool QgsFeatureRequest::acceptFeature( const QgsFeature& feature )
{
  switch ( mFilter )
  {
    case QgsFeatureRequest::FilterNone:
      return true;
      break;

    case QgsFeatureRequest::FilterRect:
      if ( feature.geometry() && feature.geometry()->intersects( mFilterRect ) )
        return true;
      else
        return false;
      break;

    case QgsFeatureRequest::FilterFid:
      if ( feature.id() == mFilterFid )
        return true;
      else
        return false;
      break;

    case QgsFeatureRequest::FilterExpression:
      if ( mFilterExpression->evaluate( feature ).toBool() )
        return true;
      else
        return false;
      break;

    case QgsFeatureRequest::FilterFids:
      if ( mFilterFids.contains( feature.id() ) )
        return true;
      else
        return false;
      break;
  }

  return true;
}

QgsFeatureRequest& QgsFeatureRequest::setCoordinateTransform( const QgsCoordinateTransform* ct )
{
  mMapCoordTransform = ct;
  return *this;
}

QgsFeatureRequest& QgsFeatureRequest::setMapToPixel( const QgsMapToPixel* mtp )
{
  mMapToPixel = mtp;
  return *this;
}

QgsFeatureRequest& QgsFeatureRequest::setMapToPixelTol( float map2pixelTol )
{
  mMapToPixelTol = map2pixelTol;
  return *this;
}
