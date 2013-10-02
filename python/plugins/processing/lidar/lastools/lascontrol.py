# -*- coding: utf-8 -*-

"""
***************************************************************************
    lascontrol.py
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

__author__ = 'Martin Isenburg'
__date__ = 'September 2013'
__copyright__ = '(C) 2013, Martin Isenburg'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os
from processing.lidar.lastools.LAStoolsUtils import LAStoolsUtils
from processing.lidar.lastools.LAStoolsAlgorithm import LAStoolsAlgorithm

from processing.parameters.ParameterVector import ParameterVector
from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterSelection import ParameterSelection


class lascontrol(LAStoolsAlgorithm):

    POLYGON = 'POLYGON'
    INTERIOR = 'INTERIOR'
    OPERATION = 'OPERATION'
    OPERATIONS = ['clip', 'classify']
    CLASSIFY_AS = 'CLASSIFY_AS'

    def defineCharacteristics(self):
        self.name = 'lascontrol'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParameter(ParameterVector(lascontrol.POLYGON,
                          'Input polygon(s)',
                          ParameterVector.VECTOR_TYPE_POLYGON))
        self.addParameter(ParameterBoolean(lascontrol.INTERIOR, 'interior',
                          False))
        self.addParameter(ParameterSelection(lascontrol.OPERATION,
                          'what to do with isolated points',
                          lascontrol.OPERATIONS, 0))
        self.addParameter(ParameterNumber(lascontrol.CLASSIFY_AS, 'classify as'
                          , 0, None, 12))
        self.addParametersPointOutputGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'lascontrol.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        poly = self.getParameterValue(lascontrol.POLYGON)
        if poly is not None:
            commands.append('-poly')
            commands.append(poly)
        if self.getParameterValue(lascontrol.INTERIOR):
            commands.append('-interior')
        operation = self.getParameterValue(lascontrol.OPERATION)
        if operation != 0:
            commands.append('-classify')
            classify_as = self.getParameterValue(lascontrol.CLASSIFY_AS)
            commands.append(str(classify_as))
        self.addParametersPointOutputCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
