# -*- coding: utf-8 -*-

"""
***************************************************************************
    EquivalentNumField.py
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
from processing.parameters.ParameterTableField import ParameterTableField
from processing.outputs.OutputVector import OutputVector
from processing.tools import dataobjects, vector


class EquivalentNumField(GeoAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    FIELD = 'FIELD'

    def processAlgorithm(self, progress):
        fieldname = self.getParameterValue(self.FIELD)
        output = self.getOutputFromName(self.OUTPUT)
        vlayer = dataobjects.getObjectFromUri(
            self.getParameterValue(self.INPUT))
        vprovider = vlayer.dataProvider()
        fieldindex = vlayer.fieldNameIndex(fieldname)
        fields = vprovider.fields()
        fields.append(QgsField('NUM_FIELD', QVariant.Int))
        writer = output.getVectorWriter(fields, vprovider.geometryType(),
                vlayer.crs())
        outFeat = QgsFeature()
        inGeom = QgsGeometry()
        nElement = 0
        classes = {}
        features = vector.features(vlayer)
        nFeat = len(features)
        for feature in features:
            progress.setPercentage(int(100 * nElement / nFeat))
            nElement += 1
            inGeom = feature.geometry()
            outFeat.setGeometry(inGeom)
            atMap = feature.attributes()
            clazz = atMap[fieldindex]
            if clazz not in classes:
                classes[clazz] = len(classes.keys())
            atMap.append(classes[clazz])
            outFeat.setAttributes(atMap)
            writer.addFeature(outFeat)
        del writer

    def defineCharacteristics(self):
        self.name = 'Create equivalent numerical field'
        self.group = 'Vector table tools'
        self.addParameter(ParameterVector(self.INPUT, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.FIELD, 'Class field',
                          self.INPUT))
        self.addOutput(OutputVector(self.OUTPUT, 'Output layer'))
