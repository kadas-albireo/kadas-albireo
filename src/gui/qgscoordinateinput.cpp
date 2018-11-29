/***************************************************************************
 *  qgscoordinateinput.cpp                                                 *
 *  ----------------------                                                 *
 *  begin                : November 2018                                   *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
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

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegExp>

#include "qgscoordinateinput.h"
#include "qgscoordinateformat.h"
#include "qgscrscache.h"
#include "qgsproject.h"

QgsCoordinateInput::QgsCoordinateInput( QWidget *parent )
    : QWidget( parent )
{
  setLayout( new QHBoxLayout() );
  layout()->setMargin( 0 );
  layout()->setSpacing( 0 );

  mLineEdit = new QLineEdit( this );
  layout()->addWidget( mLineEdit );
  connect( mLineEdit, SIGNAL( textEdited( QString ) ), this, SLOT( entryChanged() ) );
  connect( mLineEdit, SIGNAL( editingFinished() ), this, SLOT( entryEdited() ) );

  mCrsCombo = new QComboBox( this );
  mCrsCombo->addItem( "LV03" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::Default, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:21781", sAuthidRole );
  mCrsCombo->addItem( "LV95" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::Default, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:2056", sAuthidRole );
  mCrsCombo->addItem( "DMS" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::DegMinSec, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "DM" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::DegMin, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "DD" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::Default, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "UTM" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::UTM, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "MGRS" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, QgsCoordinateFormat::MGRS, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->setCurrentIndex( QgsProject::instance()->readNumEntry( "crsdisplay", "format" ) );
  layout()->addWidget( mCrsCombo );
  connect( mCrsCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( crsChanged() ) );

  mCrs = QgsCRSCache::instance()->crsByAuthId( mCrsCombo->currentData( sAuthidRole ).toString() );
}

void QgsCoordinateInput::setCoordinate( const QgsPoint &coo, const QgsCoordinateReferenceSystem &crs )
{
  mEmpty = false;
  mCoo = QgsCoordinateTransformCache::instance()->transform( crs.authid(), mCrs.authid() )->transform( coo );
  QgsCoordinateFormat::Format format = static_cast<QgsCoordinateFormat::Format>( mCrsCombo->currentData( sFormatRole ).toInt() );
  QString authId = mCrsCombo->currentData( sAuthidRole ).toString();
  mLineEdit->setText( QgsCoordinateFormat::instance()->getDisplayString( mCoo, mCrs, format, authId ) );
  mLineEdit->setStyleSheet( "" );
  emit coordinateChanged();
}

void QgsCoordinateInput::entryChanged()
{
  mLineEdit->setStyleSheet( "" );
}

void QgsCoordinateInput::entryEdited()
{
  QString text = mLineEdit->text();
  if ( text.isEmpty() )
  {
    mCoo = QgsPoint();
    mEmpty = true;
  }
  else
  {
    QgsCoordinateFormat::Format format = static_cast<QgsCoordinateFormat::Format>( mCrsCombo->currentData( sFormatRole ).toInt() );
    bool valid = false;
    mCoo = QgsCoordinateFormat::instance()->parseCoordinate( text, format, valid );
    mEmpty = !valid;
    if ( !valid )
    {
      mLineEdit->setStyleSheet( "background: #FF7777; color: #FFFFFF;" );
    }
  }
  emit coordinateEdited();
  emit coordinateChanged();
}

void QgsCoordinateInput::crsChanged()
{
  QgsCoordinateFormat::Format format = static_cast<QgsCoordinateFormat::Format>( mCrsCombo->currentData( sFormatRole ).toInt() );
  QString authId = mCrsCombo->currentData( sAuthidRole ).toString();
  if ( !mEmpty )
  {
    mCoo = QgsCoordinateTransformCache::instance()->transform( mCrs.authid(), authId )->transform( mCoo );
  }
  mCrs = QgsCRSCache::instance()->crsByAuthId( authId );
  if ( !mEmpty )
  {
    mLineEdit->setText( QgsCoordinateFormat::instance()->getDisplayString( mCoo, mCrs, format, authId ) );
  }
  else
  {
    mLineEdit->setText( "" );
  }
  mLineEdit->setStyleSheet( "" );
}
