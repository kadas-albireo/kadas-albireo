# -*- coding: utf-8 -*-

"""
***************************************************************************
    dinfdistup.py
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

from PyQt4.QtGui import QIcon

from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.ProcessingConfig import ProcessingConfig
from processing.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException

from processing.core.parameters import ParameterRaster
from processing.core.parameters import ParameterNumber
from processing.core.parameters import ParameterBoolean
from processing.core.parameters import ParameterSelection
from processing.core.outputs import OutputRaster

from TauDEMUtils import TauDEMUtils


class DinfDistUp(GeoAlgorithm):

    DINF_FLOW_DIR_GRID = 'DINF_FLOW_DIR_GRID'
    PIT_FILLED_GRID = 'PIT_FILLED_GRID'
    SLOPE_GRID = 'SLOPE_GRID'
    THRESHOLD = 'THRESHOLD'
    STAT_METHOD = 'STAT_METHOD'
    DIST_METHOD = 'DIST_METHOD'
    EDGE_CONTAM = 'EDGE_CONTAM'

    DIST_UP_GRID = 'DIST_UP_GRID'

    STATISTICS = ['Minimum', 'Maximum', 'Average']
    STAT_DICT = {0: 'min', 1: 'max', 2: 'ave'}

    DISTANCE = ['Pythagoras', 'Horizontal', 'Vertical', 'Surface']
    DIST_DICT = {
        0: 'p',
        1: 'h',
        2: 'v',
        3: 's',
    }

    def getIcon(self):
        return QIcon(os.path.dirname(__file__) + '/../../images/taudem.png')

    def defineCharacteristics(self):
        self.name = 'D-Infinity Distance Up'
        self.cmdName = 'dinfdistup'
        self.group = 'Specialized Grid Analysis tools'

        self.addParameter(ParameterRaster(self.DINF_FLOW_DIR_GRID,
            self.tr('D-Infinity Flow Direction Grid'), False))
        self.addParameter(ParameterRaster(self.PIT_FILLED_GRID,
            self.tr('Pit Filled Elevation Grid'), False))
        self.addParameter(ParameterRaster(self.SLOPE_GRID,
            self.tr('Slope Grid'), False))
        self.addParameter(ParameterSelection(self.STAT_METHOD,
            self.tr('Statistical Method'), self.STATISTICS, 2))
        self.addParameter(ParameterSelection(self.DIST_METHOD,
            self.tr('Distance Method'), self.DISTANCE, 1))
        self.addParameter(ParameterNumber(self.THRESHOLD,
            self.tr('Proportion Threshold'), 0, None, 0.5))
        self.addParameter(ParameterBoolean(self.EDGE_CONTAM,
            self.tr('Check for edge contamination'), True))

        self.addOutput(OutputRaster(self.DIST_UP_GRID,
            self.tr('D-Infinity Distance Up')))

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
        commands.append(os.path.join(TauDEMUtils.taudemPath(), self.cmdName))
        commands.append('-ang')
        commands.append(self.getParameterValue(self.DINF_FLOW_DIR_GRID))
        commands.append('-fel')
        commands.append(self.getParameterValue(self.PIT_FILLED_GRID))
        commands.append('-m')
        commands.append(str(self.STAT_DICT[self.getParameterValue(
            self.STAT_METHOD)]))
        commands.append(str(self.DIST_DICT[self.getParameterValue(
            self.DIST_METHOD)]))
        commands.append('-thresh')
        commands.append(str(self.getParameterValue(self.THRESHOLD)))
        if str(self.getParameterValue(self.EDGE_CONTAM)).lower() == 'false':
            commands.append('-nc')
        commands.append('-du')
        commands.append(self.getOutputValue(self.DIST_UP_GRID))

        TauDEMUtils.executeTauDEM(commands, progress)
