# -*- coding: utf-8 -*-

"""
    MMQGISX - MMQGIS Wrapper for Processing
    ---------------------------------------

    begin                : 18 May 2010
    copyright            : (c) 2012 by Michael Minn
    email                : See michaelminn.com

   MMQGIS program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
"""

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *

from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.GeoAlgorithmExecutionException import \
        GeoAlgorithmExecutionException
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterSelection import ParameterSelection
from processing.parameters.ParameterString import ParameterString
from processing.parameters.ParameterTableField import ParameterTableField
from processing.parameters.ParameterVector import ParameterVector
from processing.parameters.ParameterCrs import ParameterCrs
from processing.outputs.OutputVector import OutputVector
from processing.tools import dataobjects, vector


class mmqgisx_delete_columns_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    COLUMN = 'COLUMN'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Delete column'
        self.group = 'Vector table tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.COLUMN, 'Field to delete',
                          self.LAYERNAME))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):

        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYERNAME))
        idx = layer.fieldNameIndex(self.getParameterValue(self.COLUMN))
        output = self.getOutputFromName(self.SAVENAME)
        fields = layer.pendingFields()
        newFields = []
        i = 0
        for field in fields:
            if i != idx:
                newFields.append(field)
            i += 1
        outfile = output.getVectorWriter(newFields, layer.wkbType(),
                layer.crs())
        features = vector.features(layer)
        featurecount = len(features)
        i = 0
        outFeat = QgsFeature()
        for feature in features:
            progress.setPercentage(float(i) / featurecount * 100)
            i += 1
            outFeat.setGeometry(feature.geometry())
            attributes = feature.attributes()
            newAttributes = []
            i = 0
            for attr in attributes:
                if i != idx:
                    newAttributes.append(attr)
                i += 1
            feature.setAttributes(newAttributes)
            outfile.addFeature(feature)


class mmqgisx_delete_duplicate_geometries_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Delete duplicate geometries'
        self.group = 'Vector general tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):
        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYERNAME))
        output = self.getOutputFromName(self.SAVENAME)
        fields = layer.pendingFields()
        outfile = output.getVectorWriter(fields, layer.wkbType(), layer.crs())
        features = vector.features(layer)
        featurecount = len(features)
        i = 0
        cleaned = {}
        for feature in features:
            cleaned[feature.geometry().exportToWkt()] = feature
            progress.setPercentage(float(i) / featurecount * 50)
            i += 1
        i = 0
        for feature in cleaned.values():
            progress.setPercentage(float(i) / featurecount * 50 + 50)
            outfile.addFeature(feature)


class mmqgisx_geometry_convert_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    NEWTYPE = 'NEWTYPE'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Convert geometry type'
        self.group = 'Vector geometry tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))

        self.newtypes = ['Centroids', 'Nodes', 'Linestrings',
                         'Multilinestrings', 'Polygons']
        self.addParameter(ParameterSelection(self.NEWTYPE, 'New Geometry Type'
                          , self.newtypes, default=0))

        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):

        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYERNAME))
        output = self.getOutputFromName(self.SAVENAME)
        index = self.getParameterValue(self.NEWTYPE)

        splitnodes = 0
        if index == 0:
            newtype = QGis.WKBPoint
        elif index == 1:
            newtype = QGis.WKBPoint
            splitnodes = 1
        elif index == 2:
            newtype = QGis.WKBLineString
        elif index == 3:
            newtype = QGis.WKBMultiLineString
        elif index == 4:
            newtype = QGis.WKBPolygon
        else:
            newtype = QGis.WKBPoint

        fields = layer.pendingFields()
        outfile = output.getVectorWriter(fields, newtype, layer.crs())

        features = vector.features(layer)
        feature_count = len(features)
        i = 0
        for feature in features:
            i += 1
            progress.setPercentage(float(i) / feature_count * 100)

            if feature.geometry().wkbType() == QGis.WKBPoint \
                    or feature.geometry().wkbType() == QGis.WKBPoint25D:
                if newtype == QGis.WKBPoint:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.asPoint())
                    outfile.addFeature(newfeature)
                else:
                    return 'Invalid Conversion: ' \
                        + mmqgisx_wkbtype_to_text(
                                feature.geometry().wkbType()) \
                        + ' to ' + mmqgisx_wkbtype_to_text(newtype)
            elif feature.geometry().wkbType() == QGis.WKBLineString \
                    or feature.geometry().wkbType() == QGis.WKBLineString25D:
                if newtype == QGis.WKBPoint and splitnodes:
                    polyline = feature.geometry().asPolyline()
                    for point in polyline:
                        newfeature = QgsFeature()
                        newfeature.setAttributes(feature.attributes())
                        newfeature.setGeometry(QgsGeometry.fromPoint(point))
                        outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPoint:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.geometry().centroid())
                    outfile.addFeature(newfeature)
                elif newtype == QGis.WKBLineString:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.geometry())
                    outfile.addFeature(newfeature)
                else:
                    return 'Invalid Conversion: ' \
                        + mmqgisx_wkbtype_to_text(
                                feature.geometry().wkbType()) \
                        + ' to ' + mmqgisx_wkbtype_to_text(newtype)
            elif feature.geometry().wkbType() == QGis.WKBPolygon \
                or feature.geometry().wkbType() == QGis.WKBPolygon25D:

                if newtype == QGis.WKBPoint and splitnodes:
                    polygon = feature.geometry().asPolygon()
                    for polyline in polygon:
                        for point in polyline:
                            newfeature = QgsFeature()
                            newfeature.setAttributes(feature.attributes())
                            newfeature.setGeometry(
                                    QgsGeometry.fromPoint(point))
                            outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPoint:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.geometry().centroid())
                    outfile.addFeature(newfeature)
                elif newtype == QGis.WKBMultiLineString:
                    linestrings = []
                    polygon = feature.geometry().asPolygon()
                    for polyline in polygon:
                        linestrings.append(polyline)

                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(
                            QgsGeometry.fromMultiPolyline(linestrings))
                    outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPolygon:
                    newfeature = QgsFeature()
                    newfeature.setAttributeMap(feature.attributes())
                    newfeature.setGeometry(feature.geometry())
                    outfile.addFeature(newfeature)
                else:
                    return 'Invalid Conversion: ' \
                        + mmqgisx_wkbtype_to_text(
                                feature.geometry().wkbType()) \
                        + ' to ' + mmqgisx_wkbtype_to_text(newtype)
            elif feature.geometry().wkbType() == QGis.WKBMultiPoint \
                    or feature.geometry().wkbType() == QGis.WKBMultiPoint25D:
                if newtype == QGis.WKBPoint and splitnodes:
                    points = feature.geometry().asMultiPoint()
                    for point in points:
                        newfeature = QgsFeature()
                        newfeature.setAttributes(feature.attributes())
                        newfeature.setGeometry(QgsGeometry.fromPoint(point))
                        outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPoint:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.geometry().centroid())
                    outfile.addFeature(newfeature)
                else:
                    return 'Invalid Conversion: ' \
                        + mmqgisx_wkbtype_to_text(
                                feature.geometry().wkbType()) \
                        + ' to ' + mmqgisx_wkbtype_to_text(newtype)
            elif feature.geometry().wkbType() == QGis.WKBMultiLineString \
                    or feature.geometry().wkbType() == \
                    QGis.WKBMultiLineString25D:
                if newtype == QGis.WKBPoint and splitnodes:
                    polylines = feature.geometry().asMultiPolyline()
                    for polyline in polylines:
                        for point in polyline:
                            newfeature = QgsFeature()
                            newfeature.setAttributes(feature.attributes())
                            newfeature.setGeometry(
                                    QgsGeometry.fromPoint(point))
                            outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPoint:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.geometry().centroid())
                    outfile.addFeature(newfeature)
                elif newtype == QGis.WKBLineString:
                    linestrings = feature.geometry().asMultiPolyline()
                    for linestring in linestrings:
                        newfeature = QgsFeature()
                        newfeature.setAttributes(feature.attributes())
                        newfeature.setGeometry(
                                QgsGeometry.fromPolyline(linestring))
                        outfile.addFeature(newfeature)
                elif newtype == QGis.WKBMultiLineString:

                    linestrings = feature.geometry().asMultiPolyline()
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(
                            QgsGeometry.fromMultiPolyline(linestrings))
                    outfile.addFeature(newfeature)
                else:
                    return 'Invalid Conversion: ' \
                        + mmqgisx_wkbtype_to_text(
                                feature.geometry().wkbType()) \
                        + ' to ' + mmqgisx_wkbtype_to_text(newtype)
            elif feature.geometry().wkbType() == QGis.WKBMultiPolygon \
                    or feature.geometry().wkbType() == QGis.WKBMultiPolygon25D:
                if newtype == QGis.WKBPoint and splitnodes:
                    polygons = feature.geometry().asMultiPolygon()
                    for polygon in polygons:
                        for polyline in polygon:
                            for point in polyline:
                                newfeature = QgsFeature()
                                newfeature.setAttributes(feature.attributes())
                                newfeature.setGeometry(
                                        QgsGeometry.fromPoint(point))
                                outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPoint:
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(feature.geometry().centroid())
                    outfile.addFeature(newfeature)
                elif newtype == QGis.WKBLineString:
                    polygons = feature.geometry().asMultiPolygon()
                    for polygon in polygons:
                        newfeature = QgsFeature()
                        newfeature.setAttributes(feature.attributes())
                        newfeature.setGeometry(
                                QgsGeometry.fromPolyline(polygon))
                        outfile.addFeature(newfeature)
                elif newtype == QGis.WKBPolygon:
                    polygons = feature.geometry().asMultiPolygon()
                    for polygon in polygons:
                        newfeature = QgsFeature()
                        newfeature.setAttributes(feature.attributes())
                        newfeature.setGeometry(
                                QgsGeometry.fromPolygon(polygon))
                        outfile.addFeature(newfeature)
                elif newtype == QGis.WKBMultiLineString or newtype \
                        == QGis.WKBMultiPolygon:
                    polygons = feature.geometry().asMultiPolygon()
                    newfeature = QgsFeature()
                    newfeature.setAttributes(feature.attributes())
                    newfeature.setGeometry(
                            QgsGeometry.fromMultiPolygon(polygons))
                    outfile.addFeature(newfeature)
                else:
                    return 'Invalid Conversion: ' \
                        + mmqgisx_wkbtype_to_text(
                                feature.geometry().wkbType()) \
                        + ' to ' + mmqgisx_wkbtype_to_text(newtype)

        del outfile


def mmqgisx_wkbtype_to_text(wkbtype):
    if wkbtype == QGis.WKBUnknown:
        return 'Unknown'
    if wkbtype == QGis.WKBPoint:
        return 'point'
    if wkbtype == QGis.WKBLineString:
        return 'linestring'
    if wkbtype == QGis.WKBPolygon:
        return 'polygon'
    if wkbtype == QGis.WKBMultiPoint:
        return 'multipoint'
    if wkbtype == QGis.WKBMultiLineString:
        return 'multilinestring'
    if wkbtype == QGis.WKBMultiPolygon:
        return 'multipolygon'
    if wkbtype == QGis.WKBPoint25D:
        return 'point 2.5d'
    if wkbtype == QGis.WKBLineString25D:
        return 'linestring 2.5D'
    if wkbtype == QGis.WKBPolygon25D:
        return 'multipolygon 2.5D'
    if wkbtype == QGis.WKBMultiPoint25D:
        return 'multipoint 2.5D'
    if wkbtype == QGis.WKBMultiLineString25D:
        return 'multilinestring 2.5D'
    if wkbtype == QGis.WKBMultiPolygon25D:
        return 'multipolygon 2.5D'
    return 'Unknown WKB ' + unicode(wkbtype)


class mmqgisx_grid_algorithm(GeoAlgorithm):

    HSPACING = 'HSPACING'
    VSPACING = 'VSPACING'
    WIDTH = 'WIDTH'
    HEIGHT = 'HEIGHT'
    CENTERX = 'CENTERX'
    CENTERY = 'CENTERY'
    GRIDTYPE = 'GRIDTYPE'
    SAVENAME = 'SAVENAME'
    CRS = 'CRS'

    def defineCharacteristics(self):
        self.name = 'Create grid'
        self.group = 'Vector creation tools'

        self.addParameter(ParameterNumber(self.HSPACING, 'Horizontal spacing',
                          default=10.0))
        self.addParameter(ParameterNumber(self.VSPACING, 'Vertical spacing',
                          default=10.0))
        self.addParameter(ParameterNumber(self.WIDTH, 'Width', default=360.0))
        self.addParameter(ParameterNumber(self.HEIGHT, 'Height',
                          default=180.0))
        self.addParameter(ParameterNumber(self.CENTERX, 'Center X',
                          default=0.0))
        self.addParameter(ParameterNumber(self.CENTERY, 'Center Y',
                          default=0.0))
        self.gridtype_options = ['Rectangle (line)', 'Rectangle (polygon)',
                                 'Diamond (polygon)', 'Hexagon (polygon)']
        self.addParameter(ParameterSelection(self.GRIDTYPE, 'Grid type',
                          self.gridtype_options, default=0))
        self.addParameter(ParameterCrs(self.CRS, 'CRS'))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):
        hspacing = self.getParameterValue(self.HSPACING)
        vspacing = self.getParameterValue(self.VSPACING)
        width = self.getParameterValue(self.WIDTH)
        height = self.getParameterValue(self.HEIGHT)
        centerx = self.getParameterValue(self.CENTERX)
        centery = self.getParameterValue(self.CENTERY)
        originx = centerx - width / 2.0
        originy = centery - height / 2.0
        gridtype = self.gridtype_options[self.getParameterValue(self.GRIDTYPE)]

        crsId = self.getParameterValue(self.CRS)
        self.crs = QgsCoordinateReferenceSystem(crsId)
        print self.crs.authid()

        if hspacing <= 0 or vspacing <= 0:
            raise GeoAlgorithmExecutionException('Invalid grid spacing: '
                    + unicode(hspacing) + ' / ' + unicode(vspacing))

        if width < hspacing:
            raise GeoAlgorithmExecutionException(
                    'Horizontal spacing is too small for the covered area')
        if height < vspacing:
            raise GeoAlgorithmExecutionException(
                    'Vertical spacing is too small for the covered area')

        fields = [
            QgsField('longitude', QVariant.Double, '', 24, 16, 'Longitude'),
            QgsField('latitude', QVariant.Double, '', 24, 16, 'Latitude')
            ]

        if gridtype.find('polygon') >= 0:
            shapetype = QGis.WKBPolygon
        else:
            shapetype = QGis.WKBLineString

        output = self.getOutputFromName(self.SAVENAME)
        out = output.getVectorWriter(fields, shapetype, self.crs)

        linecount = 0
        if gridtype == 'Rectangle (line)':
            x = originx
            while x <= originx + width:
                polyline = []
                geometry = QgsGeometry()
                feature = QgsFeature()

                y = originy
                while y <= originy + height:
                    polyline.append(QgsPoint(x, y))
                    y = y + vspacing

                feature.setGeometry(geometry.fromPolyline(polyline))
                feature.setAttributes([x, 0])
                out.addFeature(feature)
                linecount = linecount + 1

                x = x + hspacing

            y = originy
            while y <= originy + height:
                polyline = []
                geometry = QgsGeometry()
                feature = QgsFeature()

                x = originx
                while x <= originx + width:
                    polyline.append(QgsPoint(x, y))
                    x = x + hspacing

                feature.setGeometry(geometry.fromPolyline(polyline))
                feature.setAttributes([0, y])
                out.addFeature(feature)
                linecount = linecount + 1

                y = y + hspacing
        elif gridtype == 'Rectangle (polygon)':
            x = originx
            while x < originx + width:
                y = originy
                while y < originy + height:
                    polyline = []
                    polyline.append(QgsPoint(x, y))
                    polyline.append(QgsPoint(x + hspacing, y))
                    polyline.append(QgsPoint(x + hspacing, y + vspacing))
                    polyline.append(QgsPoint(x, y + vspacing))

                    geometry = QgsGeometry()
                    feature = QgsFeature()
                    feature.setGeometry(geometry.fromPolygon([polyline]))
                    feature.setAttributes([x + hspacing / 2.0, y + vspacing
                            / 2.0])
                    out.addFeature(feature)
                    linecount = linecount + 1
                    y = y + vspacing

                x = x + hspacing
        elif gridtype == 'Diamond (polygon)':
            x = originx
            colnum = 0
            while x < originx + width:
                if colnum % 2 == 0:
                    y = originy
                else:
                    y = originy + vspacing / 2.0

                while y < originy + height:
                    polyline = []
                    polyline.append(QgsPoint(x + hspacing / 2.0, y))
                    polyline.append(QgsPoint(x + hspacing, y + vspacing / 2.0))
                    polyline.append(QgsPoint(x + hspacing / 2.0, y + vspacing))
                    polyline.append(QgsPoint(x, y + vspacing / 2.0))

                    geometry = QgsGeometry()
                    feature = QgsFeature()
                    feature.setGeometry(geometry.fromPolygon([polyline]))
                    feature.setAttributes([x + hspacing / 2.0, y + vspacing
                            / 2.0])
                    out.addFeature(feature)
                    linecount = linecount + 1
                    y = y + vspacing

                x = x + hspacing / 2.0
                colnum = colnum + 1
        elif gridtype == 'Hexagon (polygon)':
            xvertexlo = 0.288675134594813 * vspacing
            xvertexhi = 0.577350269189626 * vspacing
            hspacing = xvertexlo + xvertexhi

            x = originx + xvertexhi

            colnum = 0
            while x < originx + width:
                if colnum % 2 == 0:
                    y = originy + vspacing / 2.0
                else:
                    y = originy + vspacing

                while y < originy + height:
                    polyline = []
                    polyline.append(QgsPoint(x + xvertexhi, y))
                    polyline.append(QgsPoint(x + xvertexlo, y + vspacing
                                    / 2.0))
                    polyline.append(QgsPoint(x - xvertexlo, y + vspacing
                                    / 2.0))
                    polyline.append(QgsPoint(x - xvertexhi, y))
                    polyline.append(QgsPoint(x - xvertexlo, y - vspacing
                                    / 2.0))
                    polyline.append(QgsPoint(x + xvertexlo, y - vspacing
                                    / 2.0))

                    geometry = QgsGeometry()
                    feature = QgsFeature()
                    feature.setGeometry(geometry.fromPolygon([polyline]))
                    feature.setAttributes([x, y])
                    out.addFeature(feature)
                    linecount = linecount + 1
                    y = y + vspacing

                x = x + hspacing
                colnum = colnum + 1

        del out


class mmqgisx_gridify_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    HSPACING = 'HSPACING'
    VSPACING = 'VSPACING'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Snap points to grid'
        self.group = 'Vector general tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterNumber(self.HSPACING, 'Horizontal spacing',
                          default=0.1))
        self.addParameter(ParameterNumber(self.VSPACING, 'Vertical spacing',
                          default=0.1))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):

        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYERNAME))
        hspacing = self.getParameterValue(self.HSPACING)
        vspacing = self.getParameterValue(self.VSPACING)

        if hspacing <= 0 or vspacing <= 0:
            return 'Invalid grid spacing: ' + unicode(hspacing) + '/' \
                + unicode(vspacing)

        output = self.getOutputFromName(self.SAVENAME)
        out = output.getVectorWriter(layer.pendingFields(), layer.wkbType(),
                                     layer.crs())

        point_count = 0
        deleted_points = 0
        feature_number = 0

        features = vector.features(layer)
        feature_count = len(features)
        for feature in features:
            progress.setPercentage(float(feature_number) / feature_count * 100)
            geometry = feature.geometry()

            if geometry.wkbType() == QGis.WKBPoint:
                (points, added, deleted) = mmqgisx_gridify_points(hspacing,
                        vspacing, [geometry.asPoint()])
                geometry = geometry.fromPoint(points[0])
                point_count += added
                deleted_points += deleted
            elif geometry.wkbType() == QGis.WKBLineString:
                (polyline, added, deleted) = mmqgisx_gridify_points(hspacing,
                        vspacing, geometry.asPolyline())
                if len(polyline) < 2:
                    geometry = None
                else:
                    geometry = geometry.fromPolyline(polyline)
                point_count += added
                deleted_points += deleted
            elif geometry.wkbType() == QGis.WKBPolygon:
                newpolygon = []
                for polyline in geometry.asPolygon():
                    (newpolyline, added, deleted) = \
                        mmqgisx_gridify_points(hspacing, vspacing, polyline)
                    point_count += added
                    deleted_points += deleted

                    if len(newpolyline) > 1:
                        newpolygon.append(newpolyline)

                if len(newpolygon) <= 0:
                    geometry = None
                else:
                    geometry = geometry.fromPolygon(newpolygon)
            elif geometry.wkbType() == QGis.WKBMultiPoint:
                (multipoints, added, deleted) = \
                    mmqgisx_gridify_points(hspacing, vspacing,
                        geometry.asMultiPoint())
                geometry = geometry.fromMultiPoint(multipoints)
                point_count += added
                deleted_points += deleted
            elif geometry.wkbType() == QGis.WKBMultiLineString:
                newmultipolyline = []
                for polyline in geometry.asMultiPolyline():
                    (newpolyline, added, deleted) = \
                        mmqgisx_gridify_points(hspacing, vspacing, polyline)
                    if len(newpolyline) > 1:
                        newmultipolyline.append(newpolyline)

                    if len(newmultipolyline) <= 0:
                        geometry = None
                    else:
                        geometry = geometry.fromMultiPolyline(newmultipolyline)

                    point_count += added
                    deleted_points += deleted
            elif geometry.wkbType() == QGis.WKBMultiPolygon:
                newmultipolygon = []
                for polygon in geometry.asMultiPolygon():
                    newpolygon = []
                    for polyline in polygon:
                        (newpolyline, added, deleted) = \
                            mmqgisx_gridify_points(hspacing, vspacing,
                                polyline)

                        if len(newpolyline) > 2:
                            newpolygon.append(newpolyline)

                        point_count += added
                        deleted_points += deleted

                    if len(newpolygon) > 0:
                        newmultipolygon.append(newpolygon)

                if len(newmultipolygon) <= 0:
                    geometry = None
                else:
                    geometry = geometry.fromMultiPolygon(newmultipolygon)
            else:
                return 'Unknown geometry type ' + unicode(geometry.wkbType()) \
                    + ' on feature ' + unicode(feature_number + 1)

            if geometry is not None:
                out_feature = QgsFeature()
                out_feature.setGeometry(geometry)
                out_feature.setAttributes(feature.attributes())
                out.addFeature(out_feature)

            feature_number += 1

        del out


def mmqgisx_gridify_points(hspacing, vspacing, points):

    point_count = 0
    deleted_points = 0
    newpoints = []
    for point in points:
        point_count += 1
        newpoints.append(QgsPoint(round(point.x() / hspacing, 0) * hspacing,
                         round(point.y() / vspacing, 0) * vspacing))

    # Delete overlapping points
    z = 0
    while z < len(newpoints) - 2:
        if newpoints[z] == newpoints[z + 1]:
            newpoints.pop(z + 1)
            deleted_points += 1
        else:
            z += 1

    # Delete line points that go out and return to the same place
    z = 0
    while z < len(newpoints) - 3:
        if newpoints[z] == newpoints[z + 2]:
            newpoints.pop(z + 1)
            newpoints.pop(z + 1)
            deleted_points += 2

            # Step back to catch arcs
            if z > 0:
                z -= 1
        else:
            z += 1

    # Delete overlapping start/end points
    while len(newpoints) > 1 and newpoints[0] == newpoints[len(newpoints) - 1]:
        newpoints.pop(len(newpoints) - 1)
        deleted_points += 2

    return (newpoints, point_count, deleted_points)


class mmqgisx_hub:

    def __init__(self, point, newname):
        self.point = point
        self.name = newname


class mmqgisx_hub_distance_algorithm(GeoAlgorithm):

    SOURCENAME = 'SOURCENAME'
    DESTNAME = 'DESTNAME'
    NAMEATTRIBUTE = 'NAMEATTRIBUTE'
    SHAPETYPE = 'SHAPETYPE'
    UNITS = 'UNITS'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Distance to nearest hub'
        self.group = 'Vector analysis tools'

        self.addParameter(ParameterVector(self.SOURCENAME,
                          'Source Points Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterVector(self.DESTNAME,
                          'Destination Hubs Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.NAMEATTRIBUTE,
                          'Hub Layer Name Attribute', self.DESTNAME))

        self.shapetypes = ['Point', 'Line to Hub']
        self.addParameter(ParameterSelection(self.SHAPETYPE,
                          'Output Shape Type', self.shapetypes, default=0))
        self.unitlist = ['Meters', 'Feet', 'Miles', 'Kilometers', 'Layer Units'
                         ]
        self.addParameter(ParameterSelection(self.UNITS, 'Measurement Unit',
                          self.unitlist, default=0))

        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):

        layersource = dataobjects.getObjectFromUri(
                self.getParameterValue(self.SOURCENAME))
        layerdest = dataobjects.getObjectFromUri(
                self.getParameterValue(self.DESTNAME))

        nameattribute = self.getParameterValue(self.NAMEATTRIBUTE)
        units = self.unitlist[self.getParameterValue(self.UNITS)]
        addlines = self.getParameterValue(self.SHAPETYPE)

        if layersource == layerdest:
            raise GeoAlgorithmExecutionException(
                    'Same layer given for both hubs and spokes')

        nameindex = layerdest.dataProvider().fieldNameIndex(nameattribute)
        if nameindex < 0:
            raise GeoAlgorithmExecutionException('Invalid name attribute: '
                    + nameattribute)

        outputtype = QGis.WKBPoint
        if addlines:
            outputtype = QGis.WKBLineString

        outfields = layersource.pendingFields()
        outfields.append(QgsField('HubName', QVariant.String))
        outfields.append(QgsField('HubDist', QVariant.Double))

        output = self.getOutputFromName(self.SAVENAME)
        out = output.getVectorWriter(outfields, outputtype, layersource.crs())

        # Create array of hubs in memory
        hubs = []
        features = vector.features(layerdest)
        for feature in features:
            hubs.append(mmqgisx_hub(feature.geometry().boundingBox().center(),
                        unicode(feature.attributes()[nameindex])))

        # Scan source points, find nearest hub, and write to output file
        writecount = 0
        features = vector.features(layersource)
        featureCount = len(features)
        for feature in features:
            source = feature.geometry().boundingBox().center()
            distance = QgsDistanceArea()
            distance.setSourceCrs(layersource.crs().srsid())
            distance.setEllipsoidalMode(True)

            closest = hubs[0]
            hubdist = distance.measureLine(source, closest.point)

            for hub in hubs:
                thisdist = distance.measureLine(source, hub.point)
                if thisdist < hubdist:
                    closest = hub
                    hubdist = thisdist

            attributes = feature.attributes()
            attributes.append(closest.name)
            if units == 'Feet':
                hubdist = hubdist * 3.2808399
            elif units == 'Miles':
                hubdist = hubdist * 0.000621371192
            elif units == 'Kilometers':
                hubdist = hubdist / 1000
            elif units != 'Meters':
                hubdist = sqrt(pow(source.x() - closest.point.x(), 2.0)
                               + pow(source.y() - closest.point.y(), 2.0))
            attributes.append(hubdist)

            outfeature = QgsFeature()
            outfeature.setAttributes(attributes)

            if outputtype == QGis.WKBPoint:
                geometry = QgsGeometry()
                outfeature.setGeometry(geometry.fromPoint(source))
            else:
                polyline = []
                polyline.append(source)
                polyline.append(closest.point)
                geometry = QgsGeometry()
                outfeature.setGeometry(geometry.fromPolyline(polyline))

            out.addFeature(outfeature)

            writecount += 1
            progress.setPercentage(float(writecount) / featureCount * 100)

        del out


class mmqgisx_hub_lines_algorithm(GeoAlgorithm):

    HUBNAME = 'HUBNAME'
    HUBATTRIBUTE = 'HUBATTRIBUTE'
    SPOKENAME = 'SPOKENAME'
    SPOKEATTRIBUTE = 'SPOKEATTRIBUTE'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Hub lines'
        self.group = 'Vector analysis tools'

        self.addParameter(ParameterVector(self.HUBNAME, 'Hub Point Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.HUBATTRIBUTE,
                          'Hub ID Attribute', self.HUBNAME))
        self.addParameter(ParameterVector(self.SPOKENAME, 'Spoke Point Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.SPOKEATTRIBUTE,
                          'Spoke Hub ID Attribute', self.SPOKENAME))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):

        hublayer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.HUBNAME))
        spokelayer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.SPOKENAME))

        hubattr = self.getParameterValue(self.HUBATTRIBUTE)
        spokeattr = self.getParameterValue(self.SPOKEATTRIBUTE)

        if hublayer == spokelayer:
            raise GeoAlgorithmExecutionException(
                    'Same layer given for both hubs and spokes')

        hubindex = hublayer.dataProvider().fieldNameIndex(hubattr)
        spokeindex = spokelayer.dataProvider().fieldNameIndex(spokeattr)

        outfields = spokelayer.pendingFields()

        output = self.getOutputFromName(self.SAVENAME)
        out = output.getVectorWriter(outfields, QGis.WKBLineString,
                                     spokelayer.crs())

        # Scan spoke points
        linecount = 0
        spokepoints = vector.features(spokelayer)
        i = 0
        for spokepoint in spokepoints:
            i += 1
            spokex = spokepoint.geometry().boundingBox().center().x()
            spokey = spokepoint.geometry().boundingBox().center().y()
            spokeid = unicode(spokepoint.attributes()[spokeindex])
            progress.setPercentage(float(i) / len(spokepoints) * 100)

            # Scan hub points to find first matching hub
            hubpoints = vector.features(hublayer)
            for hubpoint in hubpoints:
                hubid = unicode(hubpoint.attributes()[hubindex])
                if hubid == spokeid:
                    hubx = hubpoint.geometry().boundingBox().center().x()
                    huby = hubpoint.geometry().boundingBox().center().y()

                    # Write line to the output file
                    outfeature = QgsFeature()
                    outfeature.setAttributes(spokepoint.attributes())

                    polyline = []
                    polyline.append(QgsPoint(spokex, spokey))
                    polyline.append(QgsPoint(hubx, huby))
                    geometry = QgsGeometry()
                    outfeature.setGeometry(geometry.fromPolyline(polyline))
                    out.addFeature(outfeature)
                    linecount = linecount + 1
                    break

        del out

        if linecount <= 0:
            raise GeoAlgorithmExecutionException(
                    'No spoke/hub matches found to create lines')


class mmqgisx_merge_algorithm(GeoAlgorithm):

    LAYER1 = 'LAYER1'
    LAYER2 = 'LAYER2'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Merge vector layers'
        self.group = 'Vector general tools'

        self.addParameter(ParameterVector(self.LAYER1, 'Input layer 1',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterVector(self.LAYER2, 'Source layer 2',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):

        layer1 = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYER1))
        layer2 = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYER2))

        fields = []
        layers = [layer1, layer2]
        totalfeaturecount = 0

        if layer1.dataProvider().geometryType() \
            != layer2.dataProvider().geometryType():
            raise GeoAlgorithmExecutionException(
                    'Merged layers must all be same type of geometry ('
                    + mmqgisx_wkbtype_to_text(
                            layer2.dataProvider().geometryType())
                    + ' != '
                    + mmqgisx_wkbtype_to_text(
                            layer2.dataProvider().geometryType())
                    + ')')

        for layer in layers:
            totalfeaturecount += layer.featureCount()

            # Add any fields not in the composite field list
            for sfield in layer.pendingFields():
                found = None
                for dfield in fields:
                    if dfield.name() == sfield.name() and dfield.type() \
                        == sfield.type():
                        found = dfield
                        break

                if not found:
                    fields.append(sfield)

        output = self.getOutputFromName(self.SAVENAME)
        out = output.getVectorWriter(fields, layer1.wkbType(), layer1.crs())

        # Copy layer features to output file
        featurecount = 0
        for layer in layers:
            idx = {}
            for dfield in fields:
                i = 0
                for sfield in layer.pendingFields():
                    if sfield.name() == dfield.name() and sfield.type() \
                        == dfield.type():
                        idx[dfield] = i
                        break
                    i += 1

            features = vector.features(layer)
            for feature in features:
                sattributes = feature.attributes()
                dattributes = []
                for dfield in fields:
                    if dfield in idx:
                        dattributes.append(sattributes[idx[dfield]])
                    else:
                        dattributes.append(dfield.type())
                feature.setAttributes(dattributes)
                out.addFeature(feature)
                featurecount += 1
                progress.setPercentage(float(featurecount) / totalfeaturecount
                                       * 100)

        del out


class mmqgisx_select_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    ATTRIBUTE = 'ATTRIBUTE'
    COMPARISONVALUE = 'COMPARISONVALUE'
    COMPARISON = 'COMPARISON'
    RESULT = 'RESULT'

    def defineCharacteristics(self):
        self.allowOnlyOpenedLayers = True
        self.name = 'Select by attribute'
        self.group = 'Vector selection tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.ATTRIBUTE,
                          'Selection attribute', self.LAYERNAME))
        self.comparisons = [
            '==',
            '!=',
            '>',
            '>=',
            '<',
            '<=',
            'begins with',
            'contains',
            ]
        self.addParameter(ParameterSelection(self.COMPARISON, 'Comparison',
                          self.comparisons, default=0))
        self.addParameter(ParameterString(self.COMPARISONVALUE, 'Value',
                          default='0'))

        self.addOutput(OutputVector(self.RESULT, 'Output', True))

    def processAlgorithm(self, progress):

        filename = self.getParameterValue(self.LAYERNAME)
        layer = dataobjects.getObjectFromUri(filename)

        attribute = self.getParameterValue(self.ATTRIBUTE)
        comparison = self.comparisons[self.getParameterValue(self.COMPARISON)]
        comparisonvalue = self.getParameterValue(self.COMPARISONVALUE)

        selected = select(layer, attribute, comparison, comparisonvalue, progress)

        layer.setSelectedFeatures(selected)
        self.setOutputValue(self.RESULT, filename)

class mmqgisx_extract_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    ATTRIBUTE = 'ATTRIBUTE'
    COMPARISONVALUE = 'COMPARISONVALUE'
    COMPARISON = 'COMPARISON'
    RESULT = 'RESULT'

    def defineCharacteristics(self):
        self.name = 'Extract by attribute'
        self.group = 'Vector selection tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.ATTRIBUTE,
                          'Selection attribute', self.LAYERNAME))
        self.comparisons = [
            '==',
            '!=',
            '>',
            '>=',
            '<',
            '<=',
            'begins with',
            'contains',
            ]
        self.addParameter(ParameterSelection(self.COMPARISON, 'Comparison',
                          self.comparisons, default=0))
        self.addParameter(ParameterString(self.COMPARISONVALUE, 'Value',
                          default='0'))

        self.addOutput(OutputVector(self.RESULT, 'Output'))

    def processAlgorithm(self, progress):

        filename = self.getParameterValue(self.LAYERNAME)
        layer = dataobjects.getObjectFromUri(filename)

        attribute = self.getParameterValue(self.ATTRIBUTE)
        comparison = self.comparisons[self.getParameterValue(self.COMPARISON)]
        comparisonvalue = self.getParameterValue(self.COMPARISONVALUE)

        selected = select(layer, attribute, comparison, comparisonvalue, progress)

        features = vector.features(layer)
        featureCount = len(features)
        output = self.getOutputFromName(self.OUTPUT)
        writer = output.getVectorWriter(layer.fields(),
                layer.geometryType(), layer.crs())
        for (i, feat) in enumerate(features):
            if feat.id() in selected:
                writer.addFeature(feat)
            progress.setPercentage(100 * i / float(featureCount))
        del writer

def select(layer, attribute, comparison, comparisonvalue, progress):
    selectindex = layer.dataProvider().fieldNameIndex(attribute)
    selectType = layer.dataProvider().fields()[selectindex].type()
    selectionError = False

    if selectType == 2:
        try:
            y = int(comparisonvalue)
        except ValueError:
            selectionError = True
            msg = 'Cannot convert "' + unicode(comparisonvalue) \
                + '" to integer'
    elif selectType == 6:
        try:
            y = float(comparisonvalue)
        except ValueError:
            selectionError = True
            msg = 'Cannot convert "' + unicode(comparisonvalue) \
                + '" to float'
    elif selectType == 10:
        # string, boolean
        try:
            y = unicode(comparisonvalue)
        except ValueError:
            selectionError = True
            msg = 'Cannot convert "' + unicode(comparisonvalue) \
                + '" to unicode'
    elif selectType == 14:
        # Date
        dateAndFormat = comparisonvalue.split(' ')

        if len(dateAndFormat) == 1:
            # QtCore.QDate object
            y = QLocale.system().toDate(dateAndFormat[0])

            if y.isNull():
                msg = 'Cannot convert "' + unicode(dateAndFormat) \
                    + '" to date with system date format ' \
                    + QLocale.system().dateFormat()
        elif len(dateAndFormat) == 2:
            y = QDate.fromString(dateAndFormat[0], dateAndFormat[1])

            if y.isNull():
                msg = 'Cannot convert "' + unicode(dateAndFormat[0]) \
                    + '" to date with format string "' \
                    + unicode(dateAndFormat[1] + '". ')
        else:
            y = QDate()
            msg = ''

        if y.isNull():
            # Conversion was unsuccessfull
            selectionError = True
            msg += 'Enter the date and the date format, e.g. \
                    "07.26.2011" "MM.dd.yyyy".'

    if (comparison == 'begins with' or comparison == 'contains') \
        and selectType != 10:
        selectionError = True
        msg = '"' + comparison + '" can only be used with string fields'

    if selectionError:
        raise GeoAlgorithmExecutionException('Error in selection input: '
                + msg)

    readcount = 0
    selected = []
    features = vector.features(layer)
    totalcount = len(features)
    for feature in features:
        aValue = feature[selectindex]

        if aValue is None:
            continue

        if selectType == 2:
            x = int(aValue)
        elif selectType == 6:
            x = float(aValue)
        elif selectType == 10:
            # string, boolean
            x = unicode(aValue)
        elif selectType == 14:
            # date
            x = aValue

        match = False
        if comparison == '==':
            match = x == y
        elif comparison == '!=':
            match = x != y
        elif comparison == '>':
            match = x > y
        elif comparison == '>=':
            match = x >= y
        elif comparison == '<':
            match = x < y
        elif comparison == '<=':
            match = x <= y
        elif comparison == 'begins with':
            match = x.startswith(y)
        elif comparison == 'contains':
            match = x.find(y) >= 0

        readcount += 1
        if match:
            selected.append(feature.id())

        progress.setPercentage(float(readcount) / totalcount * 100)

    return selected

class mmqgisx_text_to_float_algorithm(GeoAlgorithm):

    LAYERNAME = 'LAYERNAME'
    ATTRIBUTE = 'ATTRIBUTE'
    SAVENAME = 'SAVENAME'

    def defineCharacteristics(self):
        self.name = 'Text to float'
        self.group = 'Vector table tools'

        self.addParameter(ParameterVector(self.LAYERNAME, 'Input Layer',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterTableField(self.ATTRIBUTE,
                          'Text attribute to convert to float',
                          self.LAYERNAME))
        self.addOutput(OutputVector(self.SAVENAME, 'Output'))

    def processAlgorithm(self, progress):
        layer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.LAYERNAME))
        attribute = self.getParameterValue(self.ATTRIBUTE)
        idx = layer.fieldNameIndex(attribute)
        fields = layer.pendingFields()
        fields[idx] = QgsField(fields[idx].name(), QVariant.Double)
        output = self.getOutputFromName(self.SAVENAME)
        out = output.getVectorWriter(fields, layer.wkbType(), layer.crs())

        i = 0
        features = vector.features(layer)
        featurecount = len(features)
        for feature in features:
            i += 1
            progress.setPercentage(float(i) / featurecount * 100)
            attributes = feature.attributes()
            try:
                v = unicode(attributes[idx])
                if '%' in v:
                    v = v.replace('%', '')
                    attributes[idx] = float(attributes[idx]) / float(100)
                else:
                    attributes[idx] = float(attributes[idx])
            except:
                attributes[idx] = None

            feature.setAttributes(attributes)
            out.addFeature(feature)

        del out
