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
/* $Id$ */

#include <iostream>

#include "qgsmaplayerregistry.h"
#include "qgsmaplayer.h"
#include "qgslogger.h"


//
// Static calls to enforce singleton behaviour
//
QgsMapLayerRegistry *QgsMapLayerRegistry::mInstance = 0;
QgsMapLayerRegistry *QgsMapLayerRegistry::instance()
{
  if ( mInstance == 0 )
  {
    mInstance = new QgsMapLayerRegistry();
  }
  return mInstance;
}

//
// Main class begins now...
//

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



QgsMapLayer *
QgsMapLayerRegistry::addMapLayer( QgsMapLayer * theMapLayer, bool theEmitSignal )
{
  if ( !theMapLayer->isValid() )
  {
    QgsDebugMsg( "cannot add invalid layers" );
    return 0;
  }

  //check the layer is not already registered!
  QMap<QString, QgsMapLayer*>::iterator myIterator = mMapLayers.find( theMapLayer->getLayerID() );
  //if myIterator returns mMapLayers.end() then it does not exist in registry and its safe to add it
  if ( myIterator == mMapLayers.end() )
  {
    mMapLayers[theMapLayer->getLayerID()] = theMapLayer;

    if ( theEmitSignal )
      emit layerWasAdded( theMapLayer );

    return mMapLayers[theMapLayer->getLayerID()];
  }
  else
  {
    return 0;
  }
} //  QgsMapLayerRegistry::addMapLayer



void QgsMapLayerRegistry::removeMapLayer( QString theLayerId, bool theEmitSignal )
{
  if ( theEmitSignal )
    emit layerWillBeRemoved( theLayerId );
  delete mMapLayers[theLayerId];
  mMapLayers.remove( theLayerId );
}

void QgsMapLayerRegistry::removeAllMapLayers()
{

  // moved before physically removing the layers
  emit removedAll();            // now let all canvas Observers know to clear
  // themselves, and then consequently any of
  // their map legends

  QMap<QString, QgsMapLayer *>::iterator it;
  for ( it = mMapLayers.begin(); it != mMapLayers.end() ; ++it )
  {
    emit layerWillBeRemoved( it.key() );
    delete it.value(); // delete the map layer
  }
  mMapLayers.clear();

} // QgsMapLayerRegistry::removeAllMapLayers()


QMap<QString, QgsMapLayer*> & QgsMapLayerRegistry::mapLayers()
{
  return mMapLayers;
}



void QgsMapLayerRegistry::connectNotify( const char * signal )
{
  //QgsDebugMsg("QgsMapLayerRegistry connected to " + QString(signal));
} //  QgsMapLayerRegistry::connectNotify
