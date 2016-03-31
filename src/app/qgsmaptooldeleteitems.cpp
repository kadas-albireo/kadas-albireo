/***************************************************************************
 *  qgsmaptooldeleteitems.cpp                                              *
 *  -------------------                                                    *
 *  begin                : March, 2016                                     *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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
#include "qgscrscache.h"
#include "qgsrendererv2.h"
#include "qgsmapcanvas.h"
#include "qgsmaptooldeleteitems.h"
#include "qgspluginlayer.h"
#include "qgsredlininglayer.h"
#include "qgsvectordataprovider.h"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>


QgsMapToolDeleteItems::QgsMapToolDeleteItems( QgsMapCanvas* mapCanvas )
    : QgsMapToolDrawRectangle( mapCanvas )
{
  connect( this, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
}

void QgsMapToolDeleteItems::drawFinished()
{
  QgsPoint p1, p2;
  getPart( 0, p1, p2 );
  QgsRectangle filterRect( p1, p2 );
  filterRect.normalize();
  if ( filterRect.isEmpty() )
  {
    reset();
    return;
  }
  QgsCoordinateReferenceSystem filterRectCrs = canvas()->mapSettings().destinationCrs();

  // Search annotation items
  QList<QgsAnnotationItem*> delAnnotationItems;
  foreach ( QGraphicsItem* item, canvas()->scene()->items() )
  {
    QgsAnnotationItem* aitem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( aitem && aitem->mapPositionFixed() )
    {
      QgsPoint p = QgsCoordinateTransformCache::instance()->transform( aitem->mapGeoPosCrs().authid(), filterRectCrs.authid() )->transform( aitem->mapGeoPos() );
      if ( filterRect.contains( p ) )
      {
        delAnnotationItems.append( aitem );
      }
    }
  }

  // Search redlining and plugin layers
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas()->mapSettings() );
  QMap<QgsRedliningLayer*, QgsFeatureIds> delRedliningItems;
  QMap<QgsPluginLayer*, QVariantList> delPluginItems;
  foreach ( QgsMapLayer* layer, canvas()->layers() )
  {
    if ( layer->type() == QgsMapLayer::RedliningLayer )
    {
      QgsRedliningLayer* rlayer = static_cast<QgsRedliningLayer*>( layer );

      if ( rlayer->hasScaleBasedVisibility() &&
           ( rlayer->minimumScale() > canvas()->mapSettings().scale() ||
             rlayer->maximumScale() <= canvas()->mapSettings().scale() ) )
      {
        continue;
      }


      QgsFeatureRendererV2* renderer = rlayer->rendererV2();
      bool filteredRendering = false;
      if ( renderer && renderer->capabilities() & QgsFeatureRendererV2::ScaleDependent )
      {
        // setup scale for scale dependent visibility (rule based)
        renderer->startRender( renderContext, rlayer->pendingFields() );
        filteredRendering = renderer->capabilities() & QgsFeatureRendererV2::Filter;
      }

      QgsRectangle layerFilterRect = QgsCoordinateTransformCache::instance()->transform( filterRectCrs.authid(), rlayer->crs().authid() )->transform( filterRect );
      QgsFeatureIterator fit = rlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
      QgsFeature feature;
      while ( fit.nextFeature( feature ) )
      {
        if ( filteredRendering && !renderer->willRenderFeature( feature ) )
        {
          continue;
        }
        delRedliningItems[rlayer].insert( feature.id() );
      }
      if ( renderer && renderer->capabilities() & QgsFeatureRendererV2::ScaleDependent )
      {
        renderer->stopRender( renderContext );
      }
    }
    else if ( layer->type() == QgsMapLayer::PluginLayer )
    {
      QgsPluginLayer* player = static_cast<QgsPluginLayer*>( layer );
      QgsRectangle layerFilterRect = QgsCoordinateTransformCache::instance()->transform( filterRectCrs.authid(), player->crs().authid() )->transform( filterRect );
      QVariantList items = player->getItems( layerFilterRect );
      if ( !items.isEmpty() )
      {
        delPluginItems[player] = items;
      }
    }
  }

  if ( !delAnnotationItems.isEmpty() || !delRedliningItems.isEmpty() || !delPluginItems.isEmpty() )
  {
    QCheckBox* checkBoxAnnotationItems = 0;
    QMap<QgsRedliningLayer*, QCheckBox*> checkBoxRedliningItems;
    QMap<QgsPluginLayer*, QCheckBox*> checkBoxPluginItems;
    QDialog confirmDialog;
    confirmDialog.setWindowTitle( tr( "Delete items" ) );
    confirmDialog.setLayout( new QVBoxLayout() );
    confirmDialog.layout()->addWidget( new QLabel( tr( "Do you want to delete the following items?" ) ) );
    if ( !delAnnotationItems.isEmpty() )
    {
      checkBoxAnnotationItems = new QCheckBox( tr( "%1 annotation item(s)" ).arg( delAnnotationItems.size() ) );
      checkBoxAnnotationItems->setChecked( true );
      confirmDialog.layout()->addWidget( checkBoxAnnotationItems );
    }
    foreach ( QgsRedliningLayer* layer, delRedliningItems.keys() )
    {
      QCheckBox* checkBox = new QCheckBox( tr( "%1 items(s) from layer %2" ).arg( delRedliningItems[layer].size() ).arg( layer->name() ) );
      checkBox->setChecked( true );
      checkBoxRedliningItems[layer] = checkBox;
      confirmDialog.layout()->addWidget( checkBox );
    }
    foreach ( QgsPluginLayer* layer, delPluginItems.keys() )
    {
      QCheckBox* checkBox = new QCheckBox( tr( "%1 items(s) from layer %2" ).arg( delPluginItems[layer].size() ).arg( layer->name() ) );
      checkBox->setChecked( true );
      checkBoxPluginItems[layer] = checkBox;
      confirmDialog.layout()->addWidget( checkBox );
    }
    confirmDialog.layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
    connect( bbox, SIGNAL( accepted() ), &confirmDialog, SLOT( accept() ) );
    connect( bbox, SIGNAL( rejected() ), &confirmDialog, SLOT( reject() ) );
    confirmDialog.layout()->addWidget( bbox );
    if ( confirmDialog.exec() == QDialog::Accepted )
    {
      if ( checkBoxAnnotationItems && checkBoxAnnotationItems->isChecked() )
      {
        qDeleteAll( delAnnotationItems );
      }
      foreach ( QgsRedliningLayer* layer, checkBoxRedliningItems.keys() )
      {
        if ( checkBoxRedliningItems[layer]->isChecked() )
        {
          layer->dataProvider()->deleteFeatures( delRedliningItems[layer] );
        }
        canvas()->clearCache( layer->id() );
      }
      foreach ( QgsPluginLayer* layer, checkBoxPluginItems.keys() )
      {
        if ( checkBoxPluginItems[layer]->isChecked() )
        {
          layer->deleteItems( delPluginItems[layer] );
        }
        canvas()->clearCache( layer->id() );
      }
    }
    canvas()->refresh();
  }
  reset();
}
