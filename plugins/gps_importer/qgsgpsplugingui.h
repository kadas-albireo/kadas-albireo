/***************************************************************************
                          qgsgpsplugingui.h 
 Functions:
                             -------------------
    begin                : Jan 21, 2004
    copyright            : (C) 2004 by Tim Sutton
    email                : tim@linfiniti.com
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /*  $Id$ */

#ifndef QGSGPSPLUGINGUI_H
#define QGSGPSPLUGINGUI_H

#include "../../src/qgsvectorlayer.h"
#include "ui_qgsgpspluginguibase.h"
#include <QDialog>
#include "qgsbabelformat.h"
#include "qgsgpsdevice.h"

#include <vector>

#include <qstring.h>


/**
@author Tim Sutton
*/
class QgsGPSPluginGui : public QDialog, private Ui::QgsGPSPluginGuiBase
{
  Q_OBJECT
public:
  QgsGPSPluginGui(const BabelMap& importers, 
		  std::map<QString, QgsGPSDevice*>& devices, 
		  std::vector<QgsVectorLayer*> gpxMapLayers, QWidget* parent, 
		  const char* name , bool modal , Qt::WFlags);
  ~QgsGPSPluginGui();

public slots:

  void slotOpenDeviceEditor();
  void slotDevicesUpdated();
  
private:
  
  void pbnSelectInputFile_clicked();
  void pbnSelectOutputFile_clicked();
  
  void pbnGPXSelectFile_clicked();
  
  void pbnIMPInput_clicked();
  void pbnIMPOutput_clicked();
  
  void pbnDLOutput_clicked();
  
  void enableRelevantControls();
  void pbnCancel_clicked();
  void pbnOK_clicked();
  
  void populateDeviceComboBox();
  void populateULLayerComboBox();
  void populateIMPBabelFormats();
  void populatePortComboBoxes();
  
signals:
  void drawRasterLayer(QString);
  void drawVectorLayer(QString,QString,QString);
  void loadGPXFile(QString filename, bool showWaypoints, bool showRoutes, 
		   bool showTracks);
  void importGPSFile(QString inputFilename, QgsBabelFormat* importer,
		     bool importWaypoints, bool importRoutes, 
		     bool importTracks, QString outputFilename, 
		     QString layerName);
  void downloadFromGPS(QString device, QString port, bool downloadWaypoints, 
		       bool downloadRoutes, bool downloadTracks, 
		       QString outputFilename, QString layerName);
  void uploadToGPS(QgsVectorLayer* gpxLayer, QString device, QString port);
  
private:
  
  std::vector<QgsVectorLayer*> mGPXLayers;
  const BabelMap& mImporters;
  std::map<QString, QgsGPSDevice*>& mDevices;
  QString mBabelFilter;
  QString mImpFormat;
};

#endif
