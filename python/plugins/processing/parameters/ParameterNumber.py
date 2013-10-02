# -*- coding: utf-8 -*-

"""
***************************************************************************
    ParameterNumber.py
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

from PyQt4 import QtGui

__author__ = 'Victor Olaya'
__date__ = 'August 2012'
__copyright__ = '(C) 2012, Victor Olaya'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

from PyQt4.QtGui import *
from processing.parameters.Parameter import Parameter


class ParameterNumber(Parameter):

    def __init__(self, name='', description='', minValue=None, maxValue=None,
                 default=0.0):
        Parameter.__init__(self, name, description)
        try:
            self.default = int(str(default))
            self.isInteger = True
        except:
            self.default = default
            self.isInteger = False
        self.min = minValue
        self.max = maxValue
        self.value = None

    def setValue(self, n):
        if n is None:
            self.value = self.default
            return True
        try:
            if float(n) - int(float(n)) == 0:
                value = int(float(n))
            else:
                value = float(n)
            if self.min is not None:
                if value < self.min:
                    return False
            if self.max is not None:
                if value > self.max:
                    return False
            self.value = value
            return True
        except:
            return False

    def serialize(self):
        return self.__module__.split('.')[-1] + '|' + self.name + '|' \
            + self.description + '|' + str(self.min) + '|' + str(self.max) \
            + '|' + str(self.default)

    def deserialize(self, s):
        tokens = s.split('|')
        for i in range(3, 5):
            if tokens[i] == str(None):
                tokens[i] = None
            else:
                tokens[i] = float(tokens[i])
        try:
            val = int(tokens[5])
        except:
            val = float(tokens[5])
        return ParameterNumber(tokens[1], tokens[2], tokens[3], tokens[4], val)

    def getAsScriptCode(self):
        return '##' + self.name + '=number ' + str(self.default)
