# -*- coding: utf-8 -*-

"""
***************************************************************************
    RAlgorithm.py
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
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import QtGui, QtCore
from sextante.core.SextanteConfig import SextanteConfig
from sextante.core.GeoAlgorithm import GeoAlgorithm
from sextante.parameters.ParameterRaster import ParameterRaster
from sextante.parameters.ParameterTable import ParameterTable
from sextante.parameters.ParameterVector import ParameterVector
from sextante.parameters.ParameterMultipleInput import ParameterMultipleInput
from sextante.script.WrongScriptException import WrongScriptException
from sextante.outputs.OutputTable import OutputTable
from sextante.outputs.OutputVector import OutputVector
from sextante.outputs.OutputRaster import OutputRaster
from sextante.parameters.ParameterString import ParameterString
from sextante.parameters.ParameterNumber import ParameterNumber
from sextante.parameters.ParameterBoolean import ParameterBoolean
from sextante.parameters.ParameterSelection import ParameterSelection
from sextante.parameters.ParameterTableField import ParameterTableField
from sextante.outputs.OutputHTML import OutputHTML
from sextante.r.RUtils import RUtils
from sextante.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from sextante.core.SextanteLog import SextanteLog
from sextante.core.SextanteUtils import SextanteUtils
import subprocess
from sextante.parameters.ParameterExtent import ParameterExtent
from sextante.parameters.ParameterFile import ParameterFile
from sextante.outputs.OutputFile import OutputFile
from sextante.gui.Help2Html import Help2Html

class RAlgorithm(GeoAlgorithm):

    R_CONSOLE_OUTPUT = "R_CONSOLE_OUTPUT"
    RPLOTS = "RPLOTS"

    def getCopy(self):
        newone = RAlgorithm(self.descriptionFile)
        newone.provider = self.provider
        return newone

    def __init__(self, descriptionFile, script=None):
        GeoAlgorithm.__init__(self)
        self.script = script
        self.descriptionFile = descriptionFile
        if script is not None:
            self.defineCharacteristicsFromScript()
        if descriptionFile is not None:
            self.defineCharacteristicsFromFile()

    def getIcon(self):
        return QtGui.QIcon(os.path.dirname(__file__) + "/../images/r.png")

    def defineCharacteristicsFromScript(self):
        lines = self.script.split("\n")
        self.name = "[Unnamed algorithm]"
        self.group = "User R scripts"
        self.parseDescription(iter(lines))


    def defineCharacteristicsFromFile(self):
        filename = os.path.basename(self.descriptionFile)
        self.name = filename[:filename.rfind(".")].replace("_", " ")
        self.group = "User R scripts"
        with open(self.descriptionFile, 'r') as f:
            lines = [line.strip() for line in f]
        self.parseDescription(iter(lines))

    def parseDescription(self, lines):
        self.script = ""
        self.commands=[]
        self.showPlots = False
        self.showConsoleOutput = False
        self.useRasterPackage = True
        self.passFileNames = False
        self.verboseCommands = []
        ender = 0
        line = lines.next().strip("\n").strip("\r")
        while ender < 10:
            if line.startswith("##"):
                try:
                    self.processParameterLine(line)
                except Exception:
                    raise WrongScriptException("Could not load R script:" + self.descriptionFile + ".\n Problem with line \"" + line + "\"")
            elif line.startswith(">"):
                self.commands.append(line[1:])
                self.verboseCommands.append(line[1:])
                if not self.showConsoleOutput:
                    self.addOutput(OutputHTML(RAlgorithm.R_CONSOLE_OUTPUT, "R Console Output"))
                self.showConsoleOutput = True
            else:
                if line == '':
                    ender += 1
                else:
                    ender=0
                self.commands.append(line)
            self.script += line + "\n"
            try:
                line = lines.next().strip("\n").strip("\r")
            except:
                break

    def getVerboseCommands(self):
        return self.verboseCommands

    def createDescriptiveName(self, s):
        return s.replace("_", " ")

    def processParameterLine(self,line):
        param = None
        out = None
        line = line.replace("#", "");
        if line.lower().strip().startswith("showplots"):
            self.showPlots = True
            self.addOutput(OutputHTML(RAlgorithm.RPLOTS, "R Plots"));
            return
        if line.lower().strip().startswith("dontuserasterpackage"):
            self.useRasterPackage = False
            return
        if line.lower().strip().startswith("passfilenames"):
            self.passFileNames = True
            return
        tokens = line.split("=");
        desc = self.createDescriptiveName(tokens[0])
        if tokens[1].lower().strip() == "group":
            self.group = tokens[0]
            return
        if tokens[1].lower().strip().startswith("raster"):
            param = ParameterRaster(tokens[0], desc, False)
        elif tokens[1].lower().strip() == "vector":
            param = ParameterVector(tokens[0],  desc,ParameterVector.VECTOR_TYPE_ANY)
        elif tokens[1].lower().strip() == "table":
            param = ParameterTable(tokens[0], desc, False)
        elif tokens[1].lower().strip().startswith("multiple raster"):
            param = ParameterMultipleInput(tokens[0], desc, ParameterMultipleInput.TYPE_RASTER)
            param.optional = False
        elif tokens[1].lower().strip() == "multiple vector":
            param = ParameterMultipleInput(tokens[0], desc, ParameterMultipleInput.TYPE_VECTOR_ANY)
            param.optional = False
        elif tokens[1].lower().strip().startswith("selection"):
            options = tokens[1].strip()[len("selection"):].split(";")
            param = ParameterSelection(tokens[0],  desc, options);
        elif tokens[1].lower().strip().startswith("boolean"):
            default = tokens[1].strip()[len("boolean")+1:]
            param = ParameterBoolean(tokens[0],  desc, default)
        elif tokens[1].lower().strip().startswith("number"):
            try:
                default = float(tokens[1].strip()[len("number")+1:])
                param = ParameterNumber(tokens[0],  desc, default=default)
            except:
                raise WrongScriptException("Could not load R script:" + self.descriptionFile + ".\n Problem with line \"" + line + "\"")
        elif tokens[1].lower().strip().startswith("field"):
            field = tokens[1].strip()[len("field")+1:]
            found = False
            for p in self.parameters:
                if p.name == field:
                    found = True
                    break
            if found:
                param = ParameterTableField(tokens[0],  tokens[0], field)
        elif tokens[1].lower().strip() == "extent":
            param = ParameterExtent(tokens[0],  desc)
        elif tokens[1].lower().strip() == "file":
            param = ParameterFile(tokens[0],  desc, False)
        elif tokens[1].lower().strip() == "folder":
            param = ParameterFile(tokens[0],  desc, True)
        elif tokens[1].lower().strip().startswith("string"):
            default = tokens[1].strip()[len("string")+1:]
            param = ParameterString(tokens[0],  desc, default)
        elif tokens[1].lower().strip().startswith("output raster"):
            out = OutputRaster()
        elif tokens[1].lower().strip().startswith("output vector"):
            out = OutputVector()
        elif tokens[1].lower().strip().startswith("output table"):
            out = OutputTable()
        elif tokens[1].lower().strip().startswith("output file"):
            out = OutputFile()

        if param != None:
            self.addParameter(param)
        elif out != None:
            out.name = tokens[0]
            out.description = tokens[0]
            self.addOutput(out)
        else:
            raise WrongScriptException("Could not load R script:" + self.descriptionFile + ".\n Problem with line \"" + line + "\"")

    def processAlgorithm(self, progress):
        if SextanteUtils.isWindows():
            path = RUtils.RFolder()
            if path == "":
                raise GeoAlgorithmExecutionException("R folder is not configured.\nPlease configure it before running R scripts.")
        loglines = []
        loglines.append("R execution commands")
        loglines += self.getFullSetOfRCommands()
        for line in loglines:
            progress.setCommand(line)
        SextanteLog.addToLog(SextanteLog.LOG_INFO, loglines)
        RUtils.executeRAlgorithm(self, progress)
        if self.showPlots:
            htmlfilename = self.getOutputValue(RAlgorithm.RPLOTS)
            f = open(htmlfilename, "w")
            f.write("<img src=\"" + self.plotsFilename + "\"/>")
            f.close()
        if self.showConsoleOutput:
            htmlfilename = self.getOutputValue(RAlgorithm.R_CONSOLE_OUTPUT)
            f = open(htmlfilename, "w")
            f.write(RUtils.getConsoleOutput())
            f.close()

    def getFullSetOfRCommands(self):
        commands = []
        commands += self.getImportCommands()
        commands += self.getRCommands()
        commands += self.getExportCommands()

        return commands

    def getExportCommands(self):
        commands = []
        for out in self.outputs:
            if isinstance(out, OutputRaster):
                value = out.value
                value = value.replace("\\", "/")
                if self.useRasterPackage or self.passFileNames:
                    commands.append("writeRaster(" + out.name + ",\"" + value + "\", overwrite=TRUE)")
                else:
                    if not value.endswith("tif"):
                        value = value + ".tif"
                    commands.append("writeGDAL(" + out.name + ",\"" + value + "\")")
            if isinstance(out, OutputVector):
                value = out.value
                if not value.endswith("shp"):
                    value = value + ".shp"
                value = value.replace("\\", "/")
                filename = os.path.basename(value)
                filename = filename[:-4]
                commands.append("writeOGR(" + out.name + ",\"" + value + "\",\""
                            + filename + "\", driver=\"ESRI Shapefile\")");

        if self.showPlots:
            commands.append("dev.off()");

        return commands


    def getImportCommands(self):
        commands = []
        # if rgdal is not available, try to install it
        # just use main mirror
        commands.append('options("repos"="http://cran.at.r-project.org/")')
        rLibDir = "%s/rlibs" % SextanteUtils.userFolder().replace("\\","/")
        if not os.path.isdir(rLibDir):
            os.mkdir(rLibDir)
        # .libPaths("%s") substitutes the personal libPath with "%s"! With '.libPaths(c("%s",deflibloc))' it is added without replacing and we can use all installed R packages!
        commands.append('deflibloc <- .libPaths()[1]')
        commands.append('.libPaths(c("%s",deflibloc))' % rLibDir )
        commands.append(
            'tryCatch(find.package("rgdal"), error=function(e) install.packages("rgdal", dependencies=TRUE, lib="%s"))' % rLibDir)
        commands.append("library(\"rgdal\")");
        #if not self.useRasterPackage or self.passFileNames:
        commands.append(
                'tryCatch(find.package("raster"), error=function(e) install.packages("raster", dependencies=TRUE, lib="%s"))' % rLibDir)
        commands.append("library(\"raster\")");

        for param in self.parameters:
            if isinstance(param, ParameterRaster):
                value = param.value
                value = value.replace("\\", "/")
                if self.passFileNames:
                    commands.append(param.name + " = \"" + value + "\"")
                elif self.useRasterPackage:
                    commands.append(param.name + " = " + "brick(\"" + value + "\")")
                else:
                    commands.append(param.name + " = " + "readGDAL(\"" + value + "\")")
            if isinstance(param, ParameterVector):
                value = param.getSafeExportedLayer()
                value = value.replace("\\", "/")
                filename = os.path.basename(value)
                filename = filename[:-4]
                folder = os.path.dirname(value)
                if self.passFileNames:
                    commands.append(param.name + " = \"" + value + "\"")
                else:
                    commands.append(param.name + " = readOGR(\"" + folder + "\",layer=\"" + filename + "\")")
            if isinstance(param, ParameterTable):
                value = param.value
                if not value.lower().endswith("csv"):
                    raise GeoAlgorithmExecutionException("Unsupported input file format.\n" + value)
                if self.passFileNames:
                    commands.append(param.name + " = \"" + value + "\"")
                else:
                    commands.append(param.name + " <- read.csv(\"" + value + "\", head=TRUE, sep=\",\")")
            elif isinstance(param, (ParameterTableField, ParameterString, ParameterFile)):
                commands.append(param.name + "=\"" + param.value + "\"")
            elif isinstance(param, (ParameterNumber, ParameterSelection)):
                commands.append(param.name + "=" + str(param.value))
            elif isinstance(param, ParameterBoolean):
                if param.value:
                    commands.append(param.name + "=TRUE")
                else:
                    commands.append(param.name + "=FALSE")
            elif isinstance(param, ParameterMultipleInput):
                iLayer = 0;
                if param.datatype == ParameterMultipleInput.TYPE_RASTER:
                    layers = param.value.split(";")
                    for layer in layers:
                        #if not layer.lower().endswith("asc") and not layer.lower().endswith("tif") and not self.passFileNames:
                            #raise GeoAlgorithmExecutionException("Unsupported input file format.\n" + layer)
                        layer = layer.replace("\\", "/")
                        if self.passFileNames:
                            commands.append("tempvar" + str(iLayer)+ " <- \"" + layer + "\"")
                        elif self.useRasterPackage:
                            commands.append("tempvar" + str(iLayer)+ " <- " + "brick(\"" + layer + "\")")
                        else:
                            commands.append("tempvar" + str(iLayer)+ " <- " + "readGDAL(\"" + layer + "\")")
                        iLayer+=1
                else:
                    exported = param.getSafeExportedLayers()
                    layers = exported.split(";")
                    for layer in layers:
                        if not layer.lower().endswith("shp") and not self.passFileNames:
                            raise GeoAlgorithmExecutionException("Unsupported input file format.\n" + layer)
                        layer = layer.replace("\\", "/")
                        filename = os.path.basename(layer)
                        filename = filename[:-4]
                        if self.passFileNames:
                            commands.append("tempvar" + str(iLayer)+ " <- \"" + layer + "\"")
                        else:
                            commands.append("tempvar" + str(iLayer) + " <- " + "readOGR(\"" + layer + "\",layer=\"" + filename + "\")")
                        iLayer+=1
                s = ""
                s += param.name
                s += (" = c(")
                iLayer = 0
                for layer in layers:
                    if iLayer != 0:
                        s +=","
                    s += "tempvar" + str(iLayer)
                    iLayer += 1
                s+=")\n"
                commands.append(s)

        if self.showPlots:
            htmlfilename = self.getOutputValue(RAlgorithm.RPLOTS)
            self.plotsFilename = htmlfilename +".png"
            self.plotsFilename = self.plotsFilename.replace("\\", "/");
            commands.append("png(\"" + self.plotsFilename + "\")");

        return commands


    def getRCommands(self):
        return self.commands

    def helpFile(self):
        helpfile = unicode(self.descriptionFile) + ".help"
        if os.path.exists(helpfile):
            h2h = Help2Html()
            return h2h.getHtmlFile(self, helpfile)
        else:
            return None

    def checkBeforeOpeningParametersDialog(self):
        if SextanteUtils.isWindows():
            path = RUtils.RFolder()
            if path == "":
                return "R folder is not configured.\nPlease configure it before running R scripts."

        R_INSTALLED = "R_INSTALLED"
        settings = QSettings()
        if settings.contains(R_INSTALLED):
            return
        if SextanteUtils.isWindows():
            if SextanteConfig.getSetting(RUtils.R_USE64):
                execDir = "x64"
            else:
                execDir = "i386"
            command = [RUtils.RFolder() + os.sep + "bin" + os.sep + execDir + os.sep + "R.exe", "--version"]
        else:
            command = ["R --version"]
        proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE,stderr=subprocess.STDOUT, universal_newlines=True).stdout

        for line in iter(proc.readline, ""):
            if "R version" in line:
                settings.setValue(R_INSTALLED, True)
                return
        return "It seems that R is not correctly installed in your system.\nPlease install it before running R Scripts."

