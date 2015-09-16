/***************************************************************************
 *  qgsvbsfunctionality.cpp                                                *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
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

#include "qgslegendinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessagebaritem.h"
#include "qgsvbsfunctionality.h"
#include "qgsvbscoordinatedisplayer.h"
#include "qgsvbscrsselection.h"
#include "qgsvbscrashhandler.h"
#include "qgisinterface.h"
#include "multimap/qgsvbsmultimapmanager.h"
#include "pinannotation/qgsvbsmaptoolpinannotation.h"
#include "search/qgsvbssearchbox.h"
#include "vbsfunctionality_plugin.h"
#include <QAction>
#include <QToolBar>

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mCoordinateDisplayer( 0 )
    , mCrsSelection( 0 )
    , mActionPinAnnotation( 0 )
    , mMapToolPinAnnotation( 0 )
    , mSearchToolbar( 0 )
    , mSearchBox( 0 )
    , mReprojMsgItem( 0 )
    , mCrashHandler( 0 )
    , mMultiMapManager( 0 )
{
}

void QgsVBSFunctionality::initGui()
{
  mCoordinateDisplayer = new QgsVBSCoordinateDisplayer( mQGisIface, mQGisIface->mainWindow() );
  mCrsSelection = new QgsVBSCrsSelection( mQGisIface, mQGisIface->mainWindow() );
  mMapToolPinAnnotation = new QgsVBSMapToolPinAnnotation( mQGisIface->mapCanvas(), mCoordinateDisplayer );
  connect( mQGisIface->mapCanvas(), SIGNAL( mapToolSet( QgsMapTool* ) ), this, SLOT( onMapToolSet( QgsMapTool* ) ) );
  mActionPinAnnotation = new QAction( QIcon( ":/vbsfunctionality/icons/pin_red.svg" ), tr( "Add pin" ), this );
  mActionPinAnnotation->setCheckable( true );
  mMapToolPinAnnotation->setAction( mActionPinAnnotation );
  connect( mActionPinAnnotation, SIGNAL( triggered() ), this, SLOT( activateMapToolPinAnnotation() ) );
  mQGisIface->pluginToolBar()->addAction( mActionPinAnnotation );

  mSearchToolbar = mQGisIface->addToolBar( "vbsSearchToolbar" );
  mSearchBox = new QgsVBSSearchBox( mQGisIface, mSearchToolbar );
  mSearchToolbar->addWidget( mSearchBox );

  QWidget* layerTreeToolbar = mQGisIface->mainWindow()->findChild<QWidget*>( "layerTreeToolbar" );
  if ( layerTreeToolbar ) layerTreeToolbar->setVisible( false );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( checkOnTheFlyProjection( QList<QgsMapLayer*> ) ) );
  connect( mQGisIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( checkOnTheFlyProjection() ) );

  mCrashHandler = new QgsVBSCrashHandler();

  mMultiMapManager = new QgsVBSMultiMapManager( mQGisIface, this );
}

void QgsVBSFunctionality::unload()
{
  disconnect( mQGisIface->mapCanvas(), SIGNAL( mapToolSet( QgsMapTool* ) ), this, SLOT( onMapToolSet( QgsMapTool* ) ) );

  delete mCoordinateDisplayer;
  mCoordinateDisplayer = 0;
  delete mCrsSelection;
  mCrsSelection = 0;
  delete mActionPinAnnotation;
  mActionPinAnnotation = 0;
  delete mMapToolPinAnnotation;
  mMapToolPinAnnotation = 0;
  delete mSearchToolbar;
  mSearchToolbar = 0;
  mSearchBox = 0;
  delete mCrashHandler;
  mCrashHandler = 0;
  delete mMultiMapManager;
  mMultiMapManager = 0;

  QWidget* layerTreeToolbar = mQGisIface->mainWindow()->findChild<QWidget*>( "layerTreeToolbar" );
  if ( layerTreeToolbar ) layerTreeToolbar->setVisible( true );
}

void QgsVBSFunctionality::activateMapToolPinAnnotation()
{
  mQGisIface->mapCanvas()->setMapTool( mMapToolPinAnnotation );
}

void QgsVBSFunctionality::onMapToolSet( QgsMapTool * tool )
{
  mActionPinAnnotation->setChecked( tool == mMapToolPinAnnotation );
}

void QgsVBSFunctionality::checkOnTheFlyProjection( const QList<QgsMapLayer*>& newLayers )
{
  if ( !mReprojMsgItem.isNull() )
  {
    mQGisIface->messageBar()->popWidget( mReprojMsgItem.data() );
  }
  QString destAuthId = mQGisIface->mapCanvas()->mapSettings().destinationCrs().authid();
  QStringList reprojLayers;
  // Look at legend interface instead of maplayerregistry, to only check layers
  // the user can actually see
  foreach ( QgsMapLayer* layer, mQGisIface->legendInterface()->layers() + newLayers )
  {
    if ( layer->type() != QgsMapLayer::RedliningLayer && layer->crs().authid() != destAuthId )
    {
      reprojLayers.append( layer->name() );
    }
  }
  if ( !reprojLayers.isEmpty() )
  {
    mReprojMsgItem = mQGisIface->messageBar()->createMessage( tr( "On the fly projection enabled" ), tr( "The following layers are being reprojected to the selected CRS: %1. Performance may suffer." ).arg( reprojLayers.join( ", " ) ) );
    mQGisIface->messageBar()->pushItem( mReprojMsgItem.data() );
  }
}
