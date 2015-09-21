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
#include "qgsproject.h"
#include "layertree/qgslayertreegroup.h"
#include "layertree/qgslayertreelayer.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

QgsVBSMapWidget::QgsVBSMapWidget( int number, const QString &title, QgisInterface* iface, QWidget *parent )
    : QDockWidget( parent ), mIface( iface ), mNumber( number ), mUnsetFixedSize( true )
{
  QSettings settings;

  mLayerSelectionButton = new QToolButton( this );
  mLayerSelectionButton->setAutoRaise( true );
  mLayerSelectionButton->setText( tr( "Layers" ) );
  mLayerSelectionButton->setPopupMode( QToolButton::InstantPopup );
  mLayerSelectionMenu = new QMenu( mLayerSelectionButton );
  mLayerSelectionButton->setMenu( mLayerSelectionMenu );

  mLockViewButton = new QToolButton( this );
  mLockViewButton->setAutoRaise( true );
  mLockViewButton->setToolTip( tr( "Lock with main view" ) );
  mLockViewButton->setCheckable( true );
  mLockViewButton->setIcon( QIcon( ":/images/themes/default/locked.svg" ) );
  mLockViewButton->setIconSize( QSize( 12, 12 ) );
  connect( mLockViewButton, SIGNAL( toggled( bool ) ), this, SLOT( setCanvasLocked( bool ) ) );

  mTitleStackedWidget = new QStackedWidget( this );
  mTitleLabel = new QLabel( title );
  mTitleLabel->setCursor( Qt::IBeamCursor );
  mTitleStackedWidget->addWidget( mTitleLabel );

  mTitleLineEdit = new QLineEdit( title, this );
  connect( mTitleLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( setWindowTitle( QString ) ) );
  mTitleStackedWidget->addWidget( mTitleLineEdit );

  mTitleLabel->installEventFilter( this );
  mTitleLineEdit->installEventFilter( this );

  mCloseButton = new QToolButton( this );
  mCloseButton->setAutoRaise( true );
  mCloseButton->setIcon( QIcon( ":/images/themes/default/mActionRemove.svg" ) );
  mCloseButton->setIconSize( QSize( 12, 12 ) );
  mCloseButton->setToolTip( tr( "Close" ) );
  connect( mCloseButton, SIGNAL( clicked( bool ) ), this, SLOT( close() ) );

  QWidget* titleWidget = new QWidget( this );
  titleWidget->setLayout( new QHBoxLayout() );
  titleWidget->layout()->addWidget( mLayerSelectionButton );
  titleWidget->layout()->addWidget( mLockViewButton );
  static_cast<QHBoxLayout*>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( mTitleStackedWidget );
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
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( updateLayerSelectionMenu() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerRemoved( QString ) ), this, SLOT( updateLayerSelectionMenu() ) );

  updateLayerSelectionMenu();
  mMapCanvas->setRenderFlag( false );
  updateMapProjection();
  mMapCanvas->setExtent( mIface->mapCanvas()->extent() );
  mMapCanvas->setRenderFlag( true );
}

void QgsVBSMapWidget::setInitialLayers( const QStringList &initialLayers, bool updateMenu )
{
  mInitialLayers = initialLayers;
  if ( updateMenu )
    updateLayerSelectionMenu();
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

QgsRectangle QgsVBSMapWidget::getMapExtent() const
{
  return mMapCanvas->extent();
}

void QgsVBSMapWidget::setMapExtent( const QgsRectangle& extent )
{
  mMapCanvas->setExtent( extent );
  mMapCanvas->refresh();
}

bool QgsVBSMapWidget::getLocked() const
{
  return mLockViewButton->isChecked();
}

void QgsVBSMapWidget::setLocked( bool locked )
{
  mLockViewButton->setChecked( locked );
}

void QgsVBSMapWidget::setCanvasLocked( bool locked )
{
  if ( locked )
  {
    mMapCanvas->setEnabled( false );
    syncCanvasExtents();
  }
  else
  {
    mMapCanvas->setEnabled( true );
  }
}

void QgsVBSMapWidget::syncCanvasExtents()
{
  if ( mLockViewButton->isChecked() )
  {
    QgsPoint center = mIface->mapCanvas()->extent().center();
    double w = width() * mIface->mapCanvas()->mapUnitsPerPixel();
    double h = height() * mIface->mapCanvas()->mapUnitsPerPixel();
    setMapExtent( QgsRectangle( center.x() - .5 * w, center.y() - .5 * h, center.x() + .5 * w, center.y() + .5 * h ) );
  }
}

void QgsVBSMapWidget::updateLayerSelectionMenu()
{
  QList<QgsMapLayer*> currentLayers = mMapCanvas->layers();
  mLayerSelectionMenu->clear();
  // Use layerTreeRoot to get layers ordered as in the layer tree
  foreach ( QgsLayerTreeLayer* layerTreeLayer, QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    QgsMapLayer* layer = layerTreeLayer->layer();
    if ( !layer )
      continue;
    QAction* layerAction = new QAction( layer->name(), mLayerSelectionMenu );
    layerAction->setData( layer->id() );
    layerAction->setCheckable( true );
    layerAction->setChecked( currentLayers.contains( layer ) || mInitialLayers.contains( layer->id() ) );
    connect( layerAction, SIGNAL( toggled( bool ) ), this, SLOT( updateLayerSet() ) );
    mLayerSelectionMenu->addAction( layerAction );
  }
  updateLayerSet();
  mInitialLayers.clear();
}

void QgsVBSMapWidget::updateLayerSet()
{
  QList<QgsMapCanvasLayer> layerSet;
  foreach ( QAction* layerAction, mLayerSelectionMenu->actions() )
  {
    if ( layerAction->isChecked() )
    {
      layerSet.append( QgsMapCanvasLayer( QgsMapLayerRegistry::instance()->mapLayer( layerAction->data().toString() ) ) );
    }
  }
  mMapCanvas->setLayerSet( layerSet );
}

void QgsVBSMapWidget::updateMapProjection()
{
  mMapCanvas->setDestinationCrs( mIface->mapCanvas()->mapSettings().destinationCrs() );
  mMapCanvas->setCrsTransformEnabled( mIface->mapCanvas()->hasCrsTransformEnabled() );
  mMapCanvas->setMapUnits( mIface->mapCanvas()->mapSettings().mapUnits() );
}

void QgsVBSMapWidget::showEvent( QShowEvent * )
{
  if ( mUnsetFixedSize )
  {
    // Clear previously set fixed size - which was just used to enforce the initial dimensions...
    mUnsetFixedSize = false;
    setFixedSize( QSize() );
    setMinimumSize( 200, 200 );
    setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
  }
}

bool QgsVBSMapWidget::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mTitleLabel && ev->type() == QEvent::MouseButtonPress )
  {
    mTitleStackedWidget->setCurrentWidget( mTitleLineEdit );
    mTitleLineEdit->setFocus();
    mTitleLineEdit->selectAll();
    return true;
  }
  else if ( obj == mTitleLineEdit && ev->type() == QEvent::FocusOut )
  {
    mTitleStackedWidget->setCurrentWidget( mTitleLabel );
    return true;
  }
  else
  {
    return QObject::eventFilter( obj, ev );
  }
}
