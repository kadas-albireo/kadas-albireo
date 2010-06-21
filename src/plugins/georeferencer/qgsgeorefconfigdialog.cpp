/***************************************************************************
     qgsgeorefconfigdialog.cpp
     --------------------------------------
    Date                 : 14-Feb-2010
    Copyright            : (C) 2010 by Jack R, Maxim Dubinin (GIS-Lab)
    Email                : sim@gis-lab.info
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#include <QCloseEvent>
#include <QSettings>

#include "qgsgeorefconfigdialog.h"

QgsGeorefConfigDialog::QgsGeorefConfigDialog( QWidget *parent ) :
    QDialog( parent )
{
  setupUi( this );

  readSettings();
}

void QgsGeorefConfigDialog::changeEvent( QEvent *e )
{
  QDialog::changeEvent( e );
  switch ( e->type() )
  {
    case QEvent::LanguageChange:
      retranslateUi( this );
      break;
    default:
      break;
  }
}

void QgsGeorefConfigDialog::on_buttonBox_accepted()
{
  writeSettings();
  accept();
}

void QgsGeorefConfigDialog::on_buttonBox_rejected()
{
  reject();
}

void QgsGeorefConfigDialog::readSettings()
{
  QSettings s;
  if ( s.value( "/Plugin-GeoReferencer/Config/ShowId" ).toBool() )
  {
    mShowIDsCheckBox->setChecked( true );
  }
  else
  {
    mShowIDsCheckBox->setChecked( false );
  }

  if ( s.value( "/Plugin-GeoReferencer/Config/ShowCoords" ).toBool() )
  {
    mShowCoordsCheckBox->setChecked( true );
  }
  else
  {
    mShowCoordsCheckBox->setChecked( false );
  }

  if ( s.value( "/Plugin-GeoReferencer/Config/ShowDocked" ).toBool() )
  {
    mShowDockedCheckBox->setChecked( true );
  }
  else
  {
    mShowDockedCheckBox->setChecked( false );
  }

  if ( s.value( "/Plugin-GeoReferencer/Config/ResidualUnits" ).toString() == "mapUnits" )
  {
    mMapUnitsButton->setChecked( true );
  }
  else
  {
    mPixelsButton->setChecked( true );
  }

  mLeftMarginSpinBox->setValue( s.value( "/Plugin-GeoReferencer/Config/LeftMarginPDF", "2.0" ).toDouble() );
  mRightMarginSpinBox->setValue( s.value( "/Plugin-GeoReferencer/Config/RightMarginPDF", "2.0" ).toDouble() );
}

void QgsGeorefConfigDialog::writeSettings()
{
  QSettings s;
  s.setValue( "/Plugin-GeoReferencer/Config/ShowId", mShowIDsCheckBox->isChecked() );
  s.setValue( "/Plugin-GeoReferencer/Config/ShowCoords", mShowCoordsCheckBox->isChecked() );
  s.setValue( "/Plugin-GeoReferencer/Config/ShowDocked", mShowDockedCheckBox->isChecked() );
  if ( mPixelsButton->isChecked() )
  {
    s.setValue( "/Plugin-GeoReferencer/Config/ResidualUnits", "pixels" );
  }
  else
  {
    s.setValue( "/Plugin-GeoReferencer/Config/ResidualUnits", "mapUnits" );
  }
  s.setValue( "/Plugin-GeoReferencer/Config/LeftMarginPDF", mLeftMarginSpinBox->value() );
  s.setValue( "/Plugin-GeoReferencer/Config/RightMarginPDF", mRightMarginSpinBox->value() );
}
