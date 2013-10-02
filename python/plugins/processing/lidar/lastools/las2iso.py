# -*- coding: utf-8 -*-

"""
***************************************************************************
    las2iso.py
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

from processing.parameters.ParameterNumber import ParameterNumber


class las2iso(LAStoolsAlgorithm):

    SMOOTH = 'SMOOTH'
    ISO_EVERY = 'ISO_EVERY'
    SIMPLIFY_LENGTH = 'SIMPLIFY_LENGTH'
    SIMPLIFY_AREA = 'SIMPLIFY_AREA'
    CLEAN = 'CLEAN'

    def defineCharacteristics(self):
        self.name = 'las2iso'
        self.group = 'LAStools'
        self.addParametersVerboseGUI()
        self.addParametersPointInputGUI()
        self.addParameter(ParameterNumber(las2iso.SMOOTH,
                          'smooth underlying TIN', 0, None, 0))
        self.addParameter(ParameterNumber(las2iso.ISO_EVERY,
                          'extract isoline with a spacing of', 0, None, 10.0))
        self.addParameter(ParameterNumber(las2iso.CLEAN,
                          'clean isolines shorter than (0 = do not clean)',
                          None, None, 0.0))
        self.addParameter(ParameterNumber(las2iso.SIMPLIFY_LENGTH,
            'simplify segments shorter than (0 = do not simplify)',
            None, None, 0.0))
        self.addParameter(ParameterNumber(las2iso.SIMPLIFY_AREA,
            "simplify segments pairs with area less than (0 = don't simplify)",
            None, None, 0.0))
        self.addParametersVectorOutputGUI()

    def processAlgorithm(self, progress):
        commands = [os.path.join(LAStoolsUtils.LAStoolsPath(), 'bin',
                    'las2iso.exe')]
        self.addParametersVerboseCommands(commands)
        self.addParametersPointInputCommands(commands)
        smooth = self.getParameterValue(las2iso.SMOOTH)
        if smooth != 0:
            commands.append('-smooth')
            commands.append(str(smooth))
        commands.append('-iso_every')
        commands.append(str(self.getParameterValue(las2iso.ISO_EVERY)))
        simplify_length = self.getParameterValue(las2iso.SIMPLIFY_LENGTH)
        if simplify_length != 0:
            commands.append('-simplify_length')
            commands.append(str(simplify_length))
        simplify_area = self.getParameterValue(las2iso.SIMPLIFY_AREA)
        if simplify_area != 0:
            commands.append('-simplify_area')
            commands.append(str(simplify_area))
        clean = self.getParameterValue(las2iso.CLEAN)
        if clean != 0:
            commands.append('-clean')
            commands.append(str(clean))
        self.addParametersVectorOutputCommands(commands)

        LAStoolsUtils.runLAStools(commands, progress)
