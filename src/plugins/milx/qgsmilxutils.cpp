/***************************************************************************
 *  qgsmilxutils.cpp                                                       *
 *  -----------------                                                      *
 *  begin                : August, 2017                                    *
 *  copyright            : (C) 2017 by Sandro Mani / Sourcepole AG         *
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

#include "qgsmilxutils.h"
#include "qgsmilxlayer.h"
#include "qgisinterface.h"
#include "layertree/qgslayertreeview.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QToolButton>

QgsMilXLayerSelectionWidget::QgsMilXLayerSelectionWidget( QgisInterface* iface, QWidget *parent )
    : QWidget( parent ), mIface( iface )
{
  setLayout( new QHBoxLayout() );
  layout()->setSpacing( 2 );
  layout()->setContentsMargins( 0, 0, 0, 0 );

  mLayersCombo = new QComboBox( this );
  mLayersCombo->setFixedWidth( 100 );
  connect( mLayersCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setCurrentLayer( int ) ) );
  layout()->addWidget( mLayersCombo );

  QToolButton* newLayerButton = new QToolButton();
  newLayerButton->setIcon( QIcon( ":/images/themes/default/mActionAdd.png" ) );
  connect( newLayerButton, SIGNAL( clicked( bool ) ), this, SLOT( createLayer() ) );
  layout()->addWidget( newLayerButton );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( repopulateLayers() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ), this, SLOT( repopulateLayers() ) );
  connect( mIface->mapCanvas(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setCurrentLayer( QgsMapLayer* ) ) );

  repopulateLayers();

  // Auto-create layer if necessary
  if ( mLayersCombo->count() == 0 )
  {
    QgsMilXLayer* layer = new QgsMilXLayer( mIface->layerTreeView()->menuProvider() );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    setCurrentLayer( layer );
  }
}

QgsMilXLayer* QgsMilXLayerSelectionWidget::getTargetLayer() const
{
  QString id = mLayersCombo->itemData( mLayersCombo->currentIndex() ).toString();
  return qobject_cast<QgsMilXLayer*>( QgsMapLayerRegistry::instance()->mapLayer( id ) );
}

void QgsMilXLayerSelectionWidget::repopulateLayers()
{
  // Avoid update while updating
  if ( mLayersCombo->signalsBlocked() )
  {
    return;
  }
  mLayersCombo->blockSignals( true );
  mLayersCombo->clear();
  int idx = 0, current = 0;
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsMilXLayer*>( layer ) )
    {
      connect( layer, SIGNAL( layerNameChanged() ), this, SLOT( repopulateLayers() ), Qt::UniqueConnection );
      mLayersCombo->addItem( layer->name(), layer->id() );
      if ( mIface->mapCanvas()->currentLayer() == layer )
      {
        current = idx;
      }
      ++idx;
    }
  }
  mLayersCombo->setCurrentIndex( -1 );
  mLayersCombo->blockSignals( false );
  mLayersCombo->setCurrentIndex( current );
}

void QgsMilXLayerSelectionWidget::setCurrentLayer( int idx )
{
  if ( idx >= 0 )
  {
    QgsMilXLayer* layer = qobject_cast<QgsMilXLayer*>( QgsMapLayerRegistry::instance()->mapLayer( mLayersCombo->itemData( idx ).toString() ) );
    if ( layer && mIface->layerTreeView()->currentLayer() != layer )
    {
      mIface->layerTreeView()->setCurrentLayer( layer );
    }
    emit targetLayerChanged( layer );
  }
  else
  {
    emit targetLayerChanged( 0 );
  }
}

void QgsMilXLayerSelectionWidget::setCurrentLayer( QgsMapLayer *layer )
{
  int idx = layer ? mLayersCombo->findData( layer->id() ) : -1;
  if ( idx >= 0 )
  {
    mLayersCombo->setCurrentIndex( idx );
  }
}

void QgsMilXLayerSelectionWidget::createLayer()
{
  QString layerName = QInputDialog::getText( this, tr( "Layer Name" ), tr( "Enter name of new MilX layer:" ) );
  if ( !layerName.isEmpty() )
  {
    QgsMilXLayer* layer = new QgsMilXLayer( mIface->layerTreeView()->menuProvider(), layerName );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    setCurrentLayer( layer );
  }
}
