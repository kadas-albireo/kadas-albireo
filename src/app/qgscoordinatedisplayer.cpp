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
#include "qgscoordinateutils.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"

#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>

QgsCoordinateDisplayer::QgsCoordinateDisplayer( QToolButton* crsButton, QLineEdit* coordLineEdit, QgsMapCanvas* mapCanvas,
    QWidget *parent ) : QWidget( parent ), mMapCanvas( mapCanvas ),
    mCRSSelectionButton( crsButton ), mCoordinateLineEdit( coordLineEdit )
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
  mCRSSelectionButton->setDefaultAction( crsSelectionMenu->actions().front() );

  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );

  connect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
  connect( crsSelectionMenu, SIGNAL( triggered( QAction* ) ), mCoordinateLineEdit, SLOT( clear() ) );
  connect( crsSelectionMenu, SIGNAL( triggered( QAction* ) ), this, SLOT( displayFormatChanged() ) );
  connect( crsSelectionMenu, SIGNAL( triggered( QAction* ) ), mCRSSelectionButton, SLOT( setDefaultAction( QAction* ) ) );

  syncProjectCrs();
}

void QgsCoordinateDisplayer::getCoordinateDisplayFormat( QgsCoordinateUtils::TargetFormat& format, QString& epsg )
{
  QVariant v = mCRSSelectionButton->defaultAction()->data();
  TargetFormat targetFormat = static_cast<TargetFormat>( v.toInt() );
  epsg = "EPSG:4326";
  switch ( targetFormat )
  {
    case LV03:
      format = QgsCoordinateUtils::Default;
      epsg = "EPSG:21781";
      return;
    case LV95:
      format = QgsCoordinateUtils::Default;
      epsg = "EPSG:2056";
      return;
    case DMS:
      format = QgsCoordinateUtils::DegMinSec;
      return;
    case DM:
      format = QgsCoordinateUtils::DegMin;
      return;
    case DD:
      format = QgsCoordinateUtils::DecDeg;
      return;
    case UTM:
      format = QgsCoordinateUtils::UTM;
      return;
    case MGRS:
      format = QgsCoordinateUtils::MGRS;
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
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::Default, "EPSG:21781" );
    case LV95:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::Default, "EPSG:2056" );
    case DMS:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::DegMinSec, "EPSG:4326" );
    case DM:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::DegMin, "EPSG:4326" );
    case DD:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::DecDeg, "EPSG:4326" );
    case UTM:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::UTM, "EPSG:4326" );
    case MGRS:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::MGRS, "EPSG:4326" );
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

void QgsCoordinateDisplayer::displayFormatChanged()
{
  QgsCoordinateUtils::TargetFormat format;
  QString epsg;
  getCoordinateDisplayFormat( format, epsg );
  emit coordinateDisplayFormatChanged( format, epsg );
}
