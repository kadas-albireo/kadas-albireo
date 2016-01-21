/***************************************************************************
 *  qgscrsselection.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
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

#include "qgisapp.h"
#include "qgscrsselection.h"
#include "qgsprojectionselectionwidget.h"
#include "qgsgenericprojectionselector.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"
#include "qgisinterface.h"
#include "qgscrscache.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QStatusBar>
#include <QToolButton>

QgsCrsSelection::QgsCrsSelection( QWidget *parent )
    : QToolButton( parent ), mMapCanvas( 0 )
{
  QMenu* crsSelectionMenu = new QMenu( this );
  crsSelectionMenu->addAction( QgsCRSCache::instance()->crsByAuthId( "EPSG:21781" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:21781" );
  crsSelectionMenu->addAction( QgsCRSCache::instance()->crsByAuthId( "EPSG:2056" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:2056" );
  crsSelectionMenu->addAction( QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:4326" );
  crsSelectionMenu->addAction( QgsCRSCache::instance()->crsByAuthId( "EPSG:3857" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:3857" );
  crsSelectionMenu->addSeparator();
  crsSelectionMenu->addAction( tr( "More..." ), this, SLOT( selectMapCrs() ) );

  setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  setMenu( crsSelectionMenu );
  setPopupMode( QToolButton::InstantPopup );
}

void QgsCrsSelection::setMapCanvas( QgsMapCanvas* canvas )
{
  mMapCanvas = canvas;
  if ( mMapCanvas )
  {
    QgsCoordinateReferenceSystem crs( "EPSG:21781" );
    mMapCanvas->setDestinationCrs( crs );
    mMapCanvas->setMapUnits( crs.mapUnits() );
    setText( crs.description() );

    connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncCrsButton() ) );
    connect( QgisApp::instance(), SIGNAL( newProject() ), this, SLOT( syncCrsButton() ) );
  }
}

void QgsCrsSelection::syncCrsButton()
{
  if ( mMapCanvas )
  {
    QString authid = mMapCanvas->mapSettings().destinationCrs().authid();
    setText( QgsCRSCache::instance()->crsByAuthId( authid ).description() );
  }
}

void QgsCrsSelection::selectMapCrs()
{
  if ( !mMapCanvas )
  {
    return;
  }
  QgsProjectionSelectionWidget projSelector;
  projSelector.dialog()->setSelectedAuthId( mMapCanvas->mapSettings().destinationCrs().authid() );
  if ( projSelector.dialog()->exec() != QDialog::Accepted )
  {
    return;
  }
  QgsCoordinateReferenceSystem crs( projSelector.dialog()->selectedAuthId() );
  mMapCanvas->setCrsTransformEnabled( true );
  mMapCanvas->setDestinationCrs( crs );
  mMapCanvas->setMapUnits( crs.mapUnits() );
  setText( crs.description() );
}

void QgsCrsSelection::setMapCrs()
{
  if ( !mMapCanvas )
  {
    return;
  }
  QAction* action = qobject_cast<QAction*>( QObject::sender() );
  QgsCoordinateReferenceSystem crs( action->data().toString() );
  mMapCanvas->setCrsTransformEnabled( true );
  mMapCanvas->setDestinationCrs( crs );
  mMapCanvas->setMapUnits( crs.mapUnits() );
  setText( crs.description() );
}
