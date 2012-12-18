# -*- coding: utf-8 -*-

"""
***************************************************************************
    SplitRGBBands.py
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
from sextante.core.SextanteUtils import SextanteUtils
from sextante.core.QGisLayers import QGisLayers
from sextante.saga.SagaUtils import SagaUtils

__author__ = 'Victor Olaya'
__date__ = 'August 2012'
__copyright__ = '(C) 2012, Victor Olaya'
# This will get replaced with a git SHA1 when you do a git archive
__revision__ = '$Format:%H$'

from PyQt4 import QtGui
from sextante.core.GeoAlgorithm import GeoAlgorithm
from sextante.parameters.ParameterRaster import ParameterRaster
from sextante.outputs.OutputRaster import OutputRaster
import os

class SplitRGBBands(GeoAlgorithm):

    INPUT = "INPUT"
    R = "R"
    G = "G"
    B = "B"

    def getIcon(self):
        return  QtGui.QIcon(os.path.dirname(__file__) + "/../images/saga.png")

    def defineCharacteristics(self):
        self.name = "Split RGB bands"
        self.group = "Grid - Tools"
        self.addParameter(ParameterRaster(SplitRGBBands.INPUT, "Input layer", False))
        self.addOutput(OutputRaster(SplitRGBBands.R, "Output R band layer"))
        self.addOutput(OutputRaster(SplitRGBBands.G, "Output G band layer"))
        self.addOutput(OutputRaster(SplitRGBBands.B, "Output B band layer"))

    def processAlgorithm(self, progress):
        #TODO:check correct num of bands
        input = self.getParameterValue(SplitRGBBands.INPUT)
        temp = SextanteUtils.getTempFilename();
        r = self.getOutputValue(SplitRGBBands.R)
        g = self.getOutputValue(SplitRGBBands.G)
        b = self.getOutputValue(SplitRGBBands.B)
        commands = []
        if SextanteUtils.isWindows():
            commands.append("io_gdal 0 -GRIDS \"" + temp + "\" -FILES \"" + input+"\"")
            commands.append("io_gdal 1 -GRIDS \"" + temp + "_0001.sgrd\" -FORMAT 1 -TYPE 0 -FILE \"" + r + "\"");
            commands.append("io_gdal 1 -GRIDS \"" + temp + "_0002.sgrd\" -FORMAT 1 -TYPE 0 -FILE \"" + g + "\"");
            commands.append("io_gdal 1 -GRIDS \"" + temp + "_0003.sgrd\" -FORMAT 1 -TYPE 0 -FILE \"" + b + "\"");
        else:
            commands.append("libio_gdal 0 -GRIDS \"" + temp + "\" -FILES \"" + input + "\"")
            commands.append("libio_gdal 1 -GRIDS \"" + temp + "_0001.sgrd\" -FORMAT 1 -TYPE 0 -FILE \"" + r + "\"");
            commands.append("libio_gdal 1 -GRIDS \"" + temp + "_0002.sgrd\" -FORMAT 1 -TYPE 0 -FILE \"" + g + "\"");
            commands.append("libio_gdal 1 -GRIDS \"" + temp + "_0003.sgrd\" -FORMAT 1 -TYPE 0 -FILE \"" + b + "\"");

        SagaUtils.createSagaBatchJobFileFromSagaCommands(commands)
        SagaUtils.executeSaga(progress);
