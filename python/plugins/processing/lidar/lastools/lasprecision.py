# -*- coding: utf-8 -*-

"""
***************************************************************************
    lasprecision.py
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

from processing.outputs.OutputFile import OutputFile


class lasprecision(LAStoolsAlgorithm):

    OUTPUT = 'OUTPUT'

    def defineCharacteristics(self):
        self.name = 'lasprecision'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addOutput(OutputFile(lasprecision.OUTPUT, 'Output ASCII file'))

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'lasprecision.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        commands.append('-o')
        commands.append(self.getOutputValue(lasprecision.OUTPUT))

        LAStoolsUtils.runLAStools(commands, progress)
