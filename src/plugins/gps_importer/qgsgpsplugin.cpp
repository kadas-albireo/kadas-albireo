/***************************************************************************
  qgsgpsplugin.cpp - GPS related tools
   -------------------
  Date                 : Jan 21, 2004
  Copyright            : (C) 2004 by Tim Sutton
  Email                : tim@linfiniti.com

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

#include "qgisapp.h"
#include "qgisgui.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsdataprovider.h"
#include "qgsvectordataprovider.h"
#include "qgsgpsplugin.h"


#include <qeventloop.h>
#include <QFileDialog>
#include <q3toolbar.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <q3popupmenu.h>
#include <qlineedit.h>
#include <qaction.h>
#include <qapplication.h>
#include <qcursor.h>
#include <q3process.h>
#include <q3progressdialog.h>
#include <qsettings.h>
#include <qstringlist.h>
#include <qglobal.h>

//non qt includes
#include <cassert>
#include <fstream>
#include <iostream>

//the gui subclass
#include "qgsgpsplugingui.h"

// xpm for creating the toolbar icon
#include "icon.xpm"

#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif


static const char * const ident_ = 
  "$Id$";
static const char * const name_ = "GPS Tools";
static const char * const description_ = 
  "Tools for loading and importing GPS data.";
static const char * const version_ = "Version 0.1";
static const QgisPlugin::PLUGINTYPE type_ = QgisPlugin::UI;


/**
 * Constructor for the plugin. The plugin is passed a pointer to the main app
 * and an interface object that provides access to exposed functions in QGIS.
 * @param qgis Pointer to the QGIS main window
 * @param _qI Pointer to the QGIS interface object
 */
QgsGPSPlugin::QgsGPSPlugin(QgisApp * theQGisApp, QgisIface * theQgisInterFace):
  QgisPlugin(name_,description_,version_,type_),
  mMainWindowPointer(theQGisApp), 
  mQGisInterface(theQgisInterFace)
{
  setupBabel();
}

QgsGPSPlugin::~QgsGPSPlugin()
{
  // delete all our babel formats
  BabelMap::iterator iter;
  for (iter = mImporters.begin(); iter != mImporters.end(); ++iter)
    delete iter->second;
  std::map<QString, QgsGPSDevice*>::iterator iter2;
  for (iter2 = mDevices.begin(); iter2 != mDevices.end(); ++iter2)
    delete iter2->second;
}


/*
 * Initialize the GUI interface for the plugin 
 */
void QgsGPSPlugin::initGui()
{
  // add an action to the toolbar
  mQActionPointer = new QAction(QIcon(icon), tr("&Gps Tools"), this);
  mCreateGPXAction = new QAction(QIcon(icon), tr("&Create new GPX layer"), this);

  mQActionPointer->setWhatsThis(tr("Creates a new GPX layer and displays it on the map canvas"));
  mCreateGPXAction->setWhatsThis(tr("Creates a new GPX layer and displays it on the map canvas"));
  connect(mQActionPointer, SIGNAL(activated()), this, SLOT(run()));
  connect(mCreateGPXAction, SIGNAL(activated()), this, SLOT(createGPX()));

  mQGisInterface->addToolBarIcon(mQActionPointer);
  mQGisInterface->addPluginMenu(tr("&Gps"), mQActionPointer);
  mQGisInterface->addPluginMenu(tr("&Gps"), mCreateGPXAction);
}

//method defined in interface
void QgsGPSPlugin::help()
{
  //implement me!
}

// Slot called when the buffer menu item is activated
void QgsGPSPlugin::run()
{
  // find all GPX layers
  std::vector<QgsVectorLayer*> gpxLayers;
  std::map<QString, QgsMapLayer*>::const_iterator iter;
  for (iter = mQGisInterface->getLayerRegistry()->mapLayers().begin();
       iter != mQGisInterface->getLayerRegistry()->mapLayers().end(); ++iter) {
    if (iter->second->type() == QgsMapLayer::VECTOR) {
      QgsVectorLayer* vLayer = dynamic_cast<QgsVectorLayer*>(iter->second);
      if (vLayer->providerType() == "gpx")
	gpxLayers.push_back(vLayer);
    }
  }
  
  QgsGPSPluginGui *myPluginGui = 
    new QgsGPSPluginGui(mImporters, mDevices, gpxLayers, mMainWindowPointer, 
			QgisGui::ModalDialogFlags);
  //listen for when the layer has been made so we can draw it
  connect(myPluginGui, SIGNAL(drawVectorLayer(QString,QString,QString)), 
	  this, SLOT(drawVectorLayer(QString,QString,QString)));
  connect(myPluginGui, SIGNAL(loadGPXFile(QString, bool, bool, bool)), 
	  this, SLOT(loadGPXFile(QString, bool, bool, bool)));
  connect(myPluginGui, SIGNAL(importGPSFile(QString, QgsBabelFormat*, bool, 
					    bool, bool, QString, QString)),
	  this, SLOT(importGPSFile(QString, QgsBabelFormat*, bool, bool, 
				   bool, QString, QString)));
  connect(myPluginGui, SIGNAL(downloadFromGPS(QString, QString, bool, bool,
					      bool, QString, QString)),
	  this, SLOT(downloadFromGPS(QString, QString, bool, bool, bool,
				     QString, QString)));
  connect(myPluginGui, SIGNAL(uploadToGPS(QgsVectorLayer*, QString, QString)),
	  this, SLOT(uploadToGPS(QgsVectorLayer*, QString, QString)));
  connect(this, SIGNAL(closeGui()), myPluginGui, SLOT(close()));

  myPluginGui->show();
}


void QgsGPSPlugin::createGPX() {
  QString fileName = 
    QFileDialog::getSaveFileName(mMainWindowPointer,
                 tr("Save new GPX file as..."), "." , tr("GPS eXchange file (*.gpx)"));
  if (!fileName.isEmpty()) {
    QFileInfo fileInfo(fileName);
    std::ofstream ofs((const char*)fileName);
    if (!ofs) {
      QMessageBox::warning(NULL, tr("Could not create file"),
			   tr("Unable to create a GPX file with the given name. ")+
			   tr("Try again with another name or in another ")+
			   tr("directory."));
      return;
    }
    ofs<<"<gpx></gpx>"<<std::endl;
    
    emit drawVectorLayer(fileName + "?type=track", 
			 fileInfo.baseName() + ", tracks", "gpx");
    emit drawVectorLayer(fileName + "?type=route", 
			 fileInfo.baseName() + ", routes", "gpx");
    emit drawVectorLayer(fileName + "?type=waypoint", 
			 fileInfo.baseName() + ", waypoints", "gpx");
  }
}


void QgsGPSPlugin::drawVectorLayer(QString thePathNameQString, 
				   QString theBaseNameQString, 
				   QString theProviderQString)
{
  mQGisInterface->addVectorLayer(thePathNameQString, theBaseNameQString, 
				 theProviderQString);
}

// Unload the plugin by cleaning up the GUI
void QgsGPSPlugin::unload()
{
  // remove the GUI
  mQGisInterface->removePluginMenu(tr("&Gps"),mQActionPointer);
  mQGisInterface->removePluginMenu(tr("&Gps"),mCreateGPXAction);
  mQGisInterface->removeToolBarIcon(mQActionPointer);
  delete mQActionPointer;
}

void QgsGPSPlugin::loadGPXFile(QString filename, bool loadWaypoints, bool loadRoutes,
			       bool loadTracks) {

  //check if input file is readable
  QFileInfo fileInfo(filename);
  if (!fileInfo.isReadable()) {
    QMessageBox::warning(NULL, tr("GPX Loader"),
			 tr("Unable to read the selected file.\n")+
			 tr("Please reselect a valid file.") );
    return;
  }
  
  // remember the directory
  QSettings settings;
  settings.writeEntry("/Plugin-GPS/gpxdirectory", fileInfo.dirPath());
  
  // add the requested layers
  if (loadTracks)
    emit drawVectorLayer(filename + "?type=track", 
			 fileInfo.baseName() + ", tracks", "gpx");
  if (loadRoutes)
    emit drawVectorLayer(filename + "?type=route", 
			 fileInfo.baseName() + ", routes", "gpx");
  if (loadWaypoints)
    emit drawVectorLayer(filename + "?type=waypoint", 
			 fileInfo.baseName() + ", waypoints", "gpx");
  
  emit closeGui();
}


void QgsGPSPlugin::importGPSFile(QString inputFilename, QgsBabelFormat* importer, 
				 bool importWaypoints, bool importRoutes, 
				 bool importTracks, QString outputFilename, 
				 QString layerName) {

  // what features does the user want to import?
  QString typeArg;
  if (importWaypoints)
    typeArg = "-w";
  else if (importRoutes)
    typeArg = "-r";
  else if (importTracks)
    typeArg = "-t";
  
  // try to start the gpsbabel process
  QStringList babelArgs = 
    importer->importCommand(mBabelPath, typeArg, 
			       inputFilename, outputFilename);
  Q3Process babelProcess(babelArgs);
  if (!babelProcess.start()) {
    QMessageBox::warning(NULL, tr("Could not start process"),
			 tr("Could not start GPSBabel!"));
    return;
  }
  
  // wait for gpsbabel to finish (or the user to cancel)
  Q3ProgressDialog progressDialog(tr("Importing data..."), tr("Cancel"), 0,
				 NULL, 0, true);
  progressDialog.show();
  for (int i = 0; babelProcess.isRunning(); ++i) {
    QCoreApplication::processEvents();

    progressDialog.setProgress(i/64);
    if (progressDialog.wasCanceled())
      return;
  }
  
  // did we get any data?
  if (babelProcess.exitStatus() != 0) {
    QString babelError(babelProcess.readStderr());
    QString errorMsg(tr("Could not import data from %1!\n\n")
		     .arg(inputFilename));
    errorMsg += babelError;
    QMessageBox::warning(NULL, tr("Error importing data"), errorMsg);
    return;
  }
  
  // add the layer
  if (importTracks)
    emit drawVectorLayer(outputFilename + "?type=track", 
			 layerName, "gpx");
  if (importRoutes)
    emit drawVectorLayer(outputFilename + "?type=route", 
			 layerName, "gpx");
  if (importWaypoints)
    emit drawVectorLayer(outputFilename + "?type=waypoint", 
			 layerName, "gpx");
  
  emit closeGui();
}


void QgsGPSPlugin::downloadFromGPS(QString device, QString port,
				   bool downloadWaypoints, bool downloadRoutes,
				   bool downloadTracks, QString outputFilename,
				   QString layerName) {
  
  // what does the user want to download?
  QString typeArg, features;
  if (downloadWaypoints) {
    typeArg = "-w";
    features = "waypoints";
  }
  else if (downloadRoutes) {
    typeArg = "-r";
    features = "routes";
  }
  else if (downloadTracks) {
    typeArg = "-t";
    features = "tracks";
  }
  
  // try to start the gpsbabel process
  QStringList babelArgs = 
    mDevices[device]->importCommand(mBabelPath, typeArg, 
				    port, outputFilename);
  if (babelArgs.isEmpty()) {
    QMessageBox::warning(NULL, tr("Not supported"),
			 QString(tr("This device does not support downloading ") +
				 tr("of ")) + features + ".");
    return;
  }
  Q3Process babelProcess(babelArgs);
  if (!babelProcess.start()) {
    QMessageBox::warning(NULL, tr("Could not start process"),
			 tr("Could not start GPSBabel!"));
    return;
  }
  
  // wait for gpsbabel to finish (or the user to cancel)
  Q3ProgressDialog progressDialog(tr("Downloading data..."), tr("Cancel"), 0,
				 NULL, 0, true);
  progressDialog.show();
  for (int i = 0; babelProcess.isRunning(); ++i) {
    QCoreApplication::processEvents();

    progressDialog.setProgress(i/64);
    if (progressDialog.wasCanceled())
      return;
  }
  
  // did we get any data?
  if (babelProcess.exitStatus() != 0) {
    QString babelError(babelProcess.readStderr());
    QString errorMsg(tr("Could not download data from GPS!\n\n"));
    errorMsg += babelError;
    QMessageBox::warning(NULL, tr("Error downloading data"), errorMsg);
    return;
  }
  
  // add the layer
  if (downloadWaypoints)
    emit drawVectorLayer(outputFilename + "?type=waypoint", 
			 layerName, "gpx");
  if (downloadRoutes)
    emit drawVectorLayer(outputFilename + "?type=route", 
			 layerName, "gpx");
  if (downloadTracks)
    emit drawVectorLayer(outputFilename + "?type=track", 
			 layerName, "gpx");
  
  // everything was OK, remember the device and port for next time
  QSettings settings;
  settings.writeEntry("/Plugin-GPS/lastdldevice", device);
  settings.writeEntry("/Plugin-GPS/lastdlport", port);
  
  emit closeGui();
}


void QgsGPSPlugin::uploadToGPS(QgsVectorLayer* gpxLayer, QString device,
			       QString port) {
  
  const QString& source(gpxLayer->getDataProvider()->getDataSourceUri());
  
  // what kind of data does the user want to upload?
  QString typeArg, features;
  if (source.right(8) == "waypoint") {
    typeArg = "-w";
    features = "waypoints";
  }
  else if (source.right(5) == "route") {
    typeArg = "-r";
    features = "routes";
  }
  else if (source.right(5) == "track") {
    typeArg = "-t";
    features = "tracks";
  }
  else {
    std::cerr << source.right(8).toLocal8Bit().data() << std::endl;
    assert(false);
  }
  
  // try to start the gpsbabel process
  QStringList babelArgs = 
    mDevices[device]->exportCommand(mBabelPath, typeArg, 
				       source.left(source.findRev('?')), port);
  if (babelArgs.isEmpty()) {
    QMessageBox::warning(NULL, tr("Not supported"),
			 QString(tr("This device does not support uploading of "))+
			 features + ".");
    return;
  }
  Q3Process babelProcess(babelArgs);
  if (!babelProcess.start()) {
    QMessageBox::warning(NULL, tr("Could not start process"),
			 tr("Could not start GPSBabel!"));
    return;
  }
  
  // wait for gpsbabel to finish (or the user to cancel)
  Q3ProgressDialog progressDialog(tr("Uploading data..."), tr("Cancel"), 0,
				 NULL, 0, true);
  progressDialog.show();
  for (int i = 0; babelProcess.isRunning(); ++i) {
    QCoreApplication::processEvents();

    progressDialog.setProgress(i/64);
    if (progressDialog.wasCanceled())
      return;
  }
  
  // did we get an error?
  if (babelProcess.exitStatus() != 0) {
    QString babelError(babelProcess.readStderr());
    QString errorMsg(tr("Error while uploading data to GPS!\n\n"));
    errorMsg += babelError;
    QMessageBox::warning(NULL, tr("Error uploading data"), errorMsg);
    return;
  }
  
  // everything was OK, remember this device for next time
  QSettings settings;
  settings.writeEntry("/Plugin-GPS/lastuldevice", device);
  settings.writeEntry("/Plugin-GPS/lastulport", port);
  
  emit closeGui();
}


void QgsGPSPlugin::setupBabel() {
  
  // where is gpsbabel?
  QSettings settings;
  mBabelPath = settings.value("/Plugin-GPS/gpsbabelpath", "").toString();
  if (mBabelPath.isEmpty())
    mBabelPath = "gpsbabel";
  // the importable formats
  mImporters["Geocaching.com .loc"] =
    new QgsSimpleBabelFormat("geo", true, false, false);
  mImporters["Magellan Mapsend"] = 
    new QgsSimpleBabelFormat("mapsend", true, true, true);
  mImporters["Garmin PCX5"] = 
    new QgsSimpleBabelFormat("pcx", true, false, true);
  mImporters["Garmin Mapsource"] = 
    new QgsSimpleBabelFormat("mapsource", true, true, true);
  mImporters["GPSUtil"] = 
    new QgsSimpleBabelFormat("gpsutil", true, false, false);
  mImporters["PocketStreets 2002/2003 Pushpin"] = 
    new QgsSimpleBabelFormat("psp", true, false, false);
  mImporters["CoPilot Flight Planner"] = 
    new QgsSimpleBabelFormat("copilot", true, false, false);
  mImporters["Magellan Navigator Companion"] = 
    new QgsSimpleBabelFormat("magnav", true, false, false);
  mImporters["Holux"] = 
    new QgsSimpleBabelFormat("holux", true, false, false);
  mImporters["Topo by National Geographic"] = 
    new QgsSimpleBabelFormat("tpg", true, false, false);
  mImporters["TopoMapPro"] = 
    new QgsSimpleBabelFormat("tmpro", true, false, false);
  mImporters["GeocachingDB"] = 
    new QgsSimpleBabelFormat("gcdb", true, false, false);
  mImporters["Tiger"] = 
    new QgsSimpleBabelFormat("tiger", true, false, false);
  mImporters["EasyGPS Binary Format"] = 
    new QgsSimpleBabelFormat("easygps", true, false, false);
  mImporters["Delorme Routes"] = 
    new QgsSimpleBabelFormat("saroute", false, false, true);
  mImporters["Navicache"] = 
    new QgsSimpleBabelFormat("navicache", true, false, false);
  mImporters["PSITrex"] = 
    new QgsSimpleBabelFormat("psitrex", true, true, true);
  mImporters["Delorme GPS Log"] = 
    new QgsSimpleBabelFormat("gpl", false, false, true);
  mImporters["OziExplorer"] = 
    new QgsSimpleBabelFormat("ozi", true, false, false);
  mImporters["NMEA Sentences"] = 
    new QgsSimpleBabelFormat("nmea", true, false, true);
  mImporters["Delorme Street Atlas 2004 Plus"] = 
    new QgsSimpleBabelFormat("saplus", true, false, false);
  mImporters["Microsoft Streets and Trips"] = 
    new QgsSimpleBabelFormat("s_and_t", true, false, false);
  mImporters["NIMA/GNIS Geographic Names"] = 
    new QgsSimpleBabelFormat("nima", true, false, false);
  mImporters["Maptech"] = 
    new QgsSimpleBabelFormat("mxf", true, false, false);
  mImporters["Mapopolis.com Mapconverter Application"] = 
    new QgsSimpleBabelFormat("mapconverter", true, false, false);
  mImporters["GPSman"] = 
    new QgsSimpleBabelFormat("gpsman", true, false, false);
  mImporters["GPSDrive"] = 
    new QgsSimpleBabelFormat("gpsdrive", true, false, false);
  mImporters["Fugawi"] = 
    new QgsSimpleBabelFormat("fugawi", true, false, false);
  mImporters["DNA"] = 
    new QgsSimpleBabelFormat("dna", true, false, false);

  // and the GPS devices
  mDevices["Garmin serial"] = 
    new QgsGPSDevice("%babel -w -i garmin -o gpx %in %out",
		     "%babel -w -i gpx -o garmin %in %out",
		     "%babel -r -i garmin -o gpx %in %out",
		     "%babel -r -i gpx -o garmin %in %out",
		     "%babel -t -i garmin -o gpx %in %out",
		     "%babel -t -i gpx -o garmin %in %out");
  QStringList deviceNames = settings.value("/Plugin-GPS/devicelist").
                             toStringList();

  QStringList::const_iterator iter;

  for (iter = deviceNames.begin(); iter != deviceNames.end(); ++iter) {
    QString wptDownload = settings.
      value(QString("/Plugin-GPS/devices/%1/wptdownload").
       arg(*iter), "").toString();
    QString wptUpload = settings.
      value(QString("/Plugin-GPS/devices/%1/wptupload").arg(*iter), "").
       toString();
    QString rteDownload = settings.
      value(QString("/Plugin-GPS/devices/%1/rtedownload").arg(*iter), "").
       toString();
    QString rteUpload = settings.
      value(QString("/Plugin-GPS/devices/%1/rteupload").arg(*iter), "").
       toString();
    QString trkDownload = settings.
      value(QString("/Plugin-GPS/devices/%1/trkdownload").arg(*iter), "").
       toString();
    QString trkUpload = settings.
      value(QString("/Plugin-GPS/devices/%1/trkupload").arg(*iter), "").
       toString();
    mDevices[*iter] = new QgsGPSDevice(wptDownload, wptUpload,
				       rteDownload, rteUpload,
				       trkDownload, trkUpload);
  }
}




/** 
 * Required extern functions needed  for every plugin 
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory(QgisApp * theQGisAppPointer, 
				     QgisIface * theQgisInterfacePointer)
{
  return new QgsGPSPlugin(theQGisAppPointer, theQgisInterfacePointer);
}

// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
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

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return version_;
}

// Delete ourself
QGISEXTERN void unload(QgisPlugin * thePluginPointer)
{
  delete thePluginPointer;
}
