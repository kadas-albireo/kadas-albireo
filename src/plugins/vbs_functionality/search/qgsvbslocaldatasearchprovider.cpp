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
#include "qgslinestringv2.h"
#include "qgspolygonv2.h"
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


const int QgsVBSLocalDataSearchCrawler::sResultCountLimit = 50;

void QgsVBSLocalDataSearchCrawler::run()
{
  int resultCount = 0;

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

    const QgsFields& fields = vlayer->pendingFields();
    QStringList conditions;
    for ( int idx = 0, nFields = fields.count(); idx < nFields; ++idx )
    {
      conditions.append( QString( "regexp_matchi( \"%1\" ,'%2')" ).arg( fields[idx].name(), escapedSearchText ) );
    }
    QString exprText = conditions.join( " OR " );

    QgsFeatureRequest req;
    QgsFeature feature;
    if ( !mSearchRegion.polygon.isEmpty() )
    {
      QgsLineStringV2* exterior = new QgsLineStringV2();
      QgsCoordinateTransform ct( mSearchRegion.crs, layer->crs() );
      foreach ( const QgsPoint& p, mSearchRegion.polygon )
      {
        exterior->addVertex( QgsPointV2( ct.transform( p ) ) );
      }
      QgsPolygonV2* poly = new QgsPolygonV2();
      poly->setExteriorRing( exterior );
      QgsGeometry filterGeom( poly );

      req.setFilterRect( filterGeom.boundingBox() );
      QgsExpression expr( exprText );
      expr.prepare( vlayer->pendingFields() );
      QgsFeatureIterator it = vlayer->getFeatures( req );
      while ( it.nextFeature( feature ) && resultCount < sResultCountLimit )
      {
        locker.relock();
        if ( mAborted )
        {
          break;
        }
        locker.unlock();
        if ( expr.evaluate( feature ).toBool() && filterGeom.contains( feature.geometry() ) )
        {
          buildResult( feature, vlayer );
          ++resultCount;
        }
      }
    }
    else
    {
      req.setFilterExpression( exprText );
      QgsFeatureIterator it = vlayer->getFeatures( req );
      while ( it.nextFeature( feature ) && resultCount < sResultCountLimit )
      {
        locker.relock();
        if ( mAborted )
        {
          break;
        }
        locker.unlock();
        buildResult( feature, vlayer );
        ++resultCount;
      }
    }
    if ( resultCount >= sResultCountLimit )
    {
      QgsDebugMsg( "Stopping search due to result count limit hit" );
      break;
    }
  }
  emit searchFinished();
}

void QgsVBSLocalDataSearchCrawler::buildResult( const QgsFeature &feature, QgsVectorLayer* layer )
{
  QgsVBSSearchProvider::SearchResult result;
  result.pos = feature.geometry()->boundingBox().center();
  result.category = tr( "Local data" );
  result.crs = layer->crs();
  result.zoomScale = 1000;
  QgsGeometry* pt = feature.geometry()->pointOnSurface();
  if ( pt )
  {
    result.pos = pt->asPoint();
    delete pt;
  }
  else
  {
    result.pos = result.bbox.center();
  }
  result.text = tr( "%1: Layer %2, feature %3" ).arg( mSearchText ).arg( layer->name() ).arg( feature.id() );
  emit searchResultFound( result );
}

void QgsVBSLocalDataSearchCrawler::abort()
{
  mAbortMutex.lock();
  mAborted = true;
  mAbortMutex.unlock();
}
