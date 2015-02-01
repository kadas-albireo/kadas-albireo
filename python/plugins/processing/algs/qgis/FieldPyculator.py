# -*- coding: utf-8 -*-

"""
***************************************************************************
    FieldPyculator.py
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

__author__ = 'Victor Olaya & NextGIS'
__date__ = 'August 2012'
__copyright__ = '(C) 2012, Victor Olaya & NextGIS'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import sys

from PyQt4.QtCore import QVariant
from qgis.core import QgsFeature, QgsField
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from processing.core.parameters import ParameterVector
from processing.core.parameters import ParameterString
from processing.core.parameters import ParameterNumber
from processing.core.parameters import ParameterSelection
from processing.core.outputs import OutputVector
from processing.tools import dataobjects, vector


class FieldsPyculator(GeoAlgorithm):

    INPUT_LAYER = 'INPUT_LAYER'
    FIELD_NAME = 'FIELD_NAME'
    FIELD_TYPE = 'FIELD_TYPE'
    FIELD_LENGTH = 'FIELD_LENGTH'
    FIELD_PRECISION = 'FIELD_PRECISION'
    GLOBAL = 'GLOBAL'
    FORMULA = 'FORMULA'
    OUTPUT_LAYER = 'OUTPUT_LAYER'
    RESULT_VAR_NAME = 'value'

    TYPE_NAMES = ['Integer', 'Float', 'String']
    TYPES = [QVariant.Int, QVariant.Double, QVariant.String]

    def defineCharacteristics(self):
        self.name = 'Advanced Python field calculator'
        self.group = 'Vector table tools'
        self.addParameter(ParameterVector(self.INPUT_LAYER,
            self.tr('Input layer'), [ParameterVector.VECTOR_TYPE_ANY], False))
        self.addParameter(ParameterString(self.FIELD_NAME,
            self.tr('Result field name'), 'NewField'))
        self.addParameter(ParameterSelection(self.FIELD_TYPE,
            self.tr('Field type'), self.TYPE_NAMES))
        self.addParameter(ParameterNumber(self.FIELD_LENGTH,
            self.tr('Field length'), 1, 255, 10))
        self.addParameter(ParameterNumber(self.FIELD_PRECISION,
            self.tr('Field precision'), 0, 10, 0))
        self.addParameter(ParameterString(self.GLOBAL,
            self.tr('Global expression'), multiline=True, optional=True))
        self.addParameter(ParameterString(self.FORMULA,
            self.tr('Formula'), 'value = ', multiline=True))
        self.addOutput(OutputVector(self.OUTPUT_LAYER, self.tr('Output layer')))

    def processAlgorithm(self, progress):
        fieldName = self.getParameterValue(self.FIELD_NAME)
        fieldType = self.getParameterValue(self.FIELD_TYPE)
        fieldLength = self.getParameterValue(self.FIELD_LENGTH)
        fieldPrecision = self.getParameterValue(self.FIELD_PRECISION)
        code = self.getParameterValue(self.FORMULA)
        globalExpression = self.getParameterValue(self.GLOBAL)
        output = self.getOutputFromName(self.OUTPUT_LAYER)

        layer = dataobjects.getObjectFromUri(
            self.getParameterValue(self.INPUT_LAYER))
        provider = layer.dataProvider()
        fields = provider.fields()
        fields.append(QgsField(fieldName, self.TYPES[fieldType], '',
                      fieldLength, fieldPrecision))
        writer = output.getVectorWriter(fields, provider.geometryType(),
                layer.crs())
        outFeat = QgsFeature()
        new_ns = {}

        # Run global code
        if globalExpression.strip() != '':
            try:
                bytecode = compile(globalExpression, '<string>', 'exec')
                exec bytecode in new_ns
            except:
                raise GeoAlgorithmExecutionException(
                    self.tr("FieldPyculator code execute error.Global code block can't be executed!\n%s\n%s" % (unicode(sys.exc_info()[0].__name__), unicode(sys.exc_info()[1]))))

        # Replace all fields tags
        fields = provider.fields()
        num = 0
        for field in fields:
            field_name = unicode(field.name())
            replval = '__attr[' + str(num) + ']'
            code = code.replace('<' + field_name + '>', replval)
            num += 1

        # Replace all special vars
        code = code.replace('$id', '__id')
        code = code.replace('$geom', '__geom')
        need_id = code.find('__id') != -1
        need_geom = code.find('__geom') != -1
        need_attrs = code.find('__attr') != -1

        # Compile
        try:
            bytecode = compile(code, '<string>', 'exec')
        except:
            raise GeoAlgorithmExecutionException(
                self.tr("FieldPyculator code execute error.Field code block can't be executed!\n%s\n%s" % (unicode(sys.exc_info()[0].__name__), unicode(sys.exc_info()[1]))))

        # Run
        features = vector.features(layer)
        nFeatures = len(features)
        nElement = 1
        for feat in features:
            progress.setPercentage(int(100 * nElement / nFeatures))
            attrs = feat.attributes()
            feat_id = feat.id()

            # Add needed vars
            if need_id:
                new_ns['__id'] = feat_id

            if need_geom:
                geom = feat.geometry()
                new_ns['__geom'] = geom

            if need_attrs:
                pyattrs = [a for a in attrs]
                new_ns['__attr'] = pyattrs

            # Clear old result
            if self.RESULT_VAR_NAME in new_ns:
                del new_ns[self.RESULT_VAR_NAME]

            # Exec
            exec bytecode in new_ns

            # Check result
            if self.RESULT_VAR_NAME not in new_ns:
                raise GeoAlgorithmExecutionException(
                    self.tr("FieldPyculator code execute error\n"
                            "Field code block does not return '%s1' variable! "
                            "Please declare this variable in your code!" % self.RESULT_VAR_NAME))

            # Write feature
            nElement += 1
            outFeat.setGeometry(feat.geometry())
            attrs.append(new_ns[self.RESULT_VAR_NAME])
            outFeat.setAttributes(attrs)
            writer.addFeature(outFeat)

        del writer

    def checkParameterValuesBeforeExecuting(self):
        # TODO check that formula is correct and fields exist
        pass
