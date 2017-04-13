/***************************************************************************
    qgsitemcouplingmanager.cpp
     --------------------------------------
    Date                 : Apr 2017
    Copyright            : (C) 2017 Sandro Mani / Sourcepole AG
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsitemcouplingmanager.h"
#include "qgsproject.h"
#include "qgsmaplayer.h"
#include "qgsannotationitem.h"
#include "qgsmaplayerregistry.h"
#include "qgisapp.h"


QgsItemCoupling::QgsItemCoupling( QgsMapLayer *layer, QgsAnnotationItem *annotation )
    : mLayer( layer ), mAnnotation( annotation )
{
  connect( layer, SIGNAL( destroyed( QObject* ) ), this, SLOT( layerRemoved() ) );
  connect( annotation, SIGNAL( destroyed( QObject* ) ), this, SLOT( annotationRemoved() ) );
}

void QgsItemCoupling::layerRemoved()
{
  mLayer = nullptr;
  if ( mAnnotation )
  {
    mAnnotation->deleteLater();
    mAnnotation = nullptr;
  }
  emit couplingDissolved();
}

void QgsItemCoupling::annotationRemoved()
{
  mAnnotation = nullptr;
  if ( mLayer )
  {
    QgsMapLayerRegistry::instance()->removeMapLayer( mLayer->id() );
    mLayer = nullptr;
  }
  emit couplingDissolved();
}

QgsItemCouplingManager::~QgsItemCouplingManager()
{
  clear();
}

void QgsItemCouplingManager::addCoupling( QgsMapLayer *layer, QgsAnnotationItem *annotation )
{
  QgsItemCoupling* coupling = new QgsItemCoupling( layer, annotation );
  connect( coupling, SIGNAL( couplingDissolved() ), this, SLOT( removeCoupling() ) );
  mCouplings.append( coupling );
}

void QgsItemCouplingManager::clear()
{
  qDeleteAll( mCouplings );
  mCouplings.clear();
}

void QgsItemCouplingManager::writeXml( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( documentElem.isNull() )
  {
    return;
  }

  QDomElement itemCouplingsElem = doc.createElement( "ItemCouplings" );
  foreach ( const QgsItemCoupling* coupling, mCouplings )
  {
    const QgsMapLayer* layer = coupling->layer();
    const QgsAnnotationItem* annotation = coupling->annotation();
    if ( layer && annotation )
    {
      QDomElement couplingElem = doc.createElement( "Coupling" );
      couplingElem.setAttribute( "layer", layer->id() );
      couplingElem.setAttribute( "annotation", annotation->id() );
      itemCouplingsElem.appendChild( couplingElem );
    }
  }
  documentElem.appendChild( itemCouplingsElem );
}

void QgsItemCouplingManager::readXml( const QDomDocument& doc )
{
  QDomNodeList itemCouplingsElems = doc.elementsByTagName( "ItemCouplings" );
  if ( itemCouplingsElems.isEmpty() )
  {
    return;
  }
  QDomElement itemCouplingsElem = itemCouplingsElems.at( 0 ).toElement();
  QgsMapLayerRegistry* reg = QgsMapLayerRegistry::instance();
  QList<QgsAnnotationItem*> annotationItems = QgisApp::instance()->annotationItems();
  QMap<QString, QgsAnnotationItem*> annotationItemMap;
  foreach ( QgsAnnotationItem* item, annotationItems )
  {
    annotationItemMap.insert( item->id(), item );
  }

  QDomNodeList couplingElems = itemCouplingsElem.elementsByTagName( "Coupling" );
  for ( int i = 0, n = couplingElems.size(); i < n; ++i )
  {
    QDomElement couplingElem = couplingElems.at( i ).toElement();
    QgsMapLayer* layer = reg->mapLayer( couplingElem.attribute( "layer" ) );
    QgsAnnotationItem* annotation = annotationItemMap.value( couplingElem.attribute( "annotation" ), 0 );
    if ( layer && annotation )
    {
      QgsItemCoupling* coupling = new QgsItemCoupling( layer, annotation );
      connect( coupling, SIGNAL( couplingDissolved() ), this, SLOT( removeCoupling() ) );
      mCouplings.append( coupling );
    }
  }
}

void QgsItemCouplingManager::removeCoupling()
{
  QgsItemCoupling* coupling = qobject_cast<QgsItemCoupling*>( QObject::sender() );
  mCouplings.removeOne( coupling );
  coupling->deleteLater();
}
