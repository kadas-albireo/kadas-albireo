# -*- coding: utf-8 -*-

"""
***************************************************************************
    Centroids.py
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
from sextante.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException

from sextante.parameters.ParameterVector import ParameterVector
from sextante.outputs.OutputVector import OutputVector

class Centroids(GeoAlgorithm):

    INPUT_LAYER = "INPUT_LAYER"
    OUTPUT_LAYER = "OUTPUT_LAYER"

    #===========================================================================
    # def getIcon(self):
    #    return QtGui.QIcon(os.path.dirname(__file__) + "/icons/centroids.png")
    #===========================================================================

    def defineCharacteristics(self):
        self.name = "Polygon centroids"
        self.group = "Vector geometry tools"

        self.addParameter(ParameterVector(self.INPUT_LAYER, "Input layer", ParameterVector.VECTOR_TYPE_POLYGON))

        self.addOutput(OutputVector(self.OUTPUT_LAYER, "Output layer"))

    def processAlgorithm(self, progress):
        layer = QGisLayers.getObjectFromUri(self.getParameterValue(self.INPUT_LAYER))
                
        writer = self.getOutputFromName(self.OUTPUT_LAYER).getVectorWriter(layer.pendingFields(),
                     QGis.WKBPoint, layer.dataProvider().crs())

        layer.select(layer.pendingAllAttributesList())

        inFeat = QgsFeature()
        outFeat = QgsFeature()

        total = 100.0 / float(layer.featureCount())
        current = 0

        while layer.nextFeature(inFeat):
            inGeom = inFeat.geometry()
            attrMap = inFeat.attributeMap()

            outGeom = QgsGeometry(inGeom.centroid())
            if outGeom is None:
                raise GeoAlgorithmExecutionException("Error calculating centroid")

            outFeat.setGeometry(outGeom)
            outFeat.setAttributeMap(attrMap)
            writer.addFeature(outFeat)
            current += 1
            progress.setPercentage(int(current * total))

        del writer
