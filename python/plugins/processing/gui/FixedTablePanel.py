# -*- coding: utf-8 -*-

"""
***************************************************************************
    FixedTablePanel.py
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

from processing.gui.FixedTableDialog import FixedTableDialog

from processing.ui.ui_widgetBaseSelector import Ui_Form


class FixedTablePanel(QWidget, Ui_Form):

    def __init__(self, param, parent=None):
        QWidget.__init__(self)
        self.setupUi(self)

        self.leText.setEnabled(False)

        self.param = param
        self.table = []
        for i in range(param.numRows):
            self.table.append(list())
            for j in range(len(param.cols)):
                self.table[i].append('0')

        self.leText.setText(
            self.tr('Fixed table %dx%d' % (len(param.cols), param.numRows)))

        self.btnSelect.clicked.connect(self.showFixedTableDialog)

    def showFixedTableDialog(self):
        dlg = FixedTableDialog(self.param, self.table)
        dlg.exec_()
        if dlg.rettable is not None:
            self.table = dlg.rettable

        self.leText.setText(self.tr('Fixed table %dx%d' % (
            len(self.table), len(self.table[0]))))
