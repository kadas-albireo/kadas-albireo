/***************************************************************************
 *  qgsvbsmultimapmanager.cpp                                              *
 *  -------------------                                                    *
 *  begin                : Sep 16, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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

#include "qgsvbsmultimapmanager.h"
#include "qgsvbsmapwidget.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include <QAction>
#include <QMainWindow>
#include <QToolBar>

QgsVBSMultiMapManager::QgsVBSMultiMapManager( QgisInterface *iface, QObject *parent )
    : QObject( parent ), mIface( iface )
{

  mActionAddMapWidget = new QAction( QIcon( ":/vbsfunctionality/icons/add_map_view.svg" ), tr( "Add Map View" ), this );
  mActionAddMapWidget->setToolTip( tr( "Add Map View" ) );
  connect( mActionAddMapWidget, SIGNAL( triggered( bool ) ), this, SLOT( addMapWidget() ) );

  mIface->pluginToolBar()->addAction( mActionAddMapWidget );

  connect( mIface, SIGNAL( newProjectCreated() ), this, SLOT( clearMapWidgets() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProjectSettings( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProjectSettings( QDomDocument& ) ) );
}

QgsVBSMultiMapManager::~QgsVBSMultiMapManager()
{
  clearMapWidgets();
  mIface->pluginToolBar()->removeAction( mActionAddMapWidget );
}

void QgsVBSMultiMapManager::addMapWidget()
{
  int highestNumber = 1;
  foreach ( QgsVBSMapWidget* mapWidget, mMapWidgets )
  {
    if ( mapWidget->getNumber() >= highestNumber )
    {
      highestNumber = mapWidget->getNumber() + 1;
    }
  }

  QgsVBSMapWidget* mapWidget = new QgsVBSMapWidget( highestNumber, tr( "View #%1" ).arg( highestNumber ), mIface );
  mapWidget->setAttribute( Qt::WA_DeleteOnClose );
  connect( mapWidget, SIGNAL( destroyed( QObject* ) ), this, SLOT( mapWidgetDestroyed( QObject* ) ) );

  // Determine whether to add the new dock widget in the right or bottom area
  int nRight = 0, nBottom = 0;
  double rightAreaWidth = 0;
  double bottomAreaHeight = 0;
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mIface->mainWindow() );
  foreach ( QgsVBSMapWidget* mapWidget, mMapWidgets )
  {
    Qt::DockWidgetArea area = mainWindow->dockWidgetArea( mapWidget );
    if ( area == Qt::RightDockWidgetArea )
    {
      ++nRight;
      rightAreaWidth = mapWidget->width();
    }
    else if ( area == Qt::BottomDockWidgetArea )
    {
      ++nBottom;
      bottomAreaHeight = mapWidget->height();
    }
  }
  QSize initialSize;
  Qt::DockWidgetArea addArea;
  if ( nBottom >= nRight - 1 )
  {
    addArea = Qt::RightDockWidgetArea;
    initialSize.setHeight(( mIface->mapCanvas()->height() + bottomAreaHeight ) / ( nRight + 1 ) );
    initialSize.setWidth( nRight > 0 ? rightAreaWidth : mIface->mapCanvas()->width() / 2 );
  }
  else
  {
    addArea = Qt::BottomDockWidgetArea;
    initialSize.setHeight( nBottom > 0 ? bottomAreaHeight : mIface->mapCanvas()->height() / 2 );
    initialSize.setWidth( mIface->mapCanvas()->width() / ( nBottom + 1 ) );
  }

  mapWidget->setFixedSize( initialSize );
  mIface->addDockWidget( addArea, mapWidget );
  mapWidget->resize( initialSize );
  mMapWidgets.append( mapWidget );
}

void QgsVBSMultiMapManager::clearMapWidgets()
{
  qDeleteAll( mMapWidgets );
  mMapWidgets.clear();
}

void QgsVBSMultiMapManager::mapWidgetDestroyed( QObject *mapWidget )
{
  mMapWidgets.removeAll( qobject_cast<QgsVBSMapWidget*>( mapWidget ) );
}

void QgsVBSMultiMapManager::writeProjectSettings( QDomDocument& doc )
{
  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mIface->mainWindow() );

  QDomElement mapViewsElem = doc.createElement( "MapViews" );
  foreach ( QgsVBSMapWidget* mapWidget, mMapWidgets )
  {
    QDomElement mapWidgetItemElem = doc.createElement( "MapView" );
    mapWidgetItemElem.setAttribute( "geometry", QString( mapWidget->saveGeometry().toBase64() ) );
    mapWidgetItemElem.setAttribute( "floating", mapWidget->isFloating() );
    mapWidgetItemElem.setAttribute( "area", mainWindow->dockWidgetArea( mapWidget ) );
    mapWidgetItemElem.setAttribute( "title", mapWidget->windowTitle() );
    mapWidgetItemElem.setAttribute( "number", mapWidget->getNumber() );
    QByteArray layers;
    QDataStream ds( &layers, QIODevice::WriteOnly );
    ds << mapWidget->getLayers();
    mapWidgetItemElem.setAttribute( "layers", QString( layers.toBase64() ) );
    mapViewsElem.appendChild( mapWidgetItemElem );
  }
  qgisElem.appendChild( mapViewsElem );
}

void QgsVBSMultiMapManager::readProjectSettings( const QDomDocument& doc )
{
  clearMapWidgets();

  QDomNodeList nl = doc.elementsByTagName( "MapViews" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }

  QDomElement mapViewsElem = nl.at( 0 ).toElement();
  QDomNodeList nodes = mapViewsElem.childNodes();
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement mapWidgetItemElem = nodes.at( iNode ).toElement();
    if ( mapWidgetItemElem.nodeName() == "MapView" )
    {
      QByteArray layers( QByteArray::fromBase64( mapWidgetItemElem.attribute( "layers" ).toAscii() ) );
      QStringList layersList;
      QDataStream ds( &layers, QIODevice::ReadOnly ); ds >> layersList;
      QgsVBSMapWidget* mapWidget = new QgsVBSMapWidget(
        mapWidgetItemElem.attribute( "number" ).toInt(),
        mapWidgetItemElem.attribute( "title" ),
        mIface );
      mapWidget->setAttribute( Qt::WA_DeleteOnClose );
      mapWidget->setInitialLayers( layersList );
      connect( mapWidget, SIGNAL( destroyed( QObject* ) ), this, SLOT( mapWidgetDestroyed() ) );
      mIface->addDockWidget( static_cast<Qt::DockWidgetArea>( mapWidgetItemElem.attribute( "area" ).toInt() ), mapWidget );
      mapWidget->setFloating( mapWidgetItemElem.attribute( "floating" ).toInt() );
      mapWidget->restoreGeometry( QByteArray::fromBase64( mapWidgetItemElem.attribute( "geometry" ).toAscii() ) );
      mMapWidgets.append( mapWidget );
    }
  }
}
