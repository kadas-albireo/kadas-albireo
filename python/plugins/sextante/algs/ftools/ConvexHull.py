# -*- coding: utf-8 -*-

"""
***************************************************************************
    ConvexHull.py
    ---------------------
    Date                 : August 2012
    Copyright            : (C) 2012 by Victor Olaya
    Email                : volayaf at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Victor Olaya'
__date__ = 'August 2012'
__copyright__ = '(C) 2012, Victor Olaya'
# This will get replaced with a git SHA1 when you do a git archive
__revision__ = '$Format:%H$'

from sextante.core.GeoAlgorithm import GeoAlgorithm
import os.path
from PyQt4 import QtGui
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from sextante.parameters.ParameterVector import ParameterVector
from sextante.core.QGisLayers import QGisLayers
from sextante.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from sextante.outputs.OutputVector import OutputVector
from sextante.algs.ftools import ftools_utils
from sextante.core.SextanteLog import SextanteLog
from sextante.parameters.ParameterTableField import ParameterTableField
from sextante.parameters.ParameterSelection import ParameterSelection
from sextante.parameters.ParameterBoolean import ParameterBoolean

class ConvexHull(GeoAlgorithm):

    INPUT = "INPUT"
    OUTPUT = "OUTPUT"
    FIELD = "FIELD"
    USE_SELECTED = "USE_SELECTED"
    METHOD = "METHOD"
    METHODS = ["Create single minimum convex hull", "Create convex hulls based on field"]

    #===========================================================================
    # def getIcon(self):
    #    return QtGui.QIcon(os.path.dirname(__file__) + "/icons/convex_hull.png")
    #===========================================================================

    def processAlgorithm(self, progress):
        useSelection = self.getParameterValue(ConvexHull.USE_SELECTED)
        useField = (self.getParameterValue(ConvexHull.METHOD) == 1)
        field = self.getParameterValue(ConvexHull.FIELD)
        vlayerA = QGisLayers.getObjectFromUri(self.getParameterValue(ConvexHull.INPUT))
        GEOS_EXCEPT = True
        FEATURE_EXCEPT = True
        vproviderA = vlayerA.dataProvider()
        allAttrsA = vproviderA.attributeIndexes()
        vproviderA.select(allAttrsA)
        fields = { 0 : QgsField("ID", QVariant.Int),
                    1 : QgsField("Area",  QVariant.Double),
                    2 : QgsField("Perim", QVariant.Double) }
        writer = self.getOutputFromName(ConvexHull.OUTPUT).getVectorWriter(fields, QGis.WKBPolygon, vproviderA.crs())
        inFeat = QgsFeature()
        outFeat = QgsFeature()
        inGeom = QgsGeometry()
        outGeom = QgsGeometry()
        nElement = 0
        index = vproviderA.fieldNameIndex(field)
        # there is selection in input layer
        if useSelection:
          nFeat = vlayerA.selectedFeatureCount()
          selectionA = vlayerA.selectedFeatures()
          if useField:
            unique = ftools_utils.getUniqueValues( vproviderA, index )
            nFeat = nFeat * len( unique )
            for i in unique:
              hull = []
              first = True
              outID = 0
              for inFeat in selectionA:
                atMap = inFeat.attributeMap()
                idVar = atMap[ index ]
                if idVar.toString().trimmed() == i.toString().trimmed():
                  if first:
                    outID = idVar
                    first = False
                  inGeom = QgsGeometry( inFeat.geometry() )
                  points = ftools_utils.extractPoints( inGeom )
                  hull.extend( points )
                nElement += 1
                progress.setPercentage(int(nElement/nFeat * 100))
              if len( hull ) >= 3:
                tmpGeom = QgsGeometry( outGeom.fromMultiPoint( hull ) )
                try:
                  outGeom = tmpGeom.convexHull()
                  outFeat.setGeometry( outGeom )
                  (area, perim) = self.simpleMeasure( outGeom )
                  outFeat.addAttribute( 0, QVariant( outID ) )
                  outFeat.addAttribute( 1, QVariant( area ) )
                  outFeat.addAttribute( 2, QVariant( perim ) )
                  writer.addFeature( outFeat )
                except:
                  GEOS_EXCEPT = False
                  continue
          else:
            hull = []
            for inFeat in selectionA:
              inGeom = QgsGeometry( inFeat.geometry() )
              points = ftools_utils.extractPoints( inGeom )
              hull.extend( points )
              nElement += 1
              progress.setPercentage(int(nElement/nFeat * 100))
            tmpGeom = QgsGeometry( outGeom.fromMultiPoint( hull ) )
            try:
              outGeom = tmpGeom.convexHull()
              outFeat.setGeometry( outGeom )
              writer.addFeature( outFeat )
            except:
              GEOS_EXCEPT = False
        # there is no selection in input layer
        else:
          rect = vlayerA.extent()
          nFeat = vproviderA.featureCount()
          if useField:
            unique = ftools_utils.getUniqueValues( vproviderA, index )
            nFeat = nFeat * len( unique )
            for i in unique:
              hull = []
              first = True
              outID = 0
              vproviderA.select( allAttrsA )#, rect )
              #vproviderA.rewind()
              while vproviderA.nextFeature( inFeat ):
                atMap = inFeat.attributeMap()
                idVar = atMap[ index ]
                if idVar.toString().trimmed() == i.toString().trimmed():
                  if first:
                    outID = idVar
                    first = False
                  inGeom = QgsGeometry( inFeat.geometry() )
                  points = ftools_utils.extractPoints( inGeom )
                  hull.extend( points )
                nElement += 1
                progress.setPercentage(int(nElement/nFeat * 100))
              if len( hull ) >= 3:
                tmpGeom = QgsGeometry( outGeom.fromMultiPoint( hull ) )
                try:
                  outGeom = tmpGeom.convexHull()
                  outFeat.setGeometry( outGeom )
                  (area, perim) = self.simpleMeasure( outGeom )
                  outFeat.addAttribute( 0, QVariant( outID ) )
                  outFeat.addAttribute( 1, QVariant( area ) )
                  outFeat.addAttribute( 2, QVariant( perim ) )
                  writer.addFeature( outFeat )
                except:
                  GEOS_EXCEPT = False
                  continue
          else:
            hull = []
            #vproviderA.rewind()
            vproviderA.select(allAttrsA)
            while vproviderA.nextFeature( inFeat ):
              inGeom = QgsGeometry( inFeat.geometry() )
              points = ftools_utils.extractPoints( inGeom )
              hull.extend( points )
              nElement += 1
              progress.setPercentage(int(nElement/nFeat * 100))
            tmpGeom = QgsGeometry( outGeom.fromMultiPoint( hull ) )
            try:
              outGeom = tmpGeom.convexHull()
              outFeat.setGeometry( outGeom )
              writer.addFeature( outFeat )
            except:
              GEOS_EXCEPT = False
        del writer

        if not GEOS_EXCEPT:
            SextanteLog.addToLog(SextanteLog.LOG_WARNING, "Geometry exception while computing convex hull")
        if not FEATURE_EXCEPT:
            SextanteLog.addToLog(SextanteLog.LOG_WARNING, "Feature exception while computing convex hull")

    def simpleMeasure(self, inGeom ):
        measure = QgsDistanceArea()
        attr1 = measure.measure(inGeom)
        if inGeom.type() == QGis.Polygon:
          attr2 = self.perimMeasure( inGeom, measure )
        else:
          attr2 = attr1
        return ( attr1, attr2 )

    def perimMeasure(self, inGeom, measure ):
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
    
    def defineCharacteristics(self):
        self.name = "Convex hull"
        self.group = "Vector geometry tools"
        self.addParameter(ParameterVector(ConvexHull.INPUT, "Input layer", ParameterVector.VECTOR_TYPE_ANY))
        self.addParameter(ParameterTableField(ConvexHull.FIELD, "Field", ConvexHull.INPUT))
        self.addParameter(ParameterSelection(ConvexHull.METHOD, "Method", ConvexHull.METHODS))
        self.addParameter(ParameterBoolean(ConvexHull.USE_SELECTED, "Use selected features", False))
        self.addOutput(OutputVector(ConvexHull.OUTPUT, "Convex hull"))
    #=========================================================
