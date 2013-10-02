# -*- coding: utf-8 -*-

"""
***************************************************************************
    lengtharea.py
    ---------------------
    Date                 : October 2012
    Copyright            : (C) 2012 by Alexander Bruy
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
__date__ = 'October 2012'
__copyright__ = '(C) 2012, Alexander Bruy'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os

from PyQt4.QtGui import *

from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.ProcessingLog import ProcessingLog
from processing.core.ProcessingConfig import ProcessingConfig
from processing.core.GeoAlgorithmExecutionException import \
    GeoAlgorithmExecutionException

from processing.parameters.ParameterRaster import ParameterRaster
from processing.parameters.ParameterNumber import ParameterNumber
from processing.outputs.OutputRaster import OutputRaster

from processing.tools.system import *

from processing.taudem.TauDEMUtils import TauDEMUtils


class LengthArea(GeoAlgorithm):

    LENGTH_GRID = 'LENGTH_GRID'
    CONTRIB_AREA_GRID = 'CONTRIB_AREA_GRID'
    THRESHOLD = 'THRESHOLD'
    EXPONENT = 'EXPONENT'

    STREAM_SOURCE_GRID = 'STREAM_SOURCE_GRID'

    def getIcon(self):
        return QIcon(os.path.dirname(__file__) + '/../images/taudem.png')

    def defineCharacteristics(self):
        self.name = 'Length Area Stream Source'
        self.cmdName = 'lengtharea'
        self.group = 'Stream Network Analysis tools'

        self.addParameter(ParameterRaster(self.LENGTH_GRID, 'Length Grid',
                          False))
        self.addParameter(ParameterRaster(self.CONTRIB_AREA_GRID,
                          'Contributing Area Grid', False))
        self.addParameter(ParameterNumber(self.THRESHOLD, 'Threshold', 0,
                          None, 0.03))
        self.addParameter(ParameterNumber(self.EXPONENT, 'Exponent', 0, None,
                          1.3))

        self.addOutput(OutputRaster(self.STREAM_SOURCE_GRID,
                       'Stream Source Grid'))

    def processAlgorithm(self, progress):
        commands = []
        commands.append(os.path.join(TauDEMUtils.mpiexecPath(), 'mpiexec'))

        processNum = ProcessingConfig.getSetting(TauDEMUtils.MPI_PROCESSES)
        if processNum <= 0:
            raise GeoAlgorithmExecutionException('Wrong number of MPI \
                processes used.\nPlease set correct number before running \
                TauDEM algorithms.')

        commands.append('-n')
        commands.append(str(processNum))
        commands.append(os.path.join(TauDEMUtils.taudemPath(), self.cmdName))
        commands.append('-plen')
        commands.append(self.getParameterValue(self.LENGTH_GRID))
        commands.append('-ad8')
        commands.append(self.getParameterValue(self.CONTRIB_AREA_GRID))
        commands.append('-par')
        commands.append(str(self.getParameterValue(self.THRESHOLD)))
        commands.append(str(self.getParameterValue(self.EXPONENT)))
        commands.append('-ss')
        commands.append(self.getOutputValue(self.STREAM_SOURCE_GRID))

        loglines = []
        loglines.append('TauDEM execution command')
        for line in commands:
            loglines.append(line)
        ProcessingLog.addToLog(ProcessingLog.LOG_INFO, loglines)

        TauDEMUtils.executeTauDEM(commands, progress)
