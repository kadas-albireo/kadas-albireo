# -*- coding:utf-8 -*-
"""
/***************************************************************************
Python Console for QGIS
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

from PyQt4.QtCore import Qt, QSettings, QByteArray, QCoreApplication, QFile, QSize
from PyQt4.QtGui import QDialog, QMenu, QColor, QShortcut, QKeySequence, QFont, QFontMetrics, QStandardItemModel, QStandardItem, QApplication, QClipboard
from PyQt4.Qsci import QsciScintilla, QsciLexerPython, QsciAPIs

import sys
import os
import code
import codecs
import re

from qgis.core import QgsApplication
from ui_console_history_dlg import Ui_HistoryDialogPythonConsole

_init_commands = ["from qgis.core import *", "import qgis.utils",
                  "from qgis.utils import iface"]
_historyFile = unicode( QgsApplication.qgisSettingsDirPath() + "console_history.txt" )

class ShellScintilla(QsciScintilla, code.InteractiveInterpreter):
    def __init__(self, parent=None):
        super(ShellScintilla,self).__init__(parent)
        code.InteractiveInterpreter.__init__(self, locals=None)

        self.parent = parent

        self.opening = ['(', '{', '[', "'", '"']
        self.closing = [')', '}', ']', "'", '"']

        self.settings = QSettings()

        # Enable non-ascii chars for editor
        self.setUtf8(True)

        self.new_input_line = True

        self.setMarginWidth(0, 0)
        self.setMarginWidth(1, 0)
        self.setMarginWidth(2, 0)

        self.buffer = []

        self.displayPrompt(False)

        for line in _init_commands:
            self.runsource(line)

        self.history = []
        self.historyIndex = 0
        # Read history command file
        self.readHistoryFile()

        self.historyDlg = HistoryDialog(self)

        # Brace matching: enable for a brace immediately before or after
        # the current position
        self.setBraceMatching(QsciScintilla.SloppyBraceMatch)
        self.setMatchedBraceBackgroundColor(QColor("#b7f907"))

        # Current line visible with special background color
        self.setCaretWidth(2)

        self.refreshSettingsShell()

        # Don't want to see the horizontal scrollbar at all
        # Use raw message to Scintilla here (all messages are documented
        # here: http://www.scintilla.org/ScintillaDoc.html)
        self.SendScintilla(QsciScintilla.SCI_SETHSCROLLBAR, 0)

        # not too small
        #self.setMinimumSize(500, 300)

        self.setWrapMode(QsciScintilla.WrapCharacter)
        self.SendScintilla(QsciScintilla.SCI_EMPTYUNDOBUFFER)

        ## Disable command key
        ctrl, shift = self.SCMOD_CTRL<<16, self.SCMOD_SHIFT<<16
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('L')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('T')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('D')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('Z')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('Y')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('L')+ ctrl+shift)

        ## New QShortcut = ctrl+space/ctrl+alt+space for Autocomplete
        self.newShortcutCSS = QShortcut(QKeySequence(Qt.CTRL + Qt.SHIFT + Qt.Key_Space), self)
        self.newShortcutCAS = QShortcut(QKeySequence(Qt.CTRL + Qt.ALT + Qt.Key_Space), self)
        self.newShortcutCSS.setContext(Qt.WidgetShortcut)
        self.newShortcutCAS.setContext(Qt.WidgetShortcut)
        self.newShortcutCAS.activated.connect(self.autoCompleteKeyBinding)
        self.newShortcutCSS.activated.connect(self.showHistory)

    def _setMinimumHeight(self):
        fnt = self.settings.value("pythonConsole/fontfamilytext", "Monospace")
        fntSize = self.settings.value("pythonConsole/fontsize", 10, type=int)
        fm = QFontMetrics(QFont(fnt, fntSize))

        self.setMinimumHeight(fm.height() + 10)

    def refreshSettingsShell(self):
        # Set Python lexer
        self.setLexers()
        threshold = self.settings.value("pythonConsole/autoCompThreshold", 2, type=int)
        self.setAutoCompletionThreshold(threshold)
        radioButtonSource = self.settings.value("pythonConsole/autoCompleteSource", 'fromAPI')
        autoCompEnabled = self.settings.value("pythonConsole/autoCompleteEnabled", True, type=bool)
        if autoCompEnabled:
            if radioButtonSource == 'fromDoc':
                self.setAutoCompletionSource(self.AcsDocument)
            elif radioButtonSource == 'fromAPI':
                self.setAutoCompletionSource(self.AcsAPIs)
            elif radioButtonSource == 'fromDocAPI':
                self.setAutoCompletionSource(self.AcsAll)
        else:
            self.setAutoCompletionSource(self.AcsNone)

        cursorColor = self.settings.value("pythonConsole/cursorColor", QColor(Qt.black))
        self.setCaretForegroundColor(cursorColor)

        # Sets minimum height for input area based of font metric
        self._setMinimumHeight()

    def showHistory(self):
        if not self.historyDlg.isVisible():
            self.historyDlg.show()
        self.historyDlg._reloadHistory()
        self.historyDlg.activateWindow()

    def autoCompleteKeyBinding(self):
        radioButtonSource = self.settings.value("pythonConsole/autoCompleteSource", 'fromAPI')
        autoCompEnabled = self.settings.value("pythonConsole/autoCompleteEnabled", True, type=bool)
        if autoCompEnabled:
            if radioButtonSource == 'fromDoc':
                self.autoCompleteFromDocument()
            elif radioButtonSource == 'fromAPI':
                self.autoCompleteFromAPIs()
            elif radioButtonSource == 'fromDocAPI':
                self.autoCompleteFromAll()

    def commandConsole(self, command):
        if not self.is_cursor_on_last_line():
            self.move_cursor_to_end()
        line, pos = self.getCursorPosition()
        selCmdLenght = len(self.text(line))
        self.setSelection(line, 4, line, selCmdLenght)
        self.removeSelectedText()
        if command == "processing":
            # import Processing class
            self.append('import processing')
        elif command == "qtCore":
            # import QtCore class
            self.append('from PyQt4.QtCore import *')
        elif command == "qtGui":
            # import QtGui class
            self.append('from PyQt4.QtGui import *')
        self.entered()
        self.move_cursor_to_end()
        self.setFocus()

    def setLexers(self):
        self.lexer = QsciLexerPython()

        loadFont = self.settings.value("pythonConsole/fontfamilytext", "Monospace")
        fontSize = self.settings.value("pythonConsole/fontsize", 10, type=int)

        font = QFont(loadFont)
        font.setFixedPitch(True)
        font.setPointSize(fontSize)
        font.setStyleHint(QFont.TypeWriter)
        font.setStretch(QFont.SemiCondensed)
        font.setLetterSpacing(QFont.PercentageSpacing, 87.0)
        font.setBold(False)

        self.lexer.setDefaultFont(font)
        self.lexer.setDefaultColor(QColor(self.settings.value("pythonConsole/defaultFontColor", QColor(Qt.black))))
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/commentFontColor", QColor(Qt.gray))), 1)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/keywordFontColor", QColor(Qt.darkGreen))), 5)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/classFontColor", QColor(Qt.blue))), 8)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/methodFontColor", QColor(Qt.darkGray))), 9)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/decorFontColor", QColor(Qt.darkBlue))), 15)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/commentBlockFontColor", QColor(Qt.gray))), 12)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/singleQuoteFontColor", QColor(Qt.blue))), 4)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/doubleQuoteFontColor", QColor(Qt.blue))), 3)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/tripleSingleQuoteFontColor", QColor(Qt.blue))), 6)
        self.lexer.setColor(QColor(self.settings.value("pythonConsole/tripleDoubleQuoteFontColor", QColor(Qt.blue))), 7)
        self.lexer.setFont(font, 1)
        self.lexer.setFont(font, 3)
        self.lexer.setFont(font, 4)

        for style in range(0, 33):
            paperColor = QColor(self.settings.value("pythonConsole/paperBackgroundColor", QColor(Qt.white)))
            self.lexer.setPaper(paperColor, style)

        self.api = QsciAPIs(self.lexer)
        chekBoxAPI = self.settings.value("pythonConsole/preloadAPI", True, type=bool)
        chekBoxPreparedAPI = self.settings.value("pythonConsole/usePreparedAPIFile", False, type=bool)
        if chekBoxAPI:
            pap = os.path.join(QgsApplication.pkgDataPath(), "python", "qsci_apis", "pyqgis.pap")
            self.api.loadPrepared(pap)
        elif chekBoxPreparedAPI:
            self.api.loadPrepared(self.settings.value("pythonConsole/preparedAPIFile"))
        else:
            apiPath = self.settings.value("pythonConsole/userAPI", [])
            for i in range(0, len(apiPath)):
                self.api.load(unicode(apiPath[i]))
            self.api.prepare()
            self.lexer.setAPIs(self.api)

        self.setLexer(self.lexer)

    ## TODO: show completion list for file and directory

    def getText(self):
        """ Get the text as a unicode string. """
        value = self.getBytes().decode('utf-8')
        # print (value) printing can give an error because the console font
        # may not have all unicode characters
        return value

    def getBytes(self):
        """ Get the text as bytes (utf-8 encoded). This is how
        the data is stored internally. """
        len = self.SendScintilla(self.SCI_GETLENGTH)+1
        bb = QByteArray(len,'0')
        self.SendScintilla(self.SCI_GETTEXT, len, bb)
        return bytes(bb)[:-1]

    def getTextLength(self):
        return self.SendScintilla(QsciScintilla.SCI_GETLENGTH)

    def get_end_pos(self):
        """Return (line, index) position of the last character"""
        line = self.lines() - 1
        return (line, len(self.text(line)))

    def is_cursor_at_end(self):
        """Return True if cursor is at the end of text"""
        cline, cindex = self.getCursorPosition()
        return (cline, cindex) == self.get_end_pos()

    def move_cursor_to_end(self):
        """Move cursor to end of text"""
        line, index = self.get_end_pos()
        self.setCursorPosition(line, index)
        self.ensureCursorVisible()
        self.ensureLineVisible(line)

    def is_cursor_on_last_line(self):
        """Return True if cursor is on the last line"""
        cline, _ = self.getCursorPosition()
        return cline == self.lines() - 1

    def is_cursor_on_edition_zone(self):
        """ Return True if the cursor is in the edition zone """
        cline, cindex = self.getCursorPosition()
        return cline == self.lines() - 1 and cindex >= 4

    def new_prompt(self, prompt):
        """
        Print a new prompt and save its (line, index) position
        """
        self.write(prompt, prompt=True)
        # now we update our cursor giving end of prompt
        line, index = self.getCursorPosition()
        self.ensureCursorVisible()
        self.ensureLineVisible(line)

    def displayPrompt(self, more=False):
        self.append("... ") if more else self.append(">>> ")
        self.move_cursor_to_end()

    def updateHistory(self, command):
        if isinstance(command, list):
            for line in command:
                self.history.append(line)
        elif not command == "":
            if len(self.history) <= 0 or \
               command != self.history[-1]:
                self.history.append(command)
        self.historyIndex = len(self.history)

    def writeHistoryFile(self, fromCloseConsole=False):
        ok = False
        try:
            wH = codecs.open(_historyFile, 'w', encoding='utf-8')
            for s in self.history:
                wH.write(s + '\n')
            ok = True
        except:
            raise
        wH.close()
        if ok and not fromCloseConsole:
            msgText = QCoreApplication.translate('PythonConsole',
                                                 'History saved successfully.')
            self.parent.callWidgetMessageBar(msgText)

    def readHistoryFile(self):
        fileExist = QFile.exists(_historyFile)
        if fileExist:
            rH = codecs.open(_historyFile, 'r', encoding='utf-8')
            for line in rH:
                if line != "\n":
                    l = line.rstrip('\n')
                    self.updateHistory(l)
        else:
            return

    def clearHistory(self, clearSession=False):
        if clearSession:
            self.history = []
            msgText = QCoreApplication.translate('PythonConsole',
                                                 'Session and file history cleared successfully.')
            self.parent.callWidgetMessageBar(msgText)
            return
        ok = False
        try:
            cH = codecs.open(_historyFile, 'w', encoding='utf-8')
            ok = True
        except:
            raise
        cH.close()
        if ok:
            msgText = QCoreApplication.translate('PythonConsole',
                                                 'History cleared successfully.')
            self.parent.callWidgetMessageBar(msgText)

    def clearHistorySession(self):
        self.clearHistory(True)

    def showPrevious(self):
        if self.historyIndex < len(self.history) and self.history:
            line, pos = self.getCursorPosition()
            selCmdLenght = len(self.text(line))
            self.setSelection(line, 4, line, selCmdLenght)
            self.removeSelectedText()
            self.historyIndex += 1
            if self.historyIndex == len(self.history):
                self.insert("")
                pass
            else:
                self.insert(self.history[self.historyIndex])
            self.move_cursor_to_end()
            #self.SendScintilla(QsciScintilla.SCI_DELETEBACK)

    def showNext(self):
        if self.historyIndex > 0 and self.history:
            line, pos = self.getCursorPosition()
            selCmdLenght = len(self.text(line))
            self.setSelection(line, 4, line, selCmdLenght)
            self.removeSelectedText()
            self.historyIndex -= 1
            if self.historyIndex == len(self.history):
                self.insert("")
            else:
                self.insert(self.history[self.historyIndex])
            self.move_cursor_to_end()
            #self.SendScintilla(QsciScintilla.SCI_DELETEBACK)

    def keyPressEvent(self, e):
        startLine, startPos, endLine, endPos = self.getSelection()

        # handle invalid cursor position and multiline selections
        if not self.is_cursor_on_edition_zone() or startLine < endLine:
            # allow to copy and select
            if e.modifiers() & (Qt.ControlModifier | Qt.MetaModifier):
                if e.key() in (Qt.Key_C, Qt.Key_A):
                    QsciScintilla.keyPressEvent(self, e)
                return
            # allow selection
            if e.modifiers() & Qt.ShiftModifier:
                if e.key() in (Qt.Key_Left, Qt.Key_Right, Qt.Key_Home, Qt.Key_End):
                    QsciScintilla.keyPressEvent(self, e)
                return
            # all other keystrokes get sent to the input line
            self.move_cursor_to_end()

        line, index = self.getCursorPosition()
        cmd = self.text(line)

        if e.key() in (Qt.Key_Return, Qt.Key_Enter) and not self.isListActive():
            self.entered()

        elif e.key() in (Qt.Key_Left, Qt.Key_Home):
            QsciScintilla.keyPressEvent(self, e)
            # check whether the cursor is moved out of the edition zone
            newline, newindex = self.getCursorPosition()
            if newline < line or newindex < 4:
                # fix selection and the cursor position
                if self.hasSelectedText():
                    self.setSelection(line, self.getSelection()[3], line, 4)
                else:
                    self.setCursorPosition(line, 4)

        elif e.key() in (Qt.Key_Backspace, Qt.Key_Delete):
            QsciScintilla.keyPressEvent(self, e)
            # check whether the cursor is moved out of the edition zone
            _, newindex = self.getCursorPosition()
            if newindex < 4:
                # restore the prompt chars (if removed) and
                # fix the cursor position
                self.insert( cmd[:3-newindex] + " " )
                self.setCursorPosition(line, 4)
            self.recolor()

        elif (e.modifiers() & (Qt.ControlModifier | Qt.MetaModifier) and e.key() == Qt.Key_V) or \
             (e.modifiers() & Qt.ShiftModifier and e.key() == Qt.Key_Insert):
            self.paste()
            e.accept()

        elif e.key() == Qt.Key_Down and not self.isListActive():
            self.showPrevious()
        elif e.key() == Qt.Key_Up and not self.isListActive():
            self.showNext()
        ## TODO: press event for auto-completion file directory
        else:
            t = unicode(e.text())
            self.autoCloseBracket = self.settings.value("pythonConsole/autoCloseBracket", False, type=bool)
            self.autoImport = self.settings.value("pythonConsole/autoInsertionImport", True, type=bool)
            txt = cmd[:index].replace('>>> ', '').replace('... ', '')
            ## Close bracket automatically
            if t in self.opening and self.autoCloseBracket:
                i = self.opening.index(t)
                if self.hasSelectedText() and startPos != 0:
                    selText = self.selectedText()
                    self.removeSelectedText()
                    self.insert(self.opening[i] + selText + self.closing[i])
                    self.setCursorPosition(endLine, endPos+2)
                    return
                elif t == '(' and (re.match(r'^[ \t]*def \w+$', txt)
                                   or re.match(r'^[ \t]*class \w+$', txt)):
                        self.insert('):')
                else:
                    self.insert(self.closing[i])
            ## FIXES #8392 (automatically removes the redundant char
            ## when autoclosing brackets option is enabled)
            elif t in [')', ']', '}'] and self.autoCloseBracket:
                txt = self.text(line)
                try:
                    if txt[index-1] in self.opening and t == txt[index]:
                        self.setCursorPosition(line, index+1)
                        self.SendScintilla(QsciScintilla.SCI_DELETEBACK)
                except IndexError:
                    pass
            elif t == ' ' and self.autoImport:
                ptrn = r'^[ \t]*from [\w.]+$'
                if re.match(ptrn, txt):
                    self.insert(' import')
                    self.setCursorPosition(line, index + 7)
            QsciScintilla.keyPressEvent(self, e)

    def contextMenuEvent(self, e):
        menu = QMenu(self)
        subMenu = QMenu(menu)
        titleHistoryMenu = QCoreApplication.translate("PythonConsole", "Command History")
        subMenu.setTitle(titleHistoryMenu)
        subMenu.addAction(
            QCoreApplication.translate("PythonConsole", "Show"),
            self.showHistory, 'Ctrl+Shift+SPACE')
        subMenu.addSeparator()
        subMenu.addAction(
            QCoreApplication.translate("PythonConsole", "Save"),
            self.writeHistoryFile)
        subMenu.addSeparator()
        subMenu.addAction(
            QCoreApplication.translate("PythonConsole", "Clear File"),
            self.clearHistory)
        subMenu.addAction(
            QCoreApplication.translate("PythonConsole", "Clear Session"),
            self.clearHistorySession)
        menu.addMenu(subMenu)
        menu.addSeparator()
        copyAction = menu.addAction(
            QCoreApplication.translate("PythonConsole", "Copy"),
            self.copy, QKeySequence.Copy)
        pasteAction = menu.addAction(
            QCoreApplication.translate("PythonConsole", "Paste"),
            self.paste, QKeySequence.Paste)
        copyAction.setEnabled(False)
        pasteAction.setEnabled(False)
        if self.hasSelectedText():
            copyAction.setEnabled(True)
        if QApplication.clipboard().text():
            pasteAction.setEnabled(True)
        menu.exec_(self.mapToGlobal(e.pos()))

    def mousePressEvent(self, e):
        """
        Re-implemented to handle the mouse press event.
        e: the mouse press event (QMouseEvent)
        """
        self.setFocus()
        if e.button() == Qt.MidButton:
            stringSel = unicode(QApplication.clipboard().text(QClipboard.Selection))
            if not self.is_cursor_on_last_line():
                self.move_cursor_to_end()
            self.insertFromDropPaste(stringSel)
            e.accept()
        else:
            QsciScintilla.mousePressEvent(self, e)

    def paste(self):
        """
        Method to display data from the clipboard.

        XXX: It should reimplement the virtual QScintilla.paste method,
        but it seems not used by QScintilla code.
        """
        stringPaste = unicode(QApplication.clipboard().text())
        if self.is_cursor_on_last_line():
            if self.hasSelectedText():
                self.removeSelectedText()
        else:
            self.move_cursor_to_end()
        self.insertFromDropPaste(stringPaste)

    ## Drag and drop
    def dropEvent(self, e):
        if e.mimeData().hasText():
            stringDrag = e.mimeData().text()
            self.insertFromDropPaste(stringDrag)
            self.setFocus()
            e.setDropAction(Qt.CopyAction)
            e.accept()
        else:
            QsciScintillaCompat.dropEvent(self, e)

    def insertFromDropPaste(self, textDP):
        pasteList = unicode(textDP).splitlines()
        if pasteList:
            for line in pasteList[:-1]:
                cleanLine = line.replace(">>> ", "").replace("... ", "")
                self.insert(unicode(cleanLine))
                self.move_cursor_to_end()
                self.runCommand(unicode(self.currentCommand()))
            if pasteList[-1] != "":
                line = pasteList[-1]
                cleanLine = line.replace(">>> ", "").replace("... ", "")
                self.insert(unicode(cleanLine))
                self.move_cursor_to_end()

    def insertTextFromFile(self, listOpenFile):
        for line in listOpenFile[:-1]:
            self.append(line)
            self.move_cursor_to_end()
            self.SendScintilla(QsciScintilla.SCI_DELETEBACK)
            self.runCommand(unicode(self.currentCommand()))
        self.append(unicode(listOpenFile[-1]))
        self.move_cursor_to_end()
        self.SendScintilla(QsciScintilla.SCI_DELETEBACK)

    def entered(self):
        self.move_cursor_to_end()
        self.runCommand( unicode(self.currentCommand()) )
        self.setFocus()
        self.move_cursor_to_end()

    def currentCommand(self):
        linenr, index = self.getCursorPosition()
        string = self.text()
        cmdLine = string[4:]
        cmd = unicode(cmdLine)
        return cmd

    def runCommand(self, cmd):
        self.writeCMD(cmd)
        import webbrowser
        self.updateHistory(cmd)
        if cmd in ('_pyqgis', '_api'):
            if cmd == '_pyqgis':
                webbrowser.open( "http://qgis.org/pyqgis-cookbook/" )
            elif cmd == '_api':
                webbrowser.open( "http://qgis.org/api/" )
            more = False
        else:
            self.buffer.append(cmd)
            src = u"\n".join(self.buffer)
            more = self.runsource(src, "<input>")
            if not more:
                self.buffer = []
        ## prevents to commands with more lines to break the console
        ## in the case they have a eol different from '\n'
        self.setText('')
        self.move_cursor_to_end()
        self.displayPrompt(more)

    def write(self, txt):
        sys.stderr.write(txt)

    def writeCMD(self, txt):
        if len(txt) > 0:
            getCmdString = self.text()
            prompt = getCmdString[0:4]
            sys.stdout.write(prompt+txt+'\n')

class HistoryDialog(QDialog, Ui_HistoryDialogPythonConsole):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)
        self.parent = parent
        self.setWindowTitle(QCoreApplication.translate("PythonConsole",
                                                       "Python Console - Command History"))
        self.listView.setToolTip(QCoreApplication.translate("PythonConsole",
                                                            "Double click on item to execute"))
        self.model = QStandardItemModel(self.listView)

        self._reloadHistory()

        self.deleteScut = QShortcut(QKeySequence(Qt.Key_Delete), self)
        self.deleteScut.activated.connect(self._deleteItem)
        self.listView.doubleClicked.connect(self._runHistory)
        self.reloadHistory.clicked.connect(self._reloadHistory)
        self.saveHistory.clicked.connect(self._saveHistory)

    def _runHistory(self, item):
        cmd = item.data(Qt.DisplayRole)
        self.parent.runCommand(unicode(cmd))

    def _saveHistory(self):
        self.parent.writeHistoryFile(True)

    def _reloadHistory(self):
        self.model.clear()
        for i in self.parent.history:
            item = QStandardItem(i)
            if sys.platform.startswith('win'):
                item.setSizeHint(QSize(18, 18))
            self.model.appendRow(item)

        self.listView.setModel(self.model)
        self.listView.scrollToBottom()

    def _deleteItem(self):
        itemsSelected = self.listView.selectionModel().selectedIndexes()
        if itemsSelected:
            item = itemsSelected[0].row()
            ## Remove item from the command history (just for the current session)
            self.parent.history.pop(item)
            self.parent.historyIndex -= 1
            ## Remove row from the command history dialog
            self.model.removeRow(item)
