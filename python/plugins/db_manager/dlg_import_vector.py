# -*- coding: utf-8 -*-

"""
/***************************************************************************
Name                 : DB Manager
Description          : Database manager plugin for QGIS
Date                 : Oct 13, 2011
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

from PyQt4.QtCore import Qt, QObject, SIGNAL, QSettings, QFileInfo
from PyQt4.QtGui import QDialog, QFileDialog, QMessageBox, QApplication, QCursor

import qgis.core
from qgis.utils import iface

from .ui.ui_DlgImportVector import Ui_DbManagerDlgImportVector as Ui_Dialog

class DlgImportVector(QDialog, Ui_Dialog):

        HAS_INPUT_MODE, ASK_FOR_INPUT_MODE = range(2)

        def __init__(self, inLayer, outDb, outUri, parent=None):
                QDialog.__init__(self, parent)
                self.inLayer = inLayer
                self.db = outDb
                self.outUri = outUri
                self.setupUi(self)

                self.default_pk = "id"
                self.default_geom = "geom"

                self.mode = self.ASK_FOR_INPUT_MODE if self.inLayer is None else self.HAS_INPUT_MODE

                # used to delete the inlayer whether created inside this dialog
                self.inLayerMustBeDestroyed = False

                self.populateSchemas()
                self.populateTables()
                self.populateLayers()
                self.populateEncodings()

                # updates of UI
                self.setupWorkingMode( self.mode )
                self.connect(self.cboSchema, SIGNAL("currentIndexChanged(int)"), self.populateTables)



        def setupWorkingMode(self, mode):
                """ hide the widget to select a layer/file if the input layer is already set """
                self.wdgInput.setVisible( mode == self.ASK_FOR_INPUT_MODE )
                self.resize( 450, 350 )

                self.cboTable.setEditText(self.outUri.table())

                if mode == self.ASK_FOR_INPUT_MODE:
                        QObject.connect( self.btnChooseInputFile, SIGNAL("clicked()"), self.chooseInputFile )
                        #QObject.connect( self.cboInputLayer.lineEdit(), SIGNAL("editingFinished()"), self.updateInputLayer )
                        QObject.connect( self.cboInputLayer, SIGNAL("editTextChanged(const QString &)"), self.inputPathChanged )
                        #QObject.connect( self.cboInputLayer, SIGNAL("currentIndexChanged(int)"), self.updateInputLayer )
                        QObject.connect( self.btnUpdateInputLayer, SIGNAL("clicked()"), self.updateInputLayer )
                else:
                        # set default values
                        pk = self.outUri.keyColumn()
                        self.editPrimaryKey.setText(pk if pk != "" else self.default_pk)
                        if self.inLayer.hasGeometryType():
                                geom = self.outUri.geometryColumn()
                                self.editGeomColumn.setText(geom if geom != "" else self.default_geom)

                        inCrs = self.inLayer.crs()
                        srid = inCrs.postgisSrid() if inCrs.isValid() else 4236
                        self.editSourceSrid.setText( "%s" % srid )
                        self.editTargetSrid.setText( "%s" % srid )

                        self.checkSupports()

        def checkSupports(self):
                """ update options available for the current input layer """
                allowSpatial = self.db.connector.hasSpatialSupport()
                hasGeomType = self.inLayer and self.inLayer.hasGeometryType()
                isShapefile = self.inLayer and self.inLayer.providerType() == "ogr" and self.inLayer.storageType() == "ESRI Shapefile"
                self.chkGeomColumn.setEnabled(allowSpatial and hasGeomType)
                self.chkSourceSrid.setEnabled(allowSpatial and hasGeomType)
                self.chkTargetSrid.setEnabled(allowSpatial and hasGeomType)
                self.chkSinglePart.setEnabled(allowSpatial and hasGeomType and isShapefile)
                self.chkSpatialIndex.setEnabled(allowSpatial and hasGeomType)



        def populateLayers(self):
                self.cboInputLayer.clear()
                for index, layer in enumerate( iface.legendInterface().layers() ):
                        # TODO: add import raster support!
                        if layer.type() == qgis.core.QgsMapLayer.VectorLayer:
                                self.cboInputLayer.addItem( layer.name(), index )

        def deleteInputLayer(self):
                """ unset the input layer, then destroy it but only if it was created from this dialog """
                if self.mode == self.ASK_FOR_INPUT_MODE and self.inLayer:
                        if self.inLayerMustBeDestroyed:
                                self.inLayer.deleteLater()
                        self.inLayer = None
                        self.inLayerMustBeDestroyed = False
                        return True
                return False

        def chooseInputFile(self):
                vectorFormats = qgis.core.QgsProviderRegistry.instance().fileVectorFilters()
                # get last used dir and format
                settings = QSettings()
                lastDir = settings.value("/db_manager/lastUsedDir", "")
                lastVectorFormat = settings.value("/UI/lastVectorFileFilter", "")
                # ask for a filename
                (filename, lastVectorFormat) = QFileDialog.getOpenFileNameAndFilter(self, self.tr("Choose the file to import"), lastDir, vectorFormats, lastVectorFormat)
                if filename == "":
                        return
                # store the last used dir and format
                settings.setValue("/db_manager/lastUsedDir", QFileInfo(filename).filePath())
                settings.setValue("/UI/lastVectorFileFilter", lastVectorFormat)

                self.cboInputLayer.setEditText( filename )

        def inputPathChanged(self, path):
                if self.cboInputLayer.currentIndex() < 0:
                        return
                self.cboInputLayer.blockSignals(True)
                self.cboInputLayer.setCurrentIndex( -1 )
                self.cboInputLayer.setEditText( path )
                self.cboInputLayer.blockSignals(False)

        def updateInputLayer(self):
                """ create the input layer and update available options """
                if self.mode != self.ASK_FOR_INPUT_MODE:
                        return

                self.deleteInputLayer()

                index = self.cboInputLayer.currentIndex()
                if index < 0:
                        filename = self.cboInputLayer.currentText()
                        if filename == "":
                                return False

                        layerName = QFileInfo(filename).completeBaseName()
                        layer = qgis.core.QgsVectorLayer(filename, layerName, "ogr")
                        if not layer.isValid() or layer.type() != qgis.core.QgsMapLayer.VectorLayer:
                                layer.deleteLater()
                                return False

                        self.inLayer = layer
                        self.inLayerMustBeDestroyed = True

                else:
                        legendIndex = self.cboInputLayer.itemData( index )
                        self.inLayer = iface.legendInterface().layers()[ legendIndex ]
                        self.inLayerMustBeDestroyed = False

                # update the output table name
                self.cboTable.setEditText(self.inLayer.name())

                self.checkSupports()
                return True


        def populateSchemas(self):
                if not self.db:
                        return

                self.cboSchema.clear()
                schemas = self.db.schemas()
                if schemas is None:
                        self.hideSchemas()
                        return

                index = -1
                for schema in schemas:
                        self.cboSchema.addItem(schema.name)
                        if schema.name == self.outUri.schema():
                                index = self.cboSchema.count()-1
                self.cboSchema.setCurrentIndex(index)

        def hideSchemas(self):
                self.cboSchema.setEnabled(False)

        def populateTables(self):
                if not self.db:
                        return

                currentText = self.cboTable.currentText()

                schemas = self.db.schemas()
                if schemas is not None:
                        schema_name = self.cboSchema.currentText()
                        matching_schemas = filter(lambda x: x.name == schema_name, schemas)
                        tables = matching_schemas[0].tables() if len(matching_schemas) > 0 else []
                else:
                        tables = self.db.tables()

                self.cboTable.clear()
                for table in tables:
                        self.cboTable.addItem(table.name)

                self.cboTable.setEditText(currentText)

        def populateEncodings(self):
                encodings = ['ISO-8859-1', 'ISO-8859-2', 'UTF-8', 'CP1250']
                for enc in encodings:
                        self.cboEncoding.addItem(enc)
                self.cboEncoding.setCurrentIndex(2)

        def accept(self):
                if self.mode == self.ASK_FOR_INPUT_MODE:
                        # create the input layer (if not already done) and
                        # update available options w/o changing the tablename!
                        self.cboTable.blockSignals(True)
                        table = self.cboTable.currentText()
                        self.updateInputLayer()
                        self.cboTable.setEditText(table)
                        self.cboTable.blockSignals(False)

                # sanity checks
                if self.inLayer is None:
                        QMessageBox.information(self, self.tr("Import to database"), self.tr("Input layer missing or not valid"))
                        return

                if self.cboTable.currentText() == "":
                        QMessageBox.information(self, self.tr("Import to database"), self.tr("Output table name is required"))
                        return

                if self.chkSourceSrid.isEnabled() and self.chkSourceSrid.isChecked():
                        try:
                                sourceSrid = self.editSourceSrid.text()
                        except ValueError:
                                QMessageBox.information(self, self.tr("Import to database"), self.tr("Invalid source srid: must be an integer"))
                                return

                if self.chkTargetSrid.isEnabled() and self.chkTargetSrid.isChecked():
                        try:
                                targetSrid = self.editTargetSrid.text()
                        except ValueError:
                                QMessageBox.information(self, self.tr("Import to database"), self.tr("Invalid target srid: must be an integer"))
                                return

                # override cursor
                QApplication.setOverrideCursor(QCursor(Qt.WaitCursor))
                # store current input layer crs and encoding, so I can restore it
                prevInCrs = self.inLayer.crs()
                prevInEncoding = self.inLayer.dataProvider().encoding()

                try:
                        schema = self.outUri.schema() if not self.cboSchema.isEnabled() else self.cboSchema.currentText()
                        table = self.cboTable.currentText()

                        # get pk and geom field names from the source layer or use the
                        # ones defined by the user
                        pk = self.outUri.keyColumn() if not self.chkPrimaryKey.isChecked() else self.editPrimaryKey.text()

                        if self.inLayer.hasGeometryType() and self.chkGeomColumn.isEnabled():
                                geom = self.outUri.geometryColumn() if not self.chkGeomColumn.isChecked() else self.editGeomColumn.text()
                                geom = geom if geom != "" else self.default_geom
                        else:
                                geom = None

                        # get output params, update output URI
                        self.outUri.setDataSource( schema, table, geom, "", pk )
                        uri = self.outUri.uri()

                        providerName = self.db.dbplugin().providerName()

                        options = {}
                        if self.chkDropTable.isChecked():
                                options['overwrite'] = True

                        if self.chkSinglePart.isEnabled() and self.chkSinglePart.isChecked():
                                options['forceSinglePartGeometryType'] = True

                        outCrs = None
                        if self.chkTargetSrid.isEnabled() and self.chkTargetSrid.isChecked():
                                targetSrid = int(self.editTargetSrid.text())
                                outCrs = qgis.core.QgsCoordinateReferenceSystem(targetSrid)

                        # update input layer crs and encoding
                        if self.chkSourceSrid.isEnabled() and self.chkSourceSrid.isChecked():
                                sourceSrid = int(self.editSourceSrid.text())
                                inCrs = qgis.core.QgsCoordinateReferenceSystem(sourceSrid)
                                self.inLayer.setCrs( inCrs )

                        if self.chkEncoding.isEnabled() and self.chkEncoding.isChecked():
                                enc = self.cboEncoding.currentText()
                                self.inLayer.setProviderEncoding( enc )

                        # do the import!
                        ret, errMsg = qgis.core.QgsVectorLayerImport.importLayer( self.inLayer, uri, providerName, outCrs, False, False, options )
                except Exception as e:
                        ret = -1
                        errMsg = unicode( e )

                finally:
                        # restore input layer crs and encoding
                        self.inLayer.setCrs( prevInCrs )
                        self.inLayer.setProviderEncoding( prevInEncoding )
                        # restore cursor
                        QApplication.restoreOverrideCursor()

                if ret != 0:
                        output = qgis.gui.QgsMessageViewer()
                        output.setTitle( self.tr("Import to database") )
                        output.setMessageAsPlainText( self.tr("Error %d\n%s") % (ret, errMsg) )
                        output.showMessage()
                        return

                # create spatial index
                if self.chkSpatialIndex.isEnabled() and self.chkSpatialIndex.isChecked():
                        self.db.connector.createSpatialIndex( (schema, table), geom )

                QMessageBox.information(self, self.tr("Import to database"), self.tr("Import was successful."))
                return QDialog.accept(self)


        def closeEvent(self, event):
                # destroy the input layer instance but only if it was created
                # from this dialog!
                self.deleteInputLayer()
                QDialog.closeEvent(self, event)


if __name__ == '__main__':
        import sys
        a = QApplication(sys.argv)
        dlg = DlgImportVector()
        dlg.show()
        sys.exit(a.exec_())
