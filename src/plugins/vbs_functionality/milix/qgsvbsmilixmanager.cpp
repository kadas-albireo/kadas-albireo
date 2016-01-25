/***************************************************************************
 *  qgsvbsmilixmanager.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : January, 2016                                   *
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

#include "qgsvbsmilixmanager.h"
#include "qgsvbsmilixannotationitem.h"
#include "qgscrscache.h"
#include "qgsmapcanvas.h"
#include "Client/VBSMilixClient.hpp"

QgsVBSMilixManager::QgsVBSMilixManager( QgsMapCanvas *mapCanvas, QObject *parent )
    : QObject( parent ), mMapCanvas( mapCanvas )
{
  connect( mapCanvas, SIGNAL( extentsChanged() ), this, SLOT( updateItems() ) );
}

void QgsVBSMilixManager::addItem( QgsVBSMilixAnnotationItem* item )
{
  mItems.append( item );
  connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( removeItem( QObject* ) ) );
}

void QgsVBSMilixManager::removeItem( QObject *item )
{
  mItems.removeAll( static_cast<QgsVBSMilixAnnotationItem*>( item ) );
}

void QgsVBSMilixManager::updateItems()
{
  QList<VBSMilixClient::NPointSymbol> symbols;
  QList<int> updateMap;
  for ( int i = 0, n = mItems.size(); i < n; ++i )
  {
    if ( mItems[i]->isNPoint() )
    {
      symbols.append( VBSMilixClient::NPointSymbol( mItems[i]->symbolXml(), mItems[i]->points(), mItems[i]->controlPoints(), true ) );
      updateMap.append( i );
    }
  }
  if ( symbols.isEmpty() )
  {
    return;
  }
  QList<VBSMilixClient::NPointSymbolGraphic> result;
  if ( !VBSMilixClient::updateSymbols( mMapCanvas->sceneRect().toRect(), symbols, result ) )
  {
    return;
  }
  for ( int i = 0, n = result.size(); i < n; ++i )
  {
    mItems[updateMap[i]]->setGraphic( result[i], false );
  }
}
