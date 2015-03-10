# -*- coding: utf-8 -*-

"""
***************************************************************************
    lengtharea_multi.py
    ---------------------
    Date                 : March 2015
    Copyright            : (C) 2015 by Alexander Bruy
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
__date__ = 'March 2015'
__copyright__ = '(C) 2015, Alexander Bruy'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os

from PyQt4.QtGui import QIcon

from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.ProcessingConfig import ProcessingConfig
from processing.core.GeoAlgorithmExecutionException import \
    GeoAlgorithmExecutionException

from processing.core.parameters import ParameterFile
from processing.core.parameters import ParameterNumber
from processing.core.outputs import OutputDirectory

from TauDEMUtils import TauDEMUtils


class LengthAreaMulti(GeoAlgorithm):

    LENGTH_GRID = 'LENGTH_GRID'
    CONTRIB_AREA_GRID = 'CONTRIB_AREA_GRID'
    THRESHOLD = 'THRESHOLD'
    EXPONENT = 'EXPONENT'

    STREAM_SOURCE_GRID = 'STREAM_SOURCE_GRID'

    def getIcon(self):
        return QIcon(os.path.dirname(__file__) + '/../../images/taudem.png')

    def defineCharacteristics(self):
        self.name = 'Length Area Stream Source (multifile)'
        self.cmdName = 'lengtharea'
        self.group = 'Stream Network Analysis tools'

        self.addParameter(ParameterFile(self.LENGTH_GRID,
            self.tr('Length Grid'), True, False))
        self.addParameter(ParameterFile(self.CONTRIB_AREA_GRID,
            self.tr('Contributing Area Grid'), True, False))
        self.addParameter(ParameterNumber(self.THRESHOLD,
            self.tr('Threshold'), 0, None, 0.03))
        self.addParameter(ParameterNumber(self.EXPONENT,
            self.tr('Exponent'), 0, None, 1.3))

        self.addOutput(OutputDirectory(self.STREAM_SOURCE_GRID,
            self.tr('Stream Source Grid')))

    def processAlgorithm(self, progress):
        commands = []
        commands.append(os.path.join(TauDEMUtils.mpiexecPath(), 'mpiexec'))

        processNum = ProcessingConfig.getSetting(TauDEMUtils.MPI_PROCESSES)
        if processNum <= 0:
            raise GeoAlgorithmExecutionException(
                self.tr('Wrong number of MPI processes used. Please set '
                        'correct number before running TauDEM algorithms.'))

        commands.append('-n')
        commands.append(str(processNum))
        commands.append(os.path.join(TauDEMUtils.taudemMultifilePath(), self.cmdName))
        commands.append('-plen')
        commands.append(self.getParameterValue(self.LENGTH_GRID))
        commands.append('-ad8')
        commands.append(self.getParameterValue(self.CONTRIB_AREA_GRID))
        commands.append('-par')
        commands.append(str(self.getParameterValue(self.THRESHOLD)))
        commands.append(str(self.getParameterValue(self.EXPONENT)))
        commands.append('-ss')
        commands.append(self.getOutputValue(self.STREAM_SOURCE_GRID))

        TauDEMUtils.executeTauDEM(commands, progress)
