# -*- coding: utf-8 -*-

"""
***************************************************************************
    Help2Html.py
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

import pickle
from processing.tools.system import *
import os
import codecs


class Help2Html:

    ALG_DESC = 'ALG_DESC'
    ALG_CREATOR = 'ALG_CREATOR'
    ALG_HELP_CREATOR = 'ALG_HELP_CREATOR'

    def getHtmlFile(self, alg, helpFile):
        if not os.path.exists(helpFile):
            return None
        self.alg = alg
        f = open(helpFile, 'rb')
        self.descriptions = pickle.load(f)
        s = '<html><body><h2>Algorithm description</h2>\n'
        s += '<p>' + self.getDescription(self.ALG_DESC) + '</p>\n'
        s += '<h2>Input parameters</h2>\n'
        for param in self.alg.parameters:
            s += '<h3>' + param.description + '</h3>\n'
            s += '<p>' + self.getDescription(param.name) + '</p>\n'
        s += '<h2>Outputs</h2>\n'
        for out in self.alg.outputs:
            s += '<h3>' + out.description + '</h3>\n'
            s += '<p>' + self.getDescription(out.name) + '</p>\n'
        s += '<br>'
        s += '<p align="right">Algorithm author: ' + self.getDescription(self.ALG_CREATOR) + '</p>'
        s += '<p align="right">Help author: ' + self.getDescription(self.ALG_HELP_CREATOR) + '</p>'
        s += '</body></html>'
        filename = tempFolder() + os.sep + 'temphelp.html'
        tempHtml = codecs.open(filename, 'w', encoding='utf-8')
        tempHtml.write(s)
        tempHtml.close()

        return filename

    def getDescription(self, name):
        if name in self.descriptions:
            return self.descriptions[name].replace("\n", "<br>")
        else:
            return ''
