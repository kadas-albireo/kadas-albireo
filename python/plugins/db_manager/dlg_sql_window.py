# -*- coding: utf-8 -*-

"""
/***************************************************************************
Name                 : DB Manager
Description          : Database manager plugin for QuantumGIS
Date                 : May 23, 2011
copyright            : (C) 2011 by Giuseppe Sucameli
email                : brush.tyler@gmail.com

The content of this file is based on 
- PG_Manager by Martin Dobias (GPLv2 license)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from .db_plugins.plugin import BaseError
from .dlg_db_error import DlgDbError

from .ui.ui_DlgSqlWindow import Ui_DlgSqlWindow

from .highlighter import SqlHighlighter
from .completer import SqlCompleter

class DlgSqlWindow(QDialog, Ui_DlgSqlWindow):

	def __init__(self, iface, db, parent=None):
		QDialog.__init__(self, parent)
		self.iface = iface
		self.db = db
		self.setupUi(self)
		self.setWindowTitle( u"%s - %s [%s]" % (self.windowTitle(), db.connection().connectionName(), db.connection().typeNameString()) )

		self.defaultLayerName = 'QueryLayer'

		settings = QSettings()
		self.restoreGeometry(settings.value("/DB_Manager/sqlWindow/geometry").toByteArray())

		self.editSql.setAcceptRichText(False)
		SqlCompleter(self.editSql, self.db)
		SqlHighlighter(self.editSql, self.db)

		# allow to copy results
		copyAction = QAction("copy", self)
		self.viewResult.addAction( copyAction )
		copyAction.setShortcuts(QKeySequence.Copy)
		QObject.connect(copyAction, SIGNAL("triggered()"), self.copySelectedResults)
		
		self.connect(self.btnExecute, SIGNAL("clicked()"), self.executeSql)
		self.connect(self.btnClear, SIGNAL("clicked()"), self.clearSql)
		self.connect(self.buttonBox.button(QDialogButtonBox.Close), SIGNAL("clicked()"), self.close)

		# hide the load query as layer if feature is not supported
		self._loadAsLayerAvailable = self.db.connector.hasCustomQuerySupport()
		self.loadAsLayerGroup.setVisible( self._loadAsLayerAvailable )
		if self._loadAsLayerAvailable:
			self.layerTypeWidget.hide()	# show if load as raster is supported
			self.connect(self.loadLayerBtn, SIGNAL("clicked()"), self.loadSqlLayer)
			self.connect(self.getColumnsBtn, SIGNAL("clicked()"), self.fillColumnCombos)
			self.connect(self.loadAsLayerGroup, SIGNAL("toggled(bool)"), self.loadAsLayerToggled)
			self.loadAsLayerToggled(False)


	def closeEvent(self, e):
		""" save window state """
		settings = QSettings()
		settings.setValue("/DB_Manager/sqlWindow/geometry", QVariant(self.saveGeometry()))
		
		QDialog.closeEvent(self, e)

	def loadAsLayerToggled(self, checked):
		self.loadAsLayerGroup.setChecked( checked )
		self.loadAsLayerWidget.setVisible( checked )


	def getSql(self):
		# If the selection obtained from an editor spans a line break, 
		# the text will contain a Unicode U+2029 paragraph separator 
		# character instead of a newline \n character 
		# (see https://qt-project.org/doc/qt-4.8/qtextcursor.html#selectedText)
		sql = self.editSql.textCursor().selectedText().replace(unichr(0x2029), "\n")
		if sql.isEmpty():
			sql = self.editSql.toPlainText()
		# try to sanitize query
		sql = sql.replace( QRegExp( ";\\s*$" ), "" )
		return sql

	def clearSql(self):
		self.editSql.clear()

	def executeSql(self):
		sql = self.getSql()
		if sql.isEmpty(): return

		QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))

		# delete the old model 
		old_model = self.viewResult.model()
		self.viewResult.setModel(None)
		if old_model: old_model.deleteLater()

		self.uniqueCombo.clear()
		self.geomCombo.clear()

		try:
			# set the new model 
			model = self.db.sqlResultModel( sql, self )
			self.viewResult.setModel( model )
			self.lblResult.setText("%d rows, %.1f seconds" % (model.affectedRows(), model.secs()))

		except BaseError, e:
			QApplication.restoreOverrideCursor()
			DlgDbError.showError(e, self)
			return

		cols = self.viewResult.model().columnNames()
		cols.sort()
		self.uniqueCombo.addItems( cols )
		self.geomCombo.addItems( cols )

		self.update()
		QApplication.restoreOverrideCursor()


	def loadSqlLayer(self):
		uniqueFieldName = self.uniqueCombo.currentText()
		geomFieldName = self.geomCombo.currentText()

		if geomFieldName.isEmpty() or uniqueFieldName.isEmpty():
			QMessageBox.warning(self, self.tr( "Sorry" ), self.tr( "You must fill the required fields: \ngeometry column - column with unique integer values" ) )
			return

		query = self.getSql()
		if query.isEmpty():
			return

		QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))

		from qgis.core import QgsMapLayer, QgsMapLayerRegistry
		layerType = QgsMapLayer.VectorLayer if self.vectorRadio.isChecked() else QgsMapLayer.RasterLayer

		# get a new layer name
		names = []
		for layer in QgsMapLayerRegistry.instance().mapLayers().values():
			names.append( layer.name() )

		layerName = self.layerNameEdit.text()
		if layerName.isEmpty():
			layerName = self.defaultLayerName
		newLayerName = layerName
		index = 1
		while newLayerName in names:
			index += 1
			newLayerName = u"%s_%d" % (layerName, index)

		# create the layer
		layer = self.db.toSqlLayer(query, geomFieldName, uniqueFieldName, newLayerName, layerType)
		if layer.isValid():
			QgsMapLayerRegistry.instance().addMapLayer(layer, True)

		QApplication.restoreOverrideCursor()

	def fillColumnCombos(self):
		query = self.getSql()
		if query.isEmpty(): return

		QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
		self.uniqueCombo.clear()
		self.geomCombo.clear()

		# get a new alias
		aliasIndex = 0
		while True:
			alias = "_%s__%d" % ("subQuery", aliasIndex)
			escaped = '\\b("?)' + QRegExp.escape(alias) + '\\1\\b' 
			if not query.contains( QRegExp(escaped, Qt.CaseInsensitive) ):
				break
			aliasIndex += 1

		# get all the columns
		cols = []
		connector = self.db.connector
		sql = u"SELECT * FROM (%s\n) AS %s LIMIT 0" % ( unicode(query), connector.quoteId(alias) )

		try:
			c = connector._execute(None, sql)
			cols = connector._get_cursor_columns(c)

		except BaseError, e:
			QApplication.restoreOverrideCursor()
			DlgDbError.showError(e, self)
			return

		finally:
			c.close()
			del c

		cols.sort()
		self.uniqueCombo.addItems( cols )
		self.geomCombo.addItems( cols )

		QApplication.restoreOverrideCursor()

	def copySelectedResults(self):
		if len(self.viewResult.selectedIndexes()) <= 0:
			return
		model = self.viewResult.model()

		# convert to string using tab as separator
		text = model.headerToString( "\t" )
		for idx in self.viewResult.selectionModel().selectedRows():
			text += "\n" + model.rowToString( idx.row(), "\t" )

		QApplication.clipboard().setText( text, QClipboard.Selection )
		QApplication.clipboard().setText( text, QClipboard.Clipboard )

