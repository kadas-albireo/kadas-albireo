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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from ui_frmGeometry import Ui_Dialog
import ftools_utils
import math
from itertools import izip
import voronoi
from sets import Set

class GeometryDialog(QDialog, Ui_Dialog):

  def __init__(self, iface, function):
    QDialog.__init__(self)
    self.iface = iface
    self.setupUi(self)
    self.myFunction = function
    self.buttonOk = self.buttonBox_2.button( QDialogButtonBox.Ok )
    QObject.connect(self.toolOut, SIGNAL("clicked()"), self.outFile)
    if self.myFunction == 1:
      QObject.connect(self.inShape, SIGNAL("currentIndexChanged(QString)"), self.update)
    self.manageGui()
    self.success = False
    self.cancel_close = self.buttonBox_2.button( QDialogButtonBox.Close )
    self.progressBar.setValue(0)

  def update(self):
    self.cmbField.clear()
    inputLayer = unicode(self.inShape.currentText())
    if inputLayer != "":
      changedLayer = ftools_utils.getVectorLayerByName(inputLayer)
      changedField = ftools_utils.getFieldList(changedLayer)
      for i in changedField:
        self.cmbField.addItem(unicode(changedField[i].name()))
      self.cmbField.addItem("--- " + self.tr( "Merge all" ) + " ---")

  def accept(self):
    if self.inShape.currentText() == "":
      QMessageBox.information(self, self.tr("Geometry"), self.tr( "Please specify input vector layer" ) )
    elif self.outShape.text() == "":
      QMessageBox.information(self, self.tr("Geometry"), self.tr( "Please specify output shapefile" ) )
    elif self.lineEdit.isVisible() and self.lineEdit.value() < 0.00:
      QMessageBox.information(self, self.tr("Geometry"), self.tr( "Please specify valid tolerance value" ) )
    elif self.cmbField.isVisible() and self.cmbField.currentText() == "":
      QMessageBox.information(self, self.tr("Geometry"), self.tr( "Please specify valid UID field" ) )
    else:
      self.outShape.clear()
      self.geometry( self.inShape.currentText(), self.lineEdit.value(), self.cmbField.currentText() )

  def outFile(self):
    self.outShape.clear()
    (self.shapefileName, self.encoding) = ftools_utils.saveDialog(self)
    if self.shapefileName is None or self.encoding is None:
      return
    self.outShape.setText(QString(self.shapefileName))

  def manageGui(self):
    if self.myFunction == 1: # Singleparts to multipart
      self.setWindowTitle( self.tr( "Singleparts to multipart" ) )
      self.lineEdit.setVisible(False)
      self.label.setVisible(False)
      self.label_2.setText( self.tr( "Output shapefile" ) )
      self.cmbField.setVisible(True)
      self.field_label.setVisible(True)
    elif self.myFunction == 2: # Multipart to singleparts
      self.setWindowTitle( self.tr( "Multipart to singleparts" ) )
      self.lineEdit.setVisible(False)
      self.label.setVisible(False)
      self.label_2.setText(self.tr(  "Output shapefile" ) )
      self.cmbField.setVisible(False)
      self.field_label.setVisible(False)
    elif self.myFunction == 3: # Extract nodes
      self.setWindowTitle( self.tr( "Extract nodes" ) )
      self.lineEdit.setVisible(False)
      self.label.setVisible(False)
      self.cmbField.setVisible(False)
      self.field_label.setVisible(False)
    elif self.myFunction == 4: # Polygons to lines
      self.setWindowTitle( self.tr(  "Polygons to lines" ) )
      self.label_2.setText( self.tr( "Output shapefile" ) )
      self.label_3.setText( self.tr( "Input polygon vector layer" ) )
      self.label.setVisible(False)
      self.lineEdit.setVisible(False)
      self.cmbField.setVisible(False)
      self.field_label.setVisible(False)
    elif self.myFunction == 5: # Export/Add geometry columns
      self.setWindowTitle( self.tr(  "Export/Add geometry columns" ) )
      self.label_2.setText( self.tr( "Output shapefile" ) )
      self.label_3.setText( self.tr( "Input vector layer" ) )
      self.label.setVisible(False)
      self.lineEdit.setVisible(False)
      self.cmbField.setVisible(False)
      self.field_label.setVisible(False)
    elif self.myFunction == 7: # Polygon centroids
      self.setWindowTitle( self.tr( "Polygon centroids" ) )
      self.label_2.setText( self.tr( "Output point shapefile" ) )
      self.label_3.setText( self.tr( "Input polygon vector layer" ) )
      self.label.setVisible( False )
      self.lineEdit.setVisible( False )
      self.cmbField.setVisible( False )
      self.field_label.setVisible( False )
    else:
      if self.myFunction == 8: # Delaunay triangulation
        self.setWindowTitle( self.tr( "Delaunay triangulation" ) )
        self.label_3.setText( self.tr( "Input point vector layer" ) )
        self.label.setVisible( False )
        self.lineEdit.setVisible( False )
      elif self.myFunction == 10: # Voronoi Polygons
        self.setWindowTitle( self.tr( "Voronoi polygon" ) )
        self.label_3.setText( self.tr( "Input point vector layer" ) )
        self.label.setText( self.tr( "Buffer region" ) )
        self.lineEdit.setSuffix(" %")
        self.lineEdit.setRange(0, 100)
        self.lineEdit.setSingleStep(5)
        self.lineEdit.setValue(0)
      elif self.myFunction == 11: #Lines to polygons
        self.setWindowTitle( self.tr(  "Lines to polygons" ) )
        self.label_2.setText( self.tr( "Output shapefile" ) )
        self.label_3.setText( self.tr( "Input line vector layer" ) )
        self.label.setVisible(False)
        self.lineEdit.setVisible(False)
        self.cmbField.setVisible(False)
        self.field_label.setVisible(False)

      else: # Polygon from layer extent
        self.setWindowTitle( self.tr( "Polygon from layer extent" ) )
        self.label_3.setText( self.tr( "Input layer" ) )
        self.label.setVisible( False )
        self.lineEdit.setVisible( False )
      self.label_2.setText( self.tr( "Output polygon shapefile" ) )
      self.cmbField.setVisible( False )
      self.field_label.setVisible( False )
    self.resize( 381, 100 )
    self.populateLayers()

  def populateLayers( self ):
    self.inShape.clear()
    if self.myFunction == 3 or self.myFunction == 6:
      myList = ftools_utils.getLayerNames( [ QGis.Polygon, QGis.Line ] )
    elif self.myFunction == 4 or self.myFunction == 7:
      myList = ftools_utils.getLayerNames( [ QGis.Polygon ] )
    elif self.myFunction == 8 or self.myFunction == 10:
      myList = ftools_utils.getLayerNames( [ QGis.Point ] )
    elif self.myFunction == 9:
      myList = ftools_utils.getLayerNames( "all" )
    elif self.myFunction == 11:
      myList = ftools_utils.getLayerNames( [ QGis.Line ] )
    else:
      myList = ftools_utils.getLayerNames( [ QGis.Point, QGis.Line, QGis.Polygon ] )
    self.inShape.addItems( myList )

#1:  Singleparts to multipart
#2:  Multipart to singleparts
#3:  Extract nodes
#4:  Polygons to lines
#5:  Export/Add geometry columns
#6:  Simplify geometries (disabled)
#7:  Polygon centroids
#8: Delaunay triangulation
#9: Polygon from layer extent
#10:Voronoi polygons
#11: Lines to polygons

  def geometry( self, myLayer, myParam, myField ):
    if self.myFunction == 9:
      vlayer = ftools_utils.getMapLayerByName( myLayer )
    else:
      vlayer = ftools_utils.getVectorLayerByName( myLayer )
    error = False
    check = QFile( self.shapefileName )
    if check.exists():
      if not QgsVectorFileWriter.deleteShapeFile( self.shapefileName ):
        QMessageBox.warning( self, self.tr("Geoprocessing"), self.tr( "Unable to delete existing shapefile." ) )
        return
    self.buttonOk.setEnabled( False )
    self.testThread = geometryThread( self.iface.mainWindow(), self, self.myFunction, vlayer, myParam,
    myField, self.shapefileName, self.encoding )
    QObject.connect( self.testThread, SIGNAL( "runFinished(PyQt_PyObject)" ), self.runFinishedFromThread )
    QObject.connect( self.testThread, SIGNAL( "runStatus(PyQt_PyObject)" ), self.runStatusFromThread )
    QObject.connect( self.testThread, SIGNAL( "runRange(PyQt_PyObject)" ), self.runRangeFromThread )
    self.cancel_close.setText( self.tr("Cancel") )
    QObject.connect( self.cancel_close, SIGNAL( "clicked()" ), self.cancelThread )
    self.testThread.start()

  def cancelThread( self ):
    self.testThread.stop()
    self.buttonOk.setEnabled( True )

  def runFinishedFromThread( self, success ):
    self.testThread.stop()
    self.buttonOk.setEnabled( True )
    extra = ""
    if success == "math_error":
      QMessageBox.warning( self, self.tr("Geometry"), self.tr("Error processing specified tolerance!\nPlease choose larger tolerance...") )
      if not QgsVectorFileWriter.deleteShapeFile( self.shapefileName ):
        QMessageBox.warning( self, self.tr("Geometry"), self.tr( "Unable to delete incomplete shapefile." ) )
    elif success == "attr_error":
      QMessageBox.warning( self, self.tr("Geometry"), self.tr("At least two features must have same attribute value!\nPlease choose another field...") )
      if not QgsVectorFileWriter.deleteShapeFile( self.shapefileName ):
        QMessageBox.warning( self, self.tr("Geometry"), self.tr( "Unable to delete incomplete shapefile." ) )
    else:
      if success == "valid_error":
        extra = self.tr("One or more features in the output layer may have invalid "
                      + "geometry, please check using the check validity tool\n")
        success = True
      self.cancel_close.setText( "Close" )
      QObject.disconnect( self.cancel_close, SIGNAL( "clicked()" ), self.cancelThread )
      if success:
        addToTOC = QMessageBox.question( self, self.tr("Geometry"),
          self.tr( "Created output shapefile:\n%1\n%2\n\nWould you like to add the new layer to the TOC?" ).arg( unicode( self.shapefileName ) ).arg( extra ),
          QMessageBox.Yes, QMessageBox.No, QMessageBox.NoButton )
        if addToTOC == QMessageBox.Yes:
          if not ftools_utils.addShapeToCanvas( unicode( self.shapefileName ) ):
            QMessageBox.warning( self, self.tr("Geoprocessing"), self.tr( "Error loading output shapefile:\n%1" ).arg( unicode( self.shapefileName ) ))
          self.populateLayers()
      else:
        QMessageBox.warning( self, self.tr("Geometry"), self.tr( "Error writing output shapefile." ) )

  def runStatusFromThread( self, status ):
    self.progressBar.setValue( status )

  def runRangeFromThread( self, range_vals ):
    self.progressBar.setRange( range_vals[ 0 ], range_vals[ 1 ] )

class geometryThread( QThread ):
  def __init__( self, parentThread, parentObject, function, vlayer, myParam, myField, myName, myEncoding ):
    QThread.__init__( self, parentThread )
    self.parent = parentObject
    self.running = False
    self.myFunction = function
    self.vlayer = vlayer
    self.myParam = myParam
    self.myField = myField
    self.myName = myName
    self.myEncoding = myEncoding

  def run( self ):
    self.running = True
    if self.myFunction == 1: # Singleparts to multipart
      success = self.single_to_multi()
    elif self.myFunction == 2: # Multipart to singleparts
      success = self.multi_to_single()
    elif self.myFunction == 3: # Extract nodes
      success = self.extract_nodes()
    elif self.myFunction == 4: # Polygons to lines
      success = self.polygons_to_lines()
    elif self.myFunction == 5: # Export/Add geometry columns
      success = self.export_geometry_info()
    # note that 6 used to be associated with simplify_geometry
    elif self.myFunction == 7: # Polygon centroids
      success = self.polygon_centroids()
    elif self.myFunction == 8: # Delaunay triangulation
      success = self.delaunay_triangulation()
    elif self.myFunction == 9: # Polygon from layer extent
      success = self.layer_extent()
    elif self.myFunction == 10: # Voronoi Polygons
      success = self.voronoi_polygons()
    elif self.myFunction == 11: # Lines to polygons
      success = self.lines_to_polygons()
    self.emit( SIGNAL( "runFinished(PyQt_PyObject)" ), success )
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )

  def stop(self):
    self.running = False

  def single_to_multi( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = vprovider.fields()
    allValid = True
    geomType = self.singleToMultiGeom(vprovider.geometryType())
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
        fields, geomType, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    inGeom = QgsGeometry()
    outGeom = QgsGeometry()
    index = vprovider.fieldNameIndex( self.myField )
    if not index == -1:
      unique = ftools_utils.getUniqueValues( vprovider, int( index ) )
    else:
      unique = [QVariant(QString())]
    nFeat = vprovider.featureCount() * len( unique )
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    merge_all = self.myField == QString("--- " + self.tr( "Merge all" ) + " ---")
    if not len( unique ) == self.vlayer.featureCount() \
    or merge_all:
      for i in unique:
        vprovider.rewind()
        multi_feature= []
        first = True
        vprovider.select(allAttrs)
        while vprovider.nextFeature( inFeat ):
          atMap = inFeat.attributeMap()
          if not merge_all:
            idVar = atMap[ index ]
          else:
            idVar = QVariant(QString())
          if idVar.toString().trimmed() == i.toString().trimmed() \
          or merge_all:
            if first:
              atts = atMap
              first = False
            inGeom = QgsGeometry( inFeat.geometry() )
            vType = inGeom.type()
            feature_list = self.extractAsMulti( inGeom )
            multi_feature.extend( feature_list )
          nElement += 1
          self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
        outFeat.setAttributeMap( atts )
        outGeom = QgsGeometry( self.convertGeometry(multi_feature, vType) )
        if not outGeom.isGeosValid():
          allValid = "valid_error"
        outFeat.setGeometry(outGeom)
        writer.addFeature(outFeat)
      del writer
    else:
      return "attr_error"
    return allValid

  def multi_to_single( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = vprovider.fields()
    geomType = self.multiToSingleGeom(vprovider.geometryType())
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
        fields, geomType, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    inGeom = QgsGeometry()
    outGeom = QgsGeometry()
    nFeat = vprovider.featureCount()
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    while vprovider.nextFeature( inFeat ):
      nElement += 1
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), nElement )
      inGeom = inFeat.geometry()
      atMap = inFeat.attributeMap()
      featList = self.extractAsSingle( inGeom )
      outFeat.setAttributeMap( atMap )
      for i in featList:
        outFeat.setGeometry( i )
        writer.addFeature( outFeat )
    del writer
    return True

  def extract_nodes( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = vprovider.fields()
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, QGis.WKBPoint, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    inGeom = QgsGeometry()
    outGeom = QgsGeometry()
    nFeat = vprovider.featureCount()
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    while vprovider.nextFeature( inFeat ):
      nElement += 1
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
      inGeom = inFeat.geometry()
      atMap = inFeat.attributeMap()
      pointList = ftools_utils.extractPoints( inGeom )
      outFeat.setAttributeMap( atMap )
      for i in pointList:
        outFeat.setGeometry( outGeom.fromPoint( i ) )
        writer.addFeature( outFeat )
    del writer
    return True

  def polygons_to_lines( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = vprovider.fields()
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, QGis.WKBLineString, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    inGeom = QgsGeometry()
    outGeom = QgsGeometry()
    nFeat = vprovider.featureCount()
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0)
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    while vprovider.nextFeature(inFeat):
      multi = False
      nElement += 1
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
      inGeom = inFeat.geometry()
      if inGeom.isMultipart():
        multi = True
      atMap = inFeat.attributeMap()
      lineList = self.extractAsLine( inGeom )
      outFeat.setAttributeMap( atMap )
      for h in lineList:
        outFeat.setGeometry( outGeom.fromPolyline( h ) )
        writer.addFeature( outFeat )
    del writer
    return True

  def lines_to_polygons( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = vprovider.fields()
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, QGis.WKBPolygon, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    inGeom = QgsGeometry()
    nFeat = vprovider.featureCount()
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0)
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    while vprovider.nextFeature(inFeat):
      outGeomList = []
      multi = False
      nElement += 1
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
      if inFeat.geometry().isMultipart():
        outGeomList = inFeat.geometry().asMultiPolyline()
        multi = True
      else:
        outGeomList.append( inFeat.geometry().asPolyline() )
      polyGeom = self.remove_bad_lines( outGeomList )
      if len(polyGeom) <> 0:
        outFeat.setGeometry( QgsGeometry.fromPolygon( polyGeom ) )
        atMap = inFeat.attributeMap()
        outFeat.setAttributeMap( atMap )
        writer.addFeature( outFeat )
    del writer
    return True

  def export_geometry_info( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    ( fields, index1, index2 ) = self.checkGeometryFields( self.vlayer )
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, vprovider.geometryType(), vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    inGeom = QgsGeometry()
    nFeat = vprovider.featureCount()
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0)
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    while vprovider.nextFeature(inFeat):
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
      nElement += 1
      inGeom = inFeat.geometry()
      ( attr1, attr2 ) = self.simpleMeasure( inGeom )
      outFeat.setGeometry( inGeom )
      atMap = inFeat.attributeMap()
      outFeat.setAttributeMap( atMap )
      outFeat.addAttribute( index1, QVariant( attr1 ) )
      outFeat.addAttribute( index2, QVariant( attr2 ) )
      writer.addFeature( outFeat )
    del writer
    return True

  def polygon_centroids( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = vprovider.fields()
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, QGis.WKBPoint, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    nFeat = vprovider.featureCount()
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    while vprovider.nextFeature( inFeat ):
      nElement += 1
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
      inGeom = inFeat.geometry()
      atMap = inFeat.attributeMap()
      outGeom = QgsGeometry(inGeom.centroid())
      if outGeom is None:
        return "math_error"
      outFeat.setAttributeMap( atMap )
      outFeat.setGeometry( outGeom )
      writer.addFeature( outFeat )
    del writer
    return True

  def delaunay_triangulation( self ):
    import voronoi
    from sets import Set
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    fields = {
    0 : QgsField( "POINTA", QVariant.Double ),
    1 : QgsField( "POINTB", QVariant.Double ),
    2 : QgsField( "POINTC", QVariant.Double ) }
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, QGis.WKBPolygon, vprovider.crs() )
    inFeat = QgsFeature()
    c = voronoi.Context()
    pts = []
    ptDict = {}
    ptNdx = -1
    while vprovider.nextFeature(inFeat):
      geom = QgsGeometry(inFeat.geometry())
      point = geom.asPoint()
      x = point.x()
      y = point.y()
      pts.append((x, y))
      ptNdx +=1
      ptDict[ptNdx] = inFeat.id()
    if len(pts) < 3:
      return False
    uniqueSet = Set(item for item in pts)
    ids = [pts.index(item) for item in uniqueSet]
    sl = voronoi.SiteList([voronoi.Site(*i) for i in uniqueSet])
    c.triangulate = True
    voronoi.voronoi(sl, c)
    triangles = c.triangles
    feat = QgsFeature()
    nFeat = len( triangles )
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    for triangle in triangles:
      indicies = list(triangle)
      indicies.append(indicies[0])
      polygon = []
      step = 0
      for index in indicies:
        vprovider.featureAtId(ptDict[ids[index]], inFeat, True,  allAttrs)
        geom = QgsGeometry(inFeat.geometry())
        point = QgsPoint(geom.asPoint())
        polygon.append(point)
        if step <= 3: feat.addAttribute(step, QVariant(ids[index]))
        step += 1
      geometry = QgsGeometry().fromPolygon([polygon])
      feat.setGeometry(geometry)
      writer.addFeature(feat)
      nElement += 1
      self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  nElement )
    del writer
    return True

  def voronoi_polygons( self ):
    vprovider = self.vlayer.dataProvider()
    allAttrs = vprovider.attributeIndexes()
    vprovider.select( allAttrs )
    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    vprovider.fields(), QGis.WKBPolygon, vprovider.crs() )
    inFeat = QgsFeature()
    outFeat = QgsFeature()
    extent = self.vlayer.extent()
    extraX = extent.height()*(self.myParam/100.00)
    extraY = extent.width()*(self.myParam/100.00)
    height = extent.height()
    width = extent.width()
    c = voronoi.Context()
    pts = []
    ptDict = {}
    ptNdx = -1
    while vprovider.nextFeature(inFeat):
      geom = QgsGeometry(inFeat.geometry())
      point = geom.asPoint()
      x = point.x()-extent.xMinimum()
      y = point.y()-extent.yMinimum()
      pts.append((x, y))
      ptNdx +=1
      ptDict[ptNdx] = inFeat.id()
    self.vlayer = None
    if len(pts) < 3:
      return False
    uniqueSet = Set(item for item in pts)
    ids = [pts.index(item) for item in uniqueSet]
    sl = voronoi.SiteList([voronoi.Site(i[0], i[1], sitenum=j) for j, i in enumerate(uniqueSet)])
    voronoi.voronoi(sl, c)
    inFeat = QgsFeature()
    nFeat = len(c.polygons)
    nElement = 0
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, nFeat ) )
    for site, edges in c.polygons.iteritems():
      vprovider.featureAtId(ptDict[ids[site]], inFeat, True,  allAttrs)
      lines = self.clip_voronoi(edges, c, width, height, extent, extraX, extraY)
      geom = QgsGeometry.fromMultiPoint(lines)
      geom = QgsGeometry(geom.convexHull())
      outFeat.setGeometry(geom)
      outFeat.setAttributeMap(inFeat.attributeMap())
      writer.addFeature(outFeat)
      nElement += 1
      self.emit(SIGNAL("runStatus(PyQt_PyObject)" ), nElement)
    del writer
    return True


  def clip_voronoi(self, edges, c, width, height, extent, exX, exY):
    """ Clip voronoi function based on code written for Inkscape
        Copyright (C) 2010 Alvin Penner, penner@vaxxine.com
    """
    def clip_line(x1, y1, x2, y2, w, h, x, y):
      if x1 < 0-x and x2 < 0-x:
        return [0, 0, 0, 0]
      if x1 > w+x and x2 > w+x:
        return [0, 0, 0, 0]
      if x1 < 0-x:
        y1 = (y1*x2 - y2*x1)/(x2 - x1)
        x1 = 0-x
      if x2 < 0-x:
        y2 = (y1*x2 - y2*x1)/(x2 - x1)
        x2 = 0-x
      if x1 > w+x:
        y1 = y1 + (w+x - x1)*(y2 - y1)/(x2 - x1)
        x1 = w+x
      if x2 > w+x:
        y2 = y1 + (w+x - x1)*(y2 - y1)/(x2 - x1)
        x2 = w+x
      if y1 < 0-y and y2 < 0-y:
        return [0, 0, 0, 0]
      if y1 > h+y and y2 > h+y:
        return [0, 0, 0, 0]
      if x1 == x2 and y1 == y2:
        return [0, 0, 0, 0]
      if y1 < 0-y:
        x1 = (x1*y2 - x2*y1)/(y2 - y1)
        y1 = 0-y
      if y2 < 0-y:
        x2 = (x1*y2 - x2*y1)/(y2 - y1)
        y2 = 0-y
      if y1 > h+y:
        x1 = x1 + (h+y - y1)*(x2 - x1)/(y2 - y1)
        y1 = h+y
      if y2 > h+y:
        x2 = x1 + (h+y - y1)*(x2 - x1)/(y2 - y1)
        y2 = h+y
      return [x1, y1, x2, y2]
    lines = []
    hasXMin = False
    hasYMin = False
    hasXMax = False
    hasYMax = False
    for edge in edges:
      if edge[1] >= 0 and edge[2] >= 0:       # two vertices
          [x1, y1, x2, y2] = clip_line(c.vertices[edge[1]][0], c.vertices[edge[1]][1], c.vertices[edge[2]][0], c.vertices[edge[2]][1], width, height, exX, exY)
      elif edge[1] >= 0:                      # only one vertex
        if c.lines[edge[0]][1] == 0:        # vertical line
          xtemp = c.lines[edge[0]][2]/c.lines[edge[0]][0]
          if c.vertices[edge[1]][1] > (height+exY)/2:
            ytemp = height+exY
          else:
            ytemp = 0-exX
        else:
          xtemp = width+exX
          ytemp = (c.lines[edge[0]][2] - (width+exX)*c.lines[edge[0]][0])/c.lines[edge[0]][1]
        [x1, y1, x2, y2] = clip_line(c.vertices[edge[1]][0], c.vertices[edge[1]][1], xtemp, ytemp, width, height, exX, exY)
      elif edge[2] >= 0:                      # only one vertex
        if c.lines[edge[0]][1] == 0:        # vertical line
          xtemp = c.lines[edge[0]][2]/c.lines[edge[0]][0]
          if c.vertices[edge[2]][1] > (height+exY)/2:
            ytemp = height+exY
          else:
            ytemp = 0.0-exY
        else:
          xtemp = 0.0-exX
          ytemp = c.lines[edge[0]][2]/c.lines[edge[0]][1]
        [x1, y1, x2, y2] = clip_line(xtemp, ytemp, c.vertices[edge[2]][0], c.vertices[edge[2]][1], width, height, exX, exY)
      if x1 or x2 or y1 or y2:
        lines.append(QgsPoint(x1+extent.xMinimum(),y1+extent.yMinimum()))
        lines.append(QgsPoint(x2+extent.xMinimum(),y2+extent.yMinimum()))
        if 0-exX in (x1, x2):
          hasXMin = True
        if 0-exY in (y1, y2):
          hasYMin = True
        if height+exY in (y1, y2):
          hasYMax = True
        if width+exX in (x1, x2):
          hasXMax = True
    if hasXMin:
      if hasYMax:
        lines.append(QgsPoint(extent.xMinimum()-exX, height+extent.yMinimum()+exY))
      if hasYMin:
        lines.append(QgsPoint(extent.xMinimum()-exX, extent.yMinimum()-exY))
    if hasXMax:
      if hasYMax:
        lines.append(QgsPoint(width+extent.xMinimum()+exX, height+extent.yMinimum()+exY))
      if hasYMin:
        lines.append(QgsPoint(width+extent.xMinimum()+exX, extent.yMinimum()-exY))
    return lines

  def layer_extent( self ):
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ), 0 )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, 0 ) )
    fields = {
    0 : QgsField( "MINX", QVariant.Double ),
    1 : QgsField( "MINY", QVariant.Double ),
    2 : QgsField( "MAXX", QVariant.Double ),
    3 : QgsField( "MAXY", QVariant.Double ),
    4 : QgsField( "CNTX", QVariant.Double ),
    5 : QgsField( "CNTY", QVariant.Double ),
    6 : QgsField( "AREA", QVariant.Double ),
    7 : QgsField( "PERIM", QVariant.Double ),
    8 : QgsField( "HEIGHT", QVariant.Double ),
    9 : QgsField( "WIDTH", QVariant.Double ) }

    writer = QgsVectorFileWriter( self.myName, self.myEncoding,
    fields, QGis.WKBPolygon, self.vlayer.crs() )
    rect = self.vlayer.extent()
    minx = rect.xMinimum()
    miny = rect.yMinimum()
    maxx = rect.xMaximum()
    maxy = rect.yMaximum()
    height = rect.height()
    width = rect.width()
    cntx = minx + ( width / 2.0 )
    cnty = miny + ( height / 2.0 )
    area = width * height
    perim = ( 2 * width ) + (2 * height )
    rect = [
    QgsPoint( minx, miny ),
    QgsPoint( minx, maxy ),
    QgsPoint( maxx, maxy ),
    QgsPoint( maxx, miny ),
    QgsPoint( minx, miny ) ]
    geometry = QgsGeometry().fromPolygon( [ rect ] )
    feat = QgsFeature()
    feat.setGeometry( geometry )
    feat.setAttributeMap( {
    0 : QVariant( minx ),
    1 : QVariant( miny ),
    2 : QVariant( maxx ),
    3 : QVariant( maxy ),
    4 : QVariant( cntx ),
    5 : QVariant( cnty ),
    6 : QVariant( area ),
    7 : QVariant( perim ),
    8 : QVariant( height ),
    9 : QVariant( width ) } )
    writer.addFeature( feat )
    self.emit( SIGNAL( "runRange(PyQt_PyObject)" ), ( 0, 100 ) )
    self.emit( SIGNAL( "runStatus(PyQt_PyObject)" ),  0 )
    del writer

    return True

  def simpleMeasure( self, inGeom ):
    if inGeom.wkbType() in (QGis.WKBPoint, QGis.WKBPoint25D):
      pt = QgsPoint()
      pt = inGeom.asPoint()
      attr1 = pt.x()
      attr2 = pt.y()
    else:
      measure = QgsDistanceArea()
      attr1 = measure.measure(inGeom)
      if inGeom.type() == QGis.Polygon:
        attr2 = self.perimMeasure( inGeom, measure )
      else:
        attr2 = attr1
    return ( attr1, attr2 )

  def perimMeasure( self, inGeom, measure ):
    value = 0.00
    if inGeom.isMultipart():
      poly = inGeom.asMultiPolygon()
      for k in poly:
        for j in k:
          value = value + measure.measureLine( j )
    else:
      poly = inGeom.asPolygon()
      for k in poly:
        value = value + measure.measureLine( k )
    return value

  def checkForField( self, L, e ):
    e = QString( e ).toLower()
    fieldRange = range( 0,len( L ) )
    for item in fieldRange:
      if L[ item ].toLower() == e:
        return True, item
    return False, len( L )

  def checkGeometryFields( self, vlayer ):
    vprovider = vlayer.dataProvider()
    nameList = []
    fieldList = vprovider.fields()
    geomType = vlayer.geometryType()
    for i in fieldList.keys():
      nameList.append( fieldList[ i ].name().toLower() )
    if geomType == QGis.Polygon:
      plp = "Poly"
      ( found, index1 ) = self.checkForField( nameList, "AREA" )
      if not found:
        field = QgsField( "AREA", QVariant.Double, "double", 21, 6, self.tr("Polygon area") )
        index1 = len( fieldList.keys() )
        fieldList[ index1 ] = field
      ( found, index2 ) = self.checkForField( nameList, "PERIMETER" )

      if not found:
        field = QgsField( "PERIMETER", QVariant.Double, "double", 21, 6, self.tr("Polygon perimeter") )
        index2 = len( fieldList.keys() )
        fieldList[ index2 ] = field
    elif geomType == QGis.Line:
      plp = "Line"
      (found, index1) = self.checkForField(nameList, "LENGTH")
      if not found:
        field = QgsField("LENGTH", QVariant.Double, "double", 21, 6, self.tr("Line length") )
        index1 = len(fieldList.keys())
        fieldList[index1] = field
      index2 = index1
    else:
      plp = "Point"
      (found, index1) = self.checkForField(nameList, "XCOORD")
      if not found:
        field = QgsField("XCOORD", QVariant.Double, "double", 21, 6, self.tr("Point x coordinate") )
        index1 = len(fieldList.keys())
        fieldList[index1] = field
      (found, index2) = self.checkForField(nameList, "YCOORD")
      if not found:
        field = QgsField("YCOORD", QVariant.Double, "double", 21, 6, self.tr("Point y coordinate") )
        index2 = len(fieldList.keys())
        fieldList[index2] = field
    return (fieldList, index1, index2)

  def extractAsLine( self, geom ):
    multi_geom = QgsGeometry()
    temp_geom = []
    if geom.type() == 2:
      if geom.isMultipart():
        multi_geom = geom.asMultiPolygon()
        for i in multi_geom:
          temp_geom.extend(i)
      else:
        multi_geom = geom.asPolygon()
        temp_geom = multi_geom
      return temp_geom
    else:
      return []

  def remove_bad_lines( self, lines ):
    temp_geom = []
    if len(lines)==1:
      if len(lines[0]) > 2:
        temp_geom = lines
      else:
        temp_geom = []
    else:
      temp_geom = [elem for elem in lines if len(elem) > 2]
    return temp_geom

  def singleToMultiGeom(self, wkbType):
      try:
          if wkbType in (QGis.WKBPoint, QGis.WKBMultiPoint,
                         QGis.WKBPoint25D, QGis.WKBMultiPoint25D):
              return QGis.WKBMultiPoint
          elif wkbType in (QGis.WKBLineString, QGis.WKBMultiLineString,
                           QGis.WKBMultiLineString25D, QGis.WKBLineString25D):
              return QGis.WKBMultiLineString
          elif wkbType in (QGis.WKBPolygon, QGis.WKBMultiPolygon,
                           QGis.WKBMultiPolygon25D, QGis.WKBPolygon25D):
              return QGis.WKBMultiPolygon
          else:
              return QGis.WKBUnknown
      except Exception, err:
          print str(err)

  def multiToSingleGeom(self, wkbType):
      try:
          if wkbType in (QGis.WKBPoint, QGis.WKBMultiPoint,
                         QGis.WKBPoint25D, QGis.WKBMultiPoint25D):
              return QGis.WKBPoint
          elif wkbType in (QGis.WKBLineString, QGis.WKBMultiLineString,
                           QGis.WKBMultiLineString25D, QGis.WKBLineString25D):
              return QGis.WKBLineString
          elif wkbType in (QGis.WKBPolygon, QGis.WKBMultiPolygon,
                           QGis.WKBMultiPolygon25D, QGis.WKBPolygon25D):
              return QGis.WKBPolygon
          else:
              return QGis.WKBUnknown
      except Exception, err:
          print str(err)

  def extractAsSingle( self, geom ):
    multi_geom = QgsGeometry()
    temp_geom = []
    if geom.type() == 0:
      if geom.isMultipart():
        multi_geom = geom.asMultiPoint()
        for i in multi_geom:
          temp_geom.append( QgsGeometry().fromPoint ( i ) )
      else:
        temp_geom.append( geom )
    elif geom.type() == 1:
      if geom.isMultipart():
        multi_geom = geom.asMultiPolyline()
        for i in multi_geom:
          temp_geom.append( QgsGeometry().fromPolyline( i ) )
      else:
        temp_geom.append( geom )
    elif geom.type() == 2:
      if geom.isMultipart():
        multi_geom = geom.asMultiPolygon()
        for i in multi_geom:
          temp_geom.append( QgsGeometry().fromPolygon( i ) )
      else:
        temp_geom.append( geom )
    return temp_geom

  def extractAsMulti( self, geom ):
    temp_geom = []
    if geom.type() == 0:
      if geom.isMultipart():
        return geom.asMultiPoint()
      else:
        return [ geom.asPoint() ]
    elif geom.type() == 1:
      if geom.isMultipart():
        return geom.asMultiPolyline()
      else:
        return [ geom.asPolyline() ]
    else:
      if geom.isMultipart():
        return geom.asMultiPolygon()
      else:
        return [ geom.asPolygon() ]

  def convertGeometry( self, geom_list, vType ):
    if vType == 0:
      return QgsGeometry().fromMultiPoint(geom_list)
    elif vType == 1:
      return QgsGeometry().fromMultiPolyline(geom_list)
    else:
      return QgsGeometry().fromMultiPolygon(geom_list)
