# -*- coding: utf-8 -*-

"""
/***************************************************************************
Name                 : DB Manager
Description          : Database manager plugin for QuantumGIS
Date                 : May 23, 2011
copyright            : (C) 2011 by Giuseppe Sucameli
email                : brush.tyler@gmail.com

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from ..info_model import DatabaseInfo
from ..html_elems import HtmlParagraph, HtmlTable

class SLDatabaseInfo(DatabaseInfo):
	def __init__(self, db):
		self.db = db

	def connectionDetails(self):
		tbl = [ 
			("Filename:", self.db.connector.dbname) 
		]
		return HtmlTable( tbl )

	def spatialInfo(self):
		ret = []

		info = self.db.connector.getSpatialInfo()
		if info == None:
			return

		tbl = [
			("Library:", info[0]), 
			("GEOS:", info[1]), 
			("Proj:", info[2]) 
		]
		ret.append( HtmlTable( tbl ) )

		if not self.db.connector.has_geometry_columns:
			ret.append( HtmlParagraph( u"<warning> geometry_columns table doesn't exist!\n" \
				"This table is essential for many GIS applications for enumeration of tables." ) )

		return ret

	def generalInfo(self):
		info = self.db.connector.getInfo()
		tbl = [
			("SQLite version", info[0])
		]
		return HtmlTable( tbl )

	def privilegesDetails(self):
		return None

