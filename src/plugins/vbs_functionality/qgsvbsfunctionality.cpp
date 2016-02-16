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
#include "milix/qgsvbsmilixlibrary.h"
#include "milix/qgsvbsmilixmanager.h"
#include "milix/qgsvbsmilixio.h"
#include <QAction>
#include <QMainWindow>
#include <QToolBar>

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mActionOvlImport( 0 )
    , mMilXLibrary( 0 )
    , mMilXManager( 0 )
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

  mActionMilx = mQGisIface->findAction( "mActionMilx" );
  connect( mActionMilx, SIGNAL( triggered( ) ), this, SLOT( toggleMilXLibrary( ) ) );

  mActionSaveMilx = mQGisIface->findAction( "mActionSaveMilx" );
  connect( mActionSaveMilx, SIGNAL( triggered( ) ), this, SLOT( saveMilx( ) ) );

  mActionLoadMilx = mQGisIface->findAction( "mActionLoadMilx" );
  connect( mActionLoadMilx, SIGNAL( triggered( ) ), this, SLOT( loadMilx( ) ) );


  mMilXManager = new QgsVBSMilixManager( mQGisIface->mapCanvas(), this );
}

void QgsVBSFunctionality::unload()
{
  delete mActionOvlImport;
  mActionOvlImport = 0;
  mActionMilx = 0;
  mActionSaveMilx = 0;
  mActionLoadMilx = 0;
  delete mMilXManager;
  mMilXManager = 0;
}

void QgsVBSFunctionality::importOVL()
{
  QgsVBSOvlImporter( mQGisIface, mQGisIface->mainWindow() ).import();
}

void QgsVBSFunctionality::toggleMilXLibrary( )
{
  if ( !mMilXLibrary )
  {
    mMilXLibrary = new QgsVBSMilixLibrary( mQGisIface, mMilXManager, mQGisIface->mapCanvas() );
  }
  mMilXLibrary->show();
  mMilXLibrary->raise();
}

void QgsVBSFunctionality::saveMilx()
{
  QgsVBSMilixIO::save( mMilXManager, mQGisIface->messageBar() );
}

void QgsVBSFunctionality::loadMilx()
{
  QgsVBSMilixIO::load( mMilXManager, mQGisIface->messageBar() );
}
