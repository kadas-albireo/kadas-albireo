# -*- coding: utf-8 -*-

"""
***************************************************************************
    RAlgorithmProvider.py
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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
import os.path
from sextante.script.WrongScriptException import WrongScriptException
from sextante.core.SextanteConfig import SextanteConfig, Setting
from sextante.core.SextanteLog import SextanteLog
from sextante.core.AlgorithmProvider import AlgorithmProvider
from PyQt4 import QtGui
from sextante.r.RUtils import RUtils
from sextante.r.RAlgorithm import RAlgorithm
from sextante.r.CreateNewRScriptAction import CreateNewRScriptAction
from sextante.r.EditRScriptAction import EditRScriptAction
from sextante.core.SextanteUtils import SextanteUtils

class RAlgorithmProvider(AlgorithmProvider):

    def __init__(self):
        AlgorithmProvider.__init__(self)
        self.activate = False
        self.actions.append(CreateNewRScriptAction())
        self.contextMenuActions = [EditRScriptAction()]

    def initializeSettings(self):
        AlgorithmProvider.initializeSettings(self)
        SextanteConfig.addSetting(Setting(self.getDescription(), RUtils.RSCRIPTS_FOLDER, "R Scripts folder", RUtils.RScriptsFolder()))
        if SextanteUtils.isWindows():
            SextanteConfig.addSetting(Setting(self.getDescription(), RUtils.R_FOLDER, "R folder", RUtils.RFolder()))

    def unload(self):
        AlgorithmProvider.unload(self)
        SextanteConfig.removeSetting(RUtils.RSCRIPTS_FOLDER)
        if SextanteUtils.isWindows():
            SextanteConfig.removeSetting(RUtils.R_FOLDER)

    def getIcon(self):
        return QtGui.QIcon(os.path.dirname(__file__) + "/../images/r.png")


    def getDescription(self):
        return "R scripts"

    def getName(self):
        return "r"

    def _loadAlgorithms(self):
        folder = RUtils.RScriptsFolder()
        self.loadFromFolder(folder)
        folder = os.path.join(os.path.dirname(__file__), "scripts")
        self.loadFromFolder(folder)


    def loadFromFolder(self, folder):
        if not os.path.exists(folder):
            return
        for descriptionFile in os.listdir(folder):
            if descriptionFile.endswith("rsx"):
                try:
                    fullpath = os.path.join(folder, descriptionFile)
                    alg = RAlgorithm(fullpath)
                    if alg.name.strip() != "":
                        self.algs.append(alg)
                except WrongScriptException,e:
                    SextanteLog.addToLog(SextanteLog.LOG_ERROR,e.msg)
                except Exception, e:
                    SextanteLog.addToLog(SextanteLog.LOG_ERROR,"Could not load R script:" + descriptionFile)



