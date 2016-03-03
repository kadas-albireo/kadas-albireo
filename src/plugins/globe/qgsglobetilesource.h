/***************************************************************************
    qgsglobetilesource.h
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

#ifndef QGSGLOBETILESOURCE_H
#define QGSGLOBETILESOURCE_H

#include <osgEarth/TileSource>
#include <osg/ImageStream>
#include <QStringList>
#include "qgsrectangle.h"

class QgsCoordinateTransform;
class QgsMapCanvas;
class QgsMapRenderer;
class QgsMapSettings;
class QgsGlobeTileSource;
class QgsMapRendererParallelJob;

class QgsGlobeTileImage : public osg::Image
{
  public:
    QgsGlobeTileImage( const QgsGlobeTileSource* tileSource, const QgsRectangle& tileExtent, int tileSize );
    ~QgsGlobeTileImage();
    bool requiresUpdateCall() const;

    void update( osg::NodeVisitor * );

  private:
    const QgsGlobeTileSource* mTileSource;
    QgsRectangle mTileExtent;
    mutable osgEarth::TimeStamp mLastUpdateTime;
    int mTileSize;
    unsigned char* mTileData;
    QgsMapRendererParallelJob* mRenderer;

    QgsMapSettings createSettings( int dpi ) const;
};


class QgsGlobeTileSource : public osgEarth::TileSource
{
    friend class QgsGlobeTileImage;

  public:
    QgsGlobeTileSource( QgsMapCanvas* canvas, const osgEarth::TileSourceOptions& options = osgEarth::TileSourceOptions() );
    Status initialize( const osgDB::Options *dbOptions ) override;
    osg::Image* createImage( const osgEarth::TileKey& key, osgEarth::ProgressCallback* progress );
    osg::HeightField* createHeightField( const osgEarth::TileKey &/*key*/, osgEarth::ProgressCallback* /*progress*/ ) { return 0; }
    bool hasDataInExtent( const osgEarth::GeoExtent &extent ) const override;
    bool hasData( const osgEarth::TileKey& key ) const override;

    bool isDynamic() const { return true; }
    osgEarth::TimeStamp getLastModifiedTime() const { return mLastModifiedTime; }

    void refresh( const QgsRectangle &updateExtent );
    void setLayerSet( const QStringList& layerSet );
    const QStringList &layerSet() const;

  private:
    QgsMapCanvas* mCanvas;
    osgEarth::TimeStamp mLastModifiedTime;
    QgsRectangle mViewExtent;
    QgsRectangle mLastUpdateExtent;
    QStringList mLayerSet;
};

#endif // QGSGLOBETILESOURCE_H
