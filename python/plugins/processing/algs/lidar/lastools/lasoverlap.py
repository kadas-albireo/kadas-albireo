# -*- coding: utf-8 -*-

"""
***************************************************************************
    lasoverlap.py
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
from LAStoolsUtils import LAStoolsUtils
from LAStoolsAlgorithm import LAStoolsAlgorithm

from processing.core.parameters import ParameterBoolean
from processing.core.parameters import ParameterNumber
from processing.core.parameters import ParameterSelection

class lasoverlap(LAStoolsAlgorithm):

    CHECK_STEP = "CHECK_STEP"
    ATTRIBUTE = "ATTRIBUTE"
    OPERATION = "OPERATION"
    ATTRIBUTES = ["elevation", "intensity", "number_of_returns", "scan_angle_abs", "density"]
    OPERATIONS = ["lowest", "highest", "average"]
    CREATE_OVERLAP_RASTER = "CREATE_OVERLAP_RASTER"
    CREATE_DIFFERENCE_RASTER = "CREATE_DIFFERENCE_RASTER"

    def defineCharacteristics(self):
        self.name = "lasoverlap"
        self.group = "LAStools"
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParametersFilter1ReturnClassFlagsGUI()
        self.addParameter(ParameterNumber(lasoverlap.CHECK_STEP,
            self.tr("size of grid used for overlap check"), 0, None, 2.0))
        self.addParameter(ParameterSelection(lasoverlap.ATTRIBUTE,
            self.tr("attribute to check"), lasoverlap.ATTRIBUTES, 0))
        self.addParameter(ParameterSelection(lasoverlap.OPERATION,
            self.tr("operation on attribute per cell"), lasoverlap.OPERATIONS, 0))
        self.addParameter(ParameterBoolean(lasoverlap.CREATE_OVERLAP_RASTER,
            self.tr("create overlap raster"), True))
        self.addParameter(ParameterBoolean(lasoverlap.CREATE_DIFFERENCE_RASTER,
            self.tr("create difference raster"), True))
        self.addParametersRasterOutputGUI()
        self.addParametersAdditionalGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), "bin", "lasoverlap")]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        self.addParametersFilter1ReturnClassFlagsCommands(commands)
        step = self.getParameterValue(lasoverlap.CHECK_STEP)
        if step != 0.0:
            commands.append("-step")
            commands.append(str(step))
        commands.append("-values")
        attribute = self.getParameterValue(lasoverlap.ATTRIBUTE)
        if attribute != 0:
            commands.append("-" + lasoverlap.ATTRIBUTES[attribute])
        operation = self.getParameterValue(lasoverlap.OPERATION)
        if operation != 0:
            commands.append("-" + lasoverlap.OPERATIONS[operation])
        if not self.getParameterValue(lasoverlap.CREATE_OVERLAP_RASTER):
            commands.append("-no_over")
        if not self.getParameterValue(lasoverlap.CREATE_DIFFERENCE_RASTER):
            commands.append("-no_diff")
        self.addParametersRasterOutputCommands(commands)
        self.addParametersAdditionalCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
