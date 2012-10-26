# -*- coding:utf-8 -*-
"""
/***************************************************************************
Python Conosle for QGIS
                             -------------------
begin                : 2012-09-10
copyright            : (C) 2012 by Salvatore Larosa
email                : lrssvtml (at) gmail (dot) com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
Some portions of code were taken from https://code.google.com/p/pydee/
"""

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.utils import iface
from console_sci import PythonEdit
from console_output import EditorOutput
from console_help import HelpDialog
from console_settings import optionsDialog

import sys
import os

_console = None

def show_console():
  """ called from QGIS to open the console """
  global _console
  if _console is None:
    parent = iface.mainWindow() if iface else None
    _console = PythonConsole( parent )
    _console.show() # force show even if it was restored as hidden

    # set focus to the console so the user can start typing
    # defer the set focus event so it works also whether the console not visible yet
    QTimer.singleShot(0, _console.activate)
  else:
    _console.setVisible(not _console.isVisible())

    # set focus to the console so the user can start typing
    if _console.isVisible():
      _console.activate()

_old_stdout = sys.stdout
_console_output = None

# hook for python console so all output will be redirected
# and then shown in console
def console_displayhook(obj):
    global _console_output
    _console_output = obj

class PythonConsole(QDockWidget):
    def __init__(self, parent=None):
        QDockWidget.__init__(self, parent)
        self.setObjectName("PythonConsole")
        self.setWindowTitle(QCoreApplication.translate("PythonConsole", "Python Console"))
        #self.setAllowedAreas(Qt.BottomDockWidgetArea) 

        self.console = PythonConsoleWidget(self)
        self.setWidget( self.console )
        self.setFocusProxy( self.console )

        # try to restore position from stored main window state
        if iface and not iface.mainWindow().restoreDockWidget(self):
            iface.mainWindow().addDockWidget(Qt.BottomDockWidgetArea, self)

    def activate(self):
        self.activateWindow()
        self.raise_()
        QDockWidget.setFocus(self)


class PythonConsoleWidget(QWidget):
    def __init__(self, parent=None):
        QWidget.__init__(self, parent)
        self.setWindowTitle(QCoreApplication.translate("PythonConsole", "Python Console"))        
        self.widgetButton = QWidget()
        #self.widgetEditors = QWidget()
        
        self.options = optionsDialog(self)
        #self.textEdit = QTextEdit()
        
        self.toolBar = QToolBar()
        self.toolBar.setEnabled(True)
        #self.toolBar.setFont(font)
        self.toolBar.setFocusPolicy(Qt.NoFocus)
        self.toolBar.setContextMenuPolicy(Qt.DefaultContextMenu)
        self.toolBar.setLayoutDirection(Qt.LeftToRight)
        self.toolBar.setIconSize(QSize(24, 24))
        self.toolBar.setOrientation(Qt.Vertical)
        self.toolBar.setMovable(True)
        self.toolBar.setFloatable(True)
        #self.toolBar.setAllowedAreas(Qt.LeftToolBarArea)
        #self.toolBar.setAllowedAreas(Qt.RightToolBarArea)
        #self.toolBar.setObjectName(_fromUtf8("toolMappa"))

        #self.gridLayout = QGridLayout(self)

        self.b = QGridLayout(self.widgetButton)
        #self.e = QGridLayout(self.widgetEditors)
        self.f = QGridLayout(self)

        #self.e.setMargin(0)
        #self.e.setSpacing(0)
        self.f.setMargin(0)
        self.f.setSpacing(0)
        self.b.setMargin(0)
        self.b.setSpacing(0)

        ## Action for Clear button
        clearBt = QCoreApplication.translate("PythonConsole", "Clear console")
        self.clearButton = QAction(parent)
        self.clearButton.setCheckable(False)
        self.clearButton.setEnabled(True)
        self.clearButton.setIcon(QIcon(":/images/console/iconClearConsole.png"))
        self.clearButton.setMenuRole(QAction.PreferencesRole)
        self.clearButton.setIconVisibleInMenu(True)
        self.clearButton.setToolTip(clearBt)
        self.clearButton.setText(clearBt)
        ## Action for settings
        optionsBt = QCoreApplication.translate("PythonConsole", "Settings")
        self.optionsButton = QAction(parent)
        self.optionsButton.setCheckable(False)
        self.optionsButton.setEnabled(True)
        self.optionsButton.setIcon(QIcon(":/images/console/iconSettingsConsole.png"))
        self.optionsButton.setMenuRole(QAction.PreferencesRole)
        self.optionsButton.setIconVisibleInMenu(True)
        self.optionsButton.setToolTip(optionsBt)
        self.optionsButton.setText(optionsBt)
        ## Action menu for class
        actionClassBt = QCoreApplication.translate("PythonConsole", "Import Class")
        self.actionClass = QAction(parent)
        self.actionClass.setCheckable(False)
        self.actionClass.setEnabled(True)
        self.actionClass.setIcon(QIcon(":/images/console/iconClassConsole.png"))
        self.actionClass.setMenuRole(QAction.PreferencesRole)
        self.actionClass.setIconVisibleInMenu(True)
        self.actionClass.setToolTip(actionClassBt)
        self.actionClass.setText(actionClassBt)
        ## Action menu Open/Save script
        actionScriptBt = QCoreApplication.translate("PythonConsole", "Manage Script")
        self.actionScript = QAction(parent)
        self.actionScript.setCheckable(False)
        self.actionScript.setEnabled(True)
        self.actionScript.setIcon(QIcon(":/images/console/iconScriptConsole.png"))
        self.actionScript.setMenuRole(QAction.PreferencesRole)
        self.actionScript.setIconVisibleInMenu(True)
        self.actionScript.setToolTip(actionScriptBt)
        self.actionScript.setText(actionScriptBt)
        ## Import Sextante class
        loadSextanteBt = QCoreApplication.translate("PythonConsole", "Import Sextante class")
        self.loadSextanteButton = QAction(parent)
        self.loadSextanteButton.setCheckable(False)
        self.loadSextanteButton.setEnabled(True)
        self.loadSextanteButton.setIcon(QIcon(":/images/console/iconSextanteConsole.png"))
        self.loadSextanteButton.setMenuRole(QAction.PreferencesRole)
        self.loadSextanteButton.setIconVisibleInMenu(True)
        self.loadSextanteButton.setToolTip(loadSextanteBt)
        self.loadSextanteButton.setText(loadSextanteBt)
        ## Import QgisInterface class
        loadIfaceBt = QCoreApplication.translate("PythonConsole", "Import QgisInterface class")
        self.loadIfaceButton = QAction(parent)
        self.loadIfaceButton.setCheckable(False)
        self.loadIfaceButton.setEnabled(True)
        self.loadIfaceButton.setIcon(QIcon(":/images/console/iconIfaceConsole.png"))
        self.loadIfaceButton.setMenuRole(QAction.PreferencesRole)
        self.loadIfaceButton.setIconVisibleInMenu(True)
        self.loadIfaceButton.setToolTip(loadIfaceBt)
        self.loadIfaceButton.setText(loadIfaceBt)
        ## Import QtCore class
        loadQtCoreBt = QCoreApplication.translate("PythonConsole", "Import PyQt.QtCore class")
        self.loadQtCoreButton = QAction(parent)
        self.loadQtCoreButton.setCheckable(False)
        self.loadQtCoreButton.setEnabled(True)
        self.loadQtCoreButton.setIcon(QIcon(":/images/console/iconQtCoreConsole.png"))
        self.loadQtCoreButton.setMenuRole(QAction.PreferencesRole)
        self.loadQtCoreButton.setIconVisibleInMenu(True)
        self.loadQtCoreButton.setToolTip(loadQtCoreBt)
        self.loadQtCoreButton.setText(loadQtCoreBt)
        ## Import QtGui class
        loadQtGuiBt = QCoreApplication.translate("PythonConsole", "Import PyQt.QtGui class")
        self.loadQtGuiButton = QAction(parent)
        self.loadQtGuiButton.setCheckable(False)
        self.loadQtGuiButton.setEnabled(True)
        self.loadQtGuiButton.setIcon(QIcon(":/images/console/iconQtGuiConsole.png"))
        self.loadQtGuiButton.setMenuRole(QAction.PreferencesRole)
        self.loadQtGuiButton.setIconVisibleInMenu(True)
        self.loadQtGuiButton.setToolTip(loadQtGuiBt)
        self.loadQtGuiButton.setText(loadQtGuiBt)
        ## Action for Open File
        openFileBt = QCoreApplication.translate("PythonConsole", "Open script file")
        self.openFileButton = QAction(parent)
        self.openFileButton.setCheckable(False)
        self.openFileButton.setEnabled(True)
        self.openFileButton.setIcon(QIcon(":/images/console/iconOpenConsole.png"))
        self.openFileButton.setMenuRole(QAction.PreferencesRole)
        self.openFileButton.setIconVisibleInMenu(True)
        self.openFileButton.setToolTip(openFileBt)
        self.openFileButton.setText(openFileBt)
        ## Action for Save File
        saveFileBt = QCoreApplication.translate("PythonConsole", "Save to script file")
        self.saveFileButton = QAction(parent)
        self.saveFileButton.setCheckable(False)
        self.saveFileButton.setEnabled(True)
        self.saveFileButton.setIcon(QIcon(":/images/console/iconSaveConsole.png"))
        self.saveFileButton.setMenuRole(QAction.PreferencesRole)
        self.saveFileButton.setIconVisibleInMenu(True)
        self.saveFileButton.setToolTip(saveFileBt)
        self.saveFileButton.setText(saveFileBt)
        ## Action for Run script
        runBt = QCoreApplication.translate("PythonConsole", "Run command")
        self.runButton = QAction(parent)
        self.runButton.setCheckable(False)
        self.runButton.setEnabled(True)
        self.runButton.setIcon(QIcon(":/images/console/iconRunConsole.png"))
        self.runButton.setMenuRole(QAction.PreferencesRole)
        self.runButton.setIconVisibleInMenu(True)
        self.runButton.setToolTip(runBt)
        self.runButton.setText(runBt)
        ## Help action
        helpBt = QCoreApplication.translate("PythonConsole", "Help")
        self.helpButton = QAction(parent)
        self.helpButton.setCheckable(False)
        self.helpButton.setEnabled(True)
        self.helpButton.setIcon(QIcon(":/images/console/iconHelpConsole.png"))
        self.helpButton.setMenuRole(QAction.PreferencesRole)
        self.helpButton.setIconVisibleInMenu(True)
        self.helpButton.setToolTip(helpBt)
        self.helpButton.setText(helpBt)

        self.toolBar.addAction(self.clearButton)
        self.toolBar.addAction(self.actionClass)
        self.toolBar.addAction(self.actionScript)
        self.toolBar.addAction(self.optionsButton)
        self.toolBar.addAction(self.helpButton)
        self.toolBar.addAction(self.runButton)
        ## Menu Import Class
        self.classMenu = QMenu(self)
        self.classMenu.addAction(self.loadIfaceButton)
        self.classMenu.addAction(self.loadSextanteButton)
        self.classMenu.addAction(self.loadQtCoreButton)
        self.classMenu.addAction(self.loadQtGuiButton)
        cM = self.toolBar.widgetForAction(self.actionClass)
        cM.setMenu(self.classMenu)
        cM.setPopupMode(QToolButton.InstantPopup)
        ## Menu Manage Script
        self.scriptMenu = QMenu(self)
        self.scriptMenu.addAction(self.openFileButton)
        self.scriptMenu.addAction(self.saveFileButton)
        sM = self.toolBar.widgetForAction(self.actionScript)
        sM.setMenu(self.scriptMenu)
        sM.setPopupMode(QToolButton.InstantPopup)

        self.b.addWidget(self.toolBar)
        self.edit = PythonEdit()
        self.textEditOut = EditorOutput()

        #self.textEdit = PythonEditOutput()
        #self.textEdit.setReadOnly(True)
        #self.textEdit.setMinimumHeight(80)
        
        self.setFocusProxy(self.edit)
        
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.widgetButton.sizePolicy().hasHeightForWidth())
        self.widgetButton.setSizePolicy(sizePolicy)
        #self.e.addWidget(self.textEdit)
        #self.e.addWidget(self.edit)

        self.consoleFrame = QFrame(self)
        self.consoleFrame.setObjectName("consoleFrame")
        self.consoleLayout = QVBoxLayout(self.consoleFrame)
        self.consoleLayout.setSpacing(0)
        self.consoleLayout.setMargin(0)

        sizePolicy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.textEditOut.sizePolicy().hasHeightForWidth())
        self.textEditOut.setSizePolicy(sizePolicy)
        self.consoleLayout.addWidget(self.textEditOut)

        self.edit.setMinimumSize(QSize(0, 32))
        self.edit.setMaximumSize(QSize(16777215, 32))
        self.consoleLayout.addWidget(self.edit)


        self.f.addWidget(self.widgetButton, 0, 0)
        self.f.addWidget(self.consoleFrame, 0, 1)
        #self.f.addWidget(self.widgetEditors)
        
        #self.f.setStretchFactor(self.widgetEditors, 1)
        
        
        self.clearButton.triggered.connect(self.textEditOut.clearConsole)
        self.optionsButton.triggered.connect(self.openSettings)
        self.loadIfaceButton.triggered.connect(self.iface)
        self.loadSextanteButton.triggered.connect(self.sextante)
        self.loadQtCoreButton.triggered.connect(self.qtCore)
        self.loadQtGuiButton.triggered.connect(self.qtGui)
        self.runButton.triggered.connect(self.edit.entered)
        self.openFileButton.triggered.connect(self.openScriptFile)
        self.saveFileButton.triggered.connect(self.saveScriptFile)
        self.helpButton.triggered.connect(self.openHelp)
        QObject.connect(self.options.buttonBox, SIGNAL("accepted()"), 
                        self.prefChanged)

    def sextante(self):
       self.edit.commandConsole('sextante')

    def iface(self):
       self.edit.commandConsole('iface')
       
    def qtCore(self):
       self.edit.commandConsole('qtCore')
    
    def qtGui(self):
       self.edit.commandConsole('qtGui')

    def openScriptFile(self):
        settings = QSettings()
        lastDirPath = settings.value("pythonConsole/lastDirPath").toString()
        scriptFile = QFileDialog.getOpenFileName(
                        self, "Open File", lastDirPath, "Script file (*.py)")
        if scriptFile.isEmpty() == False:
            oF = open(scriptFile, 'r')
            listScriptFile = []
            for line in oF:
                if line != "\n":
                    listScriptFile.append(line)
            self.edit.insertTextFromFile(listScriptFile)

            lastDirPath = QFileInfo(scriptFile).path()
            settings.setValue("pythonConsole/lastDirPath", QVariant(scriptFile))


    def saveScriptFile(self):
        scriptFile = QFileDialog()
        scriptFile.setDefaultSuffix(".py")
        fName = scriptFile.getSaveFileName(
                        self, "Save file", QString(), "Script file (*.py)")

        if fName.isEmpty() == False:
            filename = str(fName)
            if not filename.endswith(".py"):
                fName += ".py"
            sF = open(fName,'w')
            listText = self.textEditOut.getTextFromEditor()
            is_first_line = True
            for s in listText:
                if s[0:3] in (">>>", "..."):
                    s.replace(">>> ", "").replace("... ", "")
                    if is_first_line:
                        is_first_line = False
                    else:
                        sF.write('\n')
                    sF.write(s)
            sF.close()
        
    def openHelp(self):
        dlg = HelpDialog()
        dlg.exec_()
        
    def openSettings(self):
        #options = optionsDialog()
        self.options.exec_()
        
    def prefChanged(self):
        self.edit.refreshLexerProperties()
        self.textEditOut.refreshLexerProperties()

    def closeEvent(self, event):
        self.edit.writeHistoryFile()
        QWidget.closeEvent(self, event)

if __name__ == '__main__':
    a = QApplication(sys.argv)
    console = PythonConsoleWidget()
    console.show()
    a.exec_()
