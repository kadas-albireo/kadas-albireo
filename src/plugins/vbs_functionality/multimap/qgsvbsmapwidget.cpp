/***************************************************************************
 *  qgsvbsmapwidget.cpp                                                    *
 *  -------------------                                                    *
 *  begin                : Sep 16, 2015                                    *
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

#include "qgsvbsmapwidget.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptoolpan.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>

QgsVBSMapWidget::QgsVBSMapWidget( int number, const QString &title, QgisInterface* iface, QWidget *parent )
    : QDockWidget( parent ), mIface( iface ), mNumber( number )
{
  QSettings settings;

  mLayerSelectionButton = new QToolButton( this );
  mLayerSelectionButton->setText( tr( "Layers" ) );
  mLayerSelectionButton->setPopupMode( QToolButton::InstantPopup );
  mLayerSelectionMenu = new QMenu( mLayerSelectionButton );
  mLayerSelectionButton->setMenu( mLayerSelectionMenu );

  mSyncViewButton = new QToolButton( this );
  mSyncViewButton->setToolTip( tr( "Syncronize with main view" ) );
  mSyncViewButton->setCheckable( true );
  mSyncViewButton->setIcon( QIcon( ":/vbsfunctionality/icons/sync_views.svg" ) );
  connect( mSyncViewButton, SIGNAL( toggled( bool ) ), this, SLOT( syncCanvasExtents() ) );

  mTitleLineEdit = new QLineEdit( title, this );
  connect( mTitleLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( setWindowTitle( QString ) ) );

  mCloseButton = new QToolButton( this );
  mCloseButton->setIcon( QIcon( ":/images/themes/default/mActionRemove.svg" ) );
  mCloseButton->setIconSize( QSize( 16, 16 ) );
  mCloseButton->setToolTip( tr( "Close" ) );
  connect( mCloseButton, SIGNAL( clicked( bool ) ), this, SLOT( close() ) );

  QWidget* titleWidget = new QWidget( this );
  titleWidget->setLayout( new QHBoxLayout() );
  titleWidget->layout()->addWidget( mLayerSelectionButton );
  titleWidget->layout()->addWidget( mSyncViewButton );
  static_cast<QHBoxLayout*>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( mTitleLineEdit );
  static_cast<QHBoxLayout*>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( mCloseButton );
  titleWidget->layout()->setContentsMargins( 0, 0, 0, 0 );

  setWindowTitle( mTitleLineEdit->text() );
  setTitleBarWidget( titleWidget );

  mMapCanvas = new QgsMapCanvas( this );
  mMapCanvas->setCanvasColor( Qt::white );
  mMapCanvas->enableAntiAliasing( mIface->mapCanvas()->antiAliasingEnabled() );
  QgsMapCanvas::WheelAction wheelAction = static_cast<QgsMapCanvas::WheelAction>( settings.value( "/qgis/wheel_action", "0" ).toInt() );
  double zoomFactor = settings.value( "/qgis/zoom_factor", "2.0" ).toDouble();
  mMapCanvas->setWheelAction( wheelAction, zoomFactor );
  setWidget( mMapCanvas );

  QgsMapToolPan* mapTool = new QgsMapToolPan( mMapCanvas );
  connect( mapTool, SIGNAL( deactivated() ), mapTool, SLOT( deleteLater() ) );
  mMapCanvas->setMapTool( mapTool );

  connect( mIface->mapCanvas(), SIGNAL( extentsChanged() ), this, SLOT( syncCanvasExtents() ) );
  connect( mIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( updateMapProjection() ) );
  connect( mIface->mapCanvas(), SIGNAL( mapUnitsChanged() ), this, SLOT( updateMapProjection() ) );
  connect( mIface->mapCanvas(), SIGNAL( hasCrsTransformEnabledChanged( bool ) ), this, SLOT( updateMapProjection() ) );
  connect( mIface->mapCanvas(), SIGNAL( layersChanged() ), this, SLOT( updateLayerSelectionMenu() ) );
  connect( mMapCanvas, SIGNAL( extentsChanged() ), this, SLOT( syncMainCanvasExtents() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( updateLayerSelectionMenu() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerRemoved( QString ) ), this, SLOT( updateLayerSelectionMenu() ) );

  updateLayerSelectionMenu();
  mMapCanvas->setRenderFlag( false );
  updateMapProjection();
  syncCanvasExtents();
  mMapCanvas->setRenderFlag( true );
}

QStringList QgsVBSMapWidget::getLayers() const
{
  QStringList layers;
  foreach ( QAction* layerAction, mLayerSelectionMenu->actions() )
  {
    if ( layerAction->isChecked() )
    {
      layers.append( layerAction->data().toString() );
    }
  }
  return layers;
}

void QgsVBSMapWidget::syncCanvasExtents()
{
  if ( mSyncViewButton->isChecked() )
  {
    blockSignals( true );
    mMapCanvas->setExtent( mIface->mapCanvas()->extent() );
    blockSignals( false );
    mMapCanvas->refresh();
  }
}

void QgsVBSMapWidget::syncMainCanvasExtents()
{
  if ( mSyncViewButton->isChecked() )
  {
    mIface->mapCanvas()->blockSignals( true );
    mIface->mapCanvas()->setExtent( mMapCanvas->extent() );
    mIface->mapCanvas()->blockSignals( false );
    mIface->mapCanvas()->refresh();
  }
}

void QgsVBSMapWidget::updateLayerSelectionMenu()
{
  QList<QgsMapLayer*> currentLayers = mMapCanvas->layers();
  mLayerSelectionMenu->clear();
  foreach ( QgsMapLayer* layer, mIface->mapCanvas()->layers()/*QgsMapLayerRegistry::instance()->mapLayers()*/ )
  {
    QAction* layerAction = new QAction( layer->name(), mLayerSelectionMenu );
    layerAction->setData( layer->id() );
    layerAction->setCheckable( true );
    layerAction->setChecked( currentLayers.contains( layer ) || mInitialLayers.contains( layer->id() ) );
    connect( layerAction, SIGNAL( toggled( bool ) ), this, SLOT( updateLayerSet() ) );
    mLayerSelectionMenu->addAction( layerAction );
  }
  mInitialLayers.clear();
  updateLayerSet();
}

void QgsVBSMapWidget::updateLayerSet()
{
  bool wasEmpty = mMapCanvas->layers().isEmpty();
  QList<QgsMapCanvasLayer> layerSet;
  foreach ( QAction* layerAction, mLayerSelectionMenu->actions() )
  {
    if ( layerAction->isChecked() )
    {
      layerSet.append( QgsMapCanvasLayer( QgsMapLayerRegistry::instance()->mapLayer( layerAction->data().toString() ) ) );
    }
  }
  mMapCanvas->setLayerSet( layerSet );
  if ( wasEmpty )
  {
    mMapCanvas->setExtent( mIface->mapCanvas()->extent() );
    mMapCanvas->refresh();
  }
}

void QgsVBSMapWidget::updateMapProjection()
{
  mMapCanvas->setDestinationCrs( mIface->mapCanvas()->mapSettings().destinationCrs() );
  mMapCanvas->setCrsTransformEnabled( mIface->mapCanvas()->hasCrsTransformEnabled() );
  mMapCanvas->setMapUnits( mIface->mapCanvas()->mapSettings().mapUnits() );
}

void QgsVBSMapWidget::showEvent( QShowEvent * )
{
  // Clear previously set fixed size - which was just used to enforce the initial dimensions...
  setFixedSize( QSize() );
  setMinimumSize( 100, 100 );
  setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}
