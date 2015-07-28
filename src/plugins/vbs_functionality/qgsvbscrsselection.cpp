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
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"
#include "qgisinterface.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QToolButton>

QgsVBSCrsSelection::QgsVBSCrsSelection( QgisInterface *iface, QWidget *parent )
    : QWidget( parent ), mIface( iface )
{
  mIconLabel = new QLabel( this );
  mIconLabel->setPixmap( QPixmap( ":/vbsfunctionality/icons/mapcoordinates.svg" ) );

  mCrsSelectionCombo = new QComboBox( this );
  mCrsSelectionCombo->addItem( "CH1903 LV03", "EPSG:21781" );
  mCrsSelectionCombo->addItem( "CH1903+ LV95", "EPSG:2056" );
  mCrsSelectionCombo->addItem( "WGS84", "EPSG:4326" );
  mCrsSelectionCombo->addItem( "WGS Web Mercator", "EPSG:3857" );
  mCrsSelectionCombo->setCurrentIndex( 0 );

  QHBoxLayout* layout = new QHBoxLayout( this );
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 1 );
  layout->addWidget( mIconLabel );
  layout->addWidget( mCrsSelectionCombo );

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();
#warning "Looks like a compiler bug, below does not work"
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

  connect( mCrsSelectionCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setMapCrs( int ) ) );
  connect( mIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( syncCrsCombo() ) );
  connect( mIface->mapCanvas(), SIGNAL( hasCrsTransformEnabledChanged( bool ) ), this, SLOT( forceCrsTransformEnabled() ) );
  connect( mIface, SIGNAL( newProjectCreated() ), this, SLOT( syncCrsCombo() ) );
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

void QgsVBSCrsSelection::forceCrsTransformEnabled()
{
  mIface->mapCanvas()->setCrsTransformEnabled( true );
}

void QgsVBSCrsSelection::syncCrsCombo()
{
  QString authid = mIface->mapCanvas()->mapSettings().destinationCrs().authid();
  int idx = qMax( 0, mCrsSelectionCombo->findData( authid ) );
  mCrsSelectionCombo->blockSignals( true );
  mCrsSelectionCombo->setCurrentIndex( idx );
  mCrsSelectionCombo->blockSignals( false );
  setMapCrs( mCrsSelectionCombo->currentIndex() );
}

void QgsVBSCrsSelection::setMapCrs( int index )
{
  QString crsid = mCrsSelectionCombo->itemData( index ).toString();
  mIface->mapCanvas()->setCrsTransformEnabled( true );
  QgsCoordinateReferenceSystem crs = QgsCoordinateReferenceSystem( crsid );
  Q_ASSERT( crs.isValid() );
  mIface->mapCanvas()->setDestinationCrs( crs );
}
