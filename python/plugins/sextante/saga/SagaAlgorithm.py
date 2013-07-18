# -*- coding: utf-8 -*-

"""
***************************************************************************
    SagaAlgorithm.py
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
from qgis.core import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from sextante.core.GeoAlgorithm import GeoAlgorithm
from sextante.parameters.ParameterTable import ParameterTable
from sextante.outputs.OutputTable import OutputTable
from sextante.parameters.ParameterMultipleInput import ParameterMultipleInput
from sextante.parameters.ParameterRaster import ParameterRaster
from sextante.outputs.OutputRaster import OutputRaster
from sextante.parameters.ParameterVector import ParameterVector
from sextante.parameters.ParameterBoolean import ParameterBoolean
from sextante.core.SextanteUtils import SextanteUtils
from sextante.outputs.OutputVector import OutputVector
from sextante.saga.SagaUtils import SagaUtils
from sextante.saga.SagaGroupNameDecorator import SagaGroupNameDecorator
from sextante.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from sextante.parameters.ParameterFactory import ParameterFactory
from sextante.outputs.OutputFactory import OutputFactory
from sextante.core.SextanteConfig import SextanteConfig
from sextante.core.QGisLayers import QGisLayers
from sextante.parameters.ParameterNumber import ParameterNumber
from sextante.parameters.ParameterSelection import ParameterSelection
from sextante.core.LayerExporter import LayerExporter
from sextante.parameters.ParameterExtent import ParameterExtent
from sextante.parameters.ParameterFixedTable import ParameterFixedTable
from sextante.core.SextanteLog import SextanteLog

class SagaAlgorithm(GeoAlgorithm):

    OUTPUT_EXTENT = "OUTPUT_EXTENT"

    def __init__(self, descriptionfile):
        self.resample = True #True if it should resample
                             #in case several non-matching raster layers are used as input
        GeoAlgorithm.__init__(self)
        self.descriptionFile = descriptionfile
        self.defineCharacteristicsFromFile()
        if self.resample:
            #reconsider resampling policy now that we know the input parameters
            self.resample = self.setResamplingPolicy()

    def getCopy(self):
        newone = SagaAlgorithm(self.descriptionFile)
        newone.provider = self.provider
        return newone


    def setResamplingPolicy(self):
        count = 0
        for param in self.parameters:
            if isinstance(param, ParameterRaster):
                count += 1
            if isinstance(param, ParameterMultipleInput):
                if param.datatype == ParameterMultipleInput.TYPE_RASTER:
                    return True

        return count > 1

    def getIcon(self):
        return  QIcon(os.path.dirname(__file__) + "/../images/saga.png")

    def defineCharacteristicsFromFile(self):
        self.hardcodedStrings = []
        lines = open(self.descriptionFile)
        line = lines.readline().strip("\n").strip()
        self.name = line
        if "|" in self.name:
            tokens = self.name.split("|")
            self.name = tokens[0]
            self.cmdname = tokens[1]
        else:
            self.cmdname = self.name
            self.name = self.name[0].upper() + self.name[1:].lower()
        line = lines.readline().strip("\n").strip()
        self.undecoratedGroup = line
        self.group = SagaGroupNameDecorator.getDecoratedName(self.undecoratedGroup)
        while line != "":
            line = line.strip("\n").strip()
            if line.startswith("Hardcoded"):
                self.hardcodedStrings.append(line[len("Harcoded|")+1:])
            elif line.startswith("Parameter"):
                self.addParameter(ParameterFactory.getFromString(line))
            elif line.startswith("DontResample"):
                self.resample = False
            elif line.startswith("Extent"): #An extent parameter that wraps 4 SAGA numerical parameters
                self.extentParamNames = line[6:].strip().split(" ")
                self.addParameter(ParameterExtent(self.OUTPUT_EXTENT, "Output extent", "0,1,0,1"))
            else:
                self.addOutput(OutputFactory.getFromString(line))
            line = lines.readline().strip("\n").strip()
        lines.close()


    def calculateResamplingExtent(self):
        '''this method calculates the resampling extent, but it might set self.resample
        to false if, with the current layers, there is no need to resample'''
        auto = SextanteConfig.getSetting(SagaUtils.SAGA_AUTO_RESAMPLING)
        if auto:
            first = True;
            self.inputExtentsCount = 0
            for param in self.parameters:
                if param.value:
                    if isinstance(param, ParameterRaster):
                        if isinstance(param.value, QgsRasterLayer):
                            layer = param.value
                        else:
                            layer = QGisLayers.getObjectFromUri(param.value)
                        self.addToResamplingExtent(layer, first)
                        first = False
                    if isinstance(param, ParameterMultipleInput):
                        if param.datatype == ParameterMultipleInput.TYPE_RASTER:
                            layers = param.value.split(";")
                            for layername in layers:
                                layer = QGisLayers.getObjectFromUri(layername)
                                self.addToResamplingExtent(layer, first)
                                first = False
            if self.inputExtentsCount < 2:
                self.resample = False
        else:
            self.xmin = SextanteConfig.getSetting(SagaUtils.SAGA_RESAMPLING_REGION_XMIN)
            self.xmax = SextanteConfig.getSetting(SagaUtils.SAGA_RESAMPLING_REGION_XMAX)
            self.ymin = SextanteConfig.getSetting(SagaUtils.SAGA_RESAMPLING_REGION_YMIN)
            self.ymax = SextanteConfig.getSetting(SagaUtils.SAGA_RESAMPLING_REGION_YMAX)
            self.cellsize = SextanteConfig.getSetting(SagaUtils.SAGA_RESAMPLING_REGION_CELLSIZE)


    def addToResamplingExtent(self, layer, first):
        if first:
            self.inputExtentsCount = 1
            self.xmin = layer.extent().xMinimum()
            self.xmax = layer.extent().xMaximum()
            self.ymin = layer.extent().yMinimum()
            self.ymax = layer.extent().yMaximum()
            self.cellsize = (layer.extent().xMaximum() - layer.extent().xMinimum())/layer.width()
        else:
            cellsize = (layer.extent().xMaximum() - layer.extent().xMinimum())/layer.width()
            if self.xmin != layer.extent().xMinimum() \
                    or self.xmax != layer.extent().xMaximum() \
                    or self.ymin != layer.extent().yMinimum() \
                    or self.ymax != layer.extent().yMaximum() \
                    or self.cellsize != cellsize:
                self.xmin = min(self.xmin, layer.extent().xMinimum())
                self.xmax = max(self.xmax, layer.extent().xMaximum())
                self.ymin = min(self.ymin, layer.extent().yMinimum())
                self.ymax = max(self.ymax, layer.extent().yMaximum())
                self.cellsize = max(self.cellsize, cellsize)
                self.inputExtentsCount += 1


    def processAlgorithm(self, progress):
        if SextanteUtils.isWindows():
            path = SagaUtils.sagaPath()
            if path == "":
                raise GeoAlgorithmExecutionException("SAGA folder is not configured.\nPlease configure it before running SAGA algorithms.")
        commands = list()
        self.exportedLayers = {}

        #1: Export rasters to sgrd and vectors to shp
        #   Tables must be in dbf format. We check that.
        if self.resample:
            self.calculateResamplingExtent()
        for param in self.parameters:
            if isinstance(param, ParameterRaster):
                if param.value == None:
                    continue
                value = param.value
                if not value.endswith("sgrd"):
                    commands.append(self.exportRasterLayer(value))
                if self.resample:
                    commands.append(self.resampleRasterLayer(value));
            if isinstance(param, ParameterVector):
                if param.value == None:
                    continue
                layer = QGisLayers.getObjectFromUri(param.value, False)
                if layer:
                    filename = LayerExporter.exportVectorLayer(layer)
                    self.exportedLayers[param.value]=filename
                elif not param.value.endswith("shp"):
                        raise GeoAlgorithmExecutionException("Unsupported file format")
            if isinstance(param, ParameterTable):
                if param.value == None:
                    continue
                table = QGisLayers.getObjectFromUri(param.value, False)
                if table:
                    filename = LayerExporter.exportTable(table)
                    self.exportedLayers[param.value]=filename
                elif not param.value.endswith("shp"):
                        raise GeoAlgorithmExecutionException("Unsupported file format")
            if isinstance(param, ParameterMultipleInput):
                if param.value == None:
                    continue
                layers = param.value.split(";")
                if layers == None or len(layers) == 0:
                    continue
                if param.datatype == ParameterMultipleInput.TYPE_RASTER:
                    for layerfile in layers:
                        if not layerfile.endswith("sgrd"):
                            commands.append(self.exportRasterLayer(layerfile))
                        if self.resample:
                            commands.append(self.resampleRasterLayer(layerfile));
                elif param.datatype == ParameterMultipleInput.TYPE_VECTOR_ANY:
                    for layerfile in layers:
                        layer = QGisLayers.getObjectFromUri(layerfile, False)
                        if layer:
                            filename = LayerExporter.exportVectorLayer(layer)
                            self.exportedLayers[layerfile]=filename
                        elif (not layerfile.endswith("shp")):
                            raise GeoAlgorithmExecutionException("Unsupported file format")

        #2: set parameters and outputs
        if SextanteUtils.isWindows() or SextanteUtils.isMac():
            command = self.undecoratedGroup  + " \"" + self.cmdname + "\""
        else:
            command = "lib" + self.undecoratedGroup  + " \"" + self.cmdname + "\""

        if self.hardcodedStrings:
            for s in self.hardcodedStrings:
                command += " " + s

        for param in self.parameters:
            if param.value is None:
                continue
            if isinstance(param, (ParameterRaster, ParameterVector, ParameterTable)):
                value = param.value
                if value in self.exportedLayers.keys():
                    command += (" -" + param.name + " \"" + self.exportedLayers[value] + "\"")
                else:
                    command += (" -" + param.name + " \"" + value + "\"")
            elif isinstance(param, ParameterMultipleInput):
                s = param.value
                for layer in self.exportedLayers.keys():
                    s = s.replace(layer, self.exportedLayers[layer])
                command += (" -" + param.name + " \"" + s + "\"");
            elif isinstance(param, ParameterBoolean):
                if param.value:
                    command+=(" -" + param.name);
            elif isinstance(param, ParameterFixedTable):
                tempTableFile  = SextanteUtils.getTempFilename("txt")
                f = open(tempTableFile, "w")
                f.write('\t'.join([col for col in param.cols]) + "\n")
                values = param.value.split(",")
                for i in range(0, len(values), 3):
                    s = values[i] + "\t" + values[i+1] + "\t" + values[i+2] + "\n"
                    f.write(s)
                f.close()
                command+=( " -" + param.name + " \"" + tempTableFile + "\"")
            elif isinstance(param, ParameterExtent):
                #'we have to substract/add half cell size, since saga is center based, not corner based
                halfcell = self.getOutputCellsize() / 2
                offset = [halfcell, -halfcell, halfcell, -halfcell]
                values = param.value.split(",")
                for i in range(4):
                    command+=(" -" + self.extentParamNames[i] + " " + str(float(values[i]) + offset[i]));
            elif isinstance(param, (ParameterNumber, ParameterSelection)):
                command+=(" -" + param.name + " " + str(param.value));
            else:
                command+=(" -" + param.name + " \"" + str(param.value) + "\"");

        for out in self.outputs:
            if isinstance(out, OutputRaster):
                filename = out.getCompatibleFileName(self)
                filename = SextanteUtils.tempFolder() + os.sep + os.path.basename(filename) + ".sgrd"
                command+=(" -" + out.name + " \"" + filename + "\"");
            if isinstance(out, OutputVector):
                filename = out.getCompatibleFileName(self)
                command+=(" -" + out.name + " \"" + filename + "\"");
            if isinstance(out, OutputTable):
                filename = out.getCompatibleFileName(self)
                command+=(" -" + out.name + " \"" + filename + "\"");

        commands.append(command)

        #3:Export resulting raster layers
        for out in self.outputs:
            if isinstance(out, OutputRaster):
                filename = out.getCompatibleFileName(self)
                filename2 = SextanteUtils.tempFolder() + os.sep + os.path.basename(filename) + ".sgrd"
                if SextanteUtils.isWindows() or SextanteUtils.isMac():
                    commands.append("io_gdal 1 -GRIDS \"" + filename2 + "\" -FORMAT 4 -TYPE 0 -FILE \"" + filename + "\"");
                else:
                    commands.append("libio_gdal 1 -GRIDS \"" + filename2 + "\" -FORMAT 1 -TYPE 0 -FILE \"" + filename + "\"");


        #4 Run SAGA
        SagaUtils.createSagaBatchJobFileFromSagaCommands(commands)
        loglines = []
        loglines.append("SAGA execution commands")
        for line in commands:
            progress.setCommand(line)
            loglines.append(line)
        if SextanteConfig.getSetting(SagaUtils.SAGA_LOG_COMMANDS):
            SextanteLog.addToLog(SextanteLog.LOG_INFO, loglines)
        SagaUtils.executeSaga(progress);


    def getOutputCellsize(self):
        '''tries to guess the cellsize of the output, searching for a parameter with an appropriate name for it'''
        cellsize = 0;
        for param in self.parameters:
            if param.value is not None and param.name == "USER_SIZE":
                cellsize = float(param.value)
                break;
        return cellsize


    def resampleRasterLayer(self,layer):
        '''this is supposed to be run after having exported all raster layers'''
        if layer in self.exportedLayers.keys():
            inputFilename = self.exportedLayers[layer]
        else:
            inputFilename = layer
        destFilename = SextanteUtils.getTempFilename("sgrd")
        self.exportedLayers[layer]= destFilename
        if SextanteUtils.isWindows() or SextanteUtils.isMac():
            s = "grid_tools \"Resampling\" -INPUT \"" + inputFilename + "\" -TARGET 0 -SCALE_UP_METHOD 4 -SCALE_DOWN_METHOD 4 -USER_XMIN " +\
                str(self.xmin) + " -USER_XMAX " + str(self.xmax) + " -USER_YMIN " + str(self.ymin) + " -USER_YMAX "  + str(self.ymax) +\
                " -USER_SIZE " + str(self.cellsize) + " -USER_GRID \"" + destFilename + "\""
        else:
            s = "libgrid_tools \"Resampling\" -INPUT \"" + inputFilename + "\" -TARGET 0 -SCALE_UP_METHOD 4 -SCALE_DOWN_METHOD 4 -USER_XMIN " +\
                str(self.xmin) + " -USER_XMAX " + str(self.xmax) + " -USER_YMIN " + str(self.ymin) + " -USER_YMAX "  + str(self.ymax) +\
                " -USER_SIZE " + str(self.cellsize) + " -USER_GRID \"" + destFilename + "\""
        return s


    def exportRasterLayer(self, layer):
        destFilename = SextanteUtils.getTempFilenameInTempFolder(os.path.basename(layer)[0:5] + ".sgrd")
        self.exportedLayers[layer]= destFilename
        if SextanteUtils.isWindows() or SextanteUtils.isMac():
            return "io_gdal 0 -GRIDS \"" + destFilename + "\" -FILES \"" + layer+"\""
        else:
            return "libio_gdal 0 -GRIDS \"" + destFilename + "\" -FILES \"" + layer + "\""


    def checkBeforeOpeningParametersDialog(self):
        msg = SagaUtils.checkSagaIsInstalled()
        if msg is not None:
            html = ("<p>This algorithm requires SAGA to be run."
            "Unfortunately, it seems that SAGA is not installed in your system, or it is not correctly configured to be used from QGIS</p>")
            html += '<p><a href= "http://docs.qgis.org/2.0/html/en/docs/user_manual/sextante/3rdParty.html">Click here</a> to know more about how to install and configure SAGA to be used with SEXTANTE</p>'
            return html


    def checkParameterValuesBeforeExecuting(self):
        '''We check that there are no multiband layers, which are not supported by SAGA'''
        for param in self.parameters:
            if isinstance(param, ParameterRaster):
                value = param.value
                layer = QGisLayers.getObjectFromUri(value)
                if layer is not None and layer.bandCount() > 1:
                        return ("Input layer " + str(layer.name()) + " has more than one band.\n"
                                + "Multiband layers are not supported by SAGA")    

    def helpFile(self):
        return  os.path.join(os.path.dirname(__file__), "help", self.name.replace(" ", "") + ".html")

    def getPostProcessingErrorMessage(self, wrongLayers):
        html = GeoAlgorithm.getPostProcessingErrorMessage(self, wrongLayers)
        msg = SagaUtils.checkSagaIsInstalled(True)
        html += ("<p>This algorithm requires SAGA to be run. A test to check if SAGA is correctly installed "
                "and configured in your system has been performed, with the following result:</p><ul><i>")
        if msg is None:
            html += "SAGA seems to be correctly installed and configured</li></ul>"
        else:
            html += msg + "</i></li></ul>"
            html += '<p><a href= "http://docs.qgis.org/2.0/html/en/docs/user_manual/sextante/3rdParty.html">Click here</a> to know more about how to install and configure SAGA to be used with SEXTANTE</p>'

        return html
    #===========================================================================
    # def commandLineName(self):
    #    name = self.provider.getName().lower() + ":" + self.cmdname.lower()
    #    validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:"
    #    name = ''.join(c for c in name if c in validChars)
    #    return name
    #===========================================================================
