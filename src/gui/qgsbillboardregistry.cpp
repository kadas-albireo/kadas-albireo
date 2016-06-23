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
#include <QPainter>

QgsBillBoardRegistry* QgsBillBoardRegistry::instance()
{
  static QgsBillBoardRegistry instance;
  return &instance;
}

void QgsBillBoardRegistry::addItem( void* parent, const QImage &image, const QgsPoint &worldPos , int xoffset, const QString &layerId )
{
  QMap<void*, QgsBillBoardItem*>::iterator it = mItems.find( parent );
  if ( it == mItems.end() )
  {
    it = mItems.insert( parent, new QgsBillBoardItem );
  }
  if ( xoffset == 0 )
  {
    it.value()->image = image;
  }
  else
  {
    QImage newimage( image.width() + 2 * qAbs( xoffset ), image.height(), image.format() );
    newimage.fill( Qt::transparent );
    QPainter p( &newimage );
    p.drawImage( xoffset < 0 ? 0 : 2 * xoffset, 0, image );
    it.value()->image = newimage;
  }
  it.value()->worldPos = worldPos;
  it.value()->layerId = layerId;
  emit itemAdded( it.value() );
}

void QgsBillBoardRegistry::removeItem( void* parent )
{
  emit itemRemoved( mItems[parent] );
  delete mItems.take( parent );
}

QList<QgsBillBoardItem*> QgsBillBoardRegistry::items() const
{
  return mItems.values();
}
