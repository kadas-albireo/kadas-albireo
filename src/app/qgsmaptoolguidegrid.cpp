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

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QSpinBox>


QgsGuideGridTool::QgsGuideGridTool( QgsMapCanvas* canvas )
    : QgsMapTool( canvas )
{
  mWidget = new QgsGuideGridWidget( canvas );
  mWidget->setVisible( false );
  connect( mWidget, SIGNAL( requestPick( QgsGuideGridTool::PickMode ) ), this, SLOT( setPickMode( QgsGuideGridTool::PickMode ) ) );
  connect( mWidget, SIGNAL( close() ), this, SLOT( close() ) );
}

void QgsGuideGridTool::activate()
{
  mWidget->init();
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
}

void QgsGuideGridTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( e->key() == Qt::Key_Escape && mPickMode != PICK_NONE )
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

QgsGuideGridWidget::QgsGuideGridWidget( QgsMapCanvas *canvas )
    : QgsBottomBar( canvas )
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

  connect( ui.lineEditTopLeft, SIGNAL( editingFinished() ), this, SLOT( topLeftEdited() ) );
  connect( ui.toolButtonPickTopLeft, SIGNAL( clicked( bool ) ), this, SLOT( pickTopLeftPos() ) );

  connect( ui.lineEditBottomRight, SIGNAL( editingFinished() ), this, SLOT( bottomRightEdited() ) );
  connect( ui.toolButtonPickBottomRight, SIGNAL( clicked( bool ) ), this, SLOT( pickBottomRight() ) );

  ui.spinBoxCols->setRange( 0, 10000 );
  connect( ui.spinBoxCols, SIGNAL( valueChanged( int ) ), this, SLOT( updateIntervals() ) );

  ui.spinBoxRows->setRange( 0, 10000 );
  connect( ui.spinBoxRows, SIGNAL( valueChanged( int ) ), this, SLOT( updateIntervals() ) );

  ui.spinBoxWidth->setRange( 0, 99999999 );
  connect( ui.spinBoxWidth, SIGNAL( valueChanged() ), this, SLOT( updateBottomRight() ) );

  ui.spinBoxHeight->setRange( 0, 99999999 );
  connect( ui.spinBoxHeight, SIGNAL( valueChanged() ), this, SLOT( updateBottomRight() ) );

  connect( ui.toolButtonColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( updateColor( QColor ) ) );
  connect( ui.spinBoxFontSize, SIGNAL( valueChanged( int ) ), this, SLOT( updateFontSize( int ) ) );
  connect( ui.comboBoxLabeling, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateLabeling( int ) ) );
}

void QgsGuideGridWidget::init()
{
  mCurRect = mCanvas->extent();
  mCrs = mCanvas->mapSettings().destinationCrs();
  QgsGuideGridLayer* guideGridLayer = getGuideGridLayer();
  int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
  mCrs = guideGridLayer->crs();
  mCurRect = guideGridLayer->extent();
  ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  ui.spinBoxCols->blockSignals( true );
  ui.spinBoxCols->setValue( guideGridLayer->cols() );
  ui.spinBoxCols->blockSignals( false );
  ui.spinBoxRows->blockSignals( true );
  ui.spinBoxRows->setValue( guideGridLayer->rows() );
  ui.spinBoxRows->blockSignals( false );
  ui.toolButtonColor->setColor( guideGridLayer->color() );
  ui.spinBoxFontSize->setValue( guideGridLayer->fontSize() );
  ui.comboBoxLabeling->setCurrentIndex( guideGridLayer->labelingMode() );
  updateIntervals();
}

QgsGuideGridLayer* QgsGuideGridWidget::getGuideGridLayer() const
{
  // Search fpr existing guide grid layer
for ( QgsMapLayer* layer : QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsGuideGridLayer*>( layer ) )
    {
      return static_cast<QgsGuideGridLayer*>( layer );
    }
  }
  QgsGuideGridLayer* guideGridLayer = new QgsGuideGridLayer();
  guideGridLayer->setup( mCurRect, ui.spinBoxCols->value(), ui.spinBoxRows->value(), mCrs );
  QgsMapLayerRegistry::instance()->addMapLayer( guideGridLayer );
  return guideGridLayer;
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
  ui.spinBoxWidth->blockSignals( true );
  ui.spinBoxWidth->setValue( cols > 0 ? std::abs( mCurRect.width() ) / cols : 0. );
  ui.spinBoxWidth->blockSignals( false );
  ui.spinBoxHeight->blockSignals( true );
  ui.spinBoxHeight->setValue( rows > 0 ? std::abs( mCurRect.height() ) / rows : 0. );
  ui.spinBoxHeight->blockSignals( false );
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

void QgsGuideGridWidget::updateGrid()
{
  int prec = mCrs.mapUnits() == QGis::Degrees ? 3 : 0;
  mCurRect.normalize();
  ui.lineEditTopLeft->setText( QString( "%1, %2" ).arg( mCurRect.xMinimum(), 0, 'f', prec ).arg( mCurRect.yMaximum(), 0, 'f', prec ) );
  ui.lineEditBottomRight->setText( QString( "%1, %2" ).arg( mCurRect.xMaximum(), 0, 'f', prec ).arg( mCurRect.yMinimum(), 0, 'f', prec ) );
  QgsGuideGridLayer* layer = getGuideGridLayer();
  layer->setup( mCurRect, ui.spinBoxCols->value(), ui.spinBoxRows->value(), mCrs );
  layer->triggerRepaint();
}

void QgsGuideGridWidget::updateColor( const QColor& color )
{
  QgsGuideGridLayer* layer = getGuideGridLayer();
  layer->setColor( color );
  layer->triggerRepaint();
}

void QgsGuideGridWidget::updateFontSize( int fontSize )
{
  QgsGuideGridLayer* layer = getGuideGridLayer();
  layer->setFontSize( fontSize );
  layer->triggerRepaint();
}

void QgsGuideGridWidget::updateLabeling( int labelingMode )
{
  QgsGuideGridLayer* layer = getGuideGridLayer();
  layer->setLabelingMode( static_cast<QgsGuideGridLayer::LabelingMode>( labelingMode ) );
  layer->triggerRepaint();
}
