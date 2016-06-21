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
#include "layertree/qgslayertree.h"
#include "layertree/qgslayertreemodel.h"
#include "layertree/qgslayertreeview.h"
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

  mActionApprovedLayer = new QAction( tr( "Approved layer" ), this );
  mActionApprovedLayer->setCheckable( true );
  connect( mActionApprovedLayer, SIGNAL( toggled( bool ) ), this, SLOT( setApprovedLayer( bool ) ) );

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

  QgsPluginLayerRegistry::instance()->addPluginLayerType( new QgsMilXLayerType( mQGisIface->layerTreeView()->menuProvider() ) );

  mMilXLibrary = new QgsMilXLibrary( mQGisIface, mQGisIface->mapCanvas() );

  connect( mQGisIface->mapCanvas(), SIGNAL( layersChanged( QStringList ) ), this, SLOT( connectPickHandlers() ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( stopEditing() ) );
  connectPickHandlers();


  mQGisIface->layerTreeView()->menuProvider()->addLegendLayerAction( mActionApprovedLayer, "", "milx_approved_layer", QgsMapLayer::PluginLayer, false );
  connect( mQGisIface->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setApprovedActionState( QgsMapLayer* ) ) );
}

void QgsMilXPlugin::unload()
{
  disconnect( mQGisIface->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setApprovedActionState( QgsMapLayer* ) ) );
  mQGisIface->layerTreeView()->menuProvider()->removeLegendLayerAction( mActionApprovedLayer );
  QgsPluginLayerRegistry::instance()->removePluginLayerType( QgsMilXLayer::layerTypeKey() );
  mActionApprovedLayer = 0;
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
      layer->triggerRepaint();
    }
  }
}

void QgsMilXPlugin::setMilXLineWidth( int value )
{
  MilXClient::setLineWidth( value );
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      layer->triggerRepaint();
    }
  }
}

void QgsMilXPlugin::setMilXWorkMode( int idx )
{
  MilXClient::setWorkMode( idx );
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      layer->triggerRepaint();
    }
  }
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
  if ( !layer )
  {
    return;
  }
  if ( layer->isApproved() )
  {
    // Approved layers are not editable
    return;
  }
  QgsMilXEditTool* tool = new QgsMilXEditTool( mQGisIface->mapCanvas(), layer, layer->items()[symbolIdx] );
  delete layer->takeItem( symbolIdx );
  connect( tool, SIGNAL( deactivated() ), tool, SLOT( deleteLater() ) );
  mQGisIface->mapCanvas()->setMapTool( tool );
  layer->triggerRepaint();
  mActiveEditTool = tool;
}

void QgsMilXPlugin::setApprovedLayer( bool approved )
{
  QModelIndex idx = mQGisIface->layerTreeView()->currentIndex();
  if ( !idx.isValid() )
  {
    return;
  }

  QgsLayerTreeNode* node = mQGisIface->layerTreeView()->layerTreeModel()->index2node( idx );
  if ( !QgsLayerTree::isLayer( node ) )
  {
    return;
  }
  QgsMilXLayer *layer = qobject_cast<QgsMilXLayer*>( QgsLayerTree::toLayer( node )->layer() );

  if ( layer )
  {
    layer->setApproved( approved );
    layer->triggerRepaint();
  }
}

void QgsMilXPlugin::setApprovedActionState( QgsMapLayer* layer )
{
  if ( dynamic_cast<QgsMilXLayer*>( layer ) )
  {
    mActionApprovedLayer->blockSignals( true );
    mActionApprovedLayer->setChecked( static_cast<QgsMilXLayer*>( layer )->isApproved() );
    mActionApprovedLayer->blockSignals( false );
  }
}
