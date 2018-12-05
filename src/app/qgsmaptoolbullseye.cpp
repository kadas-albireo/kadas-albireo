/***************************************************************************
 *  qgsmaptoolbullseye.cpp                                                 *
 *  --------------                                                         *
 *  begin                : November 2018                                   *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
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

#include "qgsbullseyelayer.h"
#include "qgsdistancearea.h"
#include "qgsmaptoolbullseye.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgslayertreeview.h"
#include "qgscrscache.h"
#include "qgscoordinateformat.h"

#include <QAction>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QSpinBox>


QgsBullsEyeTool::QgsBullsEyeTool( QgsMapCanvas* canvas, QgsLayerTreeView* layerTreeView )
    : QgsMapTool( canvas ), mLayerTreeView( layerTreeView )
{
  mWidget = new QgsBullsEyeWidget( canvas, mLayerTreeView );
  mWidget->setVisible( false );
  connect( mWidget, SIGNAL( requestPickCenter() ), this, SLOT( setPicking() ) );
  connect( mWidget, SIGNAL( close() ), this, SLOT( close() ) );

  mActionEditLayer = new QAction( tr( "Edit" ), this );
  mActionEditLayer->setIcon( QIcon( ":/images/themes/default/mIconEditable.png" ) );
  connect( mActionEditLayer, SIGNAL( triggered( bool ) ), this, SLOT( editCurrentLayer( ) ) );

  mLayerTreeView->menuProvider()->addLegendLayerAction( mActionEditLayer, "", "edit_bulleye_layer", QgsMapLayer::PluginLayer, false );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWasAdded( QgsMapLayer* ) ), this, SLOT( addLayerTreeMenuAction( QgsMapLayer* ) ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( removeLayerTreeMenuAction( QString ) ) );
}

QgsBullsEyeTool::~QgsBullsEyeTool()
{
  mLayerTreeView->menuProvider()->removeLegendLayerAction( mActionEditLayer );
}

void QgsBullsEyeTool::addLayerTreeMenuAction( QgsMapLayer* mapLayer )
{
  if ( dynamic_cast<QgsBullsEyeLayer*>( mapLayer ) )
  {
    mLayerTreeView->menuProvider()->addLegendLayerActionForLayer( "edit_bulleye_layer", mapLayer );
  }
}

void QgsBullsEyeTool::removeLayerTreeMenuAction( const QString& mapLayerId )
{
  QgsMapLayer* mapLayer = QgsMapLayerRegistry::instance()->mapLayer( mapLayerId );
  if ( dynamic_cast<QgsBullsEyeLayer*>( mapLayer ) )
  {
    mLayerTreeView->menuProvider()->removeLegendLayerActionsForLayer( mapLayer );
  }
}

void QgsBullsEyeTool::editCurrentLayer()
{
  if ( dynamic_cast<QgsBullsEyeLayer*>( mCanvas->currentLayer() ) )
  {
    mCanvas->setMapTool( this );
  }
}

void QgsBullsEyeTool::activate()
{
  // If current layer is a BullEyeLayer, use that one
  if ( dynamic_cast<QgsBullsEyeLayer*>( mCanvas->currentLayer() ) )
  {
    mWidget->setLayer( mCanvas->currentLayer() );
  }
  else
  {
    // Search first BullEyeLayer
    bool found = false;
  for ( QgsMapLayer* layer : QgsMapLayerRegistry::instance()->mapLayers().values() )
    {
      if ( dynamic_cast<QgsBullsEyeLayer*>( layer ) )
      {
        mWidget->setLayer( layer );
        found = true;
        break;
      }
    }
    // If none found, create a new one
    if ( !found )
    {
      mWidget->createLayer( tr( "Bullseye" ) );
    }
  }
  mWidget->setVisible( true );
  QgsMapTool::activate();
}

void QgsBullsEyeTool::deactivate()
{
  mWidget->setVisible( false );
  mPicking = false;
  setCursor( Qt::ArrowCursor );
  QgsMapTool::deactivate();
}

void QgsBullsEyeTool::setPicking( bool picking )
{
  mPicking = picking;
  setCursor( picking ? Qt::CrossCursor : Qt::ArrowCursor );
}

void QgsBullsEyeTool::close()
{
  canvas()->unsetMapTool( this );
}

void QgsBullsEyeTool::canvasReleaseEvent( QMouseEvent *e )
{
  if ( mPicking )
  {
    mWidget->centerPicked( toMapCoordinates( e->pos() ) );
    setPicking( false );
  }
  else if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void QgsBullsEyeTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mPicking )
    {
      setPicking( false );
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}


QgsBullsEyeWidget::QgsBullsEyeWidget( QgsMapCanvas *canvas, QgsLayerTreeView* layerTreeView )
    : QgsBottomBar( canvas ), mLayerTreeView( layerTreeView )
{
  setLayout( new QHBoxLayout );
  layout()->setSpacing( 10 );

  QWidget* base = new QWidget();
  ui.setupUi( base );
  layout()->addWidget( base );

  QPushButton* closeButton = new QPushButton();
  closeButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
  closeButton->setIcon( QIcon( ":/images/themes/default/mIconClose.png" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, SIGNAL( clicked( bool ) ), this, SIGNAL( close() ) );
  layout()->addWidget( closeButton );
  layout()->setAlignment( closeButton, Qt::AlignTop );

  ui.comboBoxLabels->addItem( tr( "Disabled" ), static_cast<int>( QgsBullsEyeLayer::NO_LABELS ) );
  ui.comboBoxLabels->addItem( tr( "Axes" ), static_cast<int>( QgsBullsEyeLayer::LABEL_AXES ) );
  ui.comboBoxLabels->addItem( tr( "Rings" ), static_cast<int>( QgsBullsEyeLayer::LABEL_RINGS ) );
  ui.comboBoxLabels->addItem( tr( "Axes and rings" ), static_cast<int>( QgsBullsEyeLayer::LABEL_AXES_RINGS ) );

  connect( ui.toolButtonAddLayer, SIGNAL( clicked( bool ) ), this, SLOT( createLayer() ) );
  connect( ui.inputCenter, SIGNAL( coordinateChanged() ), this, SLOT( updateLayer() ) );
  connect( ui.toolButtonPickCenter, SIGNAL( clicked( bool ) ), this, SIGNAL( requestPickCenter() ) );
  connect( ui.spinBoxRings, SIGNAL( valueChanged( int ) ), this, SLOT( updateLayer() ) );

  connect( ui.spinBoxRingInterval, SIGNAL( valueChanged( double ) ), this, SLOT( updateLayer() ) );
  connect( ui.spinBoxAxesInterval, SIGNAL( valueChanged( double ) ), this, SLOT( updateLayer() ) );

  connect( ui.toolButtonColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( updateColor( QColor ) ) );
  connect( ui.spinBoxFontSize, SIGNAL( valueChanged( int ) ), this, SLOT( updateFontSize( int ) ) );
  connect( ui.comboBoxLabels, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateLabeling( int ) ) );
  connect( ui.spinBoxLineWidth, SIGNAL( valueChanged( int ) ), this, SLOT( updateLineWidth( int ) ) );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( repopulateLayers() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ), this, SLOT( repopulateLayers() ) );
}

void QgsBullsEyeWidget::createLayer( QString layerName )
{
  if ( layerName.isEmpty() )
  {
    layerName = QInputDialog::getText( this, tr( "Layer Name" ), tr( "Enter name of new layer:" ) );
  }
  if ( !layerName.isEmpty() )
  {
    QgsDistanceArea da;
    da.setEllipsoid( "WGS84" );
    da.setEllipsoidalMode( true );
    da.setSourceCrs( mCanvas->mapSettings().destinationCrs() );
    QgsRectangle extent = mCanvas->extent();
    double extentHeight = da.measureLine( QgsPoint( extent.center().x(), extent.yMinimum() ), QgsPoint( extent.center().x(), extent.yMaximum() ) );
    double interval = 0.5 * extentHeight * QGis::fromUnitToUnitFactor( QGis::Meters, QGis::NauticalMiles ) / 6; // Half height divided by nr rings+1, in nm
    QgsBullsEyeLayer* bullEyeLayer = new QgsBullsEyeLayer( layerName );
    bullEyeLayer->setup( mCanvas->extent().center(), mCanvas->mapSettings().destinationCrs(), 5, interval, 45 );
    QgsMapLayerRegistry::instance()->addMapLayer( bullEyeLayer );
    setLayer( bullEyeLayer );
  }
}

void QgsBullsEyeWidget::setLayer( QgsMapLayer *layer )
{
  mCurrentLayer = dynamic_cast<QgsBullsEyeLayer*>( layer );
  if ( !mCurrentLayer )
  {
    ui.widgetLayerSetup->setEnabled( false );
    return;
  }
  ui.comboBoxLayer->blockSignals( true );
  ui.comboBoxLayer->setCurrentIndex( ui.comboBoxLayer->findData( mCurrentLayer->id() ) );
  ui.comboBoxLayer->blockSignals( false );
  mLayerTreeView->setLayerVisible( mCurrentLayer, true );
  mCanvas->setCurrentLayer( mCurrentLayer );

  ui.inputCenter->blockSignals( true );
  ui.inputCenter->setCoordinate( mCurrentLayer->center(), mCurrentLayer->crs() );
  ui.inputCenter->blockSignals( false );
  ui.spinBoxRings->blockSignals( true );
  ui.spinBoxRings->setValue( mCurrentLayer->rings() );
  ui.spinBoxRings->blockSignals( false );
  ui.spinBoxRingInterval->blockSignals( true );
  ui.spinBoxRingInterval->setValue( mCurrentLayer->ringInterval() );
  ui.spinBoxRingInterval->blockSignals( false );
  ui.spinBoxAxesInterval->blockSignals( true );
  ui.spinBoxAxesInterval->setValue( mCurrentLayer->axesInterval() );
  ui.spinBoxAxesInterval->blockSignals( false );
  ui.toolButtonColor->setColor( mCurrentLayer->color() );
  ui.spinBoxFontSize->setValue( mCurrentLayer->fontSize() );
  ui.comboBoxLabels->setCurrentIndex( ui.comboBoxLabels->findData( static_cast<int>( mCurrentLayer->labellingMode() ) ) );
  ui.widgetLayerSetup->setEnabled( true );
}

void QgsBullsEyeWidget::centerPicked( const QgsPoint& pos )
{
  ui.inputCenter->setCoordinate( pos, mCanvas->mapSettings().destinationCrs() );
}

void QgsBullsEyeWidget::updateLayer()
{
  if ( !mCurrentLayer || ui.inputCenter->isEmpty() )
  {
    return;
  }
  QgsPoint center = ui.inputCenter->getCoordinate();
  const QgsCoordinateReferenceSystem& crs = ui.inputCenter->getCrs();
  int rings = ui.spinBoxRings->value();
  double interval = ui.spinBoxRingInterval->value();
  int axes = ui.spinBoxAxesInterval->value();
  mCurrentLayer->setup( center, crs, rings, interval, axes );
  mCurrentLayer->triggerRepaint();
}

void QgsBullsEyeWidget::updateColor( const QColor& color )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setColor( color );
    mCurrentLayer->triggerRepaint();
  }
}

void QgsBullsEyeWidget::updateFontSize( int fontSize )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setFontSize( fontSize );
    mCurrentLayer->triggerRepaint();
  }
}

void QgsBullsEyeWidget::updateLabeling( int /*index*/ )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setLabellingMode( static_cast<QgsBullsEyeLayer::LabellingMode>( ui.comboBoxLabels->currentData().toInt() ) );
    mCurrentLayer->triggerRepaint();
  }
}

void QgsBullsEyeWidget::updateLineWidth( int width )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setLineWidth( width );
    mCurrentLayer->triggerRepaint();
  }
}

void QgsBullsEyeWidget::repopulateLayers()
{
  // Avoid update while updating
  if ( ui.comboBoxLayer->signalsBlocked() )
  {
    return;
  }
  ui.comboBoxLayer->blockSignals( true );
  ui.comboBoxLayer->clear();
  int idx = 0, current = 0;
for ( QgsMapLayer* layer : QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsBullsEyeLayer*>( layer ) )
    {
      connect( layer, SIGNAL( layerNameChanged() ), this, SLOT( repopulateLayers() ), Qt::UniqueConnection );
      ui.comboBoxLayer->addItem( layer->name(), layer->id() );
      if ( mCanvas->currentLayer() == layer )
      {
        current = idx;
      }
      ++idx;
    }
  }
  ui.comboBoxLayer->setCurrentIndex( -1 );
  ui.comboBoxLayer->blockSignals( false );
  ui.comboBoxLayer->setCurrentIndex( current );
  ui.widgetLayerSetup->setEnabled( ui.comboBoxLayer->count() > 0 );
}

void QgsBullsEyeWidget::currentLayerChanged( int cur )
{
  QgsBullsEyeLayer* layer = dynamic_cast<QgsBullsEyeLayer*>( QgsMapLayerRegistry::instance()->mapLayer( ui.comboBoxLayer->itemData( cur ).toString() ) );
  if ( layer )
  {
    setLayer( layer );
  }
  else
  {
    ui.widgetLayerSetup->setEnabled( false );
  }
}
