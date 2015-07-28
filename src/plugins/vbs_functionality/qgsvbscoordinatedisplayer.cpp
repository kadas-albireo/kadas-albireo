/***************************************************************************
 *  qgsvbscoordinatedisplayer.cpp                                          *
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

#include "qgsvbscoordinatedisplayer.h"
#include "qgsvbscoordinateconverter.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"
#include "qgisinterface.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QStatusBar>


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


QgsVBSCoordinateDisplayer::QgsVBSCoordinateDisplayer( QgisInterface *iface, QWidget *parent )
    : QWidget( parent ), mQGisIface( iface )
{
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mIconLabel = new QLabel( this );
  mIconLabel->setPixmap( QPixmap( ":/vbsfunctionality/icons/mousecoordinates.svg" ) );

  mCRSSelectionCombo = new QComboBox( this );
  mCRSSelectionCombo->addItem( "LV03", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:21781", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "LV95", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:2056", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DMS", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMinSec, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DM", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMin, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DD", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DecDeg, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "UTM", ptr2variant( new QgsUTMCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "MGRS", ptr2variant( new QgsMGRSCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  mCRSSelectionCombo->setCurrentIndex( 0 );

  mCoordinateLineEdit = new QLineEdit( this );
  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );
  mCoordinateLineEdit->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  QHBoxLayout* layout = new QHBoxLayout( this );
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 1 );
  layout->addWidget( mIconLabel );
  layout->addWidget( mCRSSelectionCombo );
  layout->addWidget( mCoordinateLineEdit );

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mQGisIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();
  QLabel* coordsLabel = statusBar->findChild<QLabel*>( "mCoordsLabel" );

  statusBar->insertPermanentWidget( statusBar->children().indexOf( coordsLabel ), this, 0 );

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

QgsVBSCoordinateDisplayer::~QgsVBSCoordinateDisplayer()
{
  disconnect( mQGisIface->mapCanvas(), SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  disconnect( mQGisIface->mapCanvas(), SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>( mQGisIface->mainWindow() );
  Q_ASSERT( mainWindow );
  QStatusBar* statusBar = mainWindow->statusBar();

  statusBar->removeWidget( this );

  QLabel* coordsLabel = statusBar->findChild<QLabel*>( "mCoordsLabel" );
  if ( coordsLabel )
    coordsLabel->setVisible( true );
  QLineEdit* coordsEdit = statusBar->findChild<QLineEdit*>( "mCoordsEdit" );
  if ( coordsEdit )
    coordsEdit->setVisible( true );
}

void QgsVBSCoordinateDisplayer::displayCoordinates( const QgsPoint &p )
{
  QVariant v = mCRSSelectionCombo->itemData( mCRSSelectionCombo->currentIndex() );
  QgsVBSCoordinateConverter* conv = variant2ptr<QgsVBSCoordinateConverter>( v );
  if ( conv )
  {
    mCoordinateLineEdit->setText( conv->convert( p, mQGisIface->mapCanvas()->mapSettings().destinationCrs() ) );
  }
}

void QgsVBSCoordinateDisplayer::syncProjectCrs()
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
