# -*- coding: utf-8 -*-

"""
***************************************************************************
    SagaAlgorithm213.py
    ---------------------
    Date                 : December 2014
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

__author__ = 'Victor Olaya'
__date__ = 'December 2014'
__copyright__ = '(C) 2014, Victor Olaya'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os

from SagaAlgorithm212 import SagaAlgorithm212
from processing.core.ProcessingConfig import ProcessingConfig
from processing.core.ProcessingLog import ProcessingLog
from processing.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from processing.core.parameters import ParameterRaster, ParameterVector, ParameterTable, ParameterMultipleInput, ParameterBoolean, ParameterFixedTable, ParameterExtent, ParameterNumber, ParameterSelection
from processing.core.outputs import OutputRaster, OutputVector, OutputTable
import SagaUtils
from processing.tools import dataobjects
from processing.tools.system import getTempFilenameInTempFolder, getTempFilename, isWindows

sessionExportedLayers = {}

class SagaAlgorithm213(SagaAlgorithm212):

    OUTPUT_EXTENT = 'OUTPUT_EXTENT'

    def getCopy(self):
        newone = SagaAlgorithm213(self.descriptionFile)
        newone.provider = self.provider
        return newone


    def processAlgorithm(self, progress):
        commands = list()
        self.exportedLayers = {}

        self.preProcessInputs()

        # 1: Export rasters to sgrd and vectors to shp
        # Tables must be in dbf format. We check that.
        for param in self.parameters:
            if isinstance(param, ParameterRaster):
                if param.value is None:
                    continue
                value = param.value
                if not value.endswith('sgrd'):
                    exportCommand = self.exportRasterLayer(value)
                    if exportCommand is not None:
                        commands.append(exportCommand)
            if isinstance(param, ParameterVector):
                if param.value is None:
                    continue
                layer = dataobjects.getObjectFromUri(param.value, False)
                if layer:
                    filename = dataobjects.exportVectorLayer(layer)
                    self.exportedLayers[param.value] = filename
                elif not param.value.endswith('shp'):
                    raise GeoAlgorithmExecutionException(
                        self.tr('Unsupported file format'))
            if isinstance(param, ParameterTable):
                if param.value is None:
                    continue
                table = dataobjects.getObjectFromUri(param.value, False)
                if table:
                    filename = dataobjects.exportTable(table)
                    self.exportedLayers[param.value] = filename
                elif not param.value.endswith('shp'):
                    raise GeoAlgorithmExecutionException(
                        self.tr('Unsupported file format'))
            if isinstance(param, ParameterMultipleInput):
                if param.value is None:
                    continue
                layers = param.value.split(';')
                if layers is None or len(layers) == 0:
                    continue
                if param.datatype == ParameterMultipleInput.TYPE_RASTER:
                    for layerfile in layers:
                        if not layerfile.endswith('sgrd'):
                            exportCommand = self.exportRasterLayer(layerfile)
                            if exportCommand is not None:
                                commands.append(exportCommand)
                elif param.datatype == ParameterMultipleInput.TYPE_VECTOR_ANY:
                    for layerfile in layers:
                        layer = dataobjects.getObjectFromUri(layerfile, False)
                        if layer:
                            filename = dataobjects.exportVectorLayer(layer)
                            self.exportedLayers[layerfile] = filename
                        elif not layerfile.endswith('shp'):
                            raise GeoAlgorithmExecutionException(
                                self.tr('Unsupported file format'))

        # 2: Set parameters and outputs
        command = self.undecoratedGroup + ' "' + self.cmdname + '"'
        if self.hardcodedStrings:
            for s in self.hardcodedStrings:
                command += ' ' + s

        for param in self.parameters:
            if param.value is None:
                continue
            if isinstance(param, (ParameterRaster, ParameterVector,
                          ParameterTable)):
                value = param.value
                if value in self.exportedLayers.keys():
                    command += ' -' + param.name + ' "' \
                        + self.exportedLayers[value] + '"'
                else:
                    command += ' -' + param.name + ' "' + value + '"'
            elif isinstance(param, ParameterMultipleInput):
                s = param.value
                for layer in self.exportedLayers.keys():
                    s = s.replace(layer, self.exportedLayers[layer])
                command += ' -' + param.name + ' "' + s + '"'
            elif isinstance(param, ParameterBoolean):
                if param.value:
                    command += ' -' + param.name.strip() + " true"
                else:
                    command += ' -' + param.name.strip() + " false"
            elif isinstance(param, ParameterFixedTable):
                tempTableFile = getTempFilename('txt')
                f = open(tempTableFile, 'w')
                f.write('\t'.join([col for col in param.cols]) + '\n')
                values = param.value.split(',')
                for i in range(0, len(values), 3):
                    s = values[i] + '\t' + values[i + 1] + '\t' + values[i
                            + 2] + '\n'
                    f.write(s)
                f.close()
                command += ' -' + param.name + ' "' + tempTableFile + '"'
            elif isinstance(param, ParameterExtent):
                # 'We have to substract/add half cell size, since SAGA is
                # center based, not corner based
                halfcell = self.getOutputCellsize() / 2
                offset = [halfcell, -halfcell, halfcell, -halfcell]
                values = param.value.split(',')
                for i in range(4):
                    command += ' -' + self.extentParamNames[i] + ' ' \
                        + str(float(values[i]) + offset[i])
            elif isinstance(param, (ParameterNumber, ParameterSelection)):
                command += ' -' + param.name + ' ' + str(param.value)
            else:
                command += ' -' + param.name + ' "' + str(param.value) + '"'

        for out in self.outputs:
            if isinstance(out, OutputRaster):
                filename = out.getCompatibleFileName(self)
                filename += '.sgrd'
                command += ' -' + out.name + ' "' + filename + '"'
            if isinstance(out, OutputVector):
                filename = out.getCompatibleFileName(self)
                command += ' -' + out.name + ' "' + filename + '"'
            if isinstance(out, OutputTable):
                filename = out.getCompatibleFileName(self)
                command += ' -' + out.name + ' "' + filename + '"'

        commands.append(command)

        # 3: Export resulting raster layers
        # optim = ProcessingConfig.getSetting( SagaUtils.SAGA_IMPORT_EXPORT_OPTIMIZATION)
        for out in self.outputs:
            if isinstance(out, OutputRaster):
                filename = out.getCompatibleFileName(self)
                filename2 = filename + '.sgrd'
                formatIndex = (4 if isWindows() else 1)
                sessionExportedLayers[filename] = filename2
                # Do not export is the output is not a final output
                # of the model
                # dontExport = True
                #if self.model is not None and optim:
                #    for subalg in self.model.algOutputs:
                #        if out.name in subalg:
                #            if subalg[out.name] is not None:
                #                dontExport = False
                #                break
                #    if dontExport:
                #        continue

                if self.cmdname == 'RGB Composite':
                    commands.append('io_grid_image 0 -IS_RGB -GRID:"' + filename2
                                    + '" -FILE:"' + filename
                                    + '"')
                else:
                    commands.append('io_gdal 1 -GRIDS "' + filename2
                                    + '" -FORMAT ' + str(formatIndex)
                                    + ' -TYPE 0 -FILE "' + filename + '"')


        # 4: Run SAGA
        commands = self.editCommands(commands)
        SagaUtils.createSagaBatchJobFileFromSagaCommands(commands)
        loglines = []
        loglines.append(self.tr('SAGA execution commands'))
        for line in commands:
            progress.setCommand(line)
            loglines.append(line)
        if ProcessingConfig.getSetting(SagaUtils.SAGA_LOG_COMMANDS):
            ProcessingLog.addToLog(ProcessingLog.LOG_INFO, loglines)
        SagaUtils.executeSaga(progress)

    def exportRasterLayer(self, source):
        global sessionExportedLayers
        if source in sessionExportedLayers:
            exportedLayer = sessionExportedLayers[source]
            if os.path.exists(exportedLayer):
                self.exportedLayers[source] = exportedLayer
                return None
            else:
                del sessionExportedLayers[source]
        layer = dataobjects.getObjectFromUri(source, False)
        if layer:
            filename = layer.name()
        else:
            filename = os.path.basename(source)
        validChars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:'
        filename = ''.join(c for c in filename if c in validChars)
        if len(filename) == 0:
            filename = 'layer'
        destFilename = getTempFilenameInTempFolder(filename + '.sgrd')
        self.exportedLayers[source] = destFilename
        sessionExportedLayers[source] = destFilename
        return 'io_gdal 0 -TRANSFORM -INTERPOL 0 -GRIDS "' + destFilename + '" -FILES "' + source +  '"'
