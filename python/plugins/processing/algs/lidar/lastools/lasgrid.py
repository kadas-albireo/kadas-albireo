# -*- coding: utf-8 -*-

"""
***************************************************************************
    lasgrid.py
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
from LAStoolsUtils import LAStoolsUtils
from LAStoolsAlgorithm import LAStoolsAlgorithm

from processing.core.parameters import ParameterSelection
from processing.core.parameters import ParameterBoolean

class lasgrid(LAStoolsAlgorithm):

    ATTRIBUTE = "ATTRIBUTE"
    METHOD = "METHOD"
    ATTRIBUTES = ["elevation", "intensity", "rgb", "classification"]
    METHODS = ["lowest", "highest", "average", "stddev"]
    USE_TILE_BB = "USE_TILE_BB"

    def defineCharacteristics(self):
        self.name = "lasgrid"
        self.group = "LAStools"
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParametersFilter1ReturnClassFlagsGUI()
        self.addParametersStepGUI()
        self.addParameter(ParameterSelection(lasgrid.ATTRIBUTE,
            self.tr("Attribute"), lasgrid.ATTRIBUTES, 0))
        self.addParameter(ParameterSelection(lasgrid.METHOD,
            self.tr("Method"), lasgrid.METHODS, 0))
        self.addParameter(ParameterBoolean(lasgrid.USE_TILE_BB,
            self.tr("use tile bounding box (after tiling with buffer)"), False))
        self.addParametersRasterOutputGUI()
        self.addParametersAdditionalGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), "bin", "lasgrid")]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        self.addParametersFilter1ReturnClassFlagsCommands(commands)
        self.addParametersStepCommands(commands)
        attribute = self.getParameterValue(lasgrid.ATTRIBUTE)
        if attribute != 0:
            commands.append("-" + lasgrid.ATTRIBUTES[attribute])
        method = self.getParameterValue(lasgrid.METHOD)
        if method != 0:
            commands.append("-" + lasgrid.METHODS[method])
        if (self.getParameterValue(lasgrid.USE_TILE_BB)):
            commands.append("-use_tile_bb")
        self.addParametersRasterOutputCommands(commands)
        self.addParametersAdditionalCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
