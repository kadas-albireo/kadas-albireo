/***************************************************************************
 *  qgsvbsmilixio.h                                                        *
 *  -------------------                                                    *
 *  begin                : February 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#include "qgsvbsmilixio.h"
#include <QFileDialog>
#include <QSettings>

bool QgsVBSMilixIO::save( QgsVBSMilixManager* manager )
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString selectedFilter;
  QStringList filters = QStringList() << tr( "MilX Layer (*.milxly)" ) << tr( "Compressed MilX Layer (*.milxlyz)" );
  QString filename = QFileDialog::getSaveFileName( 0, tr( "Select Output" ), lastProjectDir, filters.join( ";;" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return false;
  }

}

bool QgsVBSMilixIO::load( QgsVBSMilixManager* manager )
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QStringList filters = QStringList() << tr( "MilX Layer (*.milxly)" ) << tr( "Compressed MilX Layer (*.milxlyz)" );
  QString filename = QFileDialog::getOpenFileName( 0, tr( "Select Milx Layer File" ), lastProjectDir, filters.join( ";;" ) );
  if ( filename.isEmpty() )
  {
    return false;
  }

}
