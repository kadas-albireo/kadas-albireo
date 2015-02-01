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

from PyQt4.QtCore import QObject, SIGNAL, QThread, QMutex, QVariant, QFile
from PyQt4.QtGui import QDialog, QDialogButtonBox, QMessageBox
import ftools_utils
from qgis.core import QGis, QgsFeatureRequest, QgsField, QgsVectorFileWriter, QgsFeature, QgsGeometry
from ui_frmPointsInPolygon import Ui_Dialog

class Dialog(QDialog, Ui_Dialog):

    def __init__(self, iface):
        QDialog.__init__(self, iface.mainWindow())
        self.iface = iface
        self.setupUi(self)

        self.setWindowTitle(self.tr("Count Points in Polygon"))
        self.btnOk = self.buttonBox.button( QDialogButtonBox.Ok )
        self.btnClose = self.buttonBox.button( QDialogButtonBox.Close )

        QObject.connect(self.toolOut, SIGNAL("clicked()"), self.outFile)
        self.progressBar.setValue(0)
        self.populateLayers()

    def populateLayers( self ):
        layers = ftools_utils.getLayerNames([QGis.Polygon])
        self.inPolygon.clear()
        self.inPolygon.addItems(layers)

        self.inPoint.clear()
        layers = ftools_utils.getLayerNames([QGis.Point])
        self.inPoint.addItems(layers)

    def outFile(self):
        self.outShape.clear()
        (self.shapefileName, self.encoding) = ftools_utils.saveDialog(self)
        if self.shapefileName is None or self.encoding is None:
            return
        self.outShape.setText(self.shapefileName)

    def accept(self):
        if self.inPolygon.currentText() == "":
            QMessageBox.information(self, self.tr("Count Points In Polygon"),
                                    self.tr("Please specify input polygon vector layer"))
        elif self.inPoint.currentText() == "":
            QMessageBox.information(self, self.tr("Count Points In Polygon"),
                                    self.tr("Please specify input point vector layer"))
        elif self.lnField.text() == "":
            QMessageBox.information(self, self.tr("Count Points In Polygon"),
                                    self.tr("Please specify output count field"))
        elif self.outShape.text() == "":
            QMessageBox.information(self, self.tr("Count Points In Polygon"),
                                    self.tr("Please specify output shapefile"))
        else:
            inPoly = ftools_utils.getVectorLayerByName(self.inPolygon.currentText())
            inPnts = ftools_utils.getVectorLayerByName(self.inPoint.currentText())

            polyProvider = inPoly.dataProvider()
            pointProvider = inPnts.dataProvider()
            if polyProvider.crs() != pointProvider.crs():
                QMessageBox.warning(self, self.tr("CRS warning!"),
                                    self.tr("Warning: Input layers have non-matching CRS.\nThis may cause unexpected results."))

            self.btnOk.setEnabled(False)

            self.workThread = PointsInPolygonThread(inPoly, inPnts, self.lnField.text(), self.outShape.text(), self.encoding)

            QObject.connect(self.workThread, SIGNAL("rangeChanged(int)"), self.setProgressRange)
            QObject.connect(self.workThread, SIGNAL("updateProgress()"), self.updateProgress)
            QObject.connect(self.workThread, SIGNAL("processingFinished()"), self.processFinished)
            QObject.connect(self.workThread, SIGNAL("processingInterrupted()"), self.processInterrupted)

            self.btnClose.setText(self.tr("Cancel"))
            QObject.disconnect(self.buttonBox, SIGNAL("rejected()"), self.reject)
            QObject.connect(self.btnClose, SIGNAL("clicked()"), self.stopProcessing)

            self.workThread.start()

    def setProgressRange(self, maxValue):
        self.progressBar.setRange(0, maxValue)
        self.progressBar.setValue(0)

    def updateProgress(self):
        self.progressBar.setValue(self.progressBar.value() + 1)

    def processFinished(self):
        self.stopProcessing()
        if self.addToCanvasCheck.isChecked():
            addCanvasCheck = ftools_utils.addShapeToCanvas(unicode(self.outShape.text()))
            if not addCanvasCheck:
                QMessageBox.warning( self, self.tr("Count Points in Polygon"), self.tr( "Error loading output shapefile:\n%s" ) % ( unicode( self.outShape.text() ) ))
            self.populateLayers()
        else:
            QMessageBox.information(self, self.tr("Count Points in Polygon"),self.tr("Created output shapefile:\n%s" ) % ( unicode( self.outShape.text() )))

        self.restoreGui()

    def processInterrupted(self):
        self.restoreGui()

    def stopProcessing(self):
        if self.workThread is not None:
            self.workThread.stop()
            self.workThread = None

    def restoreGui(self):
        self.progressBar.setRange(0, 1)
        self.progressBar.setValue(0)
        self.outShape.clear()

        QObject.disconnect(self.btnClose, SIGNAL("clicked()"), self.stopProcessing)
        QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
        self.btnClose.setText(self.tr("Close"))
        self.btnOk.setEnabled(True)

class PointsInPolygonThread(QThread):
    def __init__( self, inPoly, inPoints, fieldName, outPath, encoding ):
        QThread.__init__( self, QThread.currentThread() )
        self.mutex = QMutex()
        self.stopMe = 0
        self.interrupted = False

        self.layerPoly = inPoly
        self.layerPoints = inPoints
        self.fieldName = fieldName
        self.outPath = outPath
        self.encoding = encoding

    def run(self):
        self.mutex.lock()
        self.stopMe = 0
        self.mutex.unlock()

        interrupted = False

        polyProvider = self.layerPoly.dataProvider()
        pointProvider = self.layerPoints.dataProvider()

        fieldList = ftools_utils.getFieldList(self.layerPoly)
        index = polyProvider.fieldNameIndex(unicode(self.fieldName))
        if index == -1:
            index = polyProvider.fields().count()
            fieldList.append( QgsField(unicode(self.fieldName), QVariant.Int, "int", 10, 0, self.tr("point count field")) )

        sRs = polyProvider.crs()
        if QFile(self.outPath).exists():
            if not QgsVectorFileWriter.deleteShapeFile(self.outPath):
                return

        writer = QgsVectorFileWriter(self.outPath, self.encoding, fieldList,
                                     polyProvider.geometryType(), sRs)

        spatialIndex = ftools_utils.createIndex( pointProvider )

        self.emit(SIGNAL("rangeChanged(int)"), polyProvider.featureCount() )

        polyFeat = QgsFeature()
        pntFeat = QgsFeature()
        outFeat = QgsFeature()
        inGeom = QgsGeometry()
        polyFit = polyProvider.getFeatures()
        while polyFit.nextFeature(polyFeat):
            inGeom = polyFeat.geometry()
            atMap = polyFeat.attributes()
            outFeat.setAttributes(atMap)
            outFeat.setGeometry(inGeom)

            count = 0
            pointList = []
            hasIntersection = True
            pointList = spatialIndex.intersects(inGeom.boundingBox())
            if len(pointList) > 0:
                hasIntersection = True
            else:
                hasIntersection = False

            if hasIntersection:
                for p in pointList:
                    pointProvider.getFeatures( QgsFeatureRequest().setFilterFid( p ) ).nextFeature( pntFeat )
                    tmpGeom = QgsGeometry(pntFeat.geometry())
                    if inGeom.intersects(tmpGeom):
                        count += 1

                    self.mutex.lock()
                    s = self.stopMe
                    self.mutex.unlock()
                    if s == 1:
                        interrupted = True
                        break

            atMap.append(count)
            outFeat.setAttributes(atMap)
            writer.addFeature(outFeat)

            self.emit( SIGNAL( "updateProgress()" ) )

            self.mutex.lock()
            s = self.stopMe
            self.mutex.unlock()
            if s == 1:
                interrupted = True
                break

        del writer

        if not interrupted:
            self.emit( SIGNAL( "processingFinished()" ) )
        else:
            self.emit( SIGNAL( "processingInterrupted()" ) )

    def stop(self):
        self.mutex.lock()
        self.stopMe = 1
        self.mutex.unlock()

        QThread.wait( self )
