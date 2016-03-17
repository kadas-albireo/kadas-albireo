/***************************************************************************
 *  qgsiamauth.h                                                           *
 *  ------------                                                           *
 *  begin                : March 2016                                      *
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

#include "qgsiamauth.h"
#include "qgisinterface.h"
#include "iamauth_plugin.h"

QgsIAMAuth::QgsIAMAuth( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
{
}

void QgsIAMAuth::initGui()
{

}

void QgsIAMAuth::unload()
{

}
