/***************************************************************************
 *  qgscoordinatedisplay.cpp                                               *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
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

#include "qgscoordinatedisplay.h"
#include "qgscoordinateconverter.h"
#include "qgisinterface.h"
#include "coordinatedisplay_plugin.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QStatusBar>

QgsCoordinateDisplay::QgsCoordinateDisplay( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mContainerWidget( 0 )
    , mCRSSelectionCombo( 0 )
    , mCoordinateLineEdit( 0 )
{
}

template<class T>
static inline QVariant ptr2variant( T* ptr )
{
  return QVariant::fromValue<void*>( reinterpret_cast<void*>( ptr ) );
}

template<class T>
static inline T* variant2ptr( const QVariant& v )
{
  return reinterpret_cast<T*>( v.value<void*>() );
}

void QgsCoordinateDisplay::initGui()
{
  mContainerWidget = new QWidget();
  mContainerWidget->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mCRSSelectionCombo = new QComboBox( mContainerWidget );
  mCRSSelectionCombo->addItem( "LV03", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:21781", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "LV95", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:2056", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DMS", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMinSec, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DM", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMin, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DD", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DecDeg, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "UTM", ptr2variant( new QgsUTMCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "MGRS", ptr2variant( new QgsMGRSCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  mCRSSelectionCombo->setCurrentIndex( 0 );
  mCoordinateLineEdit = new QLineEdit( mContainerWidget );
  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 300 );
  mCoordinateLineEdit->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  QHBoxLayout* layout = new QHBoxLayout( mContainerWidget );
  layout->addWidget( mCRSSelectionCombo );
  layout->addWidget( mCoordinateLineEdit );

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mQGisIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();

  int idx = statusBar->children().indexOf( statusBar->findChild<QLabel*>( "mCoordsLabel" ) );
  statusBar->insertPermanentWidget( idx, mContainerWidget, 0 );

  QLabel* coordsLabel = statusBar->findChild<QLabel*>( "mCoordsLabel" );
  if ( coordsLabel )
    coordsLabel->setVisible( false );
  QLineEdit* coordsEdit = statusBar->findChild<QLineEdit*>( "mCoordsEdit" );
  if ( coordsEdit )
    coordsEdit->setVisible( false );

  connect( mQGisIface->mapCanvas(), SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  connect( mQGisIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), mCoordinateLineEdit, SLOT( clear() ) );

  syncProjectCrs();
}

void QgsCoordinateDisplay::unload()
{
  disconnect( mQGisIface->mapCanvas(), SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  disconnect( mQGisIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mQGisIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();

  statusBar->removeWidget( mContainerWidget );

  QLabel* coordsLabel = statusBar->findChild<QLabel*>( "mCoordsLabel" );
  if ( coordsLabel )
    coordsLabel->setVisible( true );
  QLineEdit* coordsEdit = statusBar->findChild<QLineEdit*>( "mCoordsEdit" );
  if ( coordsEdit )
    coordsEdit->setVisible( true );

  delete mContainerWidget;
}

void QgsCoordinateDisplay::displayCoordinates( const QgsPoint &p )
{
  QVariant v = mCRSSelectionCombo->itemData( mCRSSelectionCombo->currentIndex() );
  QgsCoordinateConverter* conv = variant2ptr<QgsCoordinateConverter>( v );
  if ( conv )
  {
    int dp = qMax( 0, static_cast<int>( ceil( -1.0 * log10( mQGisIface->mapCanvas()->mapUnitsPerPixel() ) ) ) );
    mCoordinateLineEdit->setText( conv->convert( p, mQGisIface->mapCanvas()->mapSettings().destinationCrs(), dp ) );
  }
}

void QgsCoordinateDisplay::syncProjectCrs()
{
  const QgsCoordinateReferenceSystem& crs = mQGisIface->mapCanvas()->mapSettings().destinationCrs();
  if ( crs.srsid() == 4326 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "DMS" ) );
  }
  else if ( crs.srsid() == 21781 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "LV03" ) );
  }
  else if ( crs.srsid() == 2056 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "LV95" ) );
  }
}
