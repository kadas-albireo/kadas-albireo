# -*- coding: utf-8 -*-

"""
***************************************************************************
    LinesIntersection.py
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

import os.path

from PyQt4 import QtGui
from PyQt4.QtCore import *

from qgis.core import *

from sextante.core.GeoAlgorithm import GeoAlgorithm
from sextante.core.QGisLayers import QGisLayers

from sextante.parameters.ParameterVector import ParameterVector
from sextante.parameters.ParameterTableField import ParameterTableField

from sextante.outputs.OutputVector import OutputVector

from sextante.algs.ftools import FToolsUtils as utils

class LinesIntersection(GeoAlgorithm):

    INPUT_A = "INPUT_A"
    INPUT_B = "INPUT_B"
    FIELD_A = "FIELD_A"
    FIELD_B = "FIELD_B"

    OUTPUT = "OUTPUT"

    #===========================================================================
    # def getIcon(self):
    #    return QtGui.QIcon(os.path.dirname(__file__) + "/icons/intersections.png")
    #===========================================================================

    def defineCharacteristics(self):
        self.name = "Line intersections"
        self.group = "Vector overlay tools"

        self.addParameter(ParameterVector(self.INPUT_A, "Input layer", ParameterVector.VECTOR_TYPE_LINE))
        self.addParameter(ParameterVector(self.INPUT_B, "Intersect layer", ParameterVector.VECTOR_TYPE_LINE))
        self.addParameter(ParameterTableField(self.FIELD_A, "Input unique ID field", self.INPUT_A))
        self.addParameter(ParameterTableField(self.FIELD_B, "Intersect unique ID field", self.INPUT_B))

        self.addOutput(OutputVector(self.OUTPUT, "Output layer"))

    def processAlgorithm(self, progress):
        layerA = QGisLayers.getObjectFromUri(self.getParameterValue(self.INPUT_A))
        layerB = QGisLayers.getObjectFromUri(self.getParameterValue(self.INPUT_B))
        fieldA = self.getParameterValue(self.FIELD_A)
        fieldB = self.getParameterValue(self.FIELD_B)
        
        providerA = layerA.dataProvider()
        providerB = layerB.dataProvider()


        idxA = layerA.fieldNameIndex(fieldA)
        idxB = layerB.fieldNameIndex(fieldB)

        fieldList = {0 : layerA.pendingFields()[idxA],
                     1 : layerB.pendingFields()[idxB]
                    }

        writer = self.getOutputFromName(self.OUTPUT).getVectorWriter(fieldList,
                     QGis.WKBPoint, providerA.crs())

        spatialIndex = utils.createSpatialIndex(providerB)

        providerA.rewind()
        layerA.select([idxA])
        providerB.rewind()
        layerB.select([idxB])

        inFeatA = QgsFeature()
        inFeatB = QgsFeature()
        outFeat = QgsFeature()
        inGeom = QgsGeometry()
        tmpGeom = QgsGeometry()

        current = 0
        total = 100.0 / float(providerA.featureCount())
        hasIntersections = False

        while layerA.nextFeature(inFeatA):
            inGeom = inFeatA.geometry()
            hasIntersections = False
            lines = spatialIndex.intersects(inGeom.boundingBox())

            if len(lines) > 0:
                hasIntersections = True

            if hasIntersections:
                layerB.select([idxB])
                for i in lines:
                    layerB.featureAtId(int(i), inFeatB)
                    tmpGeom = QgsGeometry(inFeatB.geometry())

                    points = []
                    atMapA = inFeatA.attributeMap()
                    atMapB = inFeatB.attributeMap()

                    if inGeom.intersects(tmpGeom):
                        tempGeom = inGeom.intersection(tmpGeom)
                        if tempGeom.type() == QGis.Point:
                            if tempGeom.isMultipart():
                                points = tempGeom.asMultiPoint()
                            else:
                                points.append(tempGeom.asPoint())

                            for j in points:
                                outFeat.setGeometry(tempGeom.fromPoint(j))
                                outFeat.addAttribute(0, atMapA[idxA])
                                outFeat.addAttribute(1, atMapB[idxB])
                                writer.addFeature(outFeat)

            current += 1
            progress.setPercentage(int(current * total))

        del writer
