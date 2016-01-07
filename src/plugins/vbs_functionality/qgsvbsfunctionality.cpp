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

#include "qgslegendinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessagebaritem.h"
#include "qgsvbsfunctionality.h"
#include "qgisinterface.h"
#include "ovl/qgsvbsovlimporter.h"
#include "vbsfunctionality_plugin.h"
#include <QAction>
#include <QToolBar>

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mActionOvlImport( 0 )
{
}

void QgsVBSFunctionality::initGui()
{
  mActionOvlImport = new QAction( QIcon( ":/vbsfunctionality/icons/ovl.svg" ), tr( "Import ovl" ), this );
  connect( mActionOvlImport, SIGNAL( triggered( bool ) ), this, SLOT( importOVL() ) );
  if ( mQGisIface->pluginToolBar() )
  {
    mQGisIface->pluginToolBar()->addAction( mActionOvlImport );
  }
}

void QgsVBSFunctionality::unload()
{
  delete mActionOvlImport;
  mActionOvlImport = 0;
}

void QgsVBSFunctionality::importOVL()
{
  QgsVBSOvlImporter( mQGisIface, mQGisIface->mainWindow() ).import();
}
