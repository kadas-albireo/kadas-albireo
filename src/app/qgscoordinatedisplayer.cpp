/***************************************************************************
 *  qgscoordinatedisplayer.cpp                                          *
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
#include "qgscoordinatedisplayer.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"

#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>

QgsCoordinateDisplayer::QgsCoordinateDisplayer( QToolButton* crsButton, QLineEdit* coordLineEdit, QToolButton* heightButton, QgsMapCanvas* mapCanvas,
    QWidget *parent ) : QWidget( parent ), mMapCanvas( mapCanvas ),
    mCRSSelectionButton( crsButton ), mCoordinateLineEdit( coordLineEdit ), mHeightSelectionButton( heightButton )
{
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mIconLabel = new QLabel( this );


  QMenu* crsSelectionMenu = new QMenu();
  mCRSSelectionButton->setMenu( crsSelectionMenu );

  mActionDisplayLV03 = crsSelectionMenu->addAction( "LV03" );
  mActionDisplayLV03->setData( static_cast<int>( LV03 ) );
  mActionDisplayLV95 = crsSelectionMenu->addAction( "LV95" );
  mActionDisplayLV95->setData( static_cast<int>( LV95 ) );
  mActionDisplayDMS = crsSelectionMenu->addAction( "DMS" );
  mActionDisplayDMS->setData( static_cast<int>( DMS ) );
  crsSelectionMenu->addAction( "DM" )->setData( static_cast<int>( DM ) );
  crsSelectionMenu->addAction( "DD" )->setData( static_cast<int>( DD ) );
  crsSelectionMenu->addAction( "UTM" )->setData( static_cast<int>( UTM ) );
  crsSelectionMenu->addAction( "MGRS" )->setData( static_cast<int>( MGRS ) );

  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );

  QMenu* heightSelectionMenu = new QMenu();
  mHeightSelectionButton->setMenu( heightSelectionMenu );
  heightSelectionMenu->addAction( "m" )->setData( static_cast<int>( QGis::Meters ) );
  heightSelectionMenu->addAction( "ft" )->setData( static_cast<int>( QGis::Feet ) );

  connect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
  connect( crsSelectionMenu, SIGNAL( triggered( QAction* ) ), this, SLOT( displayFormatChanged( QAction* ) ) );
  connect( heightSelectionMenu, SIGNAL( triggered( QAction* ) ), this, SLOT( heightUnitChanged( QAction* ) ) );

  syncProjectCrs();
  displayFormatChanged( crsSelectionMenu->actions().front() );
  heightUnitChanged( heightSelectionMenu->actions().front() );
}

void QgsCoordinateDisplayer::getCoordinateDisplayFormat( QgsCoordinateFormat::Format& format, QString& epsg )
{
  QVariant v = mCRSSelectionButton->defaultAction()->data();
  TargetFormat targetFormat = static_cast<TargetFormat>( v.toInt() );
  epsg = "EPSG:4326";
  switch ( targetFormat )
  {
    case LV03:
      format = QgsCoordinateFormat::Default;
      epsg = "EPSG:21781";
      return;
    case LV95:
      format = QgsCoordinateFormat::Default;
      epsg = "EPSG:2056";
      return;
    case DMS:
      format = QgsCoordinateFormat::DegMinSec;
      return;
    case DM:
      format = QgsCoordinateFormat::DegMin;
      return;
    case DD:
      format = QgsCoordinateFormat::DecDeg;
      return;
    case UTM:
      format = QgsCoordinateFormat::UTM;
      return;
    case MGRS:
      format = QgsCoordinateFormat::MGRS;
      return;
  }
}

QString QgsCoordinateDisplayer::getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs )
{
  QVariant v = mCRSSelectionButton->defaultAction()->data();
  TargetFormat format = static_cast<TargetFormat>( v.toInt() );
  switch ( format )
  {
    case LV03:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::Default, "EPSG:21781" );
    case LV95:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::Default, "EPSG:2056" );
    case DMS:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::DegMinSec, "EPSG:4326" );
    case DM:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::DegMin, "EPSG:4326" );
    case DD:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::DecDeg, "EPSG:4326" );
    case UTM:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::UTM, "EPSG:4326" );
    case MGRS:
      return QgsCoordinateFormat::getDisplayString( p, crs, QgsCoordinateFormat::MGRS, "EPSG:4326" );
  }
  return QString();
}

void QgsCoordinateDisplayer::displayCoordinates( const QgsPoint &p )
{
  mCoordinateLineEdit->setText( getDisplayString( p, mMapCanvas->mapSettings().destinationCrs() ) );
}

void QgsCoordinateDisplayer::syncProjectCrs()
{
  const QgsCoordinateReferenceSystem& crs = mMapCanvas->mapSettings().destinationCrs();
  if ( crs.srsid() == 4326 )
  {
    mCRSSelectionButton->setDefaultAction( mActionDisplayDMS );
  }
  else if ( crs.srsid() == 21781 )
  {
    mCRSSelectionButton->setDefaultAction( mActionDisplayLV03 );
  }
  else if ( crs.srsid() == 2056 )
  {
    mCRSSelectionButton->setDefaultAction( mActionDisplayLV95 );
  }
}

void QgsCoordinateDisplayer::displayFormatChanged( QAction *action )
{
  mCRSSelectionButton->setDefaultAction( action );
  mCoordinateLineEdit->clear();
  QgsCoordinateFormat::Format format;
  QString epsg;
  getCoordinateDisplayFormat( format, epsg );
  QgsCoordinateFormat::instance()->setCoordinateDisplayFormat( format, epsg );
}

void QgsCoordinateDisplayer::heightUnitChanged( QAction *action )
{
  mHeightSelectionButton->setDefaultAction( action );
  QgsCoordinateFormat::instance()->setHeightDisplayUnit( static_cast<QGis::UnitType>( action->data().toInt() ) );
}
