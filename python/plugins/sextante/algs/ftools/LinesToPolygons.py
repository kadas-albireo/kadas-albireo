# -*- coding: utf-8 -*-

"""
***************************************************************************
    LinesToPolygons.py
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

from sextante.outputs.OutputVector import OutputVector

class LinesToPolygons(GeoAlgorithm):

    INPUT = "INPUT"
    OUTPUT = "OUTPUT"

    #===========================================================================
    # def getIcon(self):
    #    return QtGui.QIcon(os.path.dirname(__file__) + "/icons/to_lines.png")
    #===========================================================================

    def defineCharacteristics(self):
        self.name = "Lines to polygons"
        self.group = "Vector geometry tools"

        self.addParameter(ParameterVector(self.INPUT, "Input layer", ParameterVector.VECTOR_TYPE_LINE))
        self.addOutput(OutputVector(self.OUTPUT, "Output layer"))

    def processAlgorithm(self, progress):
        layer = QGisLayers.getObjectFromUri(self.getParameterValue(self.INPUT))        

        provider = layer.dataProvider()
        layer.select(layer.pendingAllAttributesList())

        writer = self.getOutputFromName(self.OUTPUT).getVectorWriter(layer.pendingFields(),
                     QGis.WKBPolygon, provider.crs())

        inFeat = QgsFeature()
        outFeat = QgsFeature()
        inGeom = QgsGeometry()

        current = 0
        total = 100.0 / float(provider.featureCount())

        while layer.nextFeature(inFeat):
            outGeomList = []
            multi = False

            if inFeat.geometry().isMultipart():
                outGeomList = inFeat.geometry().asMultiPolyline()
                multi = True
            else:
                outGeomList.append(inFeat.geometry().asPolyline())

            polyGeom = self.removeBadLines(outGeomList)
            if len(polyGeom) <> 0:
                outFeat.setGeometry(QgsGeometry.fromPolygon(polyGeom))
                atMap = inFeat.attributeMap()
                outFeat.setAttributeMap(atMap)
                writer.addFeature(outFeat)

            current += 1
            progress.setPercentage(int(current * total))

        del writer

    def removeBadLines(self, lines):
        geom = []
        if len(lines) == 1:
            if len(lines[0]) > 2:
                geom = lines
            else:
                geom = []
        else:
            geom = [elem for elem in lines if len(elem) > 2]
        return geom
