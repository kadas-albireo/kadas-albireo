/***************************************************************************
                          qgsannotationlayer.cpp
                             -------------------
  begin                : August 2017
  copyright            : (C) 2017 by Sandro Mani
  email                : manisandro@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsannotationlayer.h"
#include "qgscrscache.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsannotationitem.h"

QgsAnnotationLayer* QgsAnnotationLayer::getLayer( QgsMapCanvas* canvas, const QString& itemType, const QString& layerTitle )
{
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsAnnotationLayer*>( layer ) && static_cast<QgsAnnotationLayer*>( layer )->itemType() == itemType )
    {
      return static_cast<QgsAnnotationLayer*>( layer );
    }
  }
  QgsAnnotationLayer* layer = new QgsAnnotationLayer( canvas, itemType );
  layer->setLayerName( layerTitle );
  QgsMapLayerRegistry::instance()->addMapLayer( layer );
  return layer;
}

QgsAnnotationLayer::QgsAnnotationLayer( QgsMapCanvas *canvas, const QString &itemType )
    : QgsPluginLayer( layerTypeKey() ), mCanvas( canvas ), mItemType( itemType )
{
  mValid = true;
  // This is actually irrelevant, but proj errors are emitted if the crs is empty
  setCrs( QgsCoordinateReferenceSystem( "EPSG:3857" ), false );
  connect( mCanvas, SIGNAL( layersChanged( QStringList ) ), this, SLOT( checkLayerVisibility( ) ) );
}

QgsAnnotationLayer::~QgsAnnotationLayer()
{
  foreach ( QGraphicsItem* item, mCanvas->items() )
  {
    QgsAnnotationItem* annotationItem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( annotationItem && mItemIds.contains( annotationItem->id() ) )
    {
      delete annotationItem;
    }
  }
}

void QgsAnnotationLayer::addItem( QgsAnnotationItem *item )
{
  item->setLayerId( id() );
  mItemIds.insert( item->id() );
}

QgsRectangle QgsAnnotationLayer::extent()
{
  QgsRectangle extent;
  bool empty = true;
  foreach ( QGraphicsItem* item, mCanvas->items() )
  {
    QgsAnnotationItem* annotationItem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( annotationItem && mItemIds.contains( annotationItem->id() ) )
    {
      if ( empty )
      {
        extent = QgsRectangle( annotationItem->mapPosition(), annotationItem->mapPosition() );
        empty = false;
      }
      else
      {
        extent.include( annotationItem->mapPosition() );
      }
    }
  }
  const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mCanvas->mapSettings().destinationCrs().authid(), crs().authid() );
  return t->transformBoundingBox( extent );
}

int QgsAnnotationLayer::margin() const
{
  int margin = 0;
  foreach ( QGraphicsItem* item, mCanvas->items() )
  {
    QgsAnnotationItem* annotationItem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( annotationItem && mItemIds.contains( annotationItem->id() ) )
    {
      QRectF rect = annotationItem->boundingRect();
      margin = qMax( margin, qMax( qCeil( rect.width() ), qCeil( rect.height() ) ) );
    }
  }
  return margin;
}

void QgsAnnotationLayer::setLayerTransparency( int value )
{
  QgsPluginLayer::setLayerTransparency( value );
  foreach ( QGraphicsItem* item, mCanvas->items() )
  {
    QgsAnnotationItem* annotationItem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( annotationItem && mItemIds.contains( annotationItem->id() ) )
    {
      annotationItem->setOpacity(( 100. - mTransparency ) / 100. );
    }
  }
}

void QgsAnnotationLayer::checkLayerVisibility()
{
  bool visible = mCanvas->layers().contains( this );
  foreach ( QGraphicsItem* item, mCanvas->items() )
  {
    QgsAnnotationItem* annotationItem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( annotationItem && mItemIds.contains( annotationItem->id() ) )
    {
      annotationItem->setVisible( visible );
    }
  }
}

bool QgsAnnotationLayer::readXml( const QDomNode& layer_node )
{
  QDomElement annotationLayerEl = layer_node.firstChildElement( "AnnotationLayer" );
  if ( !annotationLayerEl.isNull() )
  {
    mItemType = annotationLayerEl.attribute( "itemType" );
    QDomNodeList annotationItemEls = annotationLayerEl.elementsByTagName( "AnnotationItem" );
    for ( int i = 0, n = annotationItemEls.count(); i < n; ++i )
    {
      QDomElement annotationItemEl = annotationItemEls.at( i ).toElement();
      mItemIds.insert( annotationItemEl.attribute( "id" ) );
    }
  }
  return true;
}

bool QgsAnnotationLayer::writeXml( QDomNode & layer_node, QDomDocument & document )
{
  QDomElement layerElement = layer_node.toElement();
  layerElement.setAttribute( "type", "plugin" );
  layerElement.setAttribute( "name", layerTypeKey() );
  QDomElement annotationLayerEl = document.createElement( "AnnotationLayer" );
  annotationLayerEl.setAttribute( "itemType", mItemType );
  layerElement.appendChild( annotationLayerEl );

  // Only write ids of items which actually still exists
  foreach ( QGraphicsItem* item, mCanvas->items() )
  {
    QgsAnnotationItem* annotationItem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( annotationItem && mItemIds.contains( annotationItem->id() ) )
    {
      QDomElement annotationItemEl = document.createElement( "AnnotationItem" );
      annotationItemEl.setAttribute( "id", annotationItem->id() );
      annotationLayerEl.appendChild( annotationItemEl );
    }
  }
  return true;
}
