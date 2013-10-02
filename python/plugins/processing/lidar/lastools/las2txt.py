# -*- coding: utf-8 -*-

"""
***************************************************************************
    las2txt.py
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

from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.parameters.ParameterString import ParameterString
from processing.outputs.OutputFile import OutputFile


class las2txt(LAStoolsAlgorithm):

    PARSE_STRING = 'PARSE_STRING'
    OUTPUT = 'OUTPUT'

    def defineCharacteristics(self):
        self.name = 'las2txt'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParameter(ParameterString(las2txt.PARSE_STRING, 'parse_string'
                          , 'xyz'))
        self.addOutput(OutputFile(las2txt.OUTPUT, 'Output ASCII file'))

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'las2txt.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        parse_string = self.getParameterValue(las2txt.PARSE_STRING)
        if parse_string != 'xyz':
            commands.append('-parse_string')
            commands.append(parse_string)
        commands.append('-o')
        commands.append(self.getOutputValue(las2txt.OUTPUT))

        LAStoolsUtils.runLAStools(commands, progress)
