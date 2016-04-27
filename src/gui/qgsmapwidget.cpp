/***************************************************************************
 *  qgsmapwidget.cpp                                                    *
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

#include "qgsannotationitem.h"
#include "qgsmapwidget.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptoolpan.h"
#include "qgsproject.h"
#include "layertree/qgslayertreegroup.h"
#include "layertree/qgslayertreelayer.h"
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

QgsMapWidget::QgsMapWidget( int number, const QString &title, QgsMapCanvas *masterCanvas, QWidget *parent )
    : QDockWidget( parent ), mNumber( number ), mMasterCanvas( masterCanvas ), mUnsetFixedSize( true )
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
  titleWidget->setObjectName( "mapWidgetTitleWidget" );
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
  mMapCanvas->setCanvasColor( Qt::transparent );
  mMapCanvas->enableAntiAliasing( mMasterCanvas->antiAliasingEnabled() );
  QgsMapCanvas::WheelAction wheelAction = static_cast<QgsMapCanvas::WheelAction>( settings.value( "/qgis/wheel_action", "0" ).toInt() );
  double zoomFactor = settings.value( "/qgis/zoom_factor", "2.0" ).toDouble();
  mMapCanvas->setWheelAction( wheelAction, zoomFactor );
  setWidget( mMapCanvas );

  QgsMapToolPan* mapTool = new QgsMapToolPan( mMapCanvas, false );
  connect( mapTool, SIGNAL( deactivated() ), mapTool, SLOT( deleteLater() ) );
  mMapCanvas->setMapTool( mapTool );

  connect( mMasterCanvas, SIGNAL( extentsChanged() ), this, SLOT( syncCanvasExtents() ) );
  connect( mMasterCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( updateMapProjection() ) );
  connect( mMasterCanvas, SIGNAL( mapUnitsChanged() ), this, SLOT( updateMapProjection() ) );
  connect( mMasterCanvas, SIGNAL( hasCrsTransformEnabledChanged( bool ) ), this, SLOT( updateMapProjection() ) );
  connect( mMasterCanvas, SIGNAL( layersChanged() ), this, SLOT( updateLayerSelectionMenu() ) );
  connect( mMasterCanvas, SIGNAL( mapCanvasRefreshed() ), mMapCanvas, SLOT( refresh() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( updateLayerSelectionMenu() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerRemoved( QString ) ), this, SLOT( updateLayerSelectionMenu() ) );
  connect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), mMasterCanvas, SIGNAL( xyCoordinates( QgsPoint ) ) );

  updateLayerSelectionMenu();
  mMapCanvas->setRenderFlag( false );
  updateMapProjection();
  mMapCanvas->setExtent( mMasterCanvas->extent() );
  mMapCanvas->setRenderFlag( true );

  connect( mMasterCanvas, SIGNAL( annotationItemChanged( QgsAnnotationItem* ) ), this, SLOT( addAnnotationItem( QgsAnnotationItem* ) ) );
  foreach ( QGraphicsItem* item, mMasterCanvas->items() )
  {
    if ( dynamic_cast<QgsAnnotationItem*>( item ) )
    {
      addAnnotationItem( static_cast<QgsAnnotationItem*>( item ) );
    }
  }
}

void QgsMapWidget::setInitialLayers( const QStringList &initialLayers, bool updateMenu )
{
  mInitialLayers = initialLayers;
  if ( updateMenu )
    updateLayerSelectionMenu();
}

QStringList QgsMapWidget::getLayers() const
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

QgsRectangle QgsMapWidget::getMapExtent() const
{
  return mMapCanvas->extent();
}

void QgsMapWidget::setMapExtent( const QgsRectangle& extent )
{
  mMapCanvas->setExtent( extent );
  mMapCanvas->refresh();
}

bool QgsMapWidget::getLocked() const
{
  return mLockViewButton->isChecked();
}

void QgsMapWidget::setLocked( bool locked )
{
  mLockViewButton->setChecked( locked );
}

void QgsMapWidget::setCanvasLocked( bool locked )
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

void QgsMapWidget::syncCanvasExtents()
{
  if ( mLockViewButton->isChecked() )
  {
    QgsPoint center = mMasterCanvas->extent().center();
    double w = width() * mMasterCanvas->mapUnitsPerPixel();
    double h = height() * mMasterCanvas->mapUnitsPerPixel();
    setMapExtent( QgsRectangle( center.x() - .5 * w, center.y() - .5 * h, center.x() + .5 * w, center.y() + .5 * h ) );
  }
}

void QgsMapWidget::updateLayerSelectionMenu()
{
  QStringList prevDisabledLayers;
  QStringList prevLayers;
  foreach ( QAction* action, mLayerSelectionMenu->actions() )
  {
    prevLayers.append( action->data().toString() );
    if ( !action->isChecked() )
    {
      prevDisabledLayers.append( action->data().toString() );
    }
  }
  QStringList masterLayers;
  foreach ( const QgsMapLayer* layer, mMasterCanvas->layers() )
  {
    masterLayers.append( layer->id() );
  }

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
    if ( !mInitialLayers.isEmpty() )
    {
      layerAction->setChecked( mInitialLayers.contains( layer->id() ) );
    }
    else
    {
      bool wasDisabled = prevDisabledLayers.contains( layer->id() );
      bool isNewEnabledLayer = !prevLayers.contains( layer->id() ) && masterLayers.contains( layer->id() );
      layerAction->setChecked(( prevLayers.contains( layer->id() ) && !wasDisabled ) || isNewEnabledLayer );
    }
    connect( layerAction, SIGNAL( toggled( bool ) ), this, SLOT( updateLayerSet() ) );
    mLayerSelectionMenu->addAction( layerAction );
  }
  updateLayerSet();
  mInitialLayers.clear();
}

void QgsMapWidget::updateLayerSet()
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

void QgsMapWidget::updateMapProjection()
{
  mMapCanvas->setDestinationCrs( mMasterCanvas->mapSettings().destinationCrs() );
  mMapCanvas->setCrsTransformEnabled( mMasterCanvas->hasCrsTransformEnabled() );
  mMapCanvas->setMapUnits( mMasterCanvas->mapSettings().mapUnits() );
}

void QgsMapWidget::showEvent( QShowEvent * )
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

bool QgsMapWidget::eventFilter( QObject *obj, QEvent *ev )
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

void QgsMapWidget::addAnnotationItem( QgsAnnotationItem *item )
{
  QgsAnnotationItem* clonedItem = item->clone( mMapCanvas );
  if ( clonedItem )
  {
    clonedItem->updatePosition();
    connect( item, SIGNAL( destroyed( QObject* ) ), clonedItem, SLOT( deleteLater() ) );
    connect( item, SIGNAL( itemUpdated( QgsAnnotationItem* ) ), clonedItem, SLOT( deleteLater() ) );
  }
}

void QgsMapWidget::contextMenuEvent( QContextMenuEvent * e )
{
  e->accept();
}
