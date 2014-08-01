# -*- coding: utf-8 -*-

"""
***************************************************************************
    OgrAlgorithm.py
    ---------------------
    Date                 : November 2012
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
__date__ = 'November 2012'
__copyright__ = '(C) 2012, Victor Olaya'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import string
import re

try:
    from osgeo import ogr
    ogrAvailable = True
except:
    ogrAvailable = False

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *

from processing.algs.gdal.GdalAlgorithm import GdalAlgorithm
from processing.tools import dataobjects


class OgrAlgorithm(GdalAlgorithm):

    def ogrConnectionString(self, uri):
        ogrstr = None

        layer = dataobjects.getObjectFromUri(uri, False)
        if layer is None:
            return uri
        provider = layer.dataProvider().name()
        if provider == 'spatialite':
            # dbname='/geodata/osm_ch.sqlite' table="places" (Geometry) sql=
            regex = re.compile("dbname='(.+)'")
            r = regex.search(unicode(layer.source()))
            ogrstr = r.groups()[0]
        elif provider == 'postgres':
            # dbname='ktryjh_iuuqef' host=spacialdb.com port=9999
            # user='ktryjh_iuuqef' password='xyqwer' sslmode=disable
            # key='gid' estimatedmetadata=true srid=4326 type=MULTIPOLYGON
            # table="t4" (geom) sql=
            s = re.sub(''' sslmode=.+''', '', unicode(layer.source()))
            ogrstr = 'PG:%s' % s
        else:
            ogrstr = unicode(layer.source())
        return ogrstr
