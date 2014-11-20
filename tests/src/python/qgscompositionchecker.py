# -*- coding: utf-8 -*-
'''
qgscompositionchecker.py - check rendering of Qgscomposition against an expected image
 --------------------------------------
  Date                 : 31 Juli 2012
  Copyright            : (C) 2012 by Dr. Horst Düster / Dr. Marco Hugentobler
  email                : horst.duester@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
'''
import qgis

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *

class QgsCompositionChecker(QgsMultiRenderChecker):
    def __init__(self,  mTestName, mComposition ):
        super(QgsCompositionChecker, self).__init__()
        self.mComposition = mComposition
        self.mTestName = mTestName
        self.mDotsPerMeter = 96 / 25.4 * 1000
        self.mSize = QSize( 1122, 794 )
        self.setColorTolerance( 1 )

    def testComposition(self, page=0, pixelDiff=0 ):
        if ( self.mComposition == None):
            myMessage = "Composition not valid"
            return False, myMessage

        #load expected image
        self.setControlName("expected_"+self.mTestName);

         #get width/height, create image and render the composition to it
        outputImage = QImage( self.mSize, QImage.Format_RGB32 )

        self.mComposition.setPlotStyle( QgsComposition.Print )
        outputImage.setDotsPerMeterX( self.mDotsPerMeter )
        outputImage.setDotsPerMeterY( self.mDotsPerMeter )
        QgsMultiRenderChecker.drawBackground( outputImage )
        p = QPainter( outputImage )
        self.mComposition.renderPage( p, page )
        p.end()

        renderedFilePath = QDir.tempPath() + QDir.separator() + QFileInfo(self.mTestName).baseName() + "_rendered.png"
        outputImage.save( renderedFilePath, "PNG" )

        self.setRenderedImage( renderedFilePath )

        testResult = self.runTest( self.mTestName, pixelDiff )

        return testResult, self.report()

