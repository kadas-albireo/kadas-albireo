# -*- coding: utf-8 -*-

"""
***************************************************************************
    lasboundary.py
    ---------------------
    Date                 : August 2012
    Copyright            : (C) 2012 by Victor Olaya
    Email                : volayaf at gmail dot com
    ---------------------
    Date                 : September 2013
    Copyright            : (C) 2013 by Martin Isenburg
    Email                : martin near rapidlasso point com
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
from processing.lidar.lastools.LAStoolsUtils import LAStoolsUtils
from processing.lidar.lastools.LAStoolsAlgorithm import LAStoolsAlgorithm

from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterString import ParameterString


class lasboundary(LAStoolsAlgorithm):

    CONCAVITY = 'CONCAVITY'
    DISJOINT = 'DISJOINT'
    HOLES = 'HOLES'

    def defineCharacteristics(self):
        self.name = 'lasboundary'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParametersFilter1ReturnClassFlagsGUI()
        self.addParameter(ParameterNumber(lasboundary.CONCAVITY, 'concavity',
                          0, None, 50.0))
        self.addParameter(ParameterBoolean(lasboundary.HOLES, 'interior holes'
                          , False))
        self.addParameter(ParameterBoolean(lasboundary.DISJOINT,
                          'disjoint polygon', False))
        self.addParametersVectorOutputGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'lasboundary.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        self.addParametersFilter1ReturnClassFlagsCommands(commands)
        concavity = self.getParameterValue(lasboundary.CONCAVITY)
        commands.append('-concavity')
        commands.append(str(concavity))
        if self.getParameterValue(lasboundary.HOLES):
            commands.append('-holes')
        if self.getParameterValue(lasboundary.DISJOINT):
            commands.append('-disjoint')
        self.addParametersVectorOutputCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
