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

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

QgsCoordinateDisplayer::QgsCoordinateDisplayer( QComboBox* crsComboBox, QLineEdit* coordLineEdit, QgsMapCanvas* mapCanvas,
    QWidget *parent ) : QWidget( parent ), mMapCanvas( mapCanvas ),
    mCRSSelectionCombo( crsComboBox ), mCoordinateLineEdit( coordLineEdit )
{
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mIconLabel = new QLabel( this );

  mCRSSelectionCombo->addItem( "LV03", static_cast<int>( LV03 ) );
  mCRSSelectionCombo->addItem( "LV95", static_cast<int>( LV95 ) );
  mCRSSelectionCombo->addItem( "DMS", static_cast<int>( DMS ) );
  mCRSSelectionCombo->addItem( "DM", static_cast<int>( DM ) );
  mCRSSelectionCombo->addItem( "DD", static_cast<int>( DD ) );
  mCRSSelectionCombo->addItem( "UTM", static_cast<int>( UTM ) );
  mCRSSelectionCombo->addItem( "MGRS", static_cast<int>( MGRS ) );
  mCRSSelectionCombo->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  mCRSSelectionCombo->setCurrentIndex( 0 );

  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );
  mCoordinateLineEdit->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  connect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), mCoordinateLineEdit, SLOT( clear() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( displayFormatChanged() ) );

  syncProjectCrs();
}

void QgsCoordinateDisplayer::getCoordinateDisplayFormat( QgsCoordinateUtils::TargetFormat& format, QString& epsg )
{
  QVariant v = mCRSSelectionCombo->itemData( mCRSSelectionCombo->currentIndex() );
  TargetFormat targetFormat = static_cast<TargetFormat>( v.toInt() );
  switch ( targetFormat )
  {
    case LV03:
      format = QgsCoordinateUtils::EPSG;
      epsg = "EPSG:21781";
      return;
    case LV95:
      format = QgsCoordinateUtils::EPSG;
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
  QVariant v = mCRSSelectionCombo->itemData( mCRSSelectionCombo->currentIndex() );
  TargetFormat format = static_cast<TargetFormat>( v.toInt() );
  switch ( format )
  {
    case LV03:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::EPSG, "EPSG:21781" );
    case LV95:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::EPSG, "EPSG:2056" );
    case DMS:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::DegMinSec );
    case DM:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::DegMin );
    case DD:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::DecDeg );
    case UTM:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::UTM );
    case MGRS:
      return QgsCoordinateUtils::getDisplayString( p, crs, QgsCoordinateUtils::MGRS );
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

void QgsCoordinateDisplayer::displayFormatChanged()
{
  QgsCoordinateUtils::TargetFormat format;
  QString epsg;
  getCoordinateDisplayFormat( format, epsg );
  emit coordinateDisplayFormatChanged( format, epsg );
}
