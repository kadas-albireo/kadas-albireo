/***************************************************************************
    qgsglobetilesource.cpp
    ---------------------
    begin                : August 2010
    copyright            : (C) 2010 by Pirmin Kalberer
                           (C) 2015 Sandro Mani
    email                : pka at sourcepole dot ch
                           smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgEarth/Registry>
#include <osgEarth/ImageUtils>
#include <unistd.h>

#include "qgscrscache.h"
#include "qgsglobetilesource.h"
#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderercustompainterjob.h"

QgsGlobeTile::QgsGlobeTile( const QgsGlobeTileSource* tileSource, const QgsRectangle& tileExtent, int tileSize )
    : osg::Image()
    , mTileSource( tileSource )
    , mTileExtent( tileExtent )
    , mTileSize( tileSize )
    , mTileData( 0 )
{
  update( 0 );
}

QgsGlobeTile::~QgsGlobeTile()
{
  delete[] mTileData;
}

bool QgsGlobeTile::requiresUpdateCall() const
{
  return mLastUpdateTime < mTileSource->getLastModifiedTime();
}

void QgsGlobeTile::update( osg::NodeVisitor * )
{
  if ( !mTileSource->mViewExtent.intersects( mTileExtent ) )
  {
    // Aka osg::ImageUtils::createEmptyImage
    allocateImage( 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE );
    setInternalTextureFormat( GL_RGBA8 );
    unsigned char *d = data( 0, 0 );
    std::memset( d, 0, 4 );
    return;
  }
  QgsDebugMsg( QString( "Updating earth tile image: %1" ).arg( mTileExtent.toString( 5 ) ) );

  QgsMapSettings settings;
  settings.setBackgroundColor( QColor( 0, 0, 0, 0 ) );
  settings.setDestinationCrs( QgsCRSCache::instance()->crsByAuthId( GEO_EPSG_CRS_AUTHID ) );
  settings.setCrsTransformEnabled( true );
  settings.setExtent( mTileExtent );
  settings.setLayers( mTileSource->layerSet() );
  settings.setFlag( QgsMapSettings::DrawEditingInfo, false );
  settings.setFlag( QgsMapSettings::DrawLabeling, false );
  settings.setFlag( QgsMapSettings::DrawSelection, false );
  settings.setMapUnits( QGis::Degrees );
  settings.setOutputSize( QSize( mTileSize, mTileSize ) );
  settings.setOutputImageFormat( QImage::Format_ARGB32_Premultiplied );

  if ( !mTileData )
  {
    mTileData = new unsigned char[mTileSize * mTileSize * 4];
  }
  std::memset( mTileData, 0, mTileSize * mTileSize * 4 );
  QImage qImage( mTileData, mTileSize, mTileSize, QImage::Format_ARGB32_Premultiplied );
  QPainter painter( &qImage );
  settings.setOutputDpi( qImage.logicalDpiX() );
  QgsMapRendererCustomPainterJob job( settings, &painter );
  job.renderSynchronously();

  setImage( mTileSize, mTileSize, 1, 4, // width, height, depth, internal_format
            GL_BGRA, GL_UNSIGNED_BYTE,
            mTileData, osg::Image::NO_DELETE );
  flipVertical();
  mLastUpdateTime = osgEarth::DateTime().asTimeStamp();
}


QgsGlobeTileSource::QgsGlobeTileSource( QgsMapCanvas* canvas, const osgEarth::TileSourceOptions& options )
    : TileSource( options )
    , mCanvas( canvas )
    , mLastModifiedTime( 0 )
{
}

osgEarth::TileSource::Status QgsGlobeTileSource::initialize( const osgDB::Options* /*dbOptions*/ )
{
  setProfile( osgEarth::Registry::instance()->getGlobalGeodeticProfile() );
  mLastModifiedTime = osgEarth::DateTime().asTimeStamp();
  return STATUS_OK;
}

osg::Image* QgsGlobeTileSource::createImage( const osgEarth::TileKey& key, osgEarth::ProgressCallback* progress )
{
  Q_UNUSED( progress );

  int tileSize = getPixelsPerTile();
  if ( tileSize <= 0 )
  {
    return osgEarth::ImageUtils::createEmptyImage();
  }

  double xmin, ymin, xmax, ymax;
  key.getExtent().getBounds( xmin, ymin, xmax, ymax );
  QgsRectangle tileExtent( xmin, ymin, xmax, ymax );

  QgsDebugMsg( QString( "Create earth tile image: %1" ).arg( tileExtent.toString( 5 ) ) );
  return new QgsGlobeTile( this, tileExtent, getPixelsPerTile() );
}

bool QgsGlobeTileSource::hasDataInExtent( const osgEarth::GeoExtent &extent ) const
{
  osgEarth::Bounds bounds = extent.bounds();
  QgsRectangle requestExtent( bounds.xMin(), bounds.yMin(), bounds.xMax(), bounds.yMax() );
  return requestExtent.intersects( mViewExtent );
}

void QgsGlobeTileSource::refresh()
{
  osgEarth::TimeStamp old = mLastModifiedTime;
  mLastModifiedTime = osgEarth::DateTime().asTimeStamp();
  QgsDebugMsg( QString( "Updated QGIS map layer modified time from %1 to %2" ).arg( old ).arg( mLastModifiedTime ) );

  mViewExtent = mCanvas->fullExtent();
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mCanvas->mapSettings().destinationCrs().authid(), GEO_EPSG_CRS_AUTHID );
  if ( !crst->isShortCircuited() )
  {
    mViewExtent = crst->transform( mViewExtent );
  }
}

void QgsGlobeTileSource::setLayerSet( const QStringList &layerSet )
{
  mLayerSet = layerSet;
}

const QStringList& QgsGlobeTileSource::layerSet() const
{
  return mLayerSet;
}
