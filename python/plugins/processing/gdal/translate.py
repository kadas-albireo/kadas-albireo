# -*- coding: utf-8 -*-

"""
***************************************************************************
    translate.py
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

import os
from PyQt4 import QtGui
from qgis.core import *
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.parameters.ParameterString import ParameterString
from processing.parameters.ParameterRaster import ParameterRaster
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.parameters.ParameterSelection import ParameterSelection
from processing.parameters.ParameterExtent import ParameterExtent
from processing.parameters.ParameterCrs import ParameterCrs
from processing.outputs.OutputRaster import OutputRaster

from processing.gdal.GdalUtils import GdalUtils


class translate(GeoAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    OUTSIZE = 'OUTSIZE'
    OUTSIZE_PERC = 'OUTSIZE_PERC'
    NO_DATA = 'NO_DATA'
    EXPAND = 'EXPAND'
    PROJWIN = 'PROJWIN'
    SRS = 'SRS'
    SDS = 'SDS'
    EXTRA = 'EXTRA'

    def getIcon(self):
        filepath = os.path.dirname(__file__) + '/icons/translate.png'
        return QtGui.QIcon(filepath)

    def commandLineName(self):
        return "gdalogr:translate"

    def defineCharacteristics(self):
        self.name = 'Translate (convert format)'
        self.group = '[GDAL] Conversion'
        self.addParameter(ParameterRaster(self.INPUT, 'Input layer',
                          False))
        self.addParameter(ParameterNumber(self.OUTSIZE,
                          'Set the size of the output file (In pixels or %)',
                          1, None, 100))
        self.addParameter(ParameterBoolean(self.OUTSIZE_PERC,
                          'Output size is a percentage of input size', True))
        self.addParameter(ParameterString(self.NO_DATA,
            'Nodata value, leave as none to take the nodata value from input',
            'none'))
        self.addParameter(ParameterSelection(self.EXPAND, 'Expand',
                          ['none', 'gray', 'rgb', 'rgba']))
        self.addParameter(ParameterCrs(self.SRS,
                          'Override the projection for the output file', None))
        self.addParameter(ParameterExtent(self.PROJWIN,
                          'Subset based on georeferenced coordinates'))
        self.addParameter(ParameterBoolean(self.SDS,
            'Copy all subdatasets of this file to individual output files',
            False))
        self.addParameter(ParameterString(self.EXTRA,
                          'Additional creation parameters', ''))
        self.addOutput(OutputRaster(self.OUTPUT, 'Output layer'))

    def processAlgorithm(self, progress):
        out = self.getOutputValue(translate.OUTPUT)
        outsize = str(self.getParameterValue(self.OUTSIZE))
        outsizePerc = str(self.getParameterValue(self.OUTSIZE_PERC))
        noData = str(self.getParameterValue(self.NO_DATA))
        expand = str(self.getParameterFromName(
                self.EXPAND).options[self.getParameterValue(self.EXPAND)])
        projwin = str(self.getParameterValue(self.PROJWIN))
        crsId = self.getParameterValue(self.SRS)
        sds = self.getParameterValue(self.SDS)
        extra = str(self.getParameterValue(self.EXTRA))

        arguments = []
        arguments.append('-of')
        arguments.append(GdalUtils.getFormatShortNameFromFilename(out))
        if outsizePerc == 'True':
            arguments.append('-outsize')
            arguments.append(outsize + '%')
            arguments.append(outsize + '%')
        else:
            arguments.append('-outsize')
            arguments.append(outsize)
            arguments.append(outsize)
        arguments.append('-a_nodata')
        arguments.append(noData)
        if expand != 'none':
            arguments.append('-expand')
            arguments.append(expand)
        regionCoords = projwin.split(',')
        arguments.append('-projwin')
        arguments.append(regionCoords[0])
        arguments.append(regionCoords[3])
        arguments.append(regionCoords[1])
        arguments.append(regionCoords[2])
        if crsId is not None:
            arguments.append('-a_srs')
            arguments.append(str(crsId))
        if sds:
            arguments.append('-sds')
        if len(extra) > 0:
            arguments.append(extra)
        arguments.append(self.getParameterValue(self.INPUT))
        arguments.append(out)

        GdalUtils.runGdal(['gdal_translate',
                          GdalUtils.escapeAndJoin(arguments)], progress)
