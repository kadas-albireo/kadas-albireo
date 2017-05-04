# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------
# Copyright © 2009- The Spyder Development Team
#
# Licensed under the terms of the MIT License
# (see LICENSE.txt for details)
# -----------------------------------------------------------------------------
"""Provides QtXml classes and functions."""

# Local imports
from . import PYQT4, PYQT5, PYSIDE, PythonQtError

if PYQT5:
    from PyQt5.QtXml import *
elif PYQT4:
    from PyQt4.QtXml import *
elif PYSIDE:
    from PySide.QtXml import *
else:
    raise PythonQtError('No Qt bindings could be found')

del PYQT4, PYQT5, PYSIDE
