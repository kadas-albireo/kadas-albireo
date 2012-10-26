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
from PyQt4.Qsci import (QsciScintilla,
                        QsciScintillaBase, 
                        QsciLexerPython, 
                        QsciAPIs)

import sys
import os
import traceback
import code

_init_commands = ["from qgis.core import *", "import qgis.utils"]
_historyFile = os.path.join(str(QDir.homePath()),".qgis","console_history.txt")

class PythonEdit(QsciScintilla, code.InteractiveInterpreter):
    def __init__(self, parent=None):
        #QsciScintilla.__init__(self, parent)
        super(PythonEdit,self).__init__(parent)
        code.InteractiveInterpreter.__init__(self, locals=None)
        
        # Enable non-ascii chars for editor
        self.setUtf8(True)
        
        self.new_input_line = True 
        
        self.setMarginWidth(0, 0)
        self.setMarginWidth(1, 0)
        self.setMarginWidth(2, 0)
        
        self.buffer = []
        
        #self.insertInitText()
        self.displayPrompt(False)
        
        for line in _init_commands:
            self.runsource(line)
            
        self.history = QStringList()
        self.historyIndex = 0
        # Read history command file
        self.readHistoryFile()
          
        # Brace matching: enable for a brace immediately before or after
        # the current position
        self.setBraceMatching(QsciScintilla.SloppyBraceMatch)
        #self.moveToMatchingBrace()
        #self.selectToMatchingBrace()
        
        # Current line visible with special background color
        #self.setCaretLineVisible(True)
        #self.setCaretLineBackgroundColor(QColor("#ffe4e4"))
        self.setCaretWidth(2)
    
        # Set Python lexer
        # Set style for Python comments (style number 1) to a fixed-width
        # courier.
        self.setLexers()
               
        # Indentation
        #self.setAutoIndent(True)
        #self.setIndentationsUseTabs(False)
        #self.setIndentationWidth(4)
        #self.setTabIndents(True)
        #self.setBackspaceUnindents(True)
        #self.setTabWidth(4)

        self.setAutoCompletionThreshold(2)
        self.setAutoCompletionSource(self.AcsAPIs)

        # Don't want to see the horizontal scrollbar at all
        # Use raw message to Scintilla here (all messages are documented
        # here: http://www.scintilla.org/ScintillaDoc.html)
        self.SendScintilla(QsciScintilla.SCI_SETHSCROLLBAR, 0)
        self.SendScintilla(QsciScintilla.SCI_SETVSCROLLBAR, 0)

    
        # not too small
        #self.setMinimumSize(500, 300)
        self.setMinimumHeight(32)
        
        self.SendScintilla(QsciScintilla.SCI_SETWRAPMODE, 2)
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
        self.newShortcutCS = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_Space), self)
        self.newShortcutCAS = QShortcut(QKeySequence(Qt.CTRL + Qt.ALT + Qt.Key_Space), self)
        self.newShortcutCS.activated.connect(self.autoComplete)
        self.newShortcutCAS.activated.connect(self.showHistory)
        self.connect(self, SIGNAL('userListActivated(int, const QString)'),
                     self.completion_list_selected)
        
        self.createStandardContextMenu()
    
    def showHistory(self):
        self.showUserList(1, QStringList(self.history))
    
    def autoComplete(self):
        self.autoCompleteFromAll()
       
    def clearConsole(self):
        """Clear the contents of the console."""
        self.SendScintilla(QsciScintilla.SCI_CLEARALL)
        #self.setText('')
        #self.insertInitText()
        self.displayPrompt(False)
        self.setFocus()
        
    def commandConsole(self, command):
        if not self.is_cursor_on_last_line():
            self.move_cursor_to_end()
        line, pos = self.getCursorPosition()
        selCmdLenght = self.text(line).length()
        self.setSelection(line, 4, line, selCmdLenght)
        self.removeSelectedText()
        if command == "iface":
            """Import QgisInterface class"""
            self.append('from qgis.utils import iface')
            self.move_cursor_to_end()
        elif command == "sextante":
            """Import Sextante class"""
            self.append('from sextante.core.Sextante import Sextante')
            self.move_cursor_to_end()
        elif command == "cLayer":
            """Retrieve current Layer from map camvas"""
            self.append('cLayer = iface.mapCanvas().currentLayer()')
            self.move_cursor_to_end()
        elif command == "qtCore":
            """Import QtCore class"""
            self.append('from PyQt4.QtCore import *')
            self.move_cursor_to_end()
        elif command == "qtGui":
            """Import QtGui class"""
            self.append('from PyQt4.QtGui import *')
            self.move_cursor_to_end()
        self.setFocus()

    def setLexers(self):
        from qgis.core import QgsApplication
        
        self.lexer = QsciLexerPython()
        settings = QSettings()
        loadFont = settings.value("pythonConsole/fontfamilytext", "Monospace").toString()
        fontSize = settings.value("pythonConsole/fontsize", 10).toInt()[0]
        
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
        chekBoxAPI = settings.value( "pythonConsole/preloadAPI" ).toBool()
        if chekBoxAPI:
            self.api.loadPrepared( QgsApplication.pkgDataPath() + "/python/qsci_apis/pyqgis_master.pap" )
        else:
            apiPath = settings.value("pythonConsole/userAPI").toStringList()
            for i in range(0, len(apiPath)):
                self.api.load(QString(unicode(apiPath[i])))        
            self.api.prepare()
            self.lexer.setAPIs(self.api)

        self.setLexer(self.lexer)
            
    ## TODO: show completion list for file and directory
    
    def completion_list_selected(self, id, txt):
        if id == 1:
            txt = unicode(txt)
            # get current cursor position 
            line, pos = self.getCursorPosition()
            selCmdLength = self.text(line).length()
            # select typed text
            self.setSelection(line, 4, line, selCmdLength)
            self.removeSelectedText()
            self.insert(txt)

    def insertInitText(self):
        #self.setLexers(False)
        txtInit = QCoreApplication.translate("PythonConsole", "## Interactive Python Console for Quantum GIS\n\n")
                                             #"## To access Quantum GIS environment from this console\n"
                                             #"## use qgis.utils.iface object (instance of QgisInterface class). Read help for more info.\n\n")
        initText = self.setText(txtInit)

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
        N = self.SendScintilla(self.SCI_GETTEXT, len, bb)
        return bytes(bb)[:-1]

    def getTextLength(self):
        return self.SendScintilla(QsciScintilla.SCI_GETLENGTH)

    def get_end_pos(self):
        """Return (line, index) position of the last character"""
        line = self.lines() - 1
        return (line, self.text(line).length())
    
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
        
#    def on_new_line(self):
#        """On new input line"""
#        self.move_cursor_to_end()
#        self.new_input_line = False
        
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
        
    def refreshLexerProperties(self):
        self.setLexers()
        
#    def check_selection(self):
#        """
#        Check if selected text is r/w,
#        otherwise remove read-only parts of selection
#        """
#        #if self.current_prompt_pos is None:
#            #self.move_cursor_to_end()
#            #return
#        line_from, index_from, line_to, index_to = self.getSelection()
#        pline, pindex = self.getCursorPosition()
#        if line_from < pline or \
#           (line_from == pline and index_from < pindex):
#            self.setSelection(pline, pindex, line_to, index_to)

    def displayPrompt(self, more=False):
        self.append("... ") if more else self.append(">>> ")
        self.move_cursor_to_end()
        
    def updateHistory(self, command):
        if isinstance(command, QStringList):
            for line in command:
                self.history.append(line)
        elif not command == "":
            if len(self.history) <= 0 or \
            not command == self.history[-1]:
                self.history.append(command)
        self.historyIndex = len(self.history)
        
    def writeHistoryFile(self):
        wH = open(_historyFile, 'w')
        for s in self.history:
            wH.write(s + '\n')
        wH.close()
        
    def readHistoryFile(self):
        fileExist = QFile.exists(_historyFile)
        if fileExist:
            rH = open(_historyFile, 'r')
            for line in rH:
                if line != "\n":
                    l = line.rstrip('\n')
                    self.updateHistory(l)
        else:
            return
        
    def clearHistoryFile(self):
        cH = open(_historyFile, 'w')
        cH.close()
        
    def showPrevious(self):
        if self.historyIndex < len(self.history) and not self.history.isEmpty():
            line, pos = self.getCursorPosition()
            selCmdLenght = self.text(line).length()
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
        if  self.historyIndex > 0 and not self.history.isEmpty():
            line, pos = self.getCursorPosition()
            selCmdLenght = self.text(line).length()
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
        startLine, _, endLine, _ = self.getSelection()

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

        elif e.modifiers() & (Qt.ControlModifier | Qt.MetaModifier) and \
                e.key() == Qt.Key_V:
            self.paste()
            e.accept()

        elif e.key() == Qt.Key_Down and not self.isListActive():
            self.showPrevious()
        elif e.key() == Qt.Key_Up and not self.isListActive():
            self.showNext()
        ## TODO: press event for auto-completion file directory
        else:
            QsciScintilla.keyPressEvent(self, e)
            
    def contextMenuEvent(self, e):     
        menu = QMenu(self)
        copyAction = menu.addAction("Copy  CTRL+C")
        pasteAction = menu.addAction("Paste CTRL+V")
        action = menu.exec_(self.mapToGlobal(e.pos()))
        if action == copyAction:
            self.copy()
        elif action == pasteAction:
            self.paste()
                
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
            e.setDropAction(Qt.MoveAction)
            e.accept()
        else:
            QsciScintillaCompat.dropEvent(self, e)

    def insertFromDropPaste(self, textDP): 
        pasteList = textDP.split("\n")
        for line in pasteList[:-1]:
            line.replace(">>> ", "").replace("... ", "")
            self.insert(line)
            self.move_cursor_to_end()
            #self.SendScintilla(QsciScintilla.SCI_DELETEBACK)
            self.runCommand(unicode(self.currentCommand()))
        if pasteList[-1] != "":
            self.insert(unicode(pasteList[-1]))
            self.move_cursor_to_end()

#    def getTextFromEditor(self):
#        text = self.text()
#        textList = text.split("\n")
#        return textList
    
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
        #self.SendScintilla(QsciScintilla.SCI_EMPTYUNDOBUFFER)
        
    def currentCommand(self):
        linenr, index = self.getCursorPosition()
        #for i in range(0, linenr):
        txtLength = self.text(linenr).length()
        string = self.text()
        cmdLine = string.right(txtLength - 4)
        cmd = unicode(cmdLine)
        return cmd

    def runCommand(self, cmd):
        self.write_stdout(cmd)
        import webbrowser
        self.updateHistory(cmd)
        line, pos = self.getCursorPosition()
        selCmdLenght = self.text(line).length()
        self.setSelection(line, 0, line, selCmdLenght)
        self.removeSelectedText()
        #self.SendScintilla(QsciScintilla.SCI_NEWLINE)
        if cmd in ('_save', '_clear', '_clearAll', '_pyqgis', '_api'):
            if cmd == '_save':
                self.writeHistoryFile()
                print QCoreApplication.translate("PythonConsole", 
                                                 "## History saved successfully ##")
            elif cmd == '_clear':
                self.clearHistoryFile()
                print QCoreApplication.translate("PythonConsole", 
                                                 "## History cleared successfully ##")
            elif cmd == '_clearAll':
                res = QMessageBox.question(self, "Python Console", 
                                           QCoreApplication.translate("PythonConsole", 
                                                                      "Are you sure you want to completely\n"
                                                                      "delete the command history ?"),
                                                                      QMessageBox.Yes | QMessageBox.No)
                if res == QMessageBox.No:
                    self.SendScintilla(QsciScintilla.SCI_DELETEBACK)
                    return
                self.history = QStringList()
                self.clearHistoryFile()
                print QCoreApplication.translate("PythonConsole", 
                                                 "## History cleared successfully ##")
            elif cmd == '_pyqgis':
                webbrowser.open( "http://www.qgis.org/pyqgis-cookbook/" )
            elif cmd == '_api':
                webbrowser.open( "http://www.qgis.org/api/" )
                
            self.displayPrompt(False)
        else:
            self.buffer.append(cmd)
            src = u"\n".join(self.buffer)
            more = self.runsource(src, "<input>")
            if not more:
                self.buffer = []
            self.move_cursor_to_end()
            self.displayPrompt(more)

    def write(self, txt):
        sys.stderr.write(txt)

    def write_stdout(self, txt):
        if len(txt) > 0:
            getCmdString = self.text(2)
            prompt = getCmdString[0:4]
            sys.stdout.write(prompt+txt+'\n')
