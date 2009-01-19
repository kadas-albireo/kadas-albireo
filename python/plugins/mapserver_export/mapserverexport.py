"""
/***************************************************************************
        MapServerExport  - A QGIS plugin to export a saved project file
                            to a MapServer map file
                             -------------------
    begin                : 2008-01-07
    copyright            : (C) 2008 by Gary E.Sherman
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
"""
# Import the PyQt and QGIS libraries
from PyQt4.QtCore import * 
from PyQt4.QtGui import *
from qgis.core import *
# Initialize Qt resources from file resources.py
import resources
# Import the code for the dialog
from mapserverexportdialog import MapServerExportDialog
# Import the ms_export script that does the real work
from ms_export import *

class MapServerExport: 

  def __init__(self, iface):
    # Save reference to the QGIS interface
    self.iface = iface

  def initGui(self):  
    # Create action that will start plugin configuration
    self.action = QAction(QIcon(":/plugins/mapserver_export/icon.png"), \
        "MapServer Export", self.iface.mainWindow())
    #self.action.setWhatsThis("Configuration for Zoom To Point plugin")
    # connect the action to the run method
    QObject.connect(self.action, SIGNAL("activated()"), self.run) 

    # Add toolbar button and menu item
    self.iface.addToolBarIcon(self.action)
    self.iface.addPluginToMenu("&MapServer Export...", self.action)

  def unload(self):
    # Remove the plugin menu item and icon
    self.iface.removePluginMenu("&MapServer Export...",self.action)
    self.iface.removeToolBarIcon(self.action)

  # run method that performs all the real work
  def run(self): 
    # create and show the MapServerExport dialog 
    self.dlg = MapServerExportDialog() 
    #dlg.setupUi(self)
    # fetch the last used values from settings and intialize the
    # dialog with them
    #settings = QSettings("MicroResources", "ZoomToPoint")
    #xValue = settings.value("coordinate/x")
    #dlg.ui.xCoord.setText(str(xValue.toString()))
    #yValue = settings.value("coordinate/y")
    #dlg.ui.yCoord.setText(str(yValue.toString()))
    #scale = settings.value("zoom/scale", QVariant(4))
    #dlg.ui.spinBoxScale.setValue(scale.toInt()[0])
    QObject.connect(self.dlg.ui.btnChooseFile, SIGNAL("clicked()"), self.setSaveFile)
    QObject.connect(self.dlg.ui.btnChooseProjectFile, SIGNAL("clicked()"), self.setProjectFile)
    QObject.connect(self.dlg.ui.chkExpLayersOnly, SIGNAL("clicked(bool)"), self.toggleLayersOnly)
    QObject.connect(self.dlg.ui.btnChooseFooterFile, SIGNAL("clicked()"), self.setFooterFile)
    QObject.connect(self.dlg.ui.btnChooseHeaderFile, SIGNAL("clicked()"), self.setHeaderFile)
    QObject.connect(self.dlg.ui.btnChooseTemplateFile, SIGNAL("clicked()"), self.setTemplateFile)

    self.dlg.show()
    result = self.dlg.exec_() 
    # See if OK was pressed
    if result == 1: 
      # get the settings from the dialog and export the map file
      print "Creating exporter using %s and %s" % (self.dlg.ui.txtQgisFilePath.text(), self.dlg.ui.txtMapFilePath.text())
      exporter = Qgis2Map(unicode(self.dlg.ui.txtQgisFilePath.text()), unicode(self.dlg.ui.txtMapFilePath.text()))
      print "Setting options"
      exporter.setOptions( 
          unicode(self.dlg.ui.cmbMapUnits.itemData( self.dlg.ui.cmbMapUnits.currentIndex() ).toString()),
          unicode(self.dlg.ui.cmbMapImageType.currentText()),
          unicode(self.dlg.ui.txtMapName.text()),
          unicode(self.dlg.ui.txtMapWidth.text()),
          unicode(self.dlg.ui.txtMapHeight.text()),
          unicode(self.dlg.ui.txtWebTemplate.text()),
          unicode(self.dlg.ui.txtWebFooter.text()),
          unicode(self.dlg.ui.txtWebHeader.text())
          )
      print "Calling writeMapFile"
      result = exporter.writeMapFile()
      QMessageBox.information(None, "MapServer Export Results", result)

  def setSaveFile(self):
    mapFile = QFileDialog.getSaveFileName(self.dlg, "Name for the map file", \
      ".", "MapServer map files (*.map);;All files (*.*)","Filter list for selecting files from a dialog box")
    self.dlg.ui.txtMapFilePath.setText(mapFile)

  def setProjectFile(self):
    qgisProjectFile = QFileDialog.getOpenFileName(self.dlg, "Choose the QGIS Project file", \
        ".", "QGIS Project Files (*.qgs);;All files (*.*)", "Filter list for selecting files from a dialog box")
    self.dlg.ui.txtQgisFilePath.setText(qgisProjectFile)

  def setTemplateFile(self):
    templateFile = QFileDialog.getOpenFileName(self.dlg, 
        "Choose the MapServer template file", 
        ".", 
        "All files (*.*)", 
        "Filter list for selecting files from a dialog box")
    self.dlg.ui.txtWebTemplate.setText(templateFile)

  def setHeaderFile(self):
    headerFile = QFileDialog.getOpenFileName(self.dlg, 
        "Choose the MapServer header file", 
        ".", 
        "All files (*.*)", 
        "Filter list for selecting files from a dialog box")
    self.dlg.ui.txtWebHeader.setText(headerFile)

  def setFooterFile(self):
    footerFile = QFileDialog.getOpenFileName(self.dlg, 
        "Choose the MapServer footer file", 
        ".", 
        "All files (*.*)", 
        "Filter list for selecting files from a dialog box")
    self.dlg.ui.txtWebFooter.setText(footerFile)

  def apply(self):
    # create the map file
    foo = 'bar'
  
  def toggleLayersOnly(self, isChecked):
    # disable other sections if only layer export is desired
    self.dlg.ui.txtMapName.setEnabled(not isChecked)
    self.dlg.ui.txtMapWidth.setEnabled(not isChecked)
    self.dlg.ui.txtMapHeight.setEnabled(not isChecked)
    self.dlg.ui.cmbMapUnits.setEnabled(not isChecked)
    self.dlg.ui.cmbMapImageType.setEnabled(not isChecked)
    self.dlg.ui.txtWebTemplate.setEnabled(not isChecked)
    self.dlg.ui.txtWebHeader.setEnabled(not isChecked)
    self.dlg.ui.txtWebFooter.setEnabled(not isChecked)
    self.dlg.ui.btnChooseFooterFile.setEnabled(not isChecked)
    self.dlg.ui.btnChooseHeaderFile.setEnabled(not isChecked)
    self.dlg.ui.btnChooseTemplateFile.setEnabled(not isChecked)
