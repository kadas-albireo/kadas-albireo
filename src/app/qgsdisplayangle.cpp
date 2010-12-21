/***************************************************************************
    qgsdisplayangle.cpp
    ------------------------
    begin                : January 2010
    copyright            : (C) 2010 by Marco Hugentobler
    email                : marco at hugis dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdisplayangle.h"
#include <QSettings>
#include <cmath>

QgsDisplayAngle::QgsDisplayAngle( QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f )
{
  setupUi( this );
  QSettings settings;
  int s = settings.value( "/qgis/measure/projectionEnabled", "2" ).toInt();
  if ( s == 2 )
    mcbProjectionEnabled->setCheckState( Qt::Checked );
  else
    mcbProjectionEnabled->setCheckState( Qt::Unchecked );

  connect( mcbProjectionEnabled, SIGNAL( stateChanged(int) ),
      this, SLOT( changeState() ) );
  connect( mcbProjectionEnabled, SIGNAL( stateChanged(int) ), 
      this, SIGNAL( changeProjectionEnabledState() ) );
}

QgsDisplayAngle::~QgsDisplayAngle()
{

}

bool QgsDisplayAngle::projectionEnabled()
{
  return mcbProjectionEnabled->isChecked();
}

void QgsDisplayAngle::setValueInRadians( double value )
{
  QSettings settings;
  QString unitString = settings.value( "/qgis/measure/angleunits", "degrees" ).toString();
  if ( unitString == "degrees" )
  {
    mAngleLineEdit->setText( tr( "%1 degrees" ).arg( value * 180 / M_PI ) );
  }
  else if ( unitString == "radians" )
  {
    mAngleLineEdit->setText( tr( "%1 radians" ).arg( value ) );
  }
  else if ( unitString == "gon" )
  {
    mAngleLineEdit->setText( tr( "%1 gon" ).arg( value / M_PI * 200 ) );
  }
}

void QgsDisplayAngle::changeState()
{
  QSettings settings;
  if ( mcbProjectionEnabled->isChecked() )
    settings.setValue( "/qgis/measure/projectionEnabled", 2);
  else
    settings.setValue( "/qgis/measure/projectionEnabled", 0);
}
