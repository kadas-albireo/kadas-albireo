# -*- coding: utf-8 -*-

"""
***************************************************************************
    AdminToolsAlgorithmProvider.py
    ---------------------
    Date                 : October 2012
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
from sextante.admintools.PostGISExecuteSQL import PostGISExecuteSQL
import os
from PyQt4 import QtGui

__author__ = 'Victor Olaya'
__date__ = 'October 2012'
__copyright__ = '(C) 2012, Victor Olaya'
# This will get replaced with a git SHA1 when you do a git archive
__revision__ = '$Format:%H$'

from sextante.admintools.ImportIntoPostGIS import ImportIntoPostGIS
from sextante.admintools.ImportVectorIntoGeoServer import ImportVectorIntoGeoServer
from sextante.admintools.CreateWorkspace import CreateWorkspace
from sextante.admintools.ImportRasterIntoGeoServer import ImportRasterIntoGeoServer
from sextante.admintools.DeleteWorkspace import DeleteWorkspace
from sextante.admintools.DeleteDatastore import DeleteDatastore
from sextante.admintools.CreateStyleGeoServer import CreateStyleGeoServer
from sextante.core.AlgorithmProvider import AlgorithmProvider

class AdminToolsAlgorithmProvider(AlgorithmProvider):

    def __init__(self):
        AlgorithmProvider.__init__(self)
        self.alglist = [ImportVectorIntoGeoServer(), ImportRasterIntoGeoServer(),
                        CreateWorkspace(), DeleteWorkspace(), DeleteDatastore(),
                        CreateStyleGeoServer(), ImportIntoPostGIS(), PostGISExecuteSQL()]#, TruncateSeedGWC()]

    def initializeSettings(self):
        AlgorithmProvider.initializeSettings(self)


    def unload(self):
        AlgorithmProvider.unload(self)


    def getName(self):
        return "gspg"

    def getDescription(self):
        return "GeoServer/PostGIS tools"

    def getIcon(self):
        return QtGui.QIcon(os.path.dirname(__file__) + "/../images/database.png")

    def _loadAlgorithms(self):
        self.algs = self.alglist

    def supportsNonFileBasedOutput(self):
        return False