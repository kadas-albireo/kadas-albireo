# -*- coding: utf-8 -*-
#-----------------------------------------------------------
#
# fTools
# Copyright (C) 2008-2011  Carson Farmer
# EMAIL: carson.farmer (at) gmail.com
# WEB  : http://www.ftools.ca/fTools.html
#
# A collection of data management and analysis tools for vector data
#
#-----------------------------------------------------------
#
# licensed under the terms of GNU GPL 2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#---------------------------------------------------------------------

from PyQt4.QtCore import QObject, SIGNAL, QVariant, QFile
from PyQt4.QtGui import QDialog, QDialogButtonBox, QMessageBox
import ftools_utils
from qgis.core import QGis, QgsField, QgsFeature, QgsGeometry, QgsDistanceArea, QgsVectorFileWriter, QgsFeatureRequest
from ui_frmSumLines import Ui_Dialog

class Dialog(QDialog, Ui_Dialog):

    def __init__(self, iface):
        QDialog.__init__(self, iface.mainWindow())
        self.iface = iface
        # Set up the user interface from Designer.
        self.setupUi(self)
        QObject.connect(self.toolOut, SIGNAL("clicked()"), self.outFile)
        self.setWindowTitle(self.tr("Sum line lengths"))
        self.buttonOk = self.buttonBox_2.button( QDialogButtonBox.Ok )
        self.progressBar.setValue(0)
        self.populateLayers()

    def populateLayers( self ):
        layers = ftools_utils.getLayerNames([QGis.Line])
        self.inPoint.clear()
        self.inPoint.addItems(layers)
        layers = ftools_utils.getLayerNames([QGis.Polygon])
        self.inPolygon.clear()
        self.inPolygon.addItems(layers)

    def accept(self):
        self.buttonOk.setEnabled( False )
        if self.inPolygon.currentText() == "":
            QMessageBox.information(self, self.tr("Sum Line Lengths In Polyons"), self.tr("Please specify input polygon vector layer"))
        elif self.outShape.text() == "":
            QMessageBox.information(self, self.tr("Sum Line Lengths In Polyons"), self.tr("Please specify output shapefile"))
        elif self.inPoint.currentText() == "":
            QMessageBox.information(self, self.tr("Sum Line Lengths In Polyons"), self.tr("Please specify input line vector layer"))
        elif self.lnField.text() == "":
            QMessageBox.information(self, self.tr("Sum Line Lengths In Polyons"), self.tr("Please specify output length field"))
        else:
            inPoly = self.inPolygon.currentText()
            inLns = self.inPoint.currentText()
            inField = self.lnField.text()
            outPath = self.outShape.text()
            self.compute(inPoly, inLns, inField, outPath, self.progressBar)
            self.outShape.clear()
            if self.addToCanvasCheck.isChecked():
                addCanvasCheck = ftools_utils.addShapeToCanvas(unicode(outPath))
                if not addCanvasCheck:
                    QMessageBox.warning( self, self.tr("Sum line lengths"), self.tr( "Error loading output shapefile:\n%s" ) % ( unicode( outPath ) ))
                self.populateLayers()
            else:
                QMessageBox.information(self, self.tr("Sum line lengths"),self.tr("Created output shapefile:\n%s" ) % ( unicode( outPath )))
        self.progressBar.setValue(0)
        self.buttonOk.setEnabled( True )

    def outFile(self):
        self.outShape.clear()
        ( self.shapefileName, self.encoding ) = ftools_utils.saveDialog( self )
        if self.shapefileName is None or self.encoding is None:
            return
        self.outShape.setText( self.shapefileName )

    def compute(self, inPoly, inLns, inField, outPath, progressBar):
        polyLayer = ftools_utils.getVectorLayerByName(inPoly)
        lineLayer = ftools_utils.getVectorLayerByName(inLns)
        polyProvider = polyLayer.dataProvider()
        lineProvider = lineLayer.dataProvider()
        if polyProvider.crs() != lineProvider.crs():
            QMessageBox.warning(self, self.tr("CRS warning!"), self.tr("Warning: Input layers have non-matching CRS.\nThis may cause unexpected results."))
        fieldList = ftools_utils.getFieldList(polyLayer)
        index = polyProvider.fieldNameIndex(unicode(inField))
        if index == -1:
            index = polyProvider.fields().count()
            fieldList.append( QgsField(unicode(inField), QVariant.Double, "real", 24, 15, self.tr("length field")) )
        sRs = polyProvider.crs()
        inFeat = QgsFeature()
        inFeatB = QgsFeature()
        outFeat = QgsFeature()
        inGeom = QgsGeometry()
        outGeom = QgsGeometry()
        distArea = QgsDistanceArea()
        start = 0.00
        add = 100.00 / polyProvider.featureCount()
        check = QFile(self.shapefileName)
        if check.exists():
            if not QgsVectorFileWriter.deleteShapeFile(self.shapefileName):
                return
        writer = QgsVectorFileWriter(self.shapefileName, self.encoding, fieldList, polyProvider.geometryType(), sRs)
        spatialIndex = ftools_utils.createIndex( lineProvider )
        polyFit = polyProvider.getFeatures()
        while polyFit.nextFeature(inFeat):
            inGeom = QgsGeometry(inFeat.geometry())
            atMap = inFeat.attributes()
            lineList = []
            length = 0
            lineList = spatialIndex.intersects(inGeom.boundingBox())
            if len(lineList) > 0: check = 0
            else: check = 1
            if check == 0:
                for i in lineList:
                    lineProvider.getFeatures( QgsFeatureRequest().setFilterFid( int(i) ) ).nextFeature( inFeatB )
                    tmpGeom = QgsGeometry( inFeatB.geometry() )
                    if inGeom.intersects(tmpGeom):
                        outGeom = inGeom.intersection(tmpGeom)
                        length = length + distArea.measure(outGeom)
            outFeat.setGeometry(inGeom)
            atMap.append(length)
            outFeat.setAttributes(atMap)
            writer.addFeature(outFeat)
            start = start + 1
            progressBar.setValue( start * (add))
        del writer
