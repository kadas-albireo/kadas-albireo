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
#include "qgsvbscrashhandler.h"
#include "qgisinterface.h"
#include "multimap/qgsvbsmultimapmanager.h"
#include "ovl/qgsvbsovlimporter.h"
#include "search/qgsvbssearchbox.h"
#include "vbsfunctionality_plugin.h"
#include <QAction>
#include <QToolBar>

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mActionPinAnnotation( 0 )
    , mSearchToolbar( 0 )
    , mSearchBox( 0 )
    , mReprojMsgItem( 0 )
    , mCrashHandler( 0 )
    , mMultiMapManager( 0 )
    , mActionOvlImport( 0 )
    /*, mSlopeTool( 0 )
    , mViewshedTool( 0 )
    , mHillshadeTool( 0 )*/
{
}

void QgsVBSFunctionality::initGui()
{
  mSearchToolbar = mQGisIface->addToolBar( "vbsSearchToolbar" );
  mSearchBox = new QgsVBSSearchBox( mQGisIface, mSearchToolbar );
  mSearchToolbar->addWidget( mSearchBox );

  QWidget* layerTreeToolbar = mQGisIface->mainWindow()->findChild<QWidget*>( "layerTreeToolbar" );
  if ( layerTreeToolbar ) layerTreeToolbar->setVisible( false );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( checkOnTheFlyProjection( QList<QgsMapLayer*> ) ) );
  connect( mQGisIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( checkOnTheFlyProjection() ) );

  mCrashHandler = new QgsVBSCrashHandler();

  mMultiMapManager = new QgsVBSMultiMapManager( mQGisIface, this );

  mActionOvlImport = new QAction( QIcon( ":/vbsfunctionality/icons/ovl.svg" ), tr( "Import ovl" ), this );
  connect( mActionOvlImport, SIGNAL( triggered( bool ) ), this, SLOT( importOVL() ) );
  mQGisIface->pluginToolBar()->addAction( mActionOvlImport );

#if 0
  mActionSlope = new QAction( QIcon( ":/vbsfunctionality/icons/slope.svg" ), tr( "Compute slope" ), this );
  mActionSlope->setCheckable( true );
  connect( mActionSlope, SIGNAL( toggled( bool ) ), this, SLOT( computeSlope( bool ) ) );
  mQGisIface->pluginToolBar()->addAction( mActionSlope );

  mActionViewshed = new QAction( QIcon( ":/vbsfunctionality/icons/viewshed.svg" ), tr( "Compute viewshed" ), this );
  mActionViewshed->setCheckable( true );
  connect( mActionViewshed, SIGNAL( toggled( bool ) ), this, SLOT( computeViewshed( bool ) ) );
  mQGisIface->pluginToolBar()->addAction( mActionViewshed );

  mActionViewshedSector = new QAction( QIcon( ":/vbsfunctionality/icons/viewshed_sector.svg" ), tr( "Compute viewshed" ), this );
  mActionViewshedSector->setCheckable( true );
  connect( mActionViewshedSector, SIGNAL( toggled( bool ) ), this, SLOT( computeViewshed( bool ) ) );
  mQGisIface->pluginToolBar()->addAction( mActionViewshedSector );

  mActionHillshade = new QAction( QIcon( ":/vbsfunctionality/icons/hillshade.svg" ), tr( "Compute hillshade" ), this );
  mActionHillshade->setCheckable( true );
  connect( mActionHillshade, SIGNAL( toggled( bool ) ), this, SLOT( computeHillshade( bool ) ) );
  mQGisIface->pluginToolBar()->addAction( mActionHillshade );
#endif //0
}

void QgsVBSFunctionality::unload()
{
  disconnect( mQGisIface->mapCanvas(), SIGNAL( mapToolSet( QgsMapTool* ) ), this, SLOT( onMapToolSet( QgsMapTool* ) ) );
  delete mSearchToolbar;
  mSearchToolbar = 0;
  mSearchBox = 0;
  delete mCrashHandler;
  mCrashHandler = 0;
  delete mMultiMapManager;
  mMultiMapManager = 0;
  delete mActionOvlImport;
  mActionOvlImport = 0;
  delete mActionSlope;
  mActionSlope = 0;
  delete mActionViewshed;
  mActionViewshed = 0;
  delete mActionViewshedSector;
  mActionViewshedSector = 0;
  delete mActionHillshade;
  mActionHillshade = 0;

  QWidget* layerTreeToolbar = mQGisIface->mainWindow()->findChild<QWidget*>( "layerTreeToolbar" );
  if ( layerTreeToolbar ) layerTreeToolbar->setVisible( true );
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
    mReprojMsgItem->setDuration( 10 );
    mQGisIface->messageBar()->pushItem( mReprojMsgItem.data() );
  }
}

void QgsVBSFunctionality::importOVL()
{
  QgsVBSOvlImporter( mQGisIface, mQGisIface->mainWindow() ).import();
}

#if 0
void QgsVBSFunctionality::computeSlope( bool checked )
{
  if ( checked )
  {
    mSlopeTool = new QgsVBSSlopeTool( mQGisIface, this );
    connect( mSlopeTool, SIGNAL( finished() ), mActionSlope, SLOT( toggle() ) );
  }
  else
  {
    delete mSlopeTool;
    mSlopeTool = 0;
  }
}

void QgsVBSFunctionality::computeViewshed( bool checked )
{
  QAction* viewshedAction = qobject_cast<QAction*>( QObject::sender() );
  if ( checked )
  {
    mViewshedTool = new QgsVBSViewshedTool( mQGisIface, viewshedAction == mActionViewshedSector, this );
    connect( mViewshedTool, SIGNAL( finished() ), viewshedAction, SLOT( toggle() ) );
  }
  else
  {
    delete mViewshedTool;
    mViewshedTool = 0;
  }
}

void QgsVBSFunctionality::computeHillshade( bool checked )
{
  if ( checked )
  {
    mHillshadeTool = new QgsVBSHillshadeTool( mQGisIface, this );
    connect( mHillshadeTool, SIGNAL( finished() ), mActionHillshade, SLOT( toggle() ) );
  }
  else
  {
    delete mHillshadeTool;
    mHillshadeTool = 0;
  }
}
#endif //0
