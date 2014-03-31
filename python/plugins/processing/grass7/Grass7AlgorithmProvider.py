# -*- coding: utf-8 -*-

"""
***************************************************************************
    Grass7AlgorithmProvider.py
    ---------------------
    Date                 : April 2014
    Copyright            : (C) 2014 by Victor Olaya
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

from processing.grass7.nviz7 import nviz7

__author__ = 'Victor Olaya'
__date__ = 'April 2014'
__copyright__ = '(C) 2014, Victor Olaya'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from processing.core.ProcessingConfig import ProcessingConfig, Setting
from processing.core.AlgorithmProvider import AlgorithmProvider
from processing.core.ProcessingLog import ProcessingLog
from processing.grass7.Grass7Utils import Grass7Utils
from processing.grass7.Grass7Algorithm import Grass7Algorithm
from processing.tools.system import *


class Grass7AlgorithmProvider(AlgorithmProvider):

    def __init__(self):
        AlgorithmProvider.__init__(self)
        self.createAlgsList()  # Preloading algorithms to speed up

    def initializeSettings(self):
        AlgorithmProvider.initializeSettings(self)
        if isWindows() or isMac():
            ProcessingConfig.addSetting(Setting(self.getDescription(),
                    Grass7Utils.GRASS_FOLDER, 'GRASS7 folder',
                    Grass7Utils.grassPath()))
            ProcessingConfig.addSetting(Setting(self.getDescription(),
                    Grass7Utils.GRASS_WIN_SHELL, 'Msys folder',
                    Grass7Utils.grassWinShell()))
        ProcessingConfig.addSetting(Setting(self.getDescription(),
                                    Grass7Utils.GRASS_LOG_COMMANDS,
                                    'Log execution commands', False))
        ProcessingConfig.addSetting(Setting(self.getDescription(),
                                    Grass7Utils.GRASS_LOG_CONSOLE,
                                    'Log console output', False))

    def unload(self):
        AlgorithmProvider.unload(self)
        if isWindows() or isMac():
            ProcessingConfig.removeSetting(Grass7Utils.GRASS_FOLDER)
            ProcessingConfig.removeSetting(Grass7Utils.GRASS_WIN_SHELL)
        ProcessingConfig.removeSetting(Grass7Utils.GRASS_LOG_COMMANDS)
        ProcessingConfig.removeSetting(Grass7Utils.GRASS_LOG_CONSOLE)

    def createAlgsList(self):
        self.preloadedAlgs = []
        folder = Grass7Utils.grassDescriptionPath()
        for descriptionFile in os.listdir(folder):
            if descriptionFile.endswith('txt'):
                try:
                    alg = Grass7Algorithm(os.path.join(folder, descriptionFile))
                    if alg.name.strip() != '':
                        self.preloadedAlgs.append(alg)
                    else:
                        ProcessingLog.addToLog(ProcessingLog.LOG_ERROR,
                                'Could not open GRASS GIS 7 algorithm: '
                                + descriptionFile)
                except Exception, e:
                    ProcessingLog.addToLog(ProcessingLog.LOG_ERROR,
                            'Could not open GRASS GIS 7 algorithm: '
                            + descriptionFile)
        self.preloadedAlgs.append(nviz7())

    def _loadAlgorithms(self):
        self.algs = self.preloadedAlgs

    def getDescription(self):
        return 'GRASS GIS 7 commands'

    def getName(self):
        return 'grass70'

    def getIcon(self):
        return QIcon(os.path.dirname(__file__) + '/../images/grass.png')

    def getSupportedOutputVectorLayerExtensions(self):
        return ['shp']

    def getSupportedOutputRasterLayerExtensions(self):
        return ['tif']

