# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------
# Copyright © 2009- The Spyder Development Team
#
# Licensed under the terms of the MIT License
# (see LICENSE.txt for details)
# -----------------------------------------------------------------------------
"""Provides Qsci classes and functions."""

# Local imports
from . import PYQT4, PYQT5, PYSIDE, PythonQtError

if PYQT5:
    from PyQt5.Qsci import *
elif PYQT4:
    from PyQt4.Qsci import *
elif PYSIDE:
    from PySide.Qsci import *
else:
    raise PythonQtError('No Qt bindings could be found')

del PYQT4, PYQT5, PYSIDE
