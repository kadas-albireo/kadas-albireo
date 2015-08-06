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

#include "qgsmapcanvas.h"
#include "qgsvbsfunctionality.h"
#include "qgsvbscoordinatedisplayer.h"
#include "qgsvbscrsselection.h"
#include "qgisinterface.h"
#include "maptools/qgsvbsmaptoolpinannotation.h"
#include "search/qgsvbssearchbox.h"
#include "vbsfunctionality_plugin.h"
#include <QAction>
#include <QToolBar>

QgsVBSFunctionality::QgsVBSFunctionality( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface )
    , mCoordinateDisplayer( 0 )
    , mCrsSelection( 0 )
    , mActionPinAnnotation( 0 )
    , mMapToolPinAnnotation( 0 )
    , mSearchToolbar( 0 )
    , mSearchBox( 0 )
{
}

void QgsVBSFunctionality::initGui()
{
  mCoordinateDisplayer = new QgsVBSCoordinateDisplayer( mQGisIface, mQGisIface->mainWindow() );
  mCrsSelection = new QgsVBSCrsSelection( mQGisIface, mQGisIface->mainWindow() );
  mMapToolPinAnnotation = new QgsVBSMapToolPinAnnotation( mQGisIface->mapCanvas() );
  mActionPinAnnotation = new QAction( QIcon( ":/vbsfunctionality/icons/pin_red.svg" ), tr( "Add pin" ), this );
  mActionPinAnnotation->setCheckable( true );
  mMapToolPinAnnotation->setAction( mActionPinAnnotation );
  connect( mActionPinAnnotation, SIGNAL( triggered() ), this, SLOT( activateMapToolPinAnnotation() ) );
  mQGisIface->pluginToolBar()->addAction( mActionPinAnnotation );

  mSearchToolbar = mQGisIface->addToolBar( "vbsSearchToolbar" );
  mSearchBox = new QgsVBSSearchBox( mQGisIface, mSearchToolbar );
  mSearchToolbar->addWidget( mSearchBox );

  QWidget* layerTreeToolbar = mQGisIface->mainWindow()->findChild<QWidget*>( "layerTreeToolbar" );
  if ( layerTreeToolbar ) layerTreeToolbar->setVisible( false );
}

void QgsVBSFunctionality::unload()
{
  delete mCoordinateDisplayer;
  mCoordinateDisplayer = 0;
  delete mCrsSelection;
  mCrsSelection = 0;
  delete mActionPinAnnotation;
  mActionPinAnnotation = 0;
  delete mMapToolPinAnnotation;
  mMapToolPinAnnotation = 0;
  delete mSearchToolbar;
  mSearchToolbar = 0;
  mSearchBox = 0;

  QWidget* layerTreeToolbar = mQGisIface->mainWindow()->findChild<QWidget*>( "layerTreeToolbar" );
  if ( layerTreeToolbar ) layerTreeToolbar->setVisible( true );
}

void QgsVBSFunctionality::activateMapToolPinAnnotation()
{
  mQGisIface->mapCanvas()->setMapTool( mMapToolPinAnnotation );
}
