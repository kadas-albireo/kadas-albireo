# -*- coding: utf-8 -*-

"""
***************************************************************************
    MeanCoords.py
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
from processing.parameters.ParameterTableField import ParameterTableField
from processing.parameters.ParameterVector import ParameterVector
from processing.outputs.OutputVector import OutputVector
from processing.tools import dataobjects, vector


class MeanCoords(GeoAlgorithm):

    POINTS = 'POINTS'
    WEIGHT = 'WEIGHT'
    OUTPUT = 'OUTPUT'
    UID = 'UID'
    WEIGHT = 'WEIGHT'

    def defineCharacteristics(self):
        self.name = 'Mean coordinate(s)'
        self.group = 'Vector analysis tools'

        self.addParameter(ParameterVector(self.POINTS, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.WEIGHT, 'Weight field',
                          MeanCoords.POINTS,
                          ParameterTableField.DATA_TYPE_NUMBER))
        self.addParameter(ParameterTableField(self.UID, 'Unique ID field',
                          MeanCoords.POINTS,
                          ParameterTableField.DATA_TYPE_NUMBER))

        self.addOutput(OutputVector(MeanCoords.OUTPUT, 'Result'))

    def processAlgorithm(self, progress):
        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.POINTS))
        weightField = self.getParameterValue(self.WEIGHT)
        uniqueField = self.getParameterValue(self.UID)

        weightIndex = layer.fieldNameIndex(weightField)
        uniqueIndex = layer.fieldNameIndex(uniqueField)

        fieldList = [QgsField('MEAN_X', QVariant.Double, '', 24, 15),
                     QgsField('MEAN_Y', QVariant.Double, '', 24, 15),
                     QgsField('UID', QVariant.String, '', 255)]

        writer = self.getOutputFromName(
                self.OUTPUT).getVectorWriter(fieldList,
                                             QGis.WKBPoint, layer.crs())

        current = 0
        features = vector.features(layer)
        total = 100.0 / float(len(features))

        means = {}
        for feat in features:
            current += 1
            progress.setPercentage(current * total)
            clazz = str(feat.attributes()[uniqueIndex]).strip()
            if weightIndex == -1:
                weight = 1.00
            else:
                try:
                    weight = float(feat.attributes()[weightIndex])
                except:
                    weight = 1.00
            if clazz not in means:
                means[clazz] = (0, 0, 0)

            (cx, cy, totalweight) = means[clazz]
            geom = QgsGeometry(feat.geometry())
            geom = vector.extractPoints(geom)
            for i in geom:
                cx += i.x() * weight
                cy += i.y() * weight
                totalweight += weight
            means[clazz] = (cx, cy, totalweight)

        for (clazz, values) in means.iteritems():
            outFeat = QgsFeature()
            cx = values[0] / values[2]
            cy = values[1] / values[2]
            meanPoint = QgsPoint(cx, cy)

            outFeat.setGeometry(QgsGeometry.fromPoint(meanPoint))
            outFeat.setAttributes([cx, cy, clazz])
            writer.addFeature(outFeat)

        del writer
