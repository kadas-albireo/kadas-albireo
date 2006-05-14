/***************************************************************************
                          qgsspitplugin.cpp 
 Shapefile to PostgreSQL Import Tool plugin 
                             -------------------
    begin                : Jan 30, 2004
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
  
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /*  $Id$ */

// includes
#include <iostream>
#include <vector>

#include <QAction>
#include <QMenu>

#include "qgisapp.h"
#include "qgsspitplugin.h"
#include "qgsspit.h"
// xpm for creating the toolbar icon
#include "spiticon.xpm"

#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif

static const char * const ident_ = "$Id$";

static const char * const name_ = "SPIT";
static const char * const description_ = "Shapefile to PostgreSQL/PostGIS Import Tool";
static const char * const version_ = "Version 0.1";
static const QgisPlugin::PLUGINTYPE type_ = QgisPlugin::UI;



/**
* Constructor for the plugin. The plugin is passed a pointer to the main app
* and an interface object that provides access to exposed functions in QGIS.
* @param qgis Pointer to the QGIS main window
* @parma _qI Pointer to the QGIS interface object
*/
QgsSpitPlugin::QgsSpitPlugin(QgisApp * qgis, QgisIface * _qI)
    : qgisMainWindow(qgis), 
      qI(_qI),
      QgisPlugin(name_, description_, version_, type_ )
{
}

QgsSpitPlugin::~QgsSpitPlugin()
{

}

/*
* Initialize the GUI interface for the plugin 
*/
void QgsSpitPlugin::initGui()
{
     // Create the action for tool
    spitAction = new QAction(QIcon(spitIcon), tr("&Import Shapefiles to PostgreSQL"), this);
    spitAction->setWhatsThis(tr("Import shapefiles into a PostGIS-enabled PostgreSQL database. "
        "The schema and field names can be customized on import")); 
    // Connect the action to the spit slot
    connect(spitAction, SIGNAL(activated()), this, SLOT(spit()));
     // Add the icon to the toolbar and to the plugin menu
    qI->addToolBarIcon(spitAction); 
    qI->addPluginMenu(tr("&Spit"), spitAction); 

}

// Slot called when the shapefile to postgres menu item is activated
void QgsSpitPlugin::spit()
{
 QgsSpit *spitDlg = new QgsSpit();
 spitDlg->show();
}


// Unload the plugin by cleaning up the GUI
void QgsSpitPlugin::unload()
{
    // remove the GUI
    qI->removeToolBarIcon(spitAction);
    qI->removePluginMenu(tr("&Spit"), spitAction);
    delete spitAction;
}

/** 
* Required extern functions needed  for every plugin 
* These functions can be called prior to creating an instance
* of the plugin class
*/
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory(QgisApp * qgis, QgisIface * qI)
{
    return new QgsSpitPlugin(qgis, qI);
}

// Return the name of the plugin
QGISEXTERN QString name()
{
    return name_;
}

// Return the description
QGISEXTERN QString description()
{
    return description_;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
    return type_;
}

// Return the version
QGISEXTERN QString version()
{
  return version_;
}


// Delete ourself
QGISEXTERN void unload(QgisPlugin * p)
{
    delete p;
}
