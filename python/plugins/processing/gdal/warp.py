# -*- coding: utf-8 -*-

"""
***************************************************************************
    self.py
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
from processing.parameters.ParameterRaster import ParameterRaster
from processing.parameters.ParameterSelection import ParameterSelection
from processing.parameters.ParameterCrs import ParameterCrs
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterString import ParameterString
from processing.outputs.OutputRaster import OutputRaster
from processing.gdal.GdalUtils import GdalUtils


class warp(GeoAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    SOURCE_SRS = 'SOURCE_SRS'
    DEST_SRS = 'DEST_SRS '
    METHOD = 'METHOD'
    METHOD_OPTIONS = ['near', 'bilinear', 'cubic', 'cubicspline', 'lanczos']
    TR = 'TR'
    EXTRA = 'EXTRA'

    def getIcon(self):
        filepath = os.path.dirname(__file__) + '/icons/self.png'
        return QtGui.QIcon(filepath)

    def defineCharacteristics(self):
        self.name = 'Warp (reproject)'
        self.group = '[GDAL] Projections'
        self.addParameter(ParameterRaster(self.INPUT, 'Input layer', False))
        self.addParameter(ParameterCrs(self.SOURCE_SRS,
                          'Source SRS (EPSG Code)', 'EPSG:4326'))
        self.addParameter(ParameterCrs(self.DEST_SRS,
                          'Destination SRS (EPSG Code)', 'EPSG:4326'))
        self.addParameter(ParameterNumber(self.TR,
            'Output file resolution in target georeferenced units \
            (leave 0 for no change)', 0.0, None, 0.0))
        self.addParameter(ParameterSelection(self.METHOD, 'Resampling method',
                          self.METHOD_OPTIONS))
        self.addParameter(ParameterString(self.EXTRA,
                          'Additional creation parameters', ''))
        self.addOutput(OutputRaster(self.OUTPUT, 'Output layer'))

    def processAlgorithm(self, progress):
        arguments = []
        arguments.append('-s_srs')
        arguments.append(str(self.getParameterValue(self.SOURCE_SRS)))
        arguments.append('-t_srs')
        crsId = self.getParameterValue(self.DEST_SRS)
        self.crs = QgsCoordinateReferenceSystem(crsId)
        arguments.append(str(crsId))
        arguments.append('-r')
        arguments.append(
                self.METHOD_OPTIONS[self.getParameterValue(self.METHOD)])
        arguments.append('-of')
        out = self.getOutputValue(self.OUTPUT)
        arguments.append(GdalUtils.getFormatShortNameFromFilename(out))
        if self.getParameterValue(self.TR) != 0:
            arguments.append('-tr')
            arguments.append(str(self.getParameterValue(self.TR)))
            arguments.append(str(self.getParameterValue(self.TR)))
        extra = str(self.getParameterValue(self.EXTRA))
        if len(extra) > 0:
            arguments.append(extra)
        arguments.append(self.getParameterValue(self.INPUT))
        arguments.append(out)

        GdalUtils.runGdal(['gdalwarp', GdalUtils.escapeAndJoin(arguments)],
                          progress)
