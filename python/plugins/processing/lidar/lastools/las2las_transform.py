# -*- coding: utf-8 -*-

"""
***************************************************************************
    las2las_transform.py
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
from processing.parameters.ParameterSelection import ParameterSelection


class las2las_transform(LAStoolsAlgorithm):

    STEP = 'STEP'
    OPERATION = 'OPERATION'
    OPERATIONS = [
        '---',
        'set_point_type',
        'set_point_size',
        'set_version_minor',
        'set_version_major',
        'start_at_point',
        'stop_at_point',
        'remove_vlr',
        'auto_reoffset',
        'week_to_adjusted',
        'adjusted_to_week',
        'scale_rgb_up',
        'scale_rgb_down',
        'remove_all_vlrs',
        'remove_extra',
        'clip_to_bounding_box',
        ]
    OPERATIONARG = 'OPERATIONARG'

    def defineCharacteristics(self):
        self.name = 'las2las_transform'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParametersTransform1CoordinateGUI()
        self.addParametersTransform2CoordinateGUI()
        self.addParametersTransform1OtherGUI()
        self.addParametersTransform2OtherGUI()
        self.addParameter(ParameterSelection(las2las_transform.OPERATION,
                          'operations (first 7 need an argument)',
                          las2las_transform.OPERATIONS, 0))
        self.addParameter(ParameterString(las2las_transform.OPERATIONARG,
                          'argument for operation'))
        self.addParametersPointOutputGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'las2las.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        self.addParametersTransform1CoordinateCommands(commands)
        self.addParametersTransform2CoordinateCommands(commands)
        self.addParametersTransform1OtherCommands(commands)
        self.addParametersTransform2OtherCommands(commands)
        operation = self.getParameterValue(las2las_transform.OPERATION)
        if operation != 0:
            commands.append('-' + las2las_transform.OPERATIONS[operation])
            if operation > 7:
                commands.append(self.getParameterValue(
                        las2las_transform.OPERATIONARG))

        self.addParametersPointOutputCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
