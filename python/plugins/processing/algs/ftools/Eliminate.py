# -*- coding: utf-8 -*-

"""
***************************************************************************
    Eliminate.py
    ---------------------
    Date                 : August 2012
    Copyright            : (C) 2013 by Bernhard Str�bl
    Email                : bernhard.stroebl@jena.de
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Bernhard Ströbl'
__date__ = 'September 2013'
__copyright__ = '(C) 2013, Bernhard Ströbl'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

from PyQt4.QtCore import *
from qgis.core import *
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.GeoAlgorithmExecutionException import \
        GeoAlgorithmExecutionException
from processing.core.ProcessingLog import ProcessingLog
from processing.parameters.ParameterVector import ParameterVector
from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.parameters.ParameterTableField import ParameterTableField
from processing.parameters.ParameterString import ParameterString
from processing.parameters.ParameterSelection import ParameterSelection
from processing.outputs.OutputVector import OutputVector
from processing.tools import dataobjects


class Eliminate(GeoAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    MODE = 'MODE'
    KEEPSELECTION = 'KEEPSELECTION'
    ATTRIBUTE = 'ATTRIBUTE'
    COMPARISONVALUE = 'COMPARISONVALUE'
    COMPARISON = 'COMPARISON'

    MODES = ['Area', 'Common boundary']
    MODE_AREA = 0
    MODE_BOUNDARY = 1

    def defineCharacteristics(self):
        self.name = 'Eliminate sliver polygons'
        self.group = 'Vector geometry tools'
        self.addParameter(ParameterVector(self.INPUT, 'Input layer',
                          [ParameterVector.VECTOR_TYPE_POLYGON]))
        self.addParameter(ParameterBoolean(self.KEEPSELECTION,
                          'Use current selection in input layer (works only \
                          if called from toolbox)', False))
        self.addParameter(ParameterTableField(self.ATTRIBUTE,
                          'Selection attribute', self.INPUT))
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
        self.addParameter(ParameterSelection(self.MODE,
                          'Merge selection with the neighbouring polygon \
                          with the largest', self.MODES))
        self.addOutput(OutputVector(self.OUTPUT, 'Cleaned layer'))

    def processAlgorithm(self, progress):
        inLayer = dataobjects.getObjectFromUri(
                self.getParameterValue(self.INPUT))
        boundary = self.getParameterValue(self.MODE) == self.MODE_BOUNDARY
        keepSelection = self.getParameterValue(self.KEEPSELECTION)

        if not keepSelection:
            # Make a selection with the values provided
            attribute = self.getParameterValue(self.ATTRIBUTE)
            comparison = self.comparisons[
                    self.getParameterValue(self.COMPARISON)]
            comparisonvalue = self.getParameterValue(self.COMPARISONVALUE)

            selectindex = inLayer.dataProvider().fieldNameIndex(attribute)
            selectType = inLayer.dataProvider().fields()[selectindex].type()
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
               # 10: string, boolean
                try:
                    y = unicode(comparisonvalue)
                except ValueError:
                    selectionError = True
                    msg = 'Cannot convert "' + unicode(comparisonvalue) \
                        + '" to unicode'
            elif selectType == 14:
                # date
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
                msg = '"' + comparison \
                    + '" can only be used with string fields'

            selected = []

            if selectionError:
                raise GeoAlgorithmExecutionException(
                        'Error in selection input: ' + msg)
            else:
                for feature in inLayer.getFeatures():
                    aValue = feature.attributes()[selectindex]

                    if aValue is None:
                        continue

                    if selectType == 2:
                        x = int(aValue)
                    elif selectType == 6:
                        x = float(aValue)
                    elif selectType == 10:
                        # 10: string, boolean
                        x = unicode(aValue)
                    elif selectType == 14:
                        # date
                        x = aValue  # should be date

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

                    if match:
                        selected.append(feature.id())

            inLayer.setSelectedFeatures(selected)

        if inLayer.selectedFeatureCount() == 0:
            ProcessingLog.addToLog(ProcessingLog.LOG_WARNING,
                                   self.commandLineName()
                                   + '(No selection in input layer "'
                                   + self.getParameterValue(self.INPUT) + '")')

        # Keep references to the features to eliminate
        featToEliminate = []
        for aFeat in inLayer.selectedFeatures():
            featToEliminate.append(aFeat)

        # Delete all features to eliminate in inLayer (we won't save this)
        inLayer.startEditing()
        inLayer.deleteSelectedFeatures()

        # ANALYZE
        if len(featToEliminate) > 0:  # Prevent zero division
            start = 20.00
            add = 80.00 / len(featToEliminate)
        else:
            start = 100

        progress.setPercentage(start)
        madeProgress = True

        # We go through the list and see if we find any polygons we can
        # merge the selected with. If we have no success with some we
        # merge and then restart the whole story.
        while madeProgress:  # Check if we made any progress
            madeProgress = False
            featNotEliminated = []

            # Iterate over the polygons to eliminate
            for i in range(len(featToEliminate)):
                feat = featToEliminate.pop()
                geom2Eliminate = feat.geometry()
                bbox = geom2Eliminate.boundingBox()
                fit = inLayer.getFeatures(
                        QgsFeatureRequest().setFilterRect(bbox))
                mergeWithFid = None
                mergeWithGeom = None
                max = 0
                selFeat = QgsFeature()

                while fit.nextFeature(selFeat):
                    selGeom = selFeat.geometry()

                    if geom2Eliminate.intersects(selGeom):
                        # We have a candidate
                        iGeom = geom2Eliminate.intersection(selGeom)

                        if boundary:
                            selValue = iGeom.length()
                        else:
                            # Largest area. We need a common boundary in
                            # order to merge
                            if 0 < iGeom.length():
                                selValue = selGeom.area()
                            else:
                                selValue = 0

                        if selValue > max:
                            max = selValue
                            mergeWithFid = selFeat.id()
                            mergeWithGeom = QgsGeometry(selGeom)
                # End while fit

                if mergeWithFid is not None:
                    # A successful candidate
                    newGeom = mergeWithGeom.combine(geom2Eliminate)

                    if inLayer.changeGeometry(mergeWithFid, newGeom):
                        madeProgress = True
                    else:
                        raise GeoAlgorithmExecutionException(
                                'Could not replace geometry of feature \
                                with id %s' % mergeWithFid)

                    start = start + add
                    progress.setPercentage(start)
                else:
                    featNotEliminated.append(feat)

            # End for featToEliminate

            featToEliminate = featNotEliminated

        # End while

        # Create output
        provider = inLayer.dataProvider()
        output = self.getOutputFromName(self.OUTPUT)
        writer = output.getVectorWriter(provider.fields(),
                provider.geometryType(), inLayer.crs())

        # Write all features that are left over to output layer
        iterator = inLayer.getFeatures()
        for feature in iterator:
            writer.addFeature(feature)

        # Leave inLayer untouched
        inLayer.rollBack()

        for feature in featNotEliminated:
            writer.addFeature(feature)
