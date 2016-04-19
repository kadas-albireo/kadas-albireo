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
#include <QImage>
#include <QStringList>
#include <QLabel>
#include <QMutex>
#include "qgsrectangle.h"

//#define GLOBE_SHOW_TILE_STATS

class QgsCoordinateTransform;
class QgsMapCanvas;
class QgsMapRenderer;
class QgsMapSettings;
class QgsGlobeTileSource;
class QgsMapRendererParallelJob;

class QgsGlobeTileStatistics : public QObject
{
    Q_OBJECT
  public:
    QgsGlobeTileStatistics();
    ~QgsGlobeTileStatistics() { s_instance = 0; }
    static QgsGlobeTileStatistics* instance() { return s_instance; }
    void updateTileCount( int change );
    void updateQueueTileCount( int change );
  signals:
    void changed( int queued, int tot );
  private:
    static QgsGlobeTileStatistics* s_instance;
    QMutex mMutex;
    int mTileCount;
    int mQueueTileCount;
};

int getTileCount();

class QgsGlobeTileImage : public osg::Image
{
  public:
    QgsGlobeTileImage( QgsGlobeTileSource* tileSource, const QgsRectangle& tileExtent, int tileSize, int tileLod );
    ~QgsGlobeTileImage();
    bool requiresUpdateCall() const;
    QgsMapSettings createSettings( int dpi, const QStringList &layerSet ) const;
    void setUpdatedImage( const QImage& image ) { mUpdatedImage = image; }
    int dpi() const { return mDpi; }

    void update( osg::NodeVisitor * );

    static bool lodSort( const QgsGlobeTileImage* lhs, const QgsGlobeTileImage* rhs ) { return lhs->mLod > rhs->mLod; }

  private:
    QgsGlobeTileSource* mTileSource;
    QgsRectangle mTileExtent;
    mutable osgEarth::TimeStamp mLastUpdateTime;
    int mTileSize;
    unsigned char* mTileData;
    mutable bool mImageUpdatePending;
    int mLod;
    int mDpi;
    QImage mUpdatedImage;
};

class QgsGlobeTileUpdateManager : public QObject
{
    Q_OBJECT
  public:
    QgsGlobeTileUpdateManager( QObject* parent = 0 );
    ~QgsGlobeTileUpdateManager();
    void updateLayerSet( const QStringList& layerSet ) { mLayerSet = layerSet; }
    void addTile( QgsGlobeTileImage* tile );
    void removeTile( QgsGlobeTileImage* tile );

  signals:
    void startRendering();
    void cancelRendering();

  private:
    QStringList mLayerSet;
    QList<QgsGlobeTileImage*> mTileQueue;
    QgsGlobeTileImage* mCurrentTile;
    QgsMapRendererParallelJob* mRenderer;

  private slots:
    void start();
    void cancel();
    void renderingFinished();
};

class QgsGlobeTileSource : public osgEarth::TileSource
{
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
    friend class QgsGlobeTileImage;

    QgsMapCanvas* mCanvas;
    osgEarth::TimeStamp mLastModifiedTime;
    QgsRectangle mViewExtent;
    QgsRectangle mLastUpdateExtent;
    QStringList mLayerSet;
    QgsGlobeTileUpdateManager mTileUpdateManager;
};

#endif // QGSGLOBETILESOURCE_H
