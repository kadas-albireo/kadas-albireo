# -*- coding: utf-8 -*-

"""
***************************************************************************
    doProjection.py
    ---------------------
    Date                 : June 2010
    Copyright            : (C) 2010 by Giuseppe Sucameli
    Email                : brush dot tyler at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Giuseppe Sucameli'
__date__ = 'June 2010'
__copyright__ = '(C) 2010, Giuseppe Sucameli'
# This will get replaced with a git SHA1 when you do a git archive
__revision__ = '$Format:%H$'

from PyQt4.QtCore import Qt, QObject, SIGNAL, QCoreApplication, QFile, QFileInfo
from PyQt4.QtGui import QWidget, QMessageBox
from qgis.core import QgsMapLayerRegistry, QgsMapLayer

from ui_widgetProjection import Ui_GdalToolsWidget as Ui_Widget
from widgetBatchBase import GdalToolsBaseBatchWidget as BaseBatchWidget
from dialogSRS import GdalToolsSRSDialog as SRSDialog
import GdalTools_utils as Utils

import re


class GdalToolsDialog( QWidget, Ui_Widget, BaseBatchWidget ):

  def __init__( self, iface ):
      QWidget.__init__( self )
      self.iface = iface

      self.setupUi( self )
      BaseBatchWidget.__init__( self, self.iface, "gdalwarp" )

      self.inSelector.setType( self.inSelector.FILE )

      # set the default QSpinBoxes and QProgressBar value
      self.progressBar.setValue(0)

      self.progressBar.hide()
      self.recurseCheck.hide()

      self.setParamsStatus([
          ( self.inSelector, SIGNAL( "filenameChanged()" ) ),
          ( self.desiredSRSEdit, SIGNAL( "textChanged( const QString & )" ) )
      ])

      self.connect( self.inSelector, SIGNAL( "selectClicked()" ), self.fillInputFileEdit )
      self.connect( self.selectDesiredSRSButton, SIGNAL( "clicked()" ), self.fillDesiredSRSEdit )
      self.connect( self.batchCheck, SIGNAL( "stateChanged( int )" ), self.switchToolMode )
      self.connect( self.recurseCheck, SIGNAL( "stateChanged( int )" ), self.enableRecurse )

  def switchToolMode( self ):
      self.setCommandViewerEnabled( not self.batchCheck.isChecked() )
      self.progressBar.setVisible( self.batchCheck.isChecked() )
      self.recurseCheck.setVisible( self.batchCheck.isChecked() )

      self.inSelector.clear()

      if self.batchCheck.isChecked():
        self.inFileLabel = self.label.text()
        self.label.setText( QCoreApplication.translate( "GdalTools", "&Input directory" ) )

        QObject.disconnect( self.inSelector, SIGNAL( "selectClicked()" ), self.fillInputFileEdit )
        QObject.connect( self.inSelector, SIGNAL( "selectClicked()" ), self.fillInputDir )
      else:
        self.label.setText( self.inFileLabel )

        QObject.connect( self.inSelector, SIGNAL( "selectClicked()" ), self.fillInputFileEdit )
        QObject.disconnect( self.inSelector, SIGNAL( "selectClicked()" ), self.fillInputDir )

  def enableRecurse( self ):
    if self.recurseCheck.isChecked():
      res = QMessageBox.warning( self, self.tr( "Warning" ),
                                 self.tr( "Warning: CRS information for all raster in subfolders will be rewritten. Are you sure?" ),
                                 QMessageBox.Yes | QMessageBox.No )
      if res != QMessageBox.Yes:
        self.recurseCheck.setCheckState( Qt.Unchecked )
        return

  def fillInputFileEdit( self ):
      lastUsedFilter = Utils.FileFilter.lastUsedRasterFilter()
      inputFile = Utils.FileDialog.getOpenFileName( self, self.tr( "Select the file to analyse" ), Utils.FileFilter.allRastersFilter(), lastUsedFilter )
      if not inputFile:
        return
      Utils.FileFilter.setLastUsedRasterFilter( lastUsedFilter )
      self.inSelector.setFilename( inputFile )

  def fillInputDir( self ):
      inputDir = Utils.FileDialog.getExistingDirectory( self, self.tr( "Select the input directory with files to Assign projection" ))
      if not inputDir:
        return
      self.inSelector.setFilename( inputDir )

  def fillDesiredSRSEdit( self ):
      dialog = SRSDialog( "Select desired SRS", self )
      if dialog.exec_():
        self.desiredSRSEdit.setText( dialog.getProjection() )

  def getArguments( self ):
      arguments = []
      if self.desiredSRSEdit.text():
        arguments.append("-t_srs")
        arguments.append(self.desiredSRSEdit.text())
      if self.batchCheck.isChecked():
        return arguments

      inputFn = self.getInputFileName()
      arguments.append(inputFn)
      self.tempFile = inputFn
      self.needOverwrite = False
      if self.tempFile:
        if self.tempFile.lower().endswith(".tif") or self.tempFile.lower().endswith(".tiff") :
          self.tempFile = re.sub("\.[a-zA-Z]{2,4}$", ".tif", self.tempFile) + ".tmp"
          self.needOverwrite = True
        else:
          self.tempFile = re.sub("\.[a-zA-Z]{2,4}$", ".tif", self.tempFile)
      arguments.append(self.tempFile)
      return arguments

  def finished( self ):
      outFn = self.getOutputFileName()
      if self.needOverwrite:
        oldFile = QFile( outFn )
        newFile = QFile( self.tempFile )
        if oldFile.remove():
          newFile.rename( outFn  )

      fileInfo = QFileInfo( outFn )
      if fileInfo.exists():
        if self.base.loadCheckBox.isChecked():
          self.addLayerIntoCanvas( fileInfo )
        QMessageBox.information( self, self.tr( "Finished" ), self.tr( "Processing completed." ) )
      else:
        QMessageBox.warning( self, self.tr( "Warning" ), self.tr( "{0} not created." ).format( outFn ) )

  def getInputFileName(self):
      return self.inSelector.filename()

  def getOutputFileName( self ):
      return self.inSelector.filename()

  def getBatchOutputFileName(self, fn):
      # get GeoTiff
      fn = re.sub( r'\.[a-zA-Z]{2,4}$', r'.tif', fn )
      return BaseBatchWidget.getBatchOutputFileName( self, fn )

  def addLayerIntoCanvas(self, fileInfo):
      self.iface.addRasterLayer(fileInfo.filePath())

  def checkLayer( self ):
      layerList = []

      layerMap = QgsMapLayerRegistry.instance().mapLayers()
      for name, layer in layerMap.iteritems():
        if layer.type() == QgsMapLayer.RasterLayer:
          layerList.append( unicode( layer.source() ) )

      if unicode( self.inputFileEdit.text() ) in layerList:
        QMessageBox.warning( self, self.tr( "Assign projection" ), self.tr( "This raster already found in map canvas" ) )
        return

      self.onRun()

  def isBatchEnabled(self):
      return self.batchCheck.isChecked()

  def isRecursiveScanEnabled(self):
      return self.recurseCheck.isChecked()

  def setProgressRange(self, maximum):
      self.progressBar.setRange(0, maximum)

  def updateProgress(self, index, total):
      if index < total:
        self.progressBar.setValue(index + 1)
      else:
        self.progressBar.setValue(0)
