# -*- coding: utf-8 -*-

"""
***************************************************************************
    lasground.py
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

from processing.core.parameters import ParameterBoolean
from processing.core.parameters import ParameterSelection

class lasground(LAStoolsAlgorithm):

    NO_BULGE = "NO_BULGE"
    TERRAIN = "TERRAIN"
    TERRAINS = ["wilderness", "nature", "town", "city", "metro"]
    GRANULARITY = "GRANULARITY"
    GRANULARITIES = ["coarse", "default", "fine", "extra_fine", "ultra_fine"]

    def defineCharacteristics(self):
        self.name = "lasground"
        self.group = "LAStools"
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParametersHorizontalAndVerticalFeetGUI()
        self.addParameter(ParameterBoolean(lasground.NO_BULGE,
            self.tr("no triangle bulging during TIN refinement"), False))
        self.addParameter(ParameterSelection(lasground.TERRAIN,
            self.tr("terrain type"), lasground.TERRAINS, 1))
        self.addParameter(ParameterSelection(lasground.GRANULARITY,
            self.tr("preprocessing"), lasground.GRANULARITIES, 1))
        self.addParametersPointOutputGUI()
        self.addParametersAdditionalGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), "bin", "lasground")]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        self.addParametersHorizontalAndVerticalFeetCommands(commands)
        if (self.getParameterValue(lasground.NO_BULGE)):
            commands.append("-no_bulge")
        method = self.getParameterValue(lasground.TERRAIN)
        if (method != 1):
            commands.append("-" + lasground.TERRAINS[method])
        granularity = self.getParameterValue(lasground.GRANULARITY)
        if (granularity != 1):
            commands.append("-" + lasground.GRANULARITIES[granularity])
        self.addParametersPointOutputCommands(commands)
        self.addParametersAdditionalCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
