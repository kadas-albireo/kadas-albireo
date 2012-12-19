# -*- coding: utf-8 -*-

"""
***************************************************************************
    PointsInPolygon.py
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
from sextante.parameters.ParameterTableField import ParameterTableField

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
from sextante.core.SextanteLog import SextanteLog

from sextante.parameters.ParameterVector import ParameterVector
from sextante.parameters.ParameterString import ParameterString

from sextante.outputs.OutputVector import OutputVector

from sextante.algs.ftools import FToolsUtils as utils


class PointsInPolygonWeighted(GeoAlgorithm):

    POLYGONS = "POLYGONS"
    POINTS = "POINTS"
    OUTPUT = "OUTPUT"
    FIELD = "FIELD"
    WEIGHT = "WEIGHT"

    #===========================================================================
    # def getIcon(self):
    #    return QtGui.QIcon(os.path.dirname(__file__) + "/icons/sum_points.png")
    #===========================================================================

    def defineCharacteristics(self):
        self.name = "Count points in polygon(weighted)"
        self.group = "Vector analysis tools"

        self.addParameter(ParameterVector(self.POLYGONS, "Polygons", ParameterVector.VECTOR_TYPE_POLYGON))
        self.addParameter(ParameterVector(self.POINTS, "Points", ParameterVector.VECTOR_TYPE_POINT))
        self.addParameter(ParameterTableField(self.WEIGHT, "Weight field", self.POINTS))
        self.addParameter(ParameterString(self.FIELD, "Count field name", "NUMPOINTS"))

        self.addOutput(OutputVector(self.OUTPUT, "Result"))

    def processAlgorithm(self, progress):
        polyLayer = QGisLayers.getObjectFromUri(self.getParameterValue(self.POLYGONS))
        pointLayer = QGisLayers.getObjectFromUri(self.getParameterValue(self.POINTS))
        fieldName = self.getParameterValue(self.FIELD)
        fieldidx = pointLayer.dataProvider().fieldNameIndex(self.getParameterValue(self.WEIGHT))

        polyProvider = polyLayer.dataProvider()
        pointProvider = pointLayer.dataProvider()
        if polyProvider.crs() != pointProvider.crs():
            SextanteLog.addToLog(SextanteLog.LOG_WARNING,
                                 "CRS warning: Input layers have non-matching CRS. This may cause unexpected results.")

        idxCount, fieldList = utils.findOrCreateField(polyLayer, polyLayer.pendingFields(), fieldName)

        writer = self.getOutputFromName(self.OUTPUT).getVectorWriter(fieldList,
                     polyProvider.geometryType(), polyProvider.crs())

        spatialIndex = utils.createSpatialIndex(pointProvider)

        pointProvider.rewind()
        pointProvider.select()

        allAttrs = polyLayer.pendingAllAttributesList()
        polyLayer.select(allAttrs)

        ftPoly = QgsFeature()
        ftPoint = QgsFeature()
        outFeat = QgsFeature()
        geom = QgsGeometry()

        current = 0
        total = 100.0 / float(polyProvider.featureCount())
        hasIntersections = False

        while polyLayer.nextFeature(ftPoly):
            geom = ftPoly.geometry()
            atMap = ftPoly.attributeMap()

            count = 0
            hasIntersections = False
            points = spatialIndex.intersects(geom.boundingBox())
            if len(points) > 0:
                hasIntersections = True

            if hasIntersections:
                for i in points:
                    pointLayer.featureAtId(int(i), ftPoint, True, True)
                    tmpGeom = QgsGeometry(ftPoint.geometry())
                    if geom.contains(tmpGeom):
                        try:
                            weight = float(ftPoint.attributeMap()[fieldidx])
                        except:
                            weight = 1
                        count += weight

            outFeat.setGeometry(geom)
            outFeat.setAttributeMap(atMap)
            outFeat.addAttribute(idxCount, QVariant(count))
            writer.addFeature(outFeat)

            current += 1
            progress.setPercentage(int(current * total))

        del writer
