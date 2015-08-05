/***************************************************************************
 *  qgsvbsloccaldatasearchprovider.cpp                                     *
 *  -------------------                                                    *
 *  begin                : Aug 03, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbslocaldatasearchprovider.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaplayer.h"
#include "qgisinterface.h"
#include "qgslegendinterface.h"
#include "qgsfeaturerequest.h"
#include "qgsvectorlayer.h"
#include "geometry/qgsgeometry.h"
#include <QThread>
#include <QMutexLocker>


QgsVBSLocalDataSearchProvider::QgsVBSLocalDataSearchProvider( QgisInterface *iface )
    : QgsVBSSearchProvider( iface )
{
}

void QgsVBSLocalDataSearchProvider::startSearch( const QString& searchtext, const SearchRegion& searchRegion )
{
  QList<QgsMapLayer*> visibleLayers;
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers() )
  {
    if ( layer->type() == QgsMapLayer::VectorLayer && mIface->legendInterface()-> isLayerVisible( layer ) )
      visibleLayers.append( layer );
  }

  QThread* crawlerThread = new QThread( this );
  mCrawler = new QgsVBSLocalDataSearchCrawler( searchtext, searchRegion, visibleLayers );
  mCrawler->moveToThread( crawlerThread );
  connect( crawlerThread, SIGNAL( started() ), mCrawler, SLOT( run() ) );
  connect( crawlerThread, SIGNAL( finished() ), crawlerThread, SLOT( deleteLater() ) );
  connect( mCrawler, SIGNAL( searchResultFound( QgsVBSSearchProvider::SearchResult ) ), this, SIGNAL( searchResultFound( QgsVBSSearchProvider::SearchResult ) ) );
  connect( mCrawler, SIGNAL( searchFinished() ), this, SIGNAL( searchFinished() ) );
  connect( mCrawler, SIGNAL( searchFinished() ), crawlerThread, SLOT( quit() ) );
  connect( mCrawler, SIGNAL( searchFinished() ), mCrawler, SLOT( deleteLater() ) );
  crawlerThread->start();
}

void QgsVBSLocalDataSearchProvider::cancelSearch()
{
  if ( mCrawler )
    mCrawler->abort();
}

void QgsVBSLocalDataSearchCrawler::run()
{
  QString escapedSearchText = mSearchText;
  escapedSearchText.replace( "'", "\\'" );
  foreach ( QgsMapLayer* layer, mLayers )
  {
    QMutexLocker locker( &mAbortMutex );
    if ( mAborted )
    {
      break;
    }
    locker.unlock();

    QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( layer );
    QgsFeatureRequest req;
    if ( !mSearchRegion.rect.isEmpty() )
    {
      req.setFilterRect( QgsCoordinateTransform( mSearchRegion.crs, layer->crs() ).transform( mSearchRegion.rect ) );
    }
    const QgsFields& fields = vlayer->pendingFields();
    QStringList conditions;
    for ( int idx = 0, nFields = fields.count(); idx < nFields; ++idx )
    {
      conditions.append( QString( "regexp_matchi( \"%1\" ,'%2')" ).arg( fields[idx].name(), escapedSearchText ) );
    }
    req.setFilterExpression( conditions.join( " OR " ) );
    QgsFeatureIterator it = vlayer->getFeatures( req );
    QgsFeature f;
    while ( it.nextFeature( f ) )
    {
      locker.relock();
      if ( mAborted )
      {
        break;
      }
      locker.unlock();

      QgsVBSSearchProvider::SearchResult result;
      result.bbox = f.geometry()->boundingBox();
      result.category = tr( "Local data" );
      result.crs = vlayer->crs();
      QgsGeometry* pt = f.geometry()->pointOnSurface();
      if ( pt )
      {
        result.pos = pt->asPoint();
        delete pt;
      }
      else
      {
        result.pos = result.bbox.center();
      }
      result.text = tr( "Layer %1, feature %2" ).arg( vlayer->name() ).arg( f.id() );
      emit searchResultFound( result );
    }
  }
  emit searchFinished();
}

void QgsVBSLocalDataSearchCrawler::abort()
{
  mAbortMutex.lock();
  mAborted = true;
  mAbortMutex.unlock();
}
