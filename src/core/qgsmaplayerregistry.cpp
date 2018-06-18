/***************************************************************************
 *  QgsMapLayerRegistry.cpp  -  Singleton class for tracking mMapLayers.
 *                         -------------------
 * begin                : Sun June 02 2004
 * copyright            : (C) 2004 by Tim Sutton
 * email                : tim@linfiniti.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaplayerregistry.h"
#include "qgsmaplayer.h"
#include "qgsredlininglayer.h"
#include "qgslogger.h"

QgsMapLayerRegistry* QgsMapLayerRegistry::sInstance = 0;

QgsMapLayerRegistry* QgsMapLayerRegistry::instance()
{
  if ( sInstance == 0 )
  {
    sInstance = new QgsMapLayerRegistry();
  }
  return sInstance;
}

void QgsMapLayerRegistry::cleanup()
{
  delete sInstance;
  sInstance = 0;
}

QgsMapLayerRegistry::QgsMapLayerRegistry( QObject *parent ) : QObject( parent )
{
  // constructor does nothing
}

QgsMapLayerRegistry::~QgsMapLayerRegistry()
{
  removeAllMapLayers();
}

// get the layer count (number of registered layers)
int QgsMapLayerRegistry::count()
{
  return mMapLayers.size();
}

QgsMapLayer * QgsMapLayerRegistry::mapLayer( QString theLayerId )
{
  return mMapLayers.value( theLayerId );
}

QList<QgsMapLayer *> QgsMapLayerRegistry::mapLayersByName( QString layerName )
{
  QList<QgsMapLayer *> myResultList;
  foreach ( QgsMapLayer* layer, mMapLayers )
  {
    if ( layer->name() == layerName )
    {
      myResultList << layer;
    }
  }
  return myResultList;
}

//introduced in 1.8
QList<QgsMapLayer *> QgsMapLayerRegistry::addMapLayers(
  QList<QgsMapLayer *> theMapLayers,
  bool addToLegend,
  bool takeOwnership )
{
  QList<QgsMapLayer *> myResultList;
  for ( int i = 0; i < theMapLayers.size(); ++i )
  {
    QgsMapLayer * myLayer = theMapLayers.at( i );
    if ( !myLayer || !myLayer->isValid() )
    {
      QgsDebugMsg( "cannot add invalid layers" );
      continue;
    }
    //check the layer is not already registered!
    if ( !mMapLayers.contains( myLayer->id() ) )
    {
      mMapLayers[myLayer->id()] = myLayer;
      myResultList << mMapLayers[myLayer->id()];
      if ( takeOwnership )
        mOwnedLayers << myLayer;
      emit layerWasAdded( myLayer );
    }
  }
  if ( myResultList.count() > 0 )
  {
    emit layersAdded( myResultList );

    if ( addToLegend )
      emit legendLayersAdded( myResultList );
  }
  return myResultList;
} // QgsMapLayerRegistry::addMapLayers

//this is just a thin wrapper for addMapLayers
QgsMapLayer *
QgsMapLayerRegistry::addMapLayer( QgsMapLayer* theMapLayer,
                                  bool addToLegend,
                                  bool takeOwnership )
{
  QList<QgsMapLayer *> addedLayers;
  addedLayers = addMapLayers( QList<QgsMapLayer*>() << theMapLayer, addToLegend, takeOwnership );
  return addedLayers.isEmpty() ? 0 : addedLayers[0];
}


//introduced in 1.8
void QgsMapLayerRegistry::removeMapLayers( QStringList theLayerIds )
{
  if ( theLayerIds.isEmpty() )
  {
    return;
  }
  emit layersWillBeRemoved( theLayerIds );

  foreach ( const QString &myId, theLayerIds )
  {
    QgsMapLayer* lyr = mMapLayers[myId];
    if ( mOwnedLayers.contains( lyr ) )
    {
      emit layerWillBeRemoved( myId );
      if ( lyr == mRedliningLayer )
      {
        mRedliningLayer = nullptr;
      }
      else if ( lyr == mGpsRoutesLayer )
      {
        mGpsRoutesLayer = nullptr;
      }
      delete lyr;
      mOwnedLayers.remove( lyr );
    }
    mMapLayers.remove( myId );
    emit layerRemoved( myId );
  }
  emit layersRemoved( theLayerIds );
}

void QgsMapLayerRegistry::removeMapLayer( const QString& theLayerId )
{
  removeMapLayers( QStringList( theLayerId ) );
}

void QgsMapLayerRegistry::removeAllMapLayers()
{
  emit removeAll();
  // now let all canvas observers know to clear themselves,
  // and then consequently any of their map legends
  removeMapLayers( mMapLayers.keys() );
  mMapLayers.clear();
  mOwnedLayers.clear();
} // QgsMapLayerRegistry::removeAllMapLayers()

void QgsMapLayerRegistry::clearAllLayerCaches()
{
}

void QgsMapLayerRegistry::reloadAllLayers()
{
  QMap<QString, QgsMapLayer *>::iterator it;
  for ( it = mMapLayers.begin(); it != mMapLayers.end() ; ++it )
  {
    QgsMapLayer* layer = it.value();
    if ( layer )
    {
      layer->reload();
    }
  }
}

const QMap<QString, QgsMapLayer*>& QgsMapLayerRegistry::mapLayers()
{
  return mMapLayers;
}

void QgsMapLayerRegistry::setRedliningLayer( const QString &layerId )
{
  if ( dynamic_cast<QgsRedliningLayer*>( mapLayer( layerId ) ) )
  {
    mRedliningLayer = static_cast<QgsRedliningLayer*>( mapLayer( layerId ) );
    mRedliningLayer->setCustomProperty( "labeling/placement", "0" );
    mRedliningLayer->setCustomProperty( "labeling/obstacle", "false" );
    mRedliningLayer->setCustomProperty( "labeling/dataDefined/PositionX",  "1~~1~~$x~~" );
    mRedliningLayer->setCustomProperty( "labeling/dataDefined/PositionY", "1~~1~~$y~~" );
    mRedliningLayer->setCustomProperty( "labeling/bufferDraw", true );
    mRedliningLayer->setCustomProperty( "labeling/bufferSize", 0.5 );
    mRedliningLayer->setCustomProperty( "labeling/bufferColorA", 127 );
    mRedliningLayer->setCustomProperty( "labeling/bufferColorB", 0 );
    mRedliningLayer->setCustomProperty( "labeling/bufferColorG", 0 );
    mRedliningLayer->setCustomProperty( "labeling/bufferColorR", 0 );
  }
}

QgsRedliningLayer* QgsMapLayerRegistry::getOrCreateRedliningLayer()
{
  if ( !mRedliningLayer )
  {
    QgsRedliningLayer* layer = new QgsRedliningLayer( tr( "Redlining" ) );
    addMapLayer( layer, true, true );
    setRedliningLayer( layer->id() );
  }
  return mRedliningLayer;
}

void QgsMapLayerRegistry::setGpsRoutesLayer( const QString &layerId )
{
  if ( dynamic_cast<QgsRedliningLayer*>( mapLayer( layerId ) ) )
  {
    mGpsRoutesLayer = static_cast<QgsRedliningLayer*>( mapLayer( layerId ) );

    // Labeling tweaks to make both point and line labeling appear more or less sensible
    mGpsRoutesLayer->setCustomProperty( "labeling/placement", 2 );
    mGpsRoutesLayer->setCustomProperty( "labeling/placementFlags", 10 );
    mGpsRoutesLayer->setCustomProperty( "labeling/dist", 2 );
    mGpsRoutesLayer->setCustomProperty( "labeling/distInMapUnits", false );
    mGpsRoutesLayer->setCustomProperty( "labeling/bufferDraw", true );
    mGpsRoutesLayer->setCustomProperty( "labeling/bufferSize", 0.5 );
    mGpsRoutesLayer->setCustomProperty( "labeling/bufferColorA", 127 );
    mGpsRoutesLayer->setCustomProperty( "labeling/bufferColorB", 0 );
    mGpsRoutesLayer->setCustomProperty( "labeling/bufferColorG", 0 );
    mGpsRoutesLayer->setCustomProperty( "labeling/bufferColorR", 0 );
  }
}

QgsRedliningLayer* QgsMapLayerRegistry::getOrCreateGpsRoutesLayer()
{
  if ( !mGpsRoutesLayer )
  {
    QgsRedliningLayer* layer = new QgsRedliningLayer( tr( "GPS Routes" ), "EPSG:4326" );
    addMapLayer( layer, true, true );
    setGpsRoutesLayer( layer->id() );
  }
  return mGpsRoutesLayer;
}
