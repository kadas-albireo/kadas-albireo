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

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.Qsci import (QsciScintilla,
                        QsciScintillaBase,
                        QsciLexerPython,
                        QsciAPIs,
                        QsciStyle)
from qgis.core import QgsApplication
from qgis.gui import QgsMessageBar
import sys
import os
import subprocess
import datetime
import pyclbr
from operator import itemgetter
import traceback

class KeyFilter(QObject):
    SHORTCUTS = {
        ("Control", "T"): lambda w, t: w.newTabEditor(),
        ("Control", "M"): lambda w, t: t.save(),
        ("Control", "W"): lambda w, t: t.close()
    }
    def __init__(self, window, tab, *args):
        QObject.__init__(self, *args)
        self.window = window
        self.tab = tab
        self._handlers = {}
        for shortcut, handler in KeyFilter.SHORTCUTS.iteritems():
            modifiers = shortcut[0]
            if not isinstance(modifiers, list):
                modifiers = [modifiers]
            qt_mod_code = Qt.NoModifier
            for each in modifiers:
                qt_mod_code |= getattr(Qt, each + "Modifier")
            qt_keycode = getattr(Qt, "Key_" + shortcut[1].upper())
            handlers = self._handlers.get(qt_keycode, [])
            handlers.append((qt_mod_code, handler))
            self._handlers[qt_keycode] = handlers

    def get_handler(self, key, modifier):
        if self.window.count() > 1:
            for modifiers, handler in self._handlers.get(key, []):
                if modifiers == modifier:
                    return handler
        return None

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress and event.key() < 256:
            handler = self.get_handler(event.key(), event.modifiers())
            if handler:
                handler(self.window, self.tab)
        return QObject.eventFilter(self, obj, event)

class Editor(QsciScintilla):
    MARKER_NUM = 6
    def __init__(self, parent=None):
        super(Editor,self).__init__(parent)
        self.parent = parent
        ## recent modification time
        self.lastModified = 0
        self.opening = ['(', '{', '[', "'", '"']
        self.closing = [')', '}', ']', "'", '"']

        ## List of marker line to be deleted from check syntax
        self.bufferMarkerLine = []

        self.settings = QSettings()

        # Enable non-ascii chars for editor
        self.setUtf8(True)

        # Set the default font
        font = QFont()
        font.setFamily('Courier')
        font.setFixedPitch(True)
        font.setPointSize(10)
        self.setFont(font)
        self.setMarginsFont(font)
        # Margin 0 is used for line numbers
        #fm = QFontMetrics(font)
        fontmetrics = QFontMetrics(font)
        self.setMarginsFont(font)
        self.setMarginWidth(0, fontmetrics.width("0000") + 5)
        self.setMarginLineNumbers(0, True)
        self.setMarginsForegroundColor(QColor("#3E3EE3"))
        self.setMarginsBackgroundColor(QColor("#f9f9f9"))
        self.setCaretLineVisible(True)
        self.setCaretLineBackgroundColor(QColor("#fcf3ed"))

        self.markerDefine(QgsApplication.getThemePixmap("console/iconSyntaxErrorConsole.png"),
                          self.MARKER_NUM)

        self.setMinimumHeight(120)
        #self.setMinimumWidth(300)

        self.setBraceMatching(QsciScintilla.SloppyBraceMatch)
        self.setMatchedBraceBackgroundColor(QColor("#c6c6c6"))

        # Folding
        self.setFolding(QsciScintilla.PlainFoldStyle)
        self.setFoldMarginColors(QColor("#f4f4f4"),QColor("#f4f4f4"))
        #self.setWrapMode(QsciScintilla.WrapWord)

        ## Edge Mode
        self.setEdgeMode(QsciScintilla.EdgeLine)
        self.setEdgeColumn(80)
        self.setEdgeColor(QColor("#FF0000"))

        #self.setWrapMode(QsciScintilla.WrapCharacter)
        self.setWhitespaceVisibility(QsciScintilla.WsVisibleAfterIndent)
        #self.SendScintilla(QsciScintilla.SCI_SETHSCROLLBAR, 0)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        self.settingsEditor()

        # Annotations
        self.setAnnotationDisplay(QsciScintilla.ANNOTATION_BOXED)

        # Indentation
        self.setAutoIndent(True)
        self.setIndentationsUseTabs(False)
        self.setIndentationWidth(4)
        self.setTabIndents(True)
        self.setBackspaceUnindents(True)
        self.setTabWidth(4)
        self.setIndentationGuides(True)

        ## Disable command key
        ctrl, shift = self.SCMOD_CTRL<<16, self.SCMOD_SHIFT<<16
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('L')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('T')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('D')+ ctrl)
        self.SendScintilla(QsciScintilla.SCI_CLEARCMDKEY, ord('L')+ ctrl+shift)

        ## New QShortcut = ctrl+space/ctrl+alt+space for Autocomplete
        self.newShortcutCS = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_Space), self)
        self.newShortcutCS.setContext(Qt.WidgetShortcut)
        self.redoScut = QShortcut(QKeySequence(Qt.CTRL + Qt.SHIFT + Qt.Key_Z), self)
        self.redoScut.setContext(Qt.WidgetShortcut)
        self.redoScut.activated.connect(self.redo)
        self.newShortcutCS.activated.connect(self.autoCompleteKeyBinding)
        self.runScut = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_E), self)
        self.runScut.setContext(Qt.WidgetShortcut)
        self.runScut.activated.connect(self.runSelectedCode)
        self.runScriptScut = QShortcut(QKeySequence(Qt.SHIFT + Qt.CTRL + Qt.Key_E), self)
        self.runScriptScut.setContext(Qt.WidgetShortcut)
        self.runScriptScut.activated.connect(self.runScriptCode)

        self.syntaxCheckScut = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_4), self)
        self.syntaxCheckScut.setContext(Qt.WidgetShortcut)
        self.syntaxCheckScut.activated.connect(self.syntaxCheck)
        self.commentScut = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_3), self)
        self.commentScut.setContext(Qt.WidgetShortcut)
        self.commentScut.activated.connect(self.parent.pc.commentCode)
        self.uncommentScut = QShortcut(QKeySequence(Qt.SHIFT + Qt.CTRL + Qt.Key_3), self)
        self.uncommentScut.setContext(Qt.WidgetShortcut)
        self.uncommentScut.activated.connect(self.parent.pc.uncommentCode)
        self.modificationChanged.connect(self.parent.modified)
        self.modificationAttempted.connect(self.fileReadOnly)

    def settingsEditor(self):
        # Set Python lexer
        self.setLexers()
        threshold = self.settings.value("pythonConsole/autoCompThresholdEditor", 2).toInt()[0]
        radioButtonSource = self.settings.value("pythonConsole/autoCompleteSourceEditor", 'fromAPI').toString()
        autoCompEnabled = self.settings.value("pythonConsole/autoCompleteEnabledEditor", True).toBool()
        self.setAutoCompletionThreshold(threshold)
        if autoCompEnabled:
            if radioButtonSource == 'fromDoc':
                self.setAutoCompletionSource(self.AcsDocument)
            elif radioButtonSource == 'fromAPI':
                self.setAutoCompletionSource(self.AcsAPIs)
            elif radioButtonSource == 'fromDocAPI':
                self.setAutoCompletionSource(self.AcsAll)
        else:
            self.setAutoCompletionSource(self.AcsNone)

    def autoCompleteKeyBinding(self):
        radioButtonSource = self.settings.value("pythonConsole/autoCompleteSourceEditor").toString()
        autoCompEnabled = self.settings.value("pythonConsole/autoCompleteEnabledEditor").toBool()
        if autoCompEnabled:
            if radioButtonSource == 'fromDoc':
                self.autoCompleteFromDocument()
            elif radioButtonSource == 'fromAPI':
                self.autoCompleteFromAPIs()
            elif radioButtonSource == 'fromDocAPI':
                self.autoCompleteFromAll()

    def setLexers(self):
        from qgis.core import QgsApplication

        self.lexer = QsciLexerPython()
        self.lexer.setIndentationWarning(QsciLexerPython.Inconsistent)
        self.lexer.setFoldComments(True)
        self.lexer.setFoldQuotes(True)

        loadFont = self.settings.value("pythonConsole/fontfamilytextEditor", "Monospace").toString()
        fontSize = self.settings.value("pythonConsole/fontsizeEditor", 10).toInt()[0]

        font = QFont(loadFont)
        font.setFixedPitch(True)
        font.setPointSize(fontSize)
        font.setStyleHint(QFont.TypeWriter)
        font.setStretch(QFont.SemiCondensed)
        font.setLetterSpacing(QFont.PercentageSpacing, 87.0)
        font.setBold(False)

        self.lexer.setDefaultFont(font)
        self.lexer.setColor(Qt.red, 1)
        self.lexer.setColor(Qt.darkGreen, 5)
        self.lexer.setColor(Qt.darkBlue, 15)
        self.lexer.setFont(font, 1)
        self.lexer.setFont(font, 3)
        self.lexer.setFont(font, 4)

        self.api = QsciAPIs(self.lexer)
        chekBoxAPI = self.settings.value("pythonConsole/preloadAPI", True).toBool()
        if chekBoxAPI:
            self.api.loadPrepared( QgsApplication.pkgDataPath() + "/python/qsci_apis/pyqgis_master.pap" )
        else:
            apiPath = self.settings.value("pythonConsole/userAPI").toStringList()
            for i in range(0, len(apiPath)):
                self.api.load(QString(unicode(apiPath[i])))
            self.api.prepare()
            self.lexer.setAPIs(self.api)

        self.setLexer(self.lexer)

    def move_cursor_to_end(self):
        """Move cursor to end of text"""
        line, index = self.get_end_pos()
        self.setCursorPosition(line, index)
        self.ensureCursorVisible()
        self.ensureLineVisible(line)

    def get_end_pos(self):
        """Return (line, index) position of the last character"""
        line = self.lines() - 1
        return (line, self.text(line).length())

    def contextMenuEvent(self, e):
        menu = QMenu(self)
        iconRun = QgsApplication.getThemeIcon("console/iconRunConsole.png")
        iconRunScript = QgsApplication.getThemeIcon("console/iconRunScriptConsole.png")
        iconCodePad = QgsApplication.getThemeIcon("console/iconCodepadConsole.png")
        #iconNewEditor = QgsApplication.getThemeIcon("console/iconTabEditorConsole.png")
        iconCommentEditor = QgsApplication.getThemeIcon("console/iconCommentEditorConsole.png")
        iconUncommentEditor = QgsApplication.getThemeIcon("console/iconUncommentEditorConsole.png")
        iconSettings = QgsApplication.getThemeIcon("console/iconSettingsConsole.png")
        iconFind = QgsApplication.getThemeIcon("console/iconSearchEditorConsole.png")
        iconSyntaxCk = QgsApplication.getThemeIcon("console/iconSyntaxErrorConsole.png")
        hideEditorAction = menu.addAction("Hide Editor",
                                     self.hideEditor)
#         menu.addSeparator()
#         newTabAction = menu.addAction(iconNewEditor,
#                                     "New Tab",
#                                     self.parent.newTab, 'Ctrl+T')
#         closeTabAction = menu.addAction("Close Tab",
#                                     self.parent.close, 'Ctrl+W')
        menu.addSeparator()
        syntaxCheck = menu.addAction(iconSyntaxCk, "Check Syntax",
                                     self.syntaxCheck, 'Ctrl+4')
        menu.addSeparator()
        runSelected = menu.addAction(iconRun,
                                     "Enter selected",
                                     self.runSelectedCode, 'Ctrl+E')
        runScript = menu.addAction(iconRunScript,
                                   "Run Script",
                                   self.runScriptCode, 'Shift+Ctrl+E')
        menu.addSeparator()
        undoAction = menu.addAction("Undo", self.undo, QKeySequence.Undo)
        redoAction = menu.addAction("Redo", self.redo, 'Ctrl+Shift+Z')
        menu.addSeparator()
        findAction = menu.addAction(iconFind,
                                    "Find Text",
                                    self.showFindWidget)
        menu.addSeparator()
        cutAction = menu.addAction("Cut",
                                    self.cut,
                                    QKeySequence.Cut)
        copyAction = menu.addAction("Copy",
                                    self.copy,
                                    QKeySequence.Copy)
        pasteAction = menu.addAction("Paste", self.paste, QKeySequence.Paste)
        menu.addSeparator()
        commentCodeAction = menu.addAction(iconCommentEditor, "Comment",
                                           self.parent.pc.commentCode, 'Ctrl+3')
        uncommentCodeAction = menu.addAction(iconUncommentEditor, "Uncomment",
                                             self.parent.pc.uncommentCode,
                                             'Shift+Ctrl+3')
        menu.addSeparator()
        codePadAction = menu.addAction(iconCodePad,
                                        "Share on codepad",
                                        self.codepad)
        menu.addSeparator()
        showCodeInspection = menu.addAction("Hide/Show Object list",
                                            self.objectListEditor)
        menu.addSeparator()
        selectAllAction = menu.addAction("Select All",
                                         self.selectAll,
                                         QKeySequence.SelectAll)
        menu.addSeparator()
        settingsDialog = menu.addAction(iconSettings,
                                        "Settings",
                                        self.parent.pc.openSettings)
        syntaxCheck.setEnabled(False)
        pasteAction.setEnabled(False)
        codePadAction.setEnabled(False)
        cutAction.setEnabled(False)
        runSelected.setEnabled(False)
        copyAction.setEnabled(False)
        selectAllAction.setEnabled(False)
#         closeTabAction.setEnabled(False)
        undoAction.setEnabled(False)
        redoAction.setEnabled(False)
#         if self.parent.tw.count() > 1:
#             closeTabAction.setEnabled(True)
        if self.hasSelectedText():
            runSelected.setEnabled(True)
            copyAction.setEnabled(True)
            cutAction.setEnabled(True)
            codePadAction.setEnabled(True)
        if not self.text() == '':
            selectAllAction.setEnabled(True)
            syntaxCheck.setEnabled(True)
        if self.isUndoAvailable():
            undoAction.setEnabled(True)
        if self.isRedoAvailable():
            redoAction.setEnabled(True)
        if QApplication.clipboard().text():
            pasteAction.setEnabled(True)
        action = menu.exec_(self.mapToGlobal(e.pos()))

    def findText(self, direction=False):
        line, index = self.getCursorPosition()
        text = self.parent.pc.lineEditFind.text()
        msgText = False
        if not text.isEmpty():
            if direction:
                if not self.findFirst(text, 1, 0, line, index, forward=False):
                    msgText = True
            else:
                if not self.findFirst(text, 1, 0, line, index):
                    msgText = True
            if msgText:
                msgText = QCoreApplication.translate('PythonConsole',
                                                     '<b>"%1"</b> was not found.').arg(text)
                self.parent.pc.callWidgetMessageBarEditor(msgText, 0, True)

    def objectListEditor(self):
        listObj = self.parent.pc.listClassMethod
        if listObj.isVisible():
             listObj.hide()
             self.parent.pc.objectListButton.setChecked(False)
        else:
            listObj.show()
            self.parent.pc.objectListButton.setChecked(True)

    def codepad(self):
        import urllib2, urllib
        listText = self.selectedText().split('\n')
        getCmd = []
        for strLine in listText:
            getCmd.append(unicode(strLine))
        pasteText= u"\n".join(getCmd)
        url = 'http://codepad.org'
        values = {'lang' : 'Python',
                  'code' : pasteText,
                  'submit':'Submit'}
        try:
            response = urllib2.urlopen(url, urllib.urlencode(values))
            url = response.read()
            for href in url.split("</a>"):
                if "Link:" in href:
                    ind=href.index('Link:')
                    found = href[ind+5:]
                    for i in found.split('">'):
                        if '<a href=' in i:
                             link = i.replace('<a href="',"").strip()
            if link:
                QApplication.clipboard().setText(link)
                msgText = QCoreApplication.translate('PythonConsole', 'URL copied to clipboard.')
                self.parent.pc.callWidgetMessageBarEditor(msgText, 0, True)
        except urllib2.URLError, e:
            msgText = QCoreApplication.translate('PythonConsole', 'Connection error: ')
            self.parent.pc.callWidgetMessageBarEditor(msgText + str(e.args), 0, True)

    def hideEditor(self):
        self.parent.pc.splitterObj.hide()
        self.parent.pc.showEditorButton.setChecked(False)

    def showFindWidget(self):
        wF = self.parent.pc.widgetFind
        if wF.isVisible():
            wF.hide()
            self.parent.pc.findTextButton.setChecked(False)
        else:
            wF.show()
            self.parent.pc.findTextButton.setChecked(True)

    def commentEditorCode(self, commentCheck):
        self.beginUndoAction()
        if self.hasSelectedText():
            startLine, _, endLine, _ = self.getSelection()
            for line in range(startLine, endLine + 1):
                if commentCheck:
                    self.insertAt('#', line, 0)
                else:
                    if not self.text(line).trimmed().startsWith('#'):
                        continue
                    self.setSelection(line, self.indentation(line),
                                      line, self.indentation(line) + 1)
                    self.removeSelectedText()
        else:
            line, pos = self.getCursorPosition()
            if commentCheck:
                self.insertAt('#', line, 0)
            else:
                if not self.text(line).trimmed().startsWith('#'):
                    return
                self.setSelection(line, self.indentation(line),
                                  line, self.indentation(line) + 1)
                self.removeSelectedText()
        self.endUndoAction()

    def createTempFile(self):
        import tempfile
        fd, path = tempfile.mkstemp()
        tmpFileName = path + '.py'
        with open(path, "w") as f:
            f.write(self.text())
        os.close(fd)
        os.rename(path, tmpFileName)
        return tmpFileName

    def _runSubProcess(self, filename, tmp=False):
        dir, name = os.path.split(unicode(filename))
        if dir not in sys.path:
            sys.path.append(dir)
        try:
            ## set creationflags for running command without shell window
            if sys.platform.startswith('win'):
                p = subprocess.Popen(['python', str(filename)], shell=False, stdin=subprocess.PIPE,
                                     stderr=subprocess.PIPE, stdout=subprocess.PIPE, creationflags=0x08000000)
            else:
                p = subprocess.Popen(['python', str(filename)], shell=False, stdin=subprocess.PIPE,
                                     stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            out, _traceback = p.communicate()

            ## Fix interrupted system call on OSX
            if sys.platform == 'darwin':
                status = None
                while status is None:
                    try:
                        status = p.wait()
                    except OSError, e:
                        if e.errno == 4:
                            pass
                        else:
                            raise e
            if tmp:
                tmpFileTr = QCoreApplication.translate('PythonConsole', ' [Temporary file saved in ')
                name = name + tmpFileTr + dir + ']'
            if _traceback:
                msgTraceTr = QCoreApplication.translate('PythonConsole', '## Script error: %1').arg(name)
                print "## %s" % datetime.datetime.now()
                print msgTraceTr
                sys.stderr.write(_traceback)
                p.stderr.close()
            else:
                msgSuccessTr = QCoreApplication.translate('PythonConsole',
                                                          '## Script executed successfully: %1').arg(name)
                print "## %s" % datetime.datetime.now()
                print msgSuccessTr
                sys.stdout.write(out)
                p.stdout.close()
            del p
            if tmp:
                os.remove(filename)
        except IOError, error:
            IOErrorTr = QCoreApplication.translate('PythonConsole',
                                                   'Cannot execute file %1. Error: %2\n') \
                                                   .arg(unicode(filename)).arg(error.strerror)
            print '## Error: ' + IOErrorTr
        except:
            s = traceback.format_exc()
            print '## Error: '
            sys.stderr.write(s)

    def runScriptCode(self):
        autoSave = self.settings.value("pythonConsole/autoSaveScript").toBool()

        tabWidget = self.parent.tw.currentWidget()

        filename = tabWidget.path

        msgEditorBlank = QCoreApplication.translate('PythonConsole',
                                                    'Hey, type something for running !')
        msgEditorUnsaved = QCoreApplication.translate('PythonConsole',
                                                      'You have to save the file before running.')
        if not autoSave:
            if filename is None:
                if not self.isModified():
                    self.parent.pc.callWidgetMessageBarEditor(msgEditorBlank, 0, True)
                    return
                else:
                    self.parent.pc.callWidgetMessageBarEditor(msgEditorUnsaved, 0, True)
                    return
            if self.isModified():
                self.parent.pc.callWidgetMessageBarEditor(msgEditorUnsaved, 0, True)
                return
            else:
                if self.syntaxCheck(fromContextMenu=False):
                    self._runSubProcess(filename)
        else:
            if self.syntaxCheck(fromContextMenu=False):
                tmpFile = self.createTempFile()
                self._runSubProcess(tmpFile, True)

    def runSelectedCode(self):
        cmd = self.selectedText()
        self.parent.pc.shell.insertFromDropPaste(cmd)
        self.parent.pc.shell.entered()
        self.setFocus()

    def getTextFromEditor(self):
        text = self.text()
        textList = text.split("\n")
        return textList

    def goToLine(self, objName, linenr):
        self.SendScintilla(QsciScintilla.SCI_GOTOLINE, linenr-1)
        self.SendScintilla(QsciScintilla.SCI_SETTARGETSTART,
                           self.SendScintilla(QsciScintilla.SCI_GETCURRENTPOS))
        self.SendScintilla(QsciScintilla.SCI_SETTARGETEND, len(self.text()))
        pos = self.SendScintilla(QsciScintilla.SCI_SEARCHINTARGET, len(objName), objName)
        index = pos - self.SendScintilla(QsciScintilla.SCI_GETCURRENTPOS)
        #line, _ = self.getCursorPosition()
        self.setSelection(linenr - 1, index, linenr - 1, index + len(objName))
        self.ensureLineVisible(linenr)
        self.setFocus()

    def syntaxCheck(self, filename=None, fromContextMenu=True):
        eline = None
        ecolumn = 0
        edescr = ''
        source = unicode(self.text())
        try:
            if not filename:
                filename = self.parent.tw.currentWidget().path
            #source = open(filename, 'r').read() + '\n'
            if type(source) == type(u""):
                source = source.encode('utf-8')
            compile(source, str(filename), 'exec')
        except SyntaxError, detail:
            s = traceback.format_exception_only(SyntaxError, detail)
            fn = detail.filename
            eline = detail.lineno and detail.lineno or 1
            ecolumn = detail.offset and detail.offset or 1
            edescr = detail.msg
        if eline != None:
            eline -= 1
            for markerLine in self.bufferMarkerLine:
                self.markerDelete(markerLine)
                self.clearAnnotations(markerLine)
                self.bufferMarkerLine.remove(markerLine)
            if (eline) not in self.bufferMarkerLine:
                self.bufferMarkerLine.append(eline)
            self.markerAdd(eline, self.MARKER_NUM)
            loadFont = self.settings.value("pythonConsole/fontfamilytextEditor",
                                           "Monospace").toString()
            styleAnn = QsciStyle(-1,"Annotation",
                                 QColor(255,0,0),
                                 QColor(255,200,0),
                                 QFont(loadFont, 8,-1,True),
                                 True)
            self.annotate(eline, edescr, styleAnn)
            self.setCursorPosition(eline, ecolumn-1)
            #self.setSelection(eline, ecolumn, eline, self.lineLength(eline)-1)
            self.ensureLineVisible(eline)
            #self.ensureCursorVisible()
            return False
        else:
            self.markerDeleteAll()
            self.clearAnnotations()
            if fromContextMenu:
                msgText = QCoreApplication.translate('PythonConsole', 'Syntax ok')
                self.parent.pc.callWidgetMessageBarEditor(msgText, 0, True)
            return True

    def keyPressEvent(self, e):
        t = unicode(e.text())
        ## Close bracket automatically
        if t in self.opening:
            i = self.opening.index(t)
            self.insert(self.closing[i])
        QsciScintilla.keyPressEvent(self, e)

    def focusInEvent(self, e):
        pathfile = self.parent.path
        if pathfile:
            if not os.path.exists(pathfile):
                msgText = QCoreApplication.translate('PythonConsole',
                                                     'The file <b>"%1"</b> has been deleted or is not accessible') \
                                                     .arg(unicode(pathfile))
                self.parent.pc.callWidgetMessageBarEditor(msgText, 2, False)
                return
        if pathfile and self.lastModified != QFileInfo(pathfile).lastModified():
            self.beginUndoAction()
            self.selectAll()
            #fileReplaced = self.selectedText()
            self.removeSelectedText()
            file = open(pathfile, "r")
            fileLines = file.readlines()
            file.close()
            QApplication.setOverrideCursor(Qt.WaitCursor)
            for line in reversed(fileLines):
                self.insert(line)
            QApplication.restoreOverrideCursor()
            self.setModified(True)
            self.endUndoAction()

            self.parent.tw.listObject(self.parent.tw.currentWidget())
            self.lastModified = QFileInfo(pathfile).lastModified()
            msgText = QCoreApplication.translate('PythonConsole',
                                                 'The file <b>"%1"</b> has been changed and reloaded') \
                                                 .arg(unicode(pathfile))
            self.parent.pc.callWidgetMessageBarEditor(msgText, 1, False)
        QsciScintilla.focusInEvent(self, e)

    def fileReadOnly(self):
        tabWidget = self.parent.tw.currentWidget()
        msgText = QCoreApplication.translate('PythonConsole',
                                             'The file <b>"%1"</b> is read only, please save to different file first.') \
                                             .arg(unicode(tabWidget.path))
        self.parent.pc.callWidgetMessageBarEditor(msgText, 1, False)

class EditorTab(QWidget):
    def __init__(self, parent, parentConsole, filename, readOnly):
        super(EditorTab, self).__init__(parent)
        self.tw = parent
        self.pc = parentConsole
        self.path = None
        self.readOnly = readOnly

        self.fileExcuteList = {}
        self.fileExcuteList = dict()

        self.newEditor = Editor(self)
        if filename:
            self.path = filename
            if os.path.exists(filename):
                self.loadFile(filename, False)

        # Creates layout for message bar
        self.layout = QGridLayout(self.newEditor)
        self.layout.setContentsMargins(0, 0, 0, 0)
        spacerItem = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)
        self.layout.addItem(spacerItem, 1, 0, 1, 1)
        # messageBar instance
        self.infoBar = QgsMessageBar()
        sizePolicy = QSizePolicy(QSizePolicy.Minimum, QSizePolicy.Fixed)
        self.infoBar.setSizePolicy(sizePolicy)
        self.layout.addWidget(self.infoBar, 0, 0, 1, 1)

        self.tabLayout = QGridLayout(self)
        self.tabLayout.setContentsMargins(0, 0, 0, 0)
        self.tabLayout.addWidget(self.newEditor)

        self.keyFilter = KeyFilter(parent, self)
        self.setEventFilter(self.keyFilter)

    def loadFile(self, filename, modified):
        fn = open(unicode(filename), "rb")
        txt = fn.read()
        fn.close()
        QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
        self.newEditor.setText(txt)
        if self.readOnly:
            self.newEditor.setReadOnly(self.readOnly)
        QApplication.restoreOverrideCursor()
        self.newEditor.setModified(modified)
        self.newEditor.lastModified = QFileInfo(filename).lastModified()
        self.newEditor.recolor()

    def save(self, fileName=None):
        if fileName:
            self.path = fileName
        if self.path is None:
            index = self.tw.currentIndex()
            saveTr = QCoreApplication.translate('PythonConsole',
                                                'Python Console: Save file')
            self.path = str(QFileDialog().getSaveFileName(self,
                                                          saveTr,
                                                          self.tw.tabText(index) + '.py',
                                                          "Script file (*.py)"))
            # If the user didn't select a file, abort the save operation
            if len(self.path) == 0:
                self.path = None
                return
            msgText = QCoreApplication.translate('PythonConsole',
                                                 'Script was correctly saved.')
            self.pc.callWidgetMessageBarEditor(msgText, 0, True)
        # Rename the original file, if it exists
        path = unicode(self.path)
        overwrite = os.path.exists(path)
        if overwrite:
            try:
                permis = os.stat(path).st_mode
                #self.newEditor.lastModified = QFileInfo(path).lastModified()
                os.chmod(path, permis)
            except:
                raise

            temp_path = path + "~"
            if os.path.exists(temp_path):
                os.remove(temp_path)
            os.rename(path, temp_path)
        # Save the new contents
        with open(path, "w") as f:
            f.write(self.newEditor.text())
        if overwrite:
            os.remove(temp_path)
        if self.newEditor.isReadOnly():
            self.newEditor.setReadOnly(False)
        fN = path.split('/')[-1]
        self.tw.setTabTitle(self.tw.currentIndex(), fN)
        self.tw.setTabToolTip(self.tw.currentIndex(), path)
        self.newEditor.setModified(False)
        self.pc.saveFileButton.setEnabled(False)
        self.newEditor.lastModified = QFileInfo(path).lastModified()
        self.pc.updateTabListScript(path, action='append')
        self.tw.listObject(self)

    def modified(self, modified):
        self.tw.tabModified(self, modified)

    def close(self):
        self.tw._removeTab(self, tab2index=True)

    def setEventFilter(self, filter):
        self.newEditor.installEventFilter(filter)

    def newTab(self):
        self.tw.newTabEditor()

class EditorTabWidget(QTabWidget):
    def __init__(self, parent):
        QTabWidget.__init__(self, parent=None)
        self.parent = parent

        self.idx = -1
        # Layout for top frame (restore tabs)
        self.layoutTopFrame = QGridLayout(self)
        self.layoutTopFrame.setContentsMargins(0, 0, 0, 0)
        spacerItem = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)
        self.layoutTopFrame.addItem(spacerItem, 1, 0, 1, 1)
        self.topFrame = QFrame(self)
        self.topFrame.setStyleSheet('background-color: rgb(255, 255, 230);')
        self.topFrame.setFrameShape(QFrame.StyledPanel)
        self.topFrame.setMinimumHeight(24)
        self.layoutTopFrame2 = QGridLayout(self.topFrame)
        self.layoutTopFrame2.setContentsMargins(0, 0, 0, 0)
        label = QCoreApplication.translate("PythonConsole",
                                           "Click on button to restore all tabs from last session.")
        self.label = QLabel(label)

        self.restoreTabsButton = QToolButton()
        toolTipRestore = QCoreApplication.translate("PythonConsole",
                                                    "Restore tabs")
        self.restoreTabsButton.setToolTip(toolTipRestore)
        self.restoreTabsButton.setIcon(QgsApplication.getThemeIcon("console/iconRestoreTabsConsole.png"))
        self.restoreTabsButton.setIconSize(QSize(24, 24))
        self.restoreTabsButton.setAutoRaise(True)
        self.restoreTabsButton.setCursor(Qt.PointingHandCursor)
        self.restoreTabsButton.setStyleSheet('QToolButton:hover{border: none } \
                                              QToolButton:pressed{border: none}')

        self.clButton = QToolButton()
        toolTipClose = QCoreApplication.translate("PythonConsole",
                                                  "Close")
        self.clButton.setToolTip(toolTipClose)
        self.clButton.setIcon(QgsApplication.getThemeIcon("mIconClose.png"))
        self.clButton.setIconSize(QSize(18, 18))
        self.clButton.setCursor(Qt.PointingHandCursor)
        self.clButton.setStyleSheet('QToolButton:hover{border: none } \
                                     QToolButton:pressed{border: none}')
        self.clButton.setAutoRaise(True)

        sizePolicy = QSizePolicy(QSizePolicy.Minimum, QSizePolicy.Fixed)
        self.topFrame.setSizePolicy(sizePolicy)
        self.layoutTopFrame.addWidget(self.topFrame, 0, 0, 1, 1)
        self.layoutTopFrame2.addWidget(self.label, 0, 1, 1, 1)
        self.layoutTopFrame2.addWidget(self.restoreTabsButton, 0, 0, 1, 1)
        self.layoutTopFrame2.addWidget(self.clButton, 0, 2, 1, 1)

        self.topFrame.hide()
        self.connect(self.restoreTabsButton, SIGNAL('clicked()'), self.restoreTabs)
        self.connect(self.clButton, SIGNAL('clicked()'), self.closeRestore)

        # Restore script of the previuos session
        self.settings = QSettings()
        tabScripts = self.settings.value("pythonConsole/tabScripts")
        self.restoreTabList = tabScripts.toList()

        if self.restoreTabList:
            self.topFrame.show()
        else:
            self.newTabEditor(filename=None)

        ## Fixes #7653
        if sys.platform != 'darwin':
            self.setDocumentMode(True)

        self.setMovable(True)
        self.setTabsClosable(True)
        self.setTabPosition(QTabWidget.North)

        # Menu button list tabs
        self.fileTabMenu = QMenu()
        self.connect(self.fileTabMenu, SIGNAL("aboutToShow()"),
                     self.showFileTabMenu)
        self.connect(self.fileTabMenu, SIGNAL("triggered(QAction*)"),
                     self.showFileTabMenuTriggered)
        self.fileTabButton = QToolButton()
        txtToolTipMenuFile = QCoreApplication.translate("PythonConsole",
                                                        "List all tabs")
        self.fileTabButton.setToolTip(txtToolTipMenuFile)
        self.fileTabButton.setIcon(QgsApplication.getThemeIcon("console/iconFileTabsMenuConsole.png"))
        self.fileTabButton.setIconSize(QSize(24, 24))
        self.fileTabButton.setAutoRaise(True)
        self.fileTabButton.setPopupMode(QToolButton.InstantPopup)
        self.fileTabButton.setMenu(self.fileTabMenu)
        self.setCornerWidget(self.fileTabButton, Qt.TopRightCorner)
        self.connect(self, SIGNAL("tabCloseRequested(int)"), self._removeTab)
        self.connect(self, SIGNAL('currentChanged(int)'), self._currentWidgetChanged)

        # New Editor button
        self.newTabButton = QToolButton()
        txtToolTipNewTab = QCoreApplication.translate("PythonConsole",
                                                      "New Editor")
        self.newTabButton.setToolTip(txtToolTipNewTab)
        self.newTabButton.setAutoRaise(True)
        self.newTabButton.setIcon(QgsApplication.getThemeIcon("console/iconNewTabEditorConsole.png"))
        self.newTabButton.setIconSize(QSize(24, 24))
        self.setCornerWidget(self.newTabButton, Qt.TopLeftCorner)
        self.connect(self.newTabButton, SIGNAL('clicked()'), self.newTabEditor)

    def _currentWidgetChanged(self, tab):
        self.listObject(tab)
        self.changeLastDirPath(tab)
        self.enableSaveIfModified(tab)

    def contextMenuEvent(self, e):
        tabBar = self.tabBar()
        self.idx = tabBar.tabAt(e.pos())
        if self.widget(self.idx):
            cW = self.widget(self.idx)
            menu = QMenu(self)
            menu.addSeparator()
            newTabAction = menu.addAction("New Editor",
                                          self.newTabEditor)
            menu.addSeparator()
            closeTabAction = menu.addAction("Close Tab",
                                            cW.close)
            closeAllTabAction = menu.addAction("Close All",
                                               self.closeAll)
            closeOthersTabAction = menu.addAction("Close Others",
                                                  self.closeOthers)
            menu.addSeparator()
            saveAction = menu.addAction("Save",
                                        cW.save)
            saveAsAction = menu.addAction("Save As",
                                          self.parent.saveAsScriptFile)
            closeTabAction.setEnabled(False)
            closeAllTabAction.setEnabled(False)
            closeOthersTabAction.setEnabled(False)
            saveAction.setEnabled(False)
            if self.count() > 1:
                closeTabAction.setEnabled(True)
                closeAllTabAction.setEnabled(True)
                closeOthersTabAction.setEnabled(True)
            if self.widget(self.idx).newEditor.isModified():
                saveAction.setEnabled(True)
            action = menu.exec_(self.mapToGlobal(e.pos()))

    def closeOthers(self):
        idx = self.idx
        countTab = self.count()
        for i in range(countTab - 1, idx, -1) + range(idx - 1, -1, -1):
            self._removeTab(i)

    def closeAll(self):
        countTab = self.count()
        cI = self.currentIndex()
        for i in range(countTab - 1, 0, -1):
            self._removeTab(i)
        self.newTabEditor(tabName='Untitled-0')
        self._removeTab(0)

    def enableSaveIfModified(self, tab):
        tabWidget = self.widget(tab)
        if tabWidget:
            self.parent.saveFileButton.setEnabled(tabWidget.newEditor.isModified())

    def enableToolBarEditor(self, enable):
        if self.topFrame.isVisible():
            enable = False
        self.parent.toolBarEditor.setEnabled(enable)

    def newTabEditor(self, tabName=None, filename=None):
        readOnly = False
        if filename:
            readOnly = not QFileInfo(filename).isWritable()
            try:
                fn = open(unicode(filename), "rb")
                txt = fn.read()
                fn.close()
            except IOError, error:
                IOErrorTr = QCoreApplication.translate('PythonConsole',
                                                       'The file %1 could not be opened. Error: %2\n') \
                                                        .arg(unicode(filename)).arg(error.strerror)
                print '## Error: '
                sys.stderr.write(IOErrorTr)
                return

        nr = self.count()
        if not tabName:
            tabName = QCoreApplication.translate('PythonConsole', 'Untitled-%1').arg(nr)
        self.tab = EditorTab(self, self.parent, filename, readOnly)
        self.iconTab = QgsApplication.getThemeIcon('console/iconTabEditorConsole.png')
        self.addTab(self.tab, self.iconTab, tabName + ' (ro)' if readOnly else tabName)
        self.setCurrentWidget(self.tab)
        if filename:
            self.setTabToolTip(self.currentIndex(), unicode(filename))
        else:
            self.setTabToolTip(self.currentIndex(), tabName)

    def tabModified(self, tab, modified):
        index = self.indexOf(tab)
        color = Qt.darkGray if modified else Qt.black
        self.tabBar().setTabTextColor(index, color)
        self.parent.saveFileButton.setEnabled(modified)

    def closeTab(self, tab):
        if self.count() < 2:
            self.removeTab(self.indexOf(tab))
            self.newTabEditor()
        else:
            self.removeTab(self.indexOf(tab))
        self.currentWidget().setFocus(Qt.TabFocusReason)

    def setTabTitle(self, tab, title):
        self.setTabText(tab, title)

    def _removeTab(self, tab, tab2index=False):
        if tab2index:
            tab = self.indexOf(tab)
        tabWidget = self.widget(tab)
        if tabWidget.newEditor.isModified():
            txtSaveOnRemove = QCoreApplication.translate("PythonConsole",
                                                         "Python Console: Save File")
            txtMsgSaveOnRemove = QCoreApplication.translate("PythonConsole",
                                                            "The file <b>'%1'</b> has been modified, save changes ?") \
                                                            .arg(self.tabText(tab))
            res = QMessageBox.question( self, txtSaveOnRemove,
                                        txtMsgSaveOnRemove,
                                        QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel )
            if res == QMessageBox.Save:
                tabWidget.save()
            elif res == QMessageBox.Cancel:
                return
            if tabWidget.path:
                self.parent.updateTabListScript(tabWidget.path, action='remove')
            self.removeTab(tab)
            if self.count() < 1:
                self.newTabEditor()
        else:
            if tabWidget.path:
                self.parent.updateTabListScript(tabWidget.path, action='remove')
            if self.count() <= 1:
                self.removeTab(tab)
                self.newTabEditor()
            else:
                self.removeTab(tab)
        self.currentWidget().newEditor.setFocus(Qt.TabFocusReason)

    def buttonClosePressed(self):
        self.closeCurrentWidget()

    def closeCurrentWidget(self):
        currWidget = self.currentWidget()
        if currWidget and currWidget.close():
            self.removeTab( self.currentIndex() )
            currWidget = self.currentWidget()
            if currWidget:
                currWidget.setFocus(Qt.TabFocusReason)
        if currWidget.path in self.restoreTabList:
            self.parent.updateTabListScript(currWidget.path, action='remove')

    def restoreTabs(self):
        for script in self.restoreTabList:
            pathFile = unicode(script.toString())
            if os.path.exists(pathFile):
                tabName = pathFile.split('/')[-1]
                self.newTabEditor(tabName, pathFile)
            else:
                errOnRestore = QCoreApplication.translate("PythonConsole",
                                                          "Unable to restore the file: \n%1\n") \
                                                          .arg(unicode(pathFile))
                print  '## Error: '
                s = errOnRestore
                sys.stderr.write(s)
                self.parent.updateTabListScript(pathFile, action='remove')
        if self.count() < 1:
            self.newTabEditor(filename=None)
        self.topFrame.close()
        self.enableToolBarEditor(True)
        self.currentWidget().newEditor.setFocus(Qt.TabFocusReason)

    def closeRestore(self):
        self.parent.updateTabListScript(None)
        self.topFrame.close()
        self.newTabEditor(filename=None)
        self.enableToolBarEditor(True)

    def showFileTabMenu(self):
        self.fileTabMenu.clear()
        for index in range(self.count()):
            action = self.fileTabMenu.addAction(self.tabIcon(index), self.tabText(index))
            action.setData(QVariant(index))

    def showFileTabMenuTriggered(self, action):
        index, ok = action.data().toInt()
        if ok:
            self.setCurrentIndex(index)

    def listObject(self, tab):
        self.parent.listClassMethod.clear()
        if isinstance(tab, EditorTab):
            tabWidget = self.widget(self.indexOf(tab))
        else:
            tabWidget = self.widget(tab)
        if tabWidget:
            if tabWidget.path:
                pathFile, file = os.path.split(unicode(tabWidget.path))
                module, ext = os.path.splitext(file)
                found = False
                if pathFile not in sys.path:
                    sys.path.append(pathFile)
                    found = True
                try:
                    reload(pyclbr)
                    dictObject = {}
                    superClassName = []
                    readModule = pyclbr.readmodule(module)
                    readModuleFunction = pyclbr.readmodule_ex(module)
                    for name, class_data in sorted(readModule.items(), key=lambda x:x[1].lineno):
                        if os.path.normpath(str(class_data.file)) == os.path.normpath(str(tabWidget.path)):
                            for superClass in class_data.super:
                                if superClass == 'object':
                                    continue
                                if isinstance(superClass, basestring):
                                    superClassName.append(superClass)
                                else:
                                    superClassName.append(superClass.name)
                            classItem = QTreeWidgetItem()
                            if superClassName:
                                for i in superClassName: super = i
                                classItem.setText(0, name + ' [' + super + ']')
                                classItem.setToolTip(0, name + ' [' + super + ']')
                            else:
                                classItem.setText(0, name)
                                classItem.setToolTip(0, name)
                            classItem.setText(1, str(class_data.lineno))
                            iconClass = QgsApplication.getThemeIcon("console/iconClassTreeWidgetConsole.png")
                            classItem.setIcon(0, iconClass)
                            dictObject[name] = class_data.lineno
                            for meth, lineno in sorted(class_data.methods.items(), key=itemgetter(1)):
                                methodItem = QTreeWidgetItem()
                                methodItem.setText(0, meth + ' ')
                                methodItem.setText(1, str(lineno))
                                methodItem.setToolTip(0, meth)
                                iconMeth = QgsApplication.getThemeIcon("console/iconMethodTreeWidgetConsole.png")
                                methodItem.setIcon(0, iconMeth)
                                classItem.addChild(methodItem)
                                dictObject[meth] = lineno
                            self.parent.listClassMethod.addTopLevelItem(classItem)
                    for func_name, data in sorted(readModuleFunction.items(), key=lambda x:x[1].lineno):
                        if isinstance(data, pyclbr.Function) and \
                            os.path.normpath(str(data.file)) == os.path.normpath(str(tabWidget.path)):
                            funcItem = QTreeWidgetItem()
                            funcItem.setText(0, func_name + ' ')
                            funcItem.setText(1, str(data.lineno))
                            funcItem.setToolTip(0, func_name)
                            iconFunc = QgsApplication.getThemeIcon("console/iconFunctionTreeWidgetConsole.png")
                            funcItem.setIcon(0, iconFunc)
                            dictObject[func_name] = data.lineno
                            self.parent.listClassMethod.addTopLevelItem(funcItem)
                    if found:
                        sys.path.remove(pathFile)
                except:
                    msgItem = QTreeWidgetItem()
                    msgItem.setText(0, QCoreApplication.translate("PythonConsole", "Check Syntax"))
                    msgItem.setText(1, 'syntaxError')
                    iconWarning = QgsApplication.getThemeIcon("console/iconSyntaxErrorConsole.png")
                    msgItem.setIcon(0, iconWarning)
                    self.parent.listClassMethod.addTopLevelItem(msgItem)
                    #s = traceback.format_exc()
                    #print '## Error: '
                    #sys.stderr.write(s)
                    #pass

    def refreshSettingsEditor(self):
        countTab = self.count()
        for i in range(countTab):
            self.widget(i).newEditor.settingsEditor()

    def changeLastDirPath(self, tab):
        tabWidget = self.widget(tab)
        if tabWidget:
            self.settings.setValue("pythonConsole/lastDirPath", QVariant(tabWidget.path))

    def widgetMessageBar(self, iface, text, level, timed=True):
        messageLevel = [QgsMessageBar.INFO, QgsMessageBar.WARNING, QgsMessageBar.CRITICAL]
        if timed:
            timeout = iface.messageTimeout()
        else:
            timeout = 0
        currWidget = self.currentWidget()
        currWidget.infoBar.pushMessage(text, messageLevel[level], timeout)
