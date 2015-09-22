/***************************************************************************
 *  qgsvbscrsselection.cpp                                                 *
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

#include "qgsvbscrsselection.h"
#include "qgsprojectionselectionwidget.h"
#include "qgsgenericprojectionselector.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"
#include "qgisinterface.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QStatusBar>
#include <QToolButton>

QgsVBSCrsSelection::QgsVBSCrsSelection( QgisInterface *iface, QWidget *parent )
    : QToolButton( parent ), mIface( iface )
{
  QMenu* crsSelectionMenu = new QMenu( this );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:21781" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:21781" );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:2056" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:2056" );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:4326" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:4326" );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:3857" ).description(), this, SLOT( setMapCrs() ) )->setData( "EPSG:3857" );
  crsSelectionMenu->addSeparator();
  crsSelectionMenu->addAction( tr( "More..." ), this, SLOT( selectMapCrs() ) );

  setIcon( QIcon( ":/vbsfunctionality/icons/mapcoordinates.svg" ) );
  setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  setMenu( crsSelectionMenu );
  setPopupMode( QToolButton::InstantPopup );

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();
  // Looks like a compiler bug, below does not work
  //  QWidget* otfProjButton = statusBar->findChild<QWidget*>( "mOnTheFlyProjectionStatusButton" );
  QToolButton* otfProjButton = 0;
  foreach ( QObject* child, statusBar->children() )
  {
    if ( child->objectName() == "mOntheFlyProjectionStatusButton" )
    {
      otfProjButton = qobject_cast<QToolButton*>( child );
      break;
    }
  }

  statusBar->insertPermanentWidget( statusBar->children().indexOf( otfProjButton ), this, 0 );

  if ( otfProjButton )
    otfProjButton->setVisible( false );

  QgsCoordinateReferenceSystem crs( "EPSG:217432681" );
  mIface->mapCanvas()->setDestinationCrs( crs );
  setText( crs.description() );

  connect( mIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( syncCrsButton() ) );
  connect( mIface, SIGNAL( newProjectCreated() ), this, SLOT( syncCrsButton() ) );

}

QgsVBSCrsSelection::~QgsVBSCrsSelection()
{
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();

  statusBar->removeWidget( this );

  QToolButton* otfProjButton = statusBar->findChild<QToolButton*>( "mOnTheFlyProjectionStatusButton" );
  if ( otfProjButton )
    otfProjButton->setVisible( true );
}

void QgsVBSCrsSelection::syncCrsButton()
{
  QString authid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
  setText( authid );
}

void QgsVBSCrsSelection::selectMapCrs()
{
  QgsProjectionSelectionWidget projSelector;
  projSelector.dialog()->setSelectedAuthId( mIface->mapCanvas()->mapSettings().destinationCrs().authid() );
  if ( projSelector.dialog()->exec() != QDialog::Accepted )
  {
    return;
  }
  QgsCoordinateReferenceSystem crs( projSelector.dialog()->selectedAuthId() );
  mIface->mapCanvas()->setCrsTransformEnabled( true );
  mIface->mapCanvas()->setDestinationCrs( crs );
  setText( crs.description() );
}

void QgsVBSCrsSelection::setMapCrs()
{
  QAction* action = qobject_cast<QAction*>( QObject::sender() );
  QgsCoordinateReferenceSystem crs( action->data().toString() );
  mIface->mapCanvas()->setCrsTransformEnabled( true );
  mIface->mapCanvas()->setDestinationCrs( crs );
  setText( crs.description() );
}
