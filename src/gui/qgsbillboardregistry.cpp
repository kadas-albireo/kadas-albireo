/***************************************************************************
                              qgsbillboardregistry.cpp
                              ------------------------
  begin                : February 2016
  copyright            : (C) 2016 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsbillboardregistry.h"

QgsBillBoardRegistry* QgsBillBoardRegistry::instance()
{
  static QgsBillBoardRegistry instance;
  return &instance;
}

void QgsBillBoardRegistry::addItem( void* parent, const QImage &image, const QgsPoint &worldPos )
{
  QMap<void*, QgsBillBoardItem*>::iterator it = mItems.find( parent );
  if ( it == mItems.end() )
  {
    it = mItems.insert( parent, new QgsBillBoardItem );
  }
  it.value()->image = image;
  it.value()->worldPos = worldPos;
  emit itemAdded( it.value() );
}

void QgsBillBoardRegistry::removeItem( void* parent )
{
  emit itemRemoved( mItems[parent] );
  delete mItems.take( parent );
}
