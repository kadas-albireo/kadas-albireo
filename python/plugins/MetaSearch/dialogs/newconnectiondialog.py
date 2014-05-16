# -*- coding: utf-8 -*-
###############################################################################
#
# CSW Client
# ---------------------------------------------------------
# QGIS Catalogue Service client.
#
# Copyright (C) 2010 NextGIS (http://nextgis.org),
#                    Alexander Bruy (alexander.bruy@gmail.com),
#                    Maxim Dubinin (sim@gis-lab.info)
#
# Copyright (C) 2014 Tom Kralidis (tomkralidis@gmail.com)
#
# This source is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This code is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# A copy of the GNU General Public License is available on the World Wide Web
# at <http://www.gnu.org/copyleft/gpl.html>. You can also obtain it by writing
# to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA.
#
###############################################################################

from PyQt4.QtCore import QSettings
from PyQt4.QtGui import QDialog, QMessageBox

from MetaSearch.ui.newconnectiondialog import Ui_NewConnectionDialog


class NewConnectionDialog(QDialog, Ui_NewConnectionDialog):
    """Dialogue to add a new CSW entry"""
    def __init__(self, conn_name=None):
        """init"""

        QDialog.__init__(self)
        self.setupUi(self)
        self.settings = QSettings()
        self.conn_name = None
        self.conn_name_orig = conn_name

    def accept(self):
        """add CSW entry"""

        conn_name = self.leName.text().strip()
        conn_url = self.leURL.text().strip()

        if any([conn_name == '', conn_url == '']):
            QMessageBox.warning(self, self.tr('Save connection'),
                                self.tr('Both Name and URL must be provided'))
            return

        if conn_name is not None:
            key = '/MetaSearch/%s' % conn_name
            keyurl = '%s/url' % key
            key_orig = '/MetaSearch/%s' % self.conn_name_orig

            # warn if entry was renamed to an existing connection
            if all([self.conn_name_orig != conn_name,
                    self.settings.contains(keyurl)]):
                res = QMessageBox.warning(self, self.tr('Save connection'),
                                          self.tr('Overwrite %s?' % conn_name),
                                          QMessageBox.Ok | QMessageBox.Cancel)
                if res == QMessageBox.Cancel:
                    return

            # on rename delete original entry first
            if all([self.conn_name_orig is not None,
                    self.conn_name_orig != conn_name]):
                self.settings.remove(key_orig)

            self.settings.setValue(keyurl, conn_url)
            self.settings.setValue('/MetaSearch/selected', conn_name)

            QDialog.accept(self)

    def reject(self):
        """back out of dialogue"""

        QDialog.reject(self)
