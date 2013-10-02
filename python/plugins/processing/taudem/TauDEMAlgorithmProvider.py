# -*- coding: utf-8 -*-

"""
***************************************************************************
    TauDEMAlgorithmProvider.py
    ---------------------
    Date                 : October 2012
    Copyright            : (C) 2012 by Alexander Bruy
    Email                : alexander dot bruy at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = 'Alexander Bruy'
__date__ = 'October 2012'
__copyright__ = '(C) 2012, Alexander Bruy'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os

from PyQt4.QtGui import *

from processing.core.AlgorithmProvider import AlgorithmProvider
from processing.core.ProcessingConfig import ProcessingConfig
from processing.core.ProcessingConfig import Setting
from processing.core.ProcessingLog import ProcessingLog

from processing.taudem.TauDEMAlgorithm import TauDEMAlgorithm
from processing.taudem.TauDEMUtils import TauDEMUtils

from processing.taudem.peukerdouglas import PeukerDouglas
from processing.taudem.slopearea import SlopeArea
from processing.taudem.lengtharea import LengthArea
from processing.taudem.dropanalysis import DropAnalysis
from processing.taudem.dinfdistdown import DinfDistDown
from processing.taudem.dinfdistup import DinfDistUp
from processing.taudem.gridnet import GridNet
from processing.taudem.dinftranslimaccum import DinfTransLimAccum
from processing.taudem.dinftranslimaccum2 import DinfTransLimAccum2


class TauDEMAlgorithmProvider(AlgorithmProvider):

    def __init__(self):
        AlgorithmProvider.__init__(self)
        self.activate = False
        self.createAlgsList()

    def getDescription(self):
        return 'TauDEM (hydrologic analysis)'

    def getName(self):
        return 'taudem'

    def getIcon(self):
        return QIcon(os.path.dirname(__file__) + '/../images/taudem.png')

    def initializeSettings(self):
        AlgorithmProvider.initializeSettings(self)
        ProcessingConfig.addSetting(Setting(self.getDescription(),
                                    TauDEMUtils.TAUDEM_FOLDER,
                                    'TauDEM command line tools folder',
                                    TauDEMUtils.taudemPath()))
        ProcessingConfig.addSetting(Setting(self.getDescription(),
                                    TauDEMUtils.MPIEXEC_FOLDER,
                                    'MPICH2/OpenMPI bin directory',
                                    TauDEMUtils.mpiexecPath()))
        ProcessingConfig.addSetting(Setting(self.getDescription(),
                                    TauDEMUtils.MPI_PROCESSES,
                                    'Number of MPI parallel processes to use',
                                    2))

    def unload(self):
        AlgorithmProvider.unload(self)
        ProcessingConfig.removeSetting(TauDEMUtils.TAUDEM_FOLDER)
        ProcessingConfig.removeSetting(TauDEMUtils.MPIEXEC_FOLDER)
        ProcessingConfig.removeSetting(TauDEMUtils.MPI_PROCESSES)

    def _loadAlgorithms(self):
        self.algs = self.preloadedAlgs

    def createAlgsList(self):
        self.preloadedAlgs = []
        folder = TauDEMUtils.taudemDescriptionPath()
        for descriptionFile in os.listdir(folder):
            if descriptionFile.endswith('txt'):
                try:
                    alg = TauDEMAlgorithm(os.path.join(folder,
                            descriptionFile))
                    if alg.name.strip() != '':
                        self.preloadedAlgs.append(alg)
                    else:
                        ProcessingLog.addToLog(ProcessingLog.LOG_ERROR,
                                'Could not open TauDEM algorithm: '
                                + descriptionFile)
                except Exception, e:
                    ProcessingLog.addToLog(ProcessingLog.LOG_ERROR,
                            'Could not open TauDEM algorithm: '
                            + descriptionFile)

        self.preloadedAlgs.append(PeukerDouglas())
        self.preloadedAlgs.append(SlopeArea())
        self.preloadedAlgs.append(LengthArea())
        self.preloadedAlgs.append(DropAnalysis())
        self.preloadedAlgs.append(DinfDistDown())
        self.preloadedAlgs.append(DinfDistUp())
        self.preloadedAlgs.append(GridNet())
        self.preloadedAlgs.append(DinfTransLimAccum())
        self.preloadedAlgs.append(DinfTransLimAccum2())
