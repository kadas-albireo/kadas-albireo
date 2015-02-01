# -*- coding: utf-8 -*-

"""
***************************************************************************
    PostGISExecuteSQL.py
    ---------------------
    Date                 : October 2012
    Copyright            : (C) 2012 by Victor Olaya and Carterix Geomatics
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

__author__ = 'Victor Olaya, Carterix Geomatics'
__date__ = 'October 2012'
__copyright__ = '(C) 2012, Victor Olaya, Carterix Geomatics'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

from PyQt4.QtCore import QSettings

from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.GeoAlgorithmExecutionException import GeoAlgorithmExecutionException
from processing.core.parameters import ParameterString
from processing.algs.qgis import postgis_utils


class PostGISExecuteSQL(GeoAlgorithm):

    DATABASE = 'DATABASE'
    SQL = 'SQL'

    def processAlgorithm(self, progress):
        connection = self.getParameterValue(self.DATABASE)
        settings = QSettings()
        mySettings = '/PostgreSQL/connections/' + connection
        try:
            database = settings.value(mySettings + '/database')
            username = settings.value(mySettings + '/username')
            host = settings.value(mySettings + '/host')
            port = settings.value(mySettings + '/port', type=int)
            password = settings.value(mySettings + '/password')
        except Exception, e:
            raise GeoAlgorithmExecutionException(
                self.tr('Wrong database connection name: %s' % connection))
        try:
            self.db = postgis_utils.GeoDB(host=host, port=port,
                    dbname=database, user=username, passwd=password)
        except postgis_utils.DbError, e:
            raise GeoAlgorithmExecutionException(
                self.tr("Couldn't connect to database:\n%s" % e.message))

        sql = self.getParameterValue(self.SQL).replace('\n', ' ')
        try:
            self.db._exec_sql_and_commit(str(sql))
        except postgis_utils.DbError, e:
            raise GeoAlgorithmExecutionException(
                self.tr('Error executing SQL:\n%s' % e.message))

    def defineCharacteristics(self):
        self.name = 'PostGIS execute SQL'
        self.group = 'Database'
        self.addParameter(ParameterString(self.DATABASE, self.tr('Database')))
        self.addParameter(ParameterString(self.SQL, self.tr('SQL query'), '', True))
