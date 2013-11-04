# -*- coding: utf-8 -*-

"""
***************************************************************************
    information.py
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
from PyQt4 import QtGui
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.parameters.ParameterRaster import ParameterRaster
from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.outputs.OutputHTML import OutputHTML
from processing.gdal.GdalUtils import GdalUtils


class information(GeoAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    NOGCP = 'NOGCP'
    NOMETADATA = 'NOMETADATA'

    def getIcon(self):
        filepath = os.path.dirname(__file__) + '/icons/raster-info.png'
        return QtGui.QIcon(filepath)

    def commandLineName(self):
        return "gdalorg:rasterinfo"

    def defineCharacteristics(self):
        self.name = 'Information'
        self.group = '[GDAL] Miscellaneous'
        self.addParameter(ParameterRaster(information.INPUT, 'Input layer',
                          False))
        self.addParameter(ParameterBoolean(information.NOGCP,
                          'Suppress GCP info', False))
        self.addParameter(ParameterBoolean(information.NOMETADATA,
                          'Suppress metadata info', False))
        self.addOutput(OutputHTML(information.OUTPUT, 'Layer information'))

    def processAlgorithm(self, progress):
        arguments = []
        if self.getParameterValue(information.NOGCP):
            arguments.append('-nogcp')
        if self.getParameterValue(information.NOMETADATA):
            arguments.append('-nomd')
        arguments.append(self.getParameterValue(information.INPUT))
        GdalUtils.runGdal(['gdalinfo', GdalUtils.escapeAndJoin(arguments)],
                          progress)
        output = self.getOutputValue(information.OUTPUT)
        f = open(output, 'w')
        for s in GdalUtils.getConsoleOutput()[1:]:
            f.write('<p>' + str(s) + '</p>')
        f.close()
