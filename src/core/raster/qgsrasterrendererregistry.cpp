/***************************************************************************
                         qgsrasterrendererregistry.cpp
                         -----------------------------
    begin                : January 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrasterrendererregistry.h"
#include "qgsmultibandcolorrenderer.h"
#include "qgspalettedrasterrenderer.h"
#include "qgssinglebandcolordatarenderer.h"
#include "qgssinglebandgrayrenderer.h"
#include "qgssinglebandpseudocolorrenderer.h"

QgsRasterRendererRegistryEntry::QgsRasterRendererRegistryEntry( const QString& theName, const QString& theVisibleName,
    QgsRasterRendererCreateFunc rendererFunction,
    QgsRasterRendererWidgetCreateFunc widgetFunction ):
    name( theName ), visibleName( theVisibleName ), rendererCreateFunction( rendererFunction ),
    widgetCreateFunction( widgetFunction )
{
}

QgsRasterRendererRegistryEntry::QgsRasterRendererRegistryEntry(): rendererCreateFunction( 0 ), widgetCreateFunction( 0 )
{
}

QgsRasterRendererRegistry* QgsRasterRendererRegistry::mInstance = 0;

QgsRasterRendererRegistry* QgsRasterRendererRegistry::instance()
{
  if ( !mInstance )
  {
    mInstance = new QgsRasterRendererRegistry();
  }
  return mInstance;
}

QgsRasterRendererRegistry::QgsRasterRendererRegistry()
{
  insert( QgsRasterRendererRegistryEntry( "paletted", QObject::tr( "Paletted" ), QgsPalettedRasterRenderer::create, 0 ) );
  insert( QgsRasterRendererRegistryEntry( "multibandcolor", QObject::tr( "Multiband color" ),
                                          QgsMultiBandColorRenderer::create, 0 ) );
  insert( QgsRasterRendererRegistryEntry( "singlebandpseudocolor", QObject::tr( "Singleband pseudocolor" ),
                                          QgsSingleBandPseudoColorRenderer::create, 0 ) );
  insert( QgsRasterRendererRegistryEntry( "singlebandgray", QObject::tr( "Singleband gray" ),
                                          QgsSingleBandGrayRenderer::create, 0 ) );
  insert( QgsRasterRendererRegistryEntry( "singlebandcolordata", QObject::tr( "Singleband color data" ),
                                          QgsSingleBandColorDataRenderer::create, 0 ) );
}

QgsRasterRendererRegistry::~QgsRasterRendererRegistry()
{
}

void QgsRasterRendererRegistry::insert( QgsRasterRendererRegistryEntry entry )
{
  mEntries.insert( entry.name, entry );
}

void QgsRasterRendererRegistry::insertWidgetFunction( const QString& rendererName, QgsRasterRendererWidgetCreateFunc func )
{
  if ( !mEntries.contains( rendererName ) )
  {
    return;
  }
  mEntries[rendererName].widgetCreateFunction = func;
}

bool QgsRasterRendererRegistry::rendererData( const QString& rendererName, QgsRasterRendererRegistryEntry& data ) const
{
  QHash< QString, QgsRasterRendererRegistryEntry >::const_iterator it = mEntries.find( rendererName );
  if ( it == mEntries.constEnd() )
  {
    return false;
  }
  data = it.value();
  return true;
}

QStringList QgsRasterRendererRegistry::renderersList() const
{
  return QStringList( mEntries.keys() );
}

QList< QgsRasterRendererRegistryEntry > QgsRasterRendererRegistry::entries() const
{
  QList< QgsRasterRendererRegistryEntry > result;

  QHash< QString, QgsRasterRendererRegistryEntry >::const_iterator it = mEntries.constBegin();
  for ( ; it != mEntries.constEnd(); ++it )
  {
    result.push_back( it.value() );
  }
  return result;
}


