# -*- coding: utf-8 -*-

"""
***************************************************************************
    SextantePostprocessing.py
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

from sextante.core.QGisLayers import QGisLayers
from sextante.outputs.OutputRaster import OutputRaster
from sextante.outputs.OutputVector import OutputVector
from sextante.outputs.OutputTable import OutputTable
from sextante.core.SextanteResults import SextanteResults
from sextante.gui.ResultsDialog import ResultsDialog
from sextante.gui.RenderingStyles import RenderingStyles
from sextante.outputs.OutputHTML import OutputHTML
from PyQt4.QtGui import *
from qgis.core import *
from sextante.core.SextanteConfig import SextanteConfig
import os
class SextantePostprocessing:

    @staticmethod
    def handleAlgorithmResults(alg, showResults = True):
        htmlResults = False;
        for out in alg.outputs:
            if out.hidden or not out.open:
                continue
            if isinstance(out, (OutputRaster, OutputVector, OutputTable)):
                try:
                    if out.value.startswith("memory:"):
                        layer = out.memoryLayer
                        QgsMapLayerRegistry.instance().addMapLayer(layer)
                    else:
                        if SextanteConfig.getSetting(SextanteConfig.USE_FILENAME_AS_LAYER_NAME):
                            name = os.path.basename(out.value)
                        else:
                            name = out.description
                        QGisLayers.load(out.value, name, alg.crs, RenderingStyles.getStyle(alg.commandLineName(),out.name))
                except Exception, e:
                    QMessageBox.critical(None, "Error", str(e))
            elif isinstance(out, OutputHTML):
                SextanteResults.addResult(out.description, out.value)
                htmlResults = True
        if showResults and htmlResults:
            QApplication.restoreOverrideCursor()
            dlg = ResultsDialog()
            dlg.exec_()
