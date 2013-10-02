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
from processing.parameters.ParameterNumber import ParameterNumber
from processing.parameters.ParameterString import ParameterString
from processing.parameters.ParameterFile import ParameterFile


class shp2las(LAStoolsAlgorithm):

    INPUT = 'INPUT'
    SCALE_FACTOR_XY = 'SCALE_FACTOR_XY'
    SCALE_FACTOR_Z = 'SCALE_FACTOR_Z'

    def defineCharacteristics(self):
        self.name = 'shp2las'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParameter(ParameterFile(shp2las.INPUT, 'Input SHP file'))
        self.addParameter(ParameterNumber(shp2las.SCALE_FACTOR_XY,
                          'resolution of x and y coordinate', False, False,
                          0.01))
        self.addParameter(ParameterNumber(shp2las.SCALE_FACTOR_Z,
                          'resolution of z coordinate', False, False, 0.01))
        self.addParametersPointOutputGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'shp2las.exe')]
        self.addParametersVerboseCommands(commands)
        commands.append('-i')
        commands.append(self.getParameterValue(shp2las.INPUT))
        scale_factor_xy = self.getParameterValue(shp2las.SCALE_FACTOR_XY)
        scale_factor_z = self.getParameterValue(shp2las.SCALE_FACTOR_Z)
        if scale_factor_xy != 0.01 or scale_factor_z != 0.01:
            commands.append('-set_scale_factor')
            commands.append(str(scale_factor_xy) + ' ' + str(scale_factor_xy)
                            + ' ' + str(scale_factor_z))
        self.addParametersPointOutputCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
