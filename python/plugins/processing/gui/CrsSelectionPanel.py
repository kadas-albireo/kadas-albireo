# -*- coding: utf-8 -*-

"""
***************************************************************************
    CrsSelectionPanel.py
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

from PyQt4.QtGui import QWidget
from qgis.core import QgsCoordinateReferenceSystem
from qgis.gui import QgsGenericProjectionSelector

from processing.ui.ui_widgetBaseSelector import Ui_Form

class CrsSelectionPanel(QWidget, Ui_Form):

    def __init__(self, default):
        QWidget.__init__(self)
        self.setupUi(self)

        self.leText.setEnabled(False)

        self.btnSelect.clicked.connect(self.browseCRS)
        self.crs = QgsCoordinateReferenceSystem(default).authid()
        self.updateText()

    def setAuthId(self, authid):
        self.crs = authid
        self.updateText()

    def browseCRS(self):
        selector = QgsGenericProjectionSelector()
        selector.setSelectedAuthId(self.crs)
        if selector.exec_():
            authId = selector.selectedAuthId()
            if authId.upper().startswith("EPSG:"):
                self.crs = authId
            else:
                proj = QgsCoordinateReferenceSystem()
                proj.createFromSrsId(selector.selectedCrsId())
                self.crs = proj.toProj4()
            self.updateText()

    def updateText(self):
        if self.crs is not None:
            self.leText.setText(self.crs)

    def getValue(self):
        return self.crs
