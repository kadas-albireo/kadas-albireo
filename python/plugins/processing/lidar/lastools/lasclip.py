# -*- coding: utf-8 -*-

"""
***************************************************************************
    lasclip.py
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
from processing.lidar.lastools.LAStoolsUtils import LAStoolsUtils
from processing.lidar.lastools.LAStoolsAlgorithm import LAStoolsAlgorithm

from processing.parameters.ParameterVector import ParameterVector
from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterSelection import ParameterSelection


class lasclip(LAStoolsAlgorithm):

    POLYGON = 'POLYGON'
    INTERIOR = 'INTERIOR'
    OPERATION = 'OPERATION'
    OPERATIONS = ['clip', 'classify']
    CLASSIFY_AS = 'CLASSIFY_AS'

    def defineCharacteristics(self):
        self.name = 'lasclip'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParameter(ParameterVector(lasclip.POLYGON, 'Input polygon(s)',
                          ParameterVector.VECTOR_TYPE_POLYGON))
        self.addParameter(ParameterBoolean(lasclip.INTERIOR, 'interior',
                          False))
        self.addParameter(ParameterSelection(lasclip.OPERATION,
                          'what to do with isolated points',
                          lasclip.OPERATIONS, 0))
        self.addParameter(ParameterNumber(lasclip.CLASSIFY_AS, 'classify as',
                          0, None, 12))
        self.addParametersPointOutputGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'lasclip.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        poly = self.getParameterValue(lasclip.POLYGON)
        if poly is not None:
            commands.append('-poly')
            commands.append(poly)
        if self.getParameterValue(lasclip.INTERIOR):
            commands.append('-interior')
        operation = self.getParameterValue(lasclip.OPERATION)
        if operation != 0:
            commands.append('-classify')
            classify_as = self.getParameterValue(lasclip.CLASSIFY_AS)
            commands.append(str(classify_as))
        self.addParametersPointOutputCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
