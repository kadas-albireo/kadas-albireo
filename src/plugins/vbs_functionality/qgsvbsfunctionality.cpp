/***************************************************************************
 *  qgsvbsfunctionality.cpp                                                *
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

#include "qgsvbsfunctionality.h"
#include "qgsvbscoordinatedisplayer.h"
#include "qgsvbscrsselection.h"
#include "qgisinterface.h"
#include "vbsfunctionality_plugin.h"

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mCoordinateDisplayer( 0 )
{
}

void QgsVBSFunctionality::initGui()
{
  mCoordinateDisplayer = new QgsVBSCoordinateDisplayer( mQGisIface, mQGisIface->mainWindow() );
}

void QgsVBSFunctionality::unload()
{
  delete mCoordinateDisplayer;
  mCoordinateDisplayer = 0;
}
