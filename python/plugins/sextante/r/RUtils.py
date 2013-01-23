# -*- coding: utf-8 -*-

"""
***************************************************************************
    RUtils.py
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

from PyQt4.QtGui import *
from sextante.core.SextanteConfig import SextanteConfig
import os
from sextante.core.SextanteUtils import mkdir, SextanteUtils
import subprocess
from sextante.core.SextanteLog import SextanteLog
import stat

class RUtils:

    RSCRIPTS_FOLDER = "R_SCRIPTS_FOLDER"
    R_FOLDER = "R_FOLDER"

    @staticmethod
    def RFolder():
        folder = SextanteConfig.getSetting(RUtils.R_FOLDER)
        if folder == None:
            folder = ""

        return os.path.abspath(unicode(folder))

    @staticmethod
    def RScriptsFolder():
        folder = SextanteConfig.getSetting(RUtils.RSCRIPTS_FOLDER)
        if folder == None:
            folder = unicode(os.path.join(SextanteUtils.userFolder(), "rscripts"))
        mkdir(folder)

        return os.path.abspath(folder)

    @staticmethod
    def createRScriptFromRCommands(commands):
        scriptfile = open(RUtils.getRScriptFilename(), "w")
        for command in commands:
            scriptfile.write(command + "\n")
        scriptfile.close()

    @staticmethod
    def getRScriptFilename():
        return SextanteUtils.userFolder() + os.sep + "sextante_script.r"

    @staticmethod
    def getConsoleOutputFilename():
        return RUtils.getRScriptFilename()+".Rout"

    @staticmethod
    def executeRAlgorithm(alg, progress):
        RUtils.verboseCommands = alg.getVerboseCommands();
        RUtils.createRScriptFromRCommands(alg.getFullSetOfRCommands())
        if SextanteUtils.isWindows():
            command = [RUtils.RFolder() + os.sep + "bin" + os.sep + "R.exe", "CMD", "BATCH", "--vanilla",
                       RUtils.getRScriptFilename(), RUtils.getConsoleOutputFilename()]
        else:
            os.chmod(RUtils.getRScriptFilename(), stat.S_IEXEC | stat.S_IREAD | stat.S_IWRITE)
            command = "R CMD BATCH --vanilla " + RUtils.getRScriptFilename() + " "+ RUtils.getConsoleOutputFilename()

        proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE,stderr=subprocess.STDOUT,
                                universal_newlines=True)
        proc.wait()
        RUtils.createConsoleOutput()
        loglines = []
        loglines.append("R execution console output")
        loglines += RUtils.allConsoleResults
        for line in loglines:
            progress.setConsoleInfo(line)
        SextanteLog.addToLog(SextanteLog.LOG_INFO, loglines)

    @staticmethod
    def createConsoleOutput():
        RUtils.consoleResults = []
        RUtils.allConsoleResults = []
        add = False
        if os.path.exists(RUtils.getConsoleOutputFilename()):
            lines = open(RUtils.getConsoleOutputFilename())
            for line in lines:
                line = line.strip("\n").strip(" ")
                if line.startswith(">"):
                    line = line[1:].strip(" ")
                    if line in RUtils.verboseCommands:
                        add = True
                    else:
                        add = False
                elif add:
                    RUtils.consoleResults.append("<p>" + line + "</p>\n");
                RUtils.allConsoleResults.append(line);

    @staticmethod
    def getConsoleOutput():
        s = "<font face=\"courier\">\n"
        s+="<h2> R Output</h2>\n"
        for line in RUtils.consoleResults:
            s+=line
        s+="</font>\n"

        return s
