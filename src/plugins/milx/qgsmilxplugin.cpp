/***************************************************************************
 *  qgsmilxplugin.cpp                                                      *
 *  -----------------                                                      *
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

#include "qgsmilxplugin.h"
#include "qgsmilxmaptools.h"
#include "qgslegendinterface.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessagebaritem.h"
#include "qgspluginlayerregistry.h"
#include "qgsproject.h"
#include "qgisinterface.h"
#include "milx_plugin.h"
#include "qgsmilxlibrary.h"
#include "qgsmilxio.h"
#include "qgsmilxlayer.h"
#include <QAction>
#include <QComboBox>
#include <QMainWindow>
#include <QSlider>
#include <QToolBar>

QgsMilXPlugin::QgsMilXPlugin( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mMilXLibrary( 0 )
{
}

void QgsMilXPlugin::initGui()
{
  if ( !MilXClient::init() )
  {
    QgsDebugMsg( "Failed to initialize the MilX library." );
    return;
  }

  mActionMilx = mQGisIface->findAction( "mActionMilx" );
  connect( mActionMilx, SIGNAL( triggered( ) ), this, SLOT( toggleMilXLibrary( ) ) );

  mActionSaveMilx = mQGisIface->findAction( "mActionSaveMilx" );
  connect( mActionSaveMilx, SIGNAL( triggered( ) ), this, SLOT( saveMilx( ) ) );

  mActionLoadMilx = mQGisIface->findAction( "mActionLoadMilx" );
  connect( mActionLoadMilx, SIGNAL( triggered( ) ), this, SLOT( loadMilx( ) ) );

  QWidget* milxTab = qobject_cast<QWidget*>( mQGisIface->findObject( "mMssTab" ) );
  QTabWidget* ribbonWidget = qobject_cast<QTabWidget*>( mQGisIface->findObject( "mRibbonWidget" ) );
  if ( ribbonWidget && milxTab )
    ribbonWidget->setTabEnabled( ribbonWidget->indexOf( milxTab ), true );

  mSymbolSizeSlider = qobject_cast<QSlider*>( mQGisIface->findObject( "mSymbolSizeSlider" ) );
  mLineWidthSlider = qobject_cast<QSlider*>( mQGisIface->findObject( "mLineWidthSlider" ) );
  mWorkModeCombo = qobject_cast<QComboBox*>( mQGisIface->findObject( "mWorkModeCombo" ) );
  if ( mSymbolSizeSlider )
  {
    mSymbolSizeSlider->setValue( QSettings().value( "/milx/milx_symbol_size", "60" ).toInt() );
    setMilXSymbolSize( mSymbolSizeSlider->value() );
    connect( mSymbolSizeSlider, SIGNAL( valueChanged( int ) ), this, SLOT( setMilXSymbolSize( int ) ) );
  }
  if ( mLineWidthSlider )
  {
    mLineWidthSlider->setValue( QSettings().value( "/milx/milx_line_width", "2" ).toInt() );
    setMilXLineWidth( mLineWidthSlider->value() );
    connect( mLineWidthSlider, SIGNAL( valueChanged( int ) ), this, SLOT( setMilXLineWidth( int ) ) );
  }
  if ( mWorkModeCombo )
  {
    mWorkModeCombo->setCurrentIndex( QSettings().value( "/milx/milx_work_mode", "1" ).toInt() );
    setMilXWorkMode( mWorkModeCombo->currentIndex() );
    connect( mWorkModeCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setMilXWorkMode( int ) ) );
  }

  QgsPluginLayerRegistry::instance()->addPluginLayerType( new QgsMilXLayerType() );

  mMilXLibrary = new QgsMilXLibrary( mQGisIface, mQGisIface->mapCanvas() );

  connect( mQGisIface->mapCanvas(), SIGNAL( layersChanged() ), this, SLOT( connectPickHandlers() ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( stopEditing() ) );
  connectPickHandlers();
}

void QgsMilXPlugin::unload()
{
  QgsPluginLayerRegistry::instance()->removePluginLayerType( QgsMilXLayer::layerTypeKey() );
  mActionMilx = 0;
  mActionSaveMilx = 0;
  mActionLoadMilx = 0;
}

void QgsMilXPlugin::toggleMilXLibrary( )
{
  mMilXLibrary->autocreateLayer();
  mMilXLibrary->show();
  mMilXLibrary->raise();
}

void QgsMilXPlugin::saveMilx()
{
  stopEditing();
  QgsMilXIO::save( mQGisIface );
}

void QgsMilXPlugin::loadMilx()
{
  QgsMilXIO::load( mQGisIface );
}

void QgsMilXPlugin::setMilXSymbolSize( int value )
{
  MilXClient::setSymbolSize( value );
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      mQGisIface->mapCanvas()->clearCache( layer->id() );
    }
  }
  mQGisIface->mapCanvas()->refresh();
}

void QgsMilXPlugin::setMilXLineWidth( int value )
{
  MilXClient::setLineWidth( value );
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      mQGisIface->mapCanvas()->clearCache( layer->id() );
    }
  }
  mQGisIface->mapCanvas()->refresh();
}

void QgsMilXPlugin::setMilXWorkMode( int idx )
{
  MilXClient::setWorkMode( idx );
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      mQGisIface->mapCanvas()->clearCache( layer->id() );
    }
  }
  mQGisIface->mapCanvas()->refresh();
}

void QgsMilXPlugin::stopEditing()
{
  if ( !mActiveEditTool.isNull() )
  {
    delete mActiveEditTool.data();
  }
}

void QgsMilXPlugin::connectPickHandlers()
{
  foreach ( QgsMapLayer* layer, mQGisIface->mapCanvas()->layers() )
  {
    QgsMilXLayer* milxLayer = qobject_cast<QgsMilXLayer*>( layer );
    if ( milxLayer )
    {
      connect( milxLayer, SIGNAL( symbolPicked( int ) ), this, SLOT( manageSymbolPick( int ) ), Qt::UniqueConnection );
    }
  }
}

void QgsMilXPlugin::manageSymbolPick( int symbolIdx )
{
  QgsMilXLayer* layer = qobject_cast<QgsMilXLayer*>( QObject::sender() );
  QgsMilXEditTool* tool = new QgsMilXEditTool( mQGisIface->mapCanvas(), layer, layer->items()[symbolIdx] );
  delete layer->takeItem( symbolIdx );
  connect( tool, SIGNAL( deactivated() ), tool, SLOT( deleteLater() ) );
  mQGisIface->mapCanvas()->setMapTool( tool );
  mQGisIface->mapCanvas()->clearCache( layer->id() );
  mQGisIface->mapCanvas()->refresh();
  mActiveEditTool = tool;
}
