# -*- coding: utf-8 -*-
#-----------------------------------------------------------
# 
# Locate Line Intersections
#
# A QGIS plugin for locating line intersections, and exporting 
# them as a point shapefile.
#
# Copyright (C) 2008  Carson Farmer
#
# EMAIL: carson.farmer (at) gmail.com
# WEB  : www.geog.uvic.ca/spar/carson
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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
import ftools_utils
from qgis.core import *
from ui_frmIntersectLines import Ui_Dialog

class Dialog(QDialog, Ui_Dialog):

	def __init__(self, iface):
		QDialog.__init__(self)
		self.iface = iface
		# Set up the user interface from Designer.
		self.setupUi(self)
		QObject.connect(self.toolOut, SIGNAL("clicked()"), self.outFile)
		QObject.connect(self.inLine1, SIGNAL("currentIndexChanged(QString)"), self.update1)
		QObject.connect(self.inLine2, SIGNAL("currentIndexChanged(QString)"), self.update2)
		self.setWindowTitle( self.tr("Line intersections") )
		# populate layer list
		self.progressBar.setValue(0)
		mapCanvas = self.iface.mapCanvas()
		for i in range(mapCanvas.layerCount()):
			layer = mapCanvas.layer(i)
			if layer.type() == layer.VectorLayer:
				if layer.geometryType() == QGis.Line:
					self.inLine1.addItem(layer.name())
					self.inLine2.addItem(layer.name())

	def update1(self, inputLayer):
		self.inField1.clear()
		changedLayer = self.getVectorLayerByName(unicode(inputLayer))
		changedField = self.getFieldList(changedLayer)
		for i in changedField:
			self.inField1.addItem(unicode(changedField[i].name()))

	def update2(self, inputLayer):
		self.inField2.clear()
		changedLayer = self.getVectorLayerByName(unicode(inputLayer))
		changedField = self.getFieldList(changedLayer)
		for i in changedField:
			self.inField2.addItem(unicode(changedField[i].name()))

	def accept(self):
		if self.inLine1.currentText() == "":
			QMessageBox.information(self, self.tr("Locate Line Intersections"), self.tr("Please specify input line layer") )
		elif self.outShape.text() == "":
			QMessageBox.information(self, self.tr("Locate Line Intersections"), self.tr("Please specify output shapefile") )
		elif self.inLine2.currentText() == "":
			QMessageBox.information(self, self.tr("Locate Line Intersections"), self.tr("Please specify line intersect layer") )
		elif self.inField1.currentText() == "":
			QMessageBox.information(self, self.tr("Locate Line Intersections"), self.tr("Please specify input unique ID field") )
		elif self.inField2.currentText() == "":
			QMessageBox.information(self, self.tr("Locate Line Intersections"), self.tr("Please specify intersect unique ID field") )
		else:
			line1 = self.inLine1.currentText()
			line2 = self.inLine2.currentText()
			field1 = self.inField1.currentText()
			field2 = self.inField2.currentText()
			outPath = self.outShape.text()
			if outPath.contains("\\"):
				outName = outPath.right((outPath.length() - outPath.lastIndexOf("\\")) - 1)
			else:
				outName = outPath.right((outPath.length() - outPath.lastIndexOf("/")) - 1)
			if outName.endsWith(".shp"):
				outName = outName.left(outName.length() - 4)
			self.outShape.clear()
			self.compute(line1, line2, field1, field2, outPath, self.progressBar)
			self.progressBar.setValue(100)
			addToTOC = QMessageBox.question(self, self.tr("Generate Centroids"), self.tr("Created output point shapefile:\n%1\n\nWould you like to add the new layer to the TOC?").arg( outPath ), QMessageBox.Yes, QMessageBox.No, QMessageBox.NoButton)
			if addToTOC == QMessageBox.Yes:
				self.vlayer = QgsVectorLayer(outPath, unicode(outName), "ogr")
				QgsMapLayerRegistry.instance().addMapLayer(self.vlayer)
		self.progressBar.setValue(0)

	def outFile(self):
		self.outShape.clear()
		( self.shapefileName, self.encoding ) = ftools_utils.saveDialog( self )
		if self.shapefileName is None or self.encoding is None:
			return
		self.outShape.setText( QString( self.shapefileName ) )

	def compute(self, line1, line2, field1, field2, outPath, progressBar):
		layer1 = self.getVectorLayerByName(line1)
		layer2 = self.getVectorLayerByName(line2)
		provider1 = layer1.dataProvider()
		provider2 = layer2.dataProvider()
		allAttrs = provider1.attributeIndexes()
		provider1.select(allAttrs)
		allAttrs = provider2.attributeIndexes()
		provider2.select(allAttrs)
		fieldList = self.getFieldList(layer1)
		index1 = provider1.fieldNameIndex(field1)
		field1 = fieldList[index1]
		field1.setName(unicode(field1.name()) + "_1")
		fieldList = self.getFieldList(layer2)
		index2 = provider2.fieldNameIndex(field2)
		field2 = fieldList[index2]
		field2.setName(unicode(field2.name()) + "_2")
		fieldList = {0:field1, 1:field2}
		sRs = provider1.crs()
		check = QFile(self.shapefileName)
		if check.exists():
			if not QgsVectorFileWriter.deleteShapeFile(self.shapefileName):
				return
		writer = QgsVectorFileWriter(self.shapefileName, self.encoding, fieldList, QGis.WKBPoint, sRs)
		#writer = QgsVectorFileWriter(outPath, "UTF-8", fieldList, QGis.WKBPoint, sRs)
		inFeat = QgsFeature()
		outFeat = QgsFeature()
		inGeom = QgsGeometry()
		tempGeom = QgsGeometry()
		start = 15.00
		add = 85.00 / layer1.featureCount()
		while provider1.nextFeature(inFeat):
			inGeom = inFeat.geometry()
			lineList = []
			#(check, lineList) = layer2.featuresInRectangle(inGeom.boundingBox(), True, True)
			# Below is a work-around for featuresInRectangle
			# Which appears to have been removed for QGIS version 1.0
			layer2.select(inGeom.boundingBox(), False)
			lineList = layer2.selectedFeatures()
			if len(lineList) > 0: check = 0
			else: check = 1
			if check == 0:
				for i in lineList:
					tempGeom = i.geometry()
					tempList = []
					atMap1 = inFeat.attributeMap()
					atMap2 = i.attributeMap()
					if inGeom.intersects(tempGeom):
						tempGeom = inGeom.intersection(tempGeom)
						if tempGeom.type() == QGis.Point:
							if tempGeom.isMultipart():
								tempList = tempGeom.asMultiPoint()
							else:
								tempList.append(tempGeom.asPoint())
							for j in tempList:
								outFeat.setGeometry(tempGeom.fromPoint(j))
								outFeat.addAttribute(0, atMap1[index1])
								outFeat.addAttribute(1, atMap2[index2])
								writer.addFeature(outFeat)
			start = start + add
			progressBar.setValue(start)
		del writer

	def getVectorLayerByName(self, myName):
		mc = self.iface.mapCanvas()
		nLayers = mc.layerCount()
		for l in range(nLayers):
			layer = mc.layer(l)
			if layer.name() == unicode(myName):
				vlayer = QgsVectorLayer(unicode(layer.source()),  unicode(myName),  unicode(layer.dataProvider().name()))
				if vlayer.isValid():
					return vlayer
				else:
					QMessageBox.information(self, self.tr("Locate Line Intersections"), self.tr("Vector layer is not valid") )

	def getFieldList(self, vlayer):
		fProvider = vlayer.dataProvider()
		feat = QgsFeature()
		allAttrs = fProvider.attributeIndexes()
		fProvider.select(allAttrs)
		myFields = fProvider.fields()
		return myFields
