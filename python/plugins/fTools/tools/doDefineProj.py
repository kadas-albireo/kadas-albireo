from PyQt4.QtCore import *
from PyQt4.QtGui import *

from qgis.core import *
from qgis.gui import *

from frmReProject import Ui_Dialog

class Dialog(QDialog, Ui_Dialog):
	def __init__(self, iface):
		QDialog.__init__(self)
		self.iface = iface
		self.setupUi(self)
		self.toolOut.setEnabled(False)
		self.toolOut.setVisible(False)
		self.outShape.setEnabled(False)
		self.outShape.setVisible(False)
		self.label_2.setVisible(False)
		self.label_2.setEnabled(False)
		self.setWindowTitle("Define current projection")
		QObject.connect(self.btnProjection, SIGNAL("clicked()"), self.outProjFile)
		QObject.connect(self.inShape, SIGNAL("currentIndexChanged(QString)"), self.updateProj1)
		QObject.connect(self.cmbLayer, SIGNAL("currentIndexChanged(QString)"), self.updateProj2)
		# populate layer list
		self.progressBar.setValue(0)
		mapCanvas = self.iface.mapCanvas()
		for i in range(mapCanvas.layerCount()):
			layer = mapCanvas.layer(i)
			if layer.type() == layer.VectorLayer:
				self.inShape.addItem(layer.name())
				self.cmbLayer.addItem(layer.name())

	def updateProj1(self, layerName):
		tempLayer = self.getVectorLayerByName(layerName)
		crs = tempLayer.dataProvider().crs().toProj4()
		self.inRef.insert(unicode(crs))	

	def updateProj2(self, layerName):
		tempLayer = self.getVectorLayerByName(layerName)
		crs = tempLayer.dataProvider().crs().toProj4()
		self.outRef.insert(unicode(crs))	

	def accept(self):
		if self.inShape.currentText() == "":
			QMessageBox.information(self, "Define current projection", "No input shapefile specified")
		elif self.txtProjection.text() == "" and self.rdoProjection.isChecked():
			QMessageBox.information(self, "Define current projection", "Please specify spatial reference system")
		elif self.cmbLayer.currentText() == "" and self.rdoLayer.isChecked():
			QMessageBox.information(self, "Define current projection", "Please specify spatial reference system")
		else:
			self.progressBar.setValue(5)
			inName = self.inShape.currentText()
			self.progressBar.setValue(10)
			vLayer = self.getVectorLayerByName(inName)
			self.progressBar.setValue(30)
			if vLayer == "Error":
				QMessageBox.information(self, "Define current projection", "Cannot define projection for PostGIS data...yet!")
			else:
				srsDefine = QgsCoordinateReferenceSystem()
				if self.rdoProjection.isChecked():
					outProj = self.txtProjection.text()
					srsDefine.createFromProj4(outProj)
				else:
					destLayer = self.getVectorLayerByName(self.cmbLayer.currentText())
					srsDefine = destLayer.srs()
				if srsDefine == vLayer.srs():
					QMessageBox.information(self, "Define current projection", "Identical output spatial reference system chosen")
				else:
					provider = vLayer.dataProvider()
					self.progressBar.setValue(35)
					inPath = provider.dataSourceUri()
					self.progressBar.setValue(40)
					if inPath.endsWith(".shp"):
						inPath = inPath.left(inPath.length() - 4)
					self.progressBar.setValue(55)
					if not srsDefine.isValid():
						QMessageBox.information(self, "Define current projection", "Output spatial reference system is not valid")
					else:
						self.progressBar.setValue(60)
						outputWkt = srsDefine.toWkt()
						self.progressBar.setValue(65)
						outputPrj = open(inPath + ".prj", "w")
						self.progressBar.setValue(70)
						outputPrj.write(outputWkt)
						self.progressBar.setValue(75)
						outputPrj.close()
						mLayer = self.getMapLayerByName(inName)
						self.progressBar.setValue(90)
						if not mLayer.isValid():
							QMessageBox.information(self, "Define current projection", "Unable to dynamically define projection.\n"
				        	+ "Please reload layer manually for projection definition to take effect.")
						else:
							self.progressBar.setValue(95)
							mLayer.setCrs(srsDefine)
							self.progressBar.setValue(100)
							QMessageBox.information(self, "Define current projection", "Defined Projection For:\n" + inPath + ".shp")
		self.progressBar.setValue(0)

	def outProjFile(self):
		format = QString( "<h2>%1</h2>%2 <br/> %3" )
		header = QString( "Define layer CRS:" )
		sentence1 = QString( "Please select the projection system that defines the current layer." )
		sentence2 = QString( "Layer CRS information will be updated to the selected CRS." )
		self.projSelect = QgsGenericProjectionSelector(self, Qt.Widget)
		self.projSelect.setMessage( format.arg( header ).arg( sentence1 ).arg( sentence2 ))
		if self.projSelect.exec_():
			projString = self.projSelect.selectedProj4String()
			if projString == "":
				QMessageBox.information(self, "Export to new projection", "No Valid CRS selected")
				return
			else:
				self.txtProjection.clear()
				self.txtProjection.insert(projString)
		else:
			return

# Gets vector layer by layername in canvas
	def getVectorLayerByName(self, myName):
		mc = self.iface.mapCanvas()
		nLayers = mc.layerCount()
		for l in range(nLayers):
			layer = mc.layer(l)
			if layer.name() == unicode(myName,'latin1'):
				if unicode(layer.dataProvider().name()) == "postgres":
					return "Error"
				else:
					vlayer = QgsVectorLayer(unicode(layer.source()),  unicode(myName),  unicode(layer.dataProvider().name()))
					if vlayer.isValid():
						return vlayer

#Gets map layer by layername in canvas
#Return: QgsMapLayer
	def getMapLayerByName(self,myName):
		mc = self.iface.mapCanvas()
		nLayers = mc.layerCount()
		for l in range(nLayers):
			layer = mc.layer(l)
			if layer.name() == unicode(myName,'latin1'):
				return layer
