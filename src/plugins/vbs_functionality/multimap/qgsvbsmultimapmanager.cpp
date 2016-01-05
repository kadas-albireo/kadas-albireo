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
#include "layertree/qgslayertreegroup.h"
#include "layertree/qgslayertreelayer.h"
#include <QAction>
#include <QMainWindow>
#include <QToolBar>

QgsVBSMultiMapManager::QgsVBSMultiMapManager( QgisInterface *iface, QObject *parent )
    : QObject( parent ), mIface( iface )
{

  mActionAddMapWidget = new QAction( QIcon( ":/vbsfunctionality/icons/add_map_view.svg" ), tr( "Add Map View" ), this );
  mActionAddMapWidget->setToolTip( tr( "Add Map View" ) );
  connect( mActionAddMapWidget, SIGNAL( triggered( bool ) ), this, SLOT( addMapWidget() ) );

  if ( mIface->pluginToolBar() )
  {
    mIface->pluginToolBar()->addAction( mActionAddMapWidget );
  }

  connect( mIface, SIGNAL( newProjectCreated() ), this, SLOT( clearMapWidgets() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProjectSettings( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProjectSettings( QDomDocument& ) ) );
}

QgsVBSMultiMapManager::~QgsVBSMultiMapManager()
{
  clearMapWidgets();
  if ( mIface->pluginToolBar() )
    mIface->pluginToolBar()->removeAction( mActionAddMapWidget );
}

void QgsVBSMultiMapManager::addMapWidget()
{
  int highestNumber = 1;
  foreach ( const QPointer<QgsVBSMapWidget>& mapWidget, mMapWidgets )
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
  foreach ( const QPointer<QgsVBSMapWidget>& mapWidget, mMapWidgets )
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

  // Set initial layers
  QStringList initialLayers;
  foreach ( QgsLayerTreeLayer* layerTreeLayer, QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    initialLayers.append( layerTreeLayer->layer()->id() );
  }
  mapWidget->setInitialLayers( initialLayers, true );

  mapWidget->setFixedSize( initialSize );
  mIface->addDockWidget( addArea, mapWidget );
  mapWidget->resize( initialSize );
  mMapWidgets.append( QPointer<QgsVBSMapWidget>( mapWidget ) );
}

void QgsVBSMultiMapManager::clearMapWidgets()
{
  foreach ( const QPointer<QgsVBSMapWidget>& mapWidget, mMapWidgets )
  {
    delete mapWidget.data();
  }
  mMapWidgets.clear();
}

void QgsVBSMultiMapManager::mapWidgetDestroyed( QObject */*mapWidget*/ )
{
  mMapWidgets.removeAll( QPointer<QgsVBSMapWidget>( 0 ) );
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
    QByteArray ba;
    QDataStream ds( &ba, QIODevice::WriteOnly );

    QDomElement mapWidgetItemElem = doc.createElement( "MapView" );
    mapWidgetItemElem.setAttribute( "width", mapWidget->width() );
    mapWidgetItemElem.setAttribute( "height", mapWidget->height() );
    mapWidgetItemElem.setAttribute( "floating", mapWidget->isFloating() );
    mapWidgetItemElem.setAttribute( "islocked", mapWidget->getLocked() );
    mapWidgetItemElem.setAttribute( "area", mainWindow->dockWidgetArea( mapWidget ) );
    mapWidgetItemElem.setAttribute( "title", mapWidget->windowTitle() );
    mapWidgetItemElem.setAttribute( "number", mapWidget->getNumber() );
    ds << mapWidget->getLayers();
    mapWidgetItemElem.setAttribute( "layers", QString( ba.toBase64() ) );
    ba.clear();
    ds << mapWidget->getMapExtent().toRectF();
    mapWidgetItemElem.setAttribute( "extent", QString( ba.toBase64() ) );
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
      QDomNamedNodeMap attributes = mapWidgetItemElem.attributes();
      QByteArray ba;
      QDataStream ds( &ba, QIODevice::ReadOnly );
      QgsVBSMapWidget* mapWidget = new QgsVBSMapWidget(
        attributes.namedItem( "number" ).nodeValue().toInt(),
        attributes.namedItem( "title" ).nodeValue(),
        mIface );
      mapWidget->setAttribute( Qt::WA_DeleteOnClose );
      ba = QByteArray::fromBase64( attributes.namedItem( "layers" ).nodeValue().toAscii() );
      QStringList layersList;
      ds >> layersList;
      mapWidget->setInitialLayers( layersList );
      connect( mapWidget, SIGNAL( destroyed( QObject* ) ), this, SLOT( mapWidgetDestroyed( QObject* ) ) );
      // Compiler bug?! If I pass it directly, value is always false
      bool islocked = attributes.namedItem( "islocked" ).nodeValue().toInt();
      mapWidget->setLocked( islocked );
      mapWidget->setFloating( attributes.namedItem( "floating" ).nodeValue().toInt() );
      mapWidget->setFixedWidth( attributes.namedItem( "width" ).nodeValue().toInt() );
      mapWidget->setFixedHeight( attributes.namedItem( "height" ).nodeValue().toInt() );
      ba = QByteArray::fromBase64( attributes.namedItem( "extent" ).nodeValue().toAscii() );
      QRectF extent;
      ds >> extent;
      mapWidget->setMapExtent( QgsRectangle( extent ) );
      mIface->addDockWidget( static_cast<Qt::DockWidgetArea>( attributes.namedItem( "area" ).nodeValue().toInt() ), mapWidget );
      mMapWidgets.append( mapWidget );
    }
  }
}
