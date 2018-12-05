/***************************************************************************
 *  qgsmaptoolguidegrid.cpp                                                *
 *  --------------                                                         *
 *  begin                : March 2018                                      *
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

#include "qgsguidegridlayer.h"
#include "qgsmaptoolguidegrid.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgslayertreeview.h"

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


QgsGuideGridTool::QgsGuideGridTool( QgsMapCanvas* canvas, QgsLayerTreeView* layerTreeView )
    : QgsMapTool( canvas ), mLayerTreeView( layerTreeView )
{
  mWidget = new QgsGuideGridWidget( canvas, mLayerTreeView );
  mWidget->setVisible( false );
  connect( mWidget, SIGNAL( requestPick( QgsGuideGridTool::PickMode ) ), this, SLOT( setPickMode( QgsGuideGridTool::PickMode ) ) );
  connect( mWidget, SIGNAL( close() ), this, SLOT( close() ) );

  mActionEditLayer = new QAction( tr( "Edit" ), this );
  mActionEditLayer->setIcon( QIcon( ":/images/themes/default/mIconEditable.png" ) );
  connect( mActionEditLayer, SIGNAL( triggered( bool ) ), this, SLOT( editCurrentLayer( ) ) );

  mLayerTreeView->menuProvider()->addLegendLayerAction( mActionEditLayer, "", "edit_guidegrid_layer", QgsMapLayer::PluginLayer, false );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWasAdded( QgsMapLayer* ) ), this, SLOT( addLayerTreeMenuAction( QgsMapLayer* ) ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( removeLayerTreeMenuAction( QString ) ) );
}

QgsGuideGridTool::~QgsGuideGridTool()
{
  mLayerTreeView->menuProvider()->removeLegendLayerAction( mActionEditLayer );
}

void QgsGuideGridTool::addLayerTreeMenuAction( QgsMapLayer* mapLayer )
{
  if ( dynamic_cast<QgsGuideGridLayer*>( mapLayer ) )
  {
    mLayerTreeView->menuProvider()->addLegendLayerActionForLayer( "edit_guidegrid_layer", mapLayer );
  }
}

void QgsGuideGridTool::removeLayerTreeMenuAction( const QString& mapLayerId )
{
  QgsMapLayer* mapLayer = QgsMapLayerRegistry::instance()->mapLayer( mapLayerId );
  if ( dynamic_cast<QgsGuideGridLayer*>( mapLayer ) )
  {
    mLayerTreeView->menuProvider()->removeLegendLayerActionsForLayer( mapLayer );
  }
}

void QgsGuideGridTool::editCurrentLayer()
{
  if ( dynamic_cast<QgsGuideGridLayer*>( mCanvas->currentLayer() ) )
  {
    mCanvas->setMapTool( this );
  }
}

void QgsGuideGridTool::activate()
{
  // If current layer is a GuideGridLayer, use that one
  if ( dynamic_cast<QgsGuideGridLayer*>( mCanvas->currentLayer() ) )
  {
    mWidget->setLayer( mCanvas->currentLayer() );
  }
  else
  {
    // Search first GuideGridLayer
    bool found = false;
  for ( QgsMapLayer* layer : QgsMapLayerRegistry::instance()->mapLayers().values() )
    {
      if ( dynamic_cast<QgsGuideGridLayer*>( layer ) )
      {
        mWidget->setLayer( layer );
        found = true;
        break;
      }
    }
    // If none found, create a new one
    if ( !found )
    {
      mWidget->createLayer( tr( "Guide grid" ) );
    }
  }
  mWidget->setVisible( true );
  QgsMapTool::activate();
}

void QgsGuideGridTool::deactivate()
{
  mWidget->setVisible( false );
  mPickMode = PICK_NONE;
  setCursor( Qt::ArrowCursor );
  QgsMapTool::deactivate();
}

void QgsGuideGridTool::setPickMode( QgsGuideGridTool::PickMode pickMode )
{
  setCursor( QCursor( pickMode == PICK_NONE ? Qt::ArrowCursor : Qt::CrossCursor ) );
  mPickMode = pickMode;
}

void QgsGuideGridTool::close()
{
  canvas()->unsetMapTool( this );
}

void QgsGuideGridTool::canvasReleaseEvent( QMouseEvent *e )
{
  if ( mPickMode != PICK_NONE )
  {
    mWidget->pointPicked( mPickMode, toMapCoordinates( e->pos() ) );
    mPickMode = PICK_NONE;
    setCursor( Qt::ArrowCursor );
  }
  else if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void QgsGuideGridTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mPickMode != PICK_NONE )
    {
      mPickMode = PICK_NONE;
      setCursor( Qt::ArrowCursor );
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}


static QRegExp g_cooRegExp( "^\\s*(\\d+\\.?\\d*)[,\\s]?\\s*(\\d+\\.?\\d*)\\s*$" );

QgsGuideGridWidget::QgsGuideGridWidget( QgsMapCanvas *canvas, QgsLayerTreeView* layerTreeView )
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

  connect( ui.toolButtonAddLayer, SIGNAL( clicked( bool ) ), this, SLOT( createLayer() ) );
  connect( ui.lineEditTopLeft, SIGNAL( editingFinished() ), this, SLOT( topLeftEdited() ) );
  connect( ui.toolButtonPickTopLeft, SIGNAL( clicked( bool ) ), this, SLOT( pickTopLeftPos() ) );

  connect( ui.lineEditBottomRight, SIGNAL( editingFinished() ), this, SLOT( bottomRightEdited() ) );
  connect( ui.toolButtonPickBottomRight, SIGNAL( clicked( bool ) ), this, SLOT( pickBottomRight() ) );

  ui.spinBoxCols->setRange( 1, 10000 );
  connect( ui.spinBoxCols, SIGNAL( valueChanged( int ) ), this, SLOT( updateIntervals() ) );

  ui.spinBoxRows->setRange( 1, 10000 );
  connect( ui.spinBoxRows, SIGNAL( valueChanged( int ) ), this, SLOT( updateIntervals() ) );

  ui.spinBoxWidth->setRange( 1, 99999999 );
  connect( ui.spinBoxWidth, SIGNAL( valueChanged( double ) ), this, SLOT( updateBottomRight() ) );

  ui.spinBoxHeight->setRange( 1, 99999999 );
  connect( ui.spinBoxHeight, SIGNAL( valueChanged( double ) ), this, SLOT( updateBottomRight() ) );

  connect( ui.toolButtonLockHeight, SIGNAL( toggled( bool ) ), this, SLOT( updateLockIcon( bool ) ) );
  connect( ui.toolButtonLockWidth, SIGNAL( toggled( bool ) ), this, SLOT( updateLockIcon( bool ) ) );

  connect( ui.toolButtonColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( updateColor( QColor ) ) );
  connect( ui.spinBoxFontSize, SIGNAL( valueChanged( int ) ), this, SLOT( updateFontSize( int ) ) );
  connect( ui.comboBoxLabeling, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateLabeling( int ) ) );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( repopulateLayers() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ), this, SLOT( repopulateLayers() ) );

  repopulateLayers();
  connect( ui.comboBoxLayer, SIGNAL( currentIndexChanged( int ) ), this, SLOT( currentLayerChanged( int ) ) );
}

void QgsGuideGridWidget::createLayer( QString layerName )
{
  if ( layerName.isEmpty() )
  {
    layerName = QInputDialog::getText( this, tr( "Layer Name" ), tr( "Enter name of new layer:" ) );
  }
  if ( !layerName.isEmpty() )
  {
    QgsGuideGridLayer* guideGridLayer = new QgsGuideGridLayer( layerName );
    guideGridLayer->setup( mCanvas->extent(), 10, 10, mCanvas->mapSettings().destinationCrs(), false, false );
    QgsMapLayerRegistry::instance()->addMapLayer( guideGridLayer );
    setLayer( guideGridLayer );
  }
}

void QgsGuideGridWidget::setLayer( QgsMapLayer *layer )
{
  mCurrentLayer = dynamic_cast<QgsGuideGridLayer*>( layer );
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

  mCrs = mCurrentLayer->crs();
  int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
  mCurRect = mCurrentLayer->extent();
  ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  ui.spinBoxCols->blockSignals( true );
  ui.spinBoxCols->setValue( mCurrentLayer->cols() );
  ui.spinBoxCols->blockSignals( false );
  ui.spinBoxRows->blockSignals( true );
  ui.spinBoxRows->setValue( mCurrentLayer->rows() );
  ui.spinBoxRows->blockSignals( false );
  ui.toolButtonLockHeight->setChecked( mCurrentLayer->rowSizeLocked() );
  ui.toolButtonLockWidth->setChecked( mCurrentLayer->colSizeLocked() );
  ui.toolButtonColor->setColor( mCurrentLayer->color() );
  ui.spinBoxFontSize->setValue( mCurrentLayer->fontSize() );
  ui.comboBoxLabeling->setCurrentIndex( mCurrentLayer->labelingMode() );
  updateIntervals();
  ui.widgetLayerSetup->setEnabled( true );
}

void QgsGuideGridWidget::pointPicked( QgsGuideGridTool::PickMode pickMode, const QgsPoint& pos )
{
  int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
  if ( pickMode == QgsGuideGridTool::PICK_TOP_LEFT )
  {
    ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( pos.x(), 0, 'f', prec ).arg( pos.y(), 0, 'f', prec ) );
    mCurRect.setXMinimum( pos.x() );
    mCurRect.setYMaximum( pos.y() );
    updateGrid();
  }
  else if ( pickMode == QgsGuideGridTool::PICK_BOTTOM_RIGHT )
  {
    ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( pos.x(), 0, 'f', prec ).arg( pos.y(), 0, 'f', prec ) );
    mCurRect.setXMaximum( pos.x() );
    mCurRect.setYMinimum( pos.y() );
    updateIntervals();
  }
}

void QgsGuideGridWidget::topLeftEdited()
{
  QString text = ui.lineEditTopLeft->text();
  if ( g_cooRegExp.indexIn( text ) != -1 )
  {
    mCurRect.setXMinimum( g_cooRegExp.cap( 1 ).toDouble() );
    mCurRect.setYMaximum( g_cooRegExp.cap( 2 ).toDouble() );
    updateGrid();
  }
  else
  {
    int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
    ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  }
}

void QgsGuideGridWidget::bottomRightEdited()
{
  QString text = ui.lineEditBottomRight->text();
  if ( g_cooRegExp.indexIn( text ) != -1 )
  {
    mCurRect.setXMaximum( g_cooRegExp.cap( 1 ).toDouble() );
    mCurRect.setYMinimum( g_cooRegExp.cap( 2 ).toDouble() );
    updateIntervals();
  }
  else
  {
    int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
    ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  }
}

void QgsGuideGridWidget::updateIntervals()
{
  int cols = ui.spinBoxCols->value();
  int rows = ui.spinBoxRows->value();
  QgsRectangle newRect = mCurRect;
  if ( ui.toolButtonLockWidth->isChecked() )
  {
    newRect.setXMaximum( newRect.xMinimum() + ui.spinBoxWidth->value() * cols );
  }
  else
  {
    ui.spinBoxWidth->blockSignals( true );
    ui.spinBoxWidth->setValue( cols > 0 ? std::abs( mCurRect.width() ) / cols : 0. );
    ui.spinBoxWidth->blockSignals( false );
  }
  if ( ui.toolButtonLockHeight->isChecked() )
  {
    newRect.setYMinimum( newRect.yMaximum() - ui.spinBoxHeight->value() * rows );
  }
  else
  {
    ui.spinBoxHeight->blockSignals( true );
    ui.spinBoxHeight->setValue( rows > 0 ? std::abs( mCurRect.height() ) / rows : 0. );
    ui.spinBoxHeight->blockSignals( false );
  }
  mCurRect = newRect;
  updateGrid();
}

void QgsGuideGridWidget::updateBottomRight()
{
  int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
  mCurRect.setXMaximum( mCurRect.xMinimum() + ui.spinBoxCols->value() * ui.spinBoxWidth->value() );
  mCurRect.setYMinimum( mCurRect.yMaximum() - ui.spinBoxRows->value() * ui.spinBoxHeight->value() );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  updateGrid();
}

void QgsGuideGridWidget::updateLockIcon( bool locked )
{
  QToolButton* button = qobject_cast<QToolButton*>( QObject::sender() );
  button->setIcon( QIcon( locked ? ":/images/themes/default/locked.svg" : ":/images/themes/default/unlocked.svg" ) );
  updateGrid();
}

void QgsGuideGridWidget::updateGrid()
{
  if ( !mCurrentLayer )
  {
    return;
  }
  int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
  mCurRect.normalize();
  ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  mCurrentLayer->setup( mCurRect, ui.spinBoxCols->value(), ui.spinBoxRows->value(), mCrs, ui.toolButtonLockWidth->isChecked(), ui.toolButtonLockHeight->isChecked() );
  mCurrentLayer->triggerRepaint();
}

void QgsGuideGridWidget::updateColor( const QColor& color )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setColor( color );
  mCurrentLayer->triggerRepaint();
}

void QgsGuideGridWidget::updateFontSize( int fontSize )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setFontSize( fontSize );
  mCurrentLayer->triggerRepaint();
}

void QgsGuideGridWidget::updateLabeling( int labelingMode )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setLabelingMode( static_cast<QgsGuideGridLayer::LabellingMode>( labelingMode ) );
  mCurrentLayer->triggerRepaint();
}

void QgsGuideGridWidget::repopulateLayers()
{
  // Avoid update while updating
  if ( ui.comboBoxLayer->signalsBlocked() )
  {
    return;
  }
  ui.comboBoxLayer->blockSignals( true );
  ui.comboBoxLayer->clear();
  int idx = 0, current = 0;
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsGuideGridLayer*>( layer ) )
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

void QgsGuideGridWidget::currentLayerChanged( int cur )
{
  QgsGuideGridLayer* layer = dynamic_cast<QgsGuideGridLayer*>( QgsMapLayerRegistry::instance()->mapLayer( ui.comboBoxLayer->itemData( cur ).toString() ) );
  if ( layer )
  {
    setLayer( layer );
  }
  else
  {
    ui.widgetLayerSetup->setEnabled( false );
  }
}
