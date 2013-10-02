# -*- coding: utf-8 -*-

"""
***************************************************************************
    gdal2xyz.py
    ---------------------
    Date                 : September 2013
    Copyright            : (C) 2013 by Alexander Bruy
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
__date__ = 'September 2013'
__copyright__ = '(C) 2013, Alexander Bruy'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os
from PyQt4.QtGui import *
from PyQt4.QtCore import *

from processing.core.GeoAlgorithm import GeoAlgorithm

from processing.parameters.ParameterRaster import ParameterRaster
from processing.parameters.ParameterNumber import ParameterNumber
from processing.outputs.OutputTable import OutputTable

from processing.tools.system import *

from processing.gdal.GdalUtils import GdalUtils


class gdal2xyz(GeoAlgorithm):

    INPUT = 'INPUT'
    BAND = 'BAND'
    OUTPUT = 'OUTPUT'

    #def getIcon(self):
    #   return QIcon(os.path.dirname(__file__) + "/icons/gdal2xyz.png")

    def defineCharacteristics(self):
        self.name = 'gdal2xyz'
        self.group = '[GDAL] Conversion'
        self.addParameter(ParameterRaster(self.INPUT, 'Input layer', False))
        self.addParameter(ParameterNumber(self.BAND, 'Band number', 1, 9999,
                          1))

        self.addOutput(OutputTable(self.OUTPUT, 'Output file'))

    def processAlgorithm(self, progress):
        arguments = []
        arguments.append('-band')
        arguments.append(str(self.getParameterValue(self.BAND)))

        arguments.append('-csv')
        arguments.append(self.getParameterValue(self.INPUT))
        arguments.append(self.getOutputValue(self.OUTPUT))

        commands = []
        if isWindows():
            commands = ['cmd.exe', '/C ', 'gdal2xyz.bat',
                        GdalUtils.escapeAndJoin(arguments)]
        else:
            commands = ['gdal2xyz.py', GdalUtils.escapeAndJoin(arguments)]

        GdalUtils.runGdal(commands, progress)
