# -*- coding: utf-8 -*-

"""
***************************************************************************
    VectorSplit.py
    ---------------------
    Date                 : September 2014
    Copyright            : (C) 2014 by Alexander Bruy
    Email                : alexander dot bruy at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Alexander Bruy'
__date__ = 'September 2014'
__copyright__ = '(C) 2014, Alexander Bruy'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

from PyQt4.QtCore import *
from qgis.core import *
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.parameters import ParameterVector
from processing.core.parameters import ParameterTableField
from processing.core.outputs import OutputDirectory
from processing.tools import dataobjects, vector


class VectorSplit(GeoAlgorithm):

    INPUT = 'INPUT'
    FIELD = 'FIELD'
    OUTPUT = 'OUTPUT'

    def defineCharacteristics(self):
        self.name = 'Split vector layer'
        self.group = 'Vector general tools'
        self.addParameter(ParameterVector(self.INPUT,
            'Input layer', [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.FIELD,
            'Unique ID field', self.INPUT))

        self.addOutput(OutputDirectory(self.OUTPUT, 'Output directory'))

    def processAlgorithm(self, progress):
        layer = dataobjects.getObjectFromUri(
            self.getParameterValue(self.INPUT))
        fieldName = self.getParameterValue(self.FIELD)

        directory = self.getOutputValue(self.OUTPUT)

        if directory.find('\\') != -1:
            directory.replace('\\', '/')

        if not directory.endswith("/"):
            directory += '/'

        fieldIndex = layer.fieldNameIndex(fieldName)
        uniqueValues = vector.uniqueValues(layer, fieldIndex)
        baseName = '{0}{1}_{2}'.format(directory, layer.name(), fieldName)

        fields = layer.pendingFields()
        crs = layer.dataProvider().crs()
        geomType = layer.wkbType()

        total = 100.0 / len(uniqueValues)
        features = vector.features(layer)

        for count, i in enumerate(uniqueValues):
            fName = '{0}_{1}.shp'.format(baseName, unicode(i).strip())

            writer = vector.VectorWriter(fName, None, fields, geomType, crs)
            for f in features:
                if f[fieldName] == i:
                    writer.addFeature(f)
            del writer

            progress.setPercentage(int(count * total))
