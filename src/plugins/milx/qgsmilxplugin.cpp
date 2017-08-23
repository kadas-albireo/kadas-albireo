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
#include "qgsmilxutils.h"
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QSlider>
#include <QToolBar>

QgsMilXPlugin::QgsMilXPlugin( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mMilXLibrary( 0 )
{
  mPasteHandler = new QgsMilXPasteHandler( this );
}

QgsMilXPlugin::~QgsMilXPlugin()
{
  MilXClient::quit();
  delete mPasteHandler;
}

void QgsMilXPlugin::initGui()
{
  if ( !MilXClient::init() )
  {
    QgsDebugMsg( "Failed to initialize the MilX library." );
    return;
  }

  mActionCreateMilx = mQGisIface->findAction( "mActionMilx" );
  connect( mActionCreateMilx, SIGNAL( triggered() ), this, SLOT( createMilx() ) );

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

  mMilXLibrary = new QgsMilXLibrary( mQGisIface->mainWindow() );

  connect( mQGisIface->mapCanvas(), SIGNAL( layersChanged( QStringList ) ), this, SLOT( connectPickHandlers() ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( stopEditing() ) );
  connectPickHandlers();


  mQGisIface->layerTreeView()->menuProvider()->addLegendLayerAction( mActionApprovedLayer, "", "milx_approved_layer", QgsMapLayer::PluginLayer, false );
  connect( mQGisIface->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setApprovedActionState( QgsMapLayer* ) ) );

  mQGisIface->addPasteHandler( QGSCLIPBOARD_MILXITEMS_MIME, mPasteHandler );
}

void QgsMilXPlugin::unload()
{
  disconnect( mQGisIface->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setApprovedActionState( QgsMapLayer* ) ) );
  mQGisIface->layerTreeView()->menuProvider()->removeLegendLayerAction( mActionApprovedLayer );
  QgsPluginLayerRegistry::instance()->removePluginLayerType( QgsMilXLayer::layerTypeKey() );
  mActionApprovedLayer = 0;
  mActionCreateMilx = 0;
  mActionSaveMilx = 0;
  mActionLoadMilx = 0;
  delete mMilXLibrary;
  mMilXLibrary = 0;
  mQGisIface->removePasteHandler( QGSCLIPBOARD_MILXITEMS_MIME, mPasteHandler );
}

void QgsMilXPlugin::createMilx( )
{
  QgsMilXCreateTool* createTool = qobject_cast<QgsMilXCreateTool*>( mQGisIface->mapCanvas()->mapTool() );
  if ( createTool )
  {
    createTool->deleteLater();
  }
  else
  {
    QgsMilXCreateTool* createTool = new QgsMilXCreateTool( mQGisIface, mMilXLibrary );
    createTool->setAction( mActionCreateMilx );
    mQGisIface->mapCanvas()->setMapTool( createTool );
  }
}

void QgsMilXPlugin::paste( const QString &mimeType, const QByteArray &data, const QgsPoint *mapPos )
{
  if ( mimeType != QGSCLIPBOARD_MILXITEMS_MIME )
  {
    return;
  }
  QgsMilXLayer* layer = 0;
  if ( mActiveEditTool )
  {
    layer = mActiveEditTool->targetLayer();
  }
  if ( !layer )
  {
    QDialog dialog;
    dialog.setWindowTitle( tr( "Paste MilX Symbols" ) );
    QGridLayout* layout = new QGridLayout();
    dialog.setLayout( layout );
    layout->addWidget( new QLabel( tr( "Destination layer:" ) ), 0, 0, 1, 1 );
    QgsMilXLayerSelectionWidget* layerWidget = new QgsMilXLayerSelectionWidget( mQGisIface, &dialog );
    layout->addWidget( layerWidget, 0, 1, 1, 1 );
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    connect( bbox, SIGNAL( accepted() ), &dialog, SLOT( accept() ) );
    connect( bbox, SIGNAL( rejected() ), &dialog, SLOT( reject() ) );
    layout->addWidget( bbox, 1, 0, 1, 2 );
    dialog.setFixedSize( dialog.sizeHint() );
    if ( dialog.exec() != QDialog::Accepted )
    {
      return;
    }
    layer = layerWidget->getTargetLayer();
  }
  if ( !layer )
  {
    return;
  }
  QgsMilXEditTool* tool = new QgsMilXEditTool( mQGisIface, layer );
  mQGisIface->mapCanvas()->setMapTool( tool );
  mActiveEditTool = tool;
  tool->paste( data, mapPos );
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
      static_cast<QgsMilXLayer*>( layer )->invalidateBillboards();
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
      connect( milxLayer, SIGNAL( copySymbols( QVector<int> ) ), this, SLOT( manageSymbolCopy( QVector<int> ) ), Qt::UniqueConnection );
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
  QgsMilXEditTool* tool = new QgsMilXEditTool( mQGisIface, layer, layer->items()[symbolIdx] );
  delete layer->takeItem( symbolIdx );
  mQGisIface->mapCanvas()->setMapTool( tool );
  layer->triggerRepaint();
  mActiveEditTool = tool;
}

void QgsMilXPlugin::manageSymbolCopy( QVector<int> symbolIndices )
{
  QgsMilXLayer* layer = qobject_cast<QgsMilXLayer*>( QObject::sender() );
  if ( !layer )
  {
    return;
  }
  QList<QgsMilXItem*> milxItems;
  foreach ( int idx, symbolIndices )
  {
    milxItems.append( layer->items()[idx] );
  }
  QgsMilXIO::copyToClipboard( milxItems, mQGisIface );
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
