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
#include "milix/qgsvbsmilixio.h"
#include "milix/qgsvbsmilixlayer.h"
#include <QAction>
#include <QMainWindow>
#include <QSlider>
#include <QToolBar>

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mActionOvlImport( 0 )
    , mMilXLibrary( 0 )
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

  mSymbolSizeSlider = qobject_cast<QSlider*>( mQGisIface->findObject( "mSymbolSizeSlider" ) );
  mLineWidthSlider = qobject_cast<QSlider*>( mQGisIface->findObject( "mLineWidthSlider" ) );
  if ( mSymbolSizeSlider )
  {
    mSymbolSizeSlider->setValue( QSettings().value( "/vbsfunctionality/milix_symbol_size", "60" ).toInt() );
    setMilXSymbolSize( mSymbolSizeSlider->value() );
    connect( mSymbolSizeSlider, SIGNAL( valueChanged( int ) ), this, SLOT( setMilXSymbolSize( int ) ) );
  }
  if ( mLineWidthSlider )
  {
    mLineWidthSlider->setValue( QSettings().value( "/vbsfunctionality/milix_line_width", "2" ).toInt() );
    setMilXLineWidth( mLineWidthSlider->value() );
    connect( mLineWidthSlider, SIGNAL( valueChanged( int ) ), this, SLOT( setMilXLineWidth( int ) ) );
  }
}

void QgsVBSFunctionality::unload()
{
  delete mActionOvlImport;
  mActionOvlImport = 0;
  mActionMilx = 0;
  mActionSaveMilx = 0;
  mActionLoadMilx = 0;
}

void QgsVBSFunctionality::importOVL()
{
  QgsVBSOvlImporter( mQGisIface, mQGisIface->mainWindow() ).import();
}

void QgsVBSFunctionality::toggleMilXLibrary( )
{
  if ( !mMilXLibrary )
  {
    mMilXLibrary = new QgsVBSMilixLibrary( mQGisIface, mQGisIface->mapCanvas() );
  }
  mMilXLibrary->updateLayers();
  mMilXLibrary->autocreateLayer();
  mMilXLibrary->show();
  mMilXLibrary->raise();
}

void QgsVBSFunctionality::saveMilx()
{
//  QgsVBSMilixIO::save( mQGisIface->messageBar() );
}

void QgsVBSFunctionality::loadMilx()
{
  QgsVBSMilixIO::load( mQGisIface->messageBar() );
}

void QgsVBSFunctionality::setMilXSymbolSize( int value )
{
  VBSMilixClient::setSymbolSize( value );
  mQGisIface->mapCanvas()->refresh();
}

void QgsVBSFunctionality::setMilXLineWidth( int value )
{
  VBSMilixClient::setLineWidth( value );
  mQGisIface->mapCanvas()->refresh();
}
