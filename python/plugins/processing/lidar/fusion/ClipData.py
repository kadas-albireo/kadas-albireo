# -*- coding: utf-8 -*-

"""
***************************************************************************
    ClipData.py
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
import subprocess
from PyQt4 import QtGui
from processing.parameters.ParameterFile import ParameterFile
from processing.parameters.ParameterExtent import ParameterExtent
from processing.parameters.ParameterSelection import ParameterSelection
from processing.outputs.OutputFile import OutputFile
from processing.lidar.fusion.FusionAlgorithm import FusionAlgorithm
from processing.lidar.fusion.FusionUtils import FusionUtils


class ClipData(FusionAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    EXTENT = 'EXTENT'
    SHAPE = 'SHAPE'

    def defineCharacteristics(self):
        self.name = 'Clip Data'
        self.group = 'Points'
        self.addParameter(ParameterFile(self.INPUT, 'Input las layer'))
        self.addParameter(ParameterExtent(self.EXTENT, 'Extent'))
        self.addParameter(ParameterSelection(self.SHAPE, 'Shape',
                          ['Rectangle', 'Circle']))
        self.addOutput(OutputFile(self.OUTPUT, 'Output clipped las file'))
        self.addAdvancedModifiers()

    def processAlgorithm(self, progress):
        commands = [os.path.join(FusionUtils.FusionPath(), 'FilterData.exe')]
        commands.append('/verbose')
        self.addAdvancedModifiersToCommand(commands)
        commands.append('/shape:' + str(self.getParameterValue(self.SHAPE)))
        files = self.getParameterValue(self.INPUT).split(';')
        if len(files) == 1:
            commands.append(self.getParameterValue(self.INPUT))
        else:
            FusionUtils.createFileList(files)
            commands.append(FusionUtils.tempFileListFilepath())
        outFile = self.getOutputValue(self.OUTPUT) + '.lda'
        commands.append(outFile)
        extent = str(self.getParameterValue(self.EXTENT)).split(',')
        commands.append(extent[0])
        commands.append(extent[2])
        commands.append(extent[1])
        commands.append(extent[3])
        FusionUtils.runFusion(commands, progress)
        commands = [os.path.join(FusionUtils.FusionPath(), 'LDA2LAS.exe')]
        commands.append(outFile)
        commands.append(self.getOutputValue(self.OUTPUT))
        p = subprocess.Popen(commands, shell=True)
        p.wait()
