# -*- coding: utf-8 -*-

"""
***************************************************************************
    doInfo.py
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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *

from ui_widgetInfo import Ui_GdalToolsWidget as Ui_Widget
from widgetPluginBase import GdalToolsBasePluginWidget as BasePluginWidget
import GdalTools_utils as Utils

import platform

class GdalToolsDialog( QWidget, Ui_Widget, BasePluginWidget ):

  def __init__( self, iface ):
      QWidget.__init__( self )
      self.iface = iface

      self.setupUi( self )
      BasePluginWidget.__init__( self, self.iface, "gdalinfo" )

      # we don't need load to canvas functionality
      self.base.loadCheckBox.hide()
      # make window large
      self.base.resize( 400, 360 )

      self.setParamsStatus(
        [
          (self.inSelector, SIGNAL("filenameChanged()") ),
          ( self.suppressGCPCheck, SIGNAL( "stateChanged( int )" ) ),
          ( self.suppressMDCheck, SIGNAL( "stateChanged( int )" ) )
        ]
      )

      self.connect( self.inSelector, SIGNAL( "selectClicked()" ), self.fillInputFileEdit )

      # helper actions for copying info output
      self.copyLine = QAction( self.tr( "Copy" ), self )
      QObject.connect( self.copyLine, SIGNAL( "triggered()" ), self.doCopyLine )
      self.copyAll = QAction( self.tr( "Copy all" ), self )
      QObject.connect( self.copyAll, SIGNAL( "triggered()" ), self.doCopyAll )


  def doCopyLine( self ):
      output = QString()
      items = self.rasterInfoList.selectedItems()
      for r in items:
        output.append( r.text() + "\n" )
      if not output.isEmpty():
        clipboard = QApplication.clipboard()
        clipboard.setText( output )

  def doCopyAll( self ):
      output = QString()
      for r in range( self.rasterInfoList.count() ):
        output.append( self.rasterInfoList.item( r ).text() + "\n" )
      if not output.isEmpty():
        clipboard = QApplication.clipboard()
        clipboard.setText( output )

  def keyPressEvent( self, e ):
      if ( e.modifiers() == Qt.ControlModifier or e.modifiers() == Qt.MetaModifier ) and e.key() == Qt.Key_C:
        items = QString()
        for r in range( self.rasterInfoList.count() ):
          items.append( self.rasterInfoList.item( r ).text() + "\n" )
        if not items.isEmpty():
          clipboard = QApplication.clipboard()
          clipboard.setText( items )
      else:
        QWidget.keyPressEvent( self, e )

  def onLayersChanged(self):
      self.inSelector.setLayers( Utils.LayerRegistry.instance().getRasterLayers() )

  def finished( self ):
      self.rasterInfoList.clear()
      arr = QByteArray()
      arr = self.base.process.readAllStandardOutput()
      if platform.system() == "Windows":
        info = QString.fromLocal8Bit( arr ).trimmed().split( "\r\n" )
      else:
        info = QString( arr ).trimmed().split( "\n" )
      self.rasterInfoList.addItems( info )

  def fillInputFileEdit( self ):
      lastUsedFilter = Utils.FileFilter.lastUsedRasterFilter()
      inputFile = Utils.FileDialog.getOpenFileName( self, self.tr( "Select the file to analyse" ), Utils.FileFilter.allRastersFilter(), lastUsedFilter )
      if inputFile.isEmpty():
        return
      Utils.FileFilter.setLastUsedRasterFilter( lastUsedFilter )

      self.inSelector.setFilename( inputFile )

  def getArguments( self ):
      arguments = QStringList()
      if self.suppressGCPCheck.isChecked():
        arguments << "-nogcp"
      if self.suppressMDCheck.isChecked():
        arguments << "-nomd"
      arguments << self.getInputFileName()
      return arguments

#  def getOutputFileName( self ):
#      return self.inSelector.filename()

  def getInputFileName( self ):
      return self.inSelector.filename()


  def contextMenuEvent( self, event ):
      menu = QMenu( self )
      menu.addAction( self.copyLine )
      menu.addAction( self.copyAll )
      menu.exec_( event.globalPos() )

