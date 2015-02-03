# -*- coding: utf-8 -*-

"""
***************************************************************************
    ProcessingPlugin.py
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

import shutil
import inspect
import os
import sys

from PyQt4.QtCore import Qt, QCoreApplication, QDir
from PyQt4.QtGui import QMenu, QAction, QIcon

from processing.core.Processing import Processing
from processing.gui.ProcessingToolbox import ProcessingToolbox
from processing.gui.HistoryDialog import HistoryDialog
from processing.gui.ConfigDialog import ConfigDialog
from processing.gui.ResultsDialog import ResultsDialog
from processing.gui.CommanderWindow import CommanderWindow
from processing.modeler.ModelerDialog import ModelerDialog
from processing.tools.system import tempFolder

import processing.resources_rc

cmd_folder = os.path.split(inspect.getfile(inspect.currentframe()))[0]
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

class ProcessingPlugin:

    def __init__(self, iface):
        self.iface = iface

    def initGui(self):
        Processing.initialize()

        self.commander = None
        self.toolbox = ProcessingToolbox()
        self.iface.addDockWidget(Qt.RightDockWidgetArea, self.toolbox)
        self.toolbox.hide()
        Processing.addAlgListListener(self.toolbox)

        self.menu = QMenu(self.iface.mainWindow().menuBar())
        self.menu.setObjectName('processing')
        self.menu.setTitle(self.tr('Pro&cessing'))

        self.toolboxAction = self.toolbox.toggleViewAction()
        self.toolboxAction.setObjectName('toolboxAction')
        self.toolboxAction.setIcon(QIcon(':/processing/images/alg.png'))
        self.toolboxAction.setText(self.tr('&Toolbox'))
        self.menu.addAction(self.toolboxAction)

        self.modelerAction = QAction(QIcon(':/processing/images/model.png'),
            self.tr('Graphical &Modeler...'), self.iface.mainWindow())
        self.modelerAction.setObjectName('modelerAction')
        self.modelerAction.triggered.connect(self.openModeler)
        self.menu.addAction(self.modelerAction)

        self.historyAction = QAction(QIcon(':/processing/images/history.gif'),
            self.tr('&History and Log...'), self.iface.mainWindow())
        self.historyAction.setObjectName('historyAction')
        self.historyAction.triggered.connect(self.openHistory)
        self.menu.addAction(self.historyAction)

        self.configAction = QAction(QIcon(':/processing/images/config.png'),
            self.tr('&Options...'), self.iface.mainWindow())
        self.configAction.setObjectName('configAction')
        self.configAction.triggered.connect(self.openConfig)
        self.menu.addAction(self.configAction)

        self.resultsAction = QAction(QIcon(':/processing/images/results.png'),
            self.tr('&Results Viewer...'), self.iface.mainWindow())
        self.resultsAction.setObjectName('resultsAction')
        self.resultsAction.triggered.connect(self.openResults)
        self.menu.addAction(self.resultsAction)

        menuBar = self.iface.mainWindow().menuBar()
        menuBar.insertMenu(
            self.iface.firstRightStandardMenu().menuAction(), self.menu)

        self.commanderAction = QAction(
            QIcon(':/processing/images/commander.png'),
            self.tr('&Commander'), self.iface.mainWindow())
        self.commanderAction.setObjectName('commanderAction')
        self.commanderAction.triggered.connect(self.openCommander)
        self.menu.addAction(self.commanderAction)
        self.iface.registerMainWindowAction(self.commanderAction,
            self.tr('Ctrl+Alt+M'))

    def unload(self):
        self.toolbox.setVisible(False)
        self.menu.deleteLater()

        # delete temporary output files
        folder = tempFolder()
        if QDir(folder).exists():
            shutil.rmtree(folder, True)

        self.iface.unregisterMainWindowAction(self.commanderAction)

    def openCommander(self):
        if self.commander is None:
            self.commander = CommanderWindow(
                self.iface.mainWindow(),
                self.iface.mapCanvas())
            Processing.addAlgListListener(self.commander)
        self.commander.prepareGui()
        self.commander.show()

    def openToolbox(self):
        if self.toolbox.isVisible():
            self.toolbox.hide()
        else:
            self.toolbox.show()

    def openModeler(self):
        dlg = ModelerDialog()
        dlg.exec_()
        if dlg.update:
            Processing.updateAlgsList()
            self.toolbox.updateProvider('model')

    def openResults(self):
        dlg = ResultsDialog()
        dlg.show()
        dlg.exec_()

    def openHistory(self):
        dlg = HistoryDialog()
        dlg.exec_()

    def openConfig(self):
        dlg = ConfigDialog(self.toolbox)
        dlg.exec_()

    def tr(self, message):
        return QCoreApplication.translate('Processing', message)
