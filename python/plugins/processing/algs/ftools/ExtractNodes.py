# -*- coding: utf-8 -*-

"""
***************************************************************************
    ExtractNodes.py
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

from PyQt4.QtCore import *
from qgis.core import *
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.parameters.ParameterVector import ParameterVector
from processing.outputs.OutputVector import OutputVector
from processing.tools import dataobjects, vector


class ExtractNodes(GeoAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'

    def defineCharacteristics(self):
        self.name = 'Extract nodes'
        self.group = 'Vector geometry tools'

        self.addParameter(ParameterVector(self.INPUT, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_POLYGON,
                          ParameterVector.VECTOR_TYPE_LINE]))

        self.addOutput(OutputVector(self.OUTPUT, 'Output layer'))

    def processAlgorithm(self, progress):
        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.INPUT))

        writer = self.getOutputFromName(
                self.OUTPUT).getVectorWriter(layer.pendingFields().toList(),
                                             QGis.WKBPoint, layer.crs())

        outFeat = QgsFeature()
        inGeom = QgsGeometry()
        outGeom = QgsGeometry()

        current = 0
        features = vector.features(layer)
        total = 100.0 / float(len(features))
        for f in features:
            inGeom = f.geometry()
            attrs = f.attributes()

            points = vector.extractPoints(inGeom)
            outFeat.setAttributes(attrs)

            for i in points:
                outFeat.setGeometry(outGeom.fromPoint(i))
                writer.addFeature(outFeat)

            current += 1
            progress.setPercentage(int(current * total))

        del writer
