# -*- coding: utf-8 -*-

"""
/***************************************************************************
Name                 : TopoViewer plugin for DB Manager
Description          : Create a project to display topology schema on QGis
Date                 : Sep 23, 2011
copyright            : (C) 2011 by Giuseppe Sucameli
email                : brush.tyler@gmail.com

Based on qgis_pgis_topoview by Sandro Santilli <strk@keybit.net>
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
from qgis.core import *

import os
current_path = os.path.dirname(__file__)


# The load function is called when the "db" database or either one of its
# children db objects (table o schema) is selected by the user.
# @param db is the selected database
# @param mainwindow is the DBManager mainwindow
def load(db, mainwindow):
	# check whether the selected database has topology enabled
	# (search for topology.topology)
	sql = u"""SELECT count(*)
		FROM pg_class AS cls JOIN pg_namespace AS nsp ON nsp.oid = cls.relnamespace
		WHERE cls.relname = 'topology' AND nsp.nspname = 'topology'"""
	c = db.connector._get_cursor()
	db.connector._execute( c, sql )
	res = db.connector._fetchone( c )
	if res == None or int(res[0]) <= 0:
		return

	# add the action to the DBManager menu
	action = QAction( QIcon(), "&TopoViewer", db )
	mainwindow.registerAction( action, "&Schema", run )


# The run function is called once the user clicks on the action TopoViewer
# (look above at the load function) from the DBManager menu/toolbar.
# @param item is the selected db item (either db, schema or table)
# @param action is the clicked action on the DBManager menu/toolbar
# @param mainwindow is the DBManager mainwindow
def run(item, action, mainwindow):
	db = item.database()
	uri = db.uri()
	conninfo = uri.connectionInfo()
	iface = mainwindow.iface

	quoteId = db.connector.quoteId
	quoteStr = db.connector.quoteString

	# check if the selected item is a topology schema
	isTopoSchema = False

	if not hasattr(item, 'schema'):
		QMessageBox.critical(mainwindow, "Invalid topology", u'Select a topology schema to continue.')
		return False

	if item.schema() != None:
		sql = u"SELECT count(*) FROM topology.topology WHERE name = %s" % quoteStr(item.schema().name)
		c = db.connector._get_cursor()
		db.connector._execute( c, sql )
		res = db.connector._fetchone( c )
		isTopoSchema = res != None and int(res[0]) > 0

	if not isTopoSchema:
		QMessageBox.critical(mainwindow, "Invalid topology", u'Schema "%s" is not registered in topology.topology.' % item.schema().name)
		return False

	# load layers into the current project
	toponame = item.schema().name
	template_dir = os.path.join(current_path, 'templates')
	registry = QgsMapLayerRegistry.instance()
	legend = iface.legendInterface()

	# do not refresh the canvas until all the layers are added
	prevRenderFlagState = iface.mapCanvas().renderFlag()
	iface.mapCanvas().setRenderFlag( False )
	try:
		# NOTE: -1 parent is an attempt to always add to the root, but
		#       it is currently broken: http://hub.qgis.org/issues/6879
		group = legend.addGroup(u'%s topology' % toponame, False, -1)

		provider = db.dbplugin().providerName()
		uri = db.uri();

	  # face
		layer = db.toSqlLayer(u'SELECT face_id, topology.ST_GetFaceGeometry(%s, face_id) as geom ' \
								'FROM %s.face WHERE face_id > 0' % (quoteStr(toponame), quoteId(toponame)),
								'geom', 'face_id', u'%s.face' % toponame)
		layer.loadNamedStyle(os.path.join(template_dir, 'face.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # node
		uri.setDataSource(toponame, 'node', 'geom', '', 'node_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.nodes' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'node.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # node labels
		uri.setDataSource(toponame, 'node', 'geom', '', 'node_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.node label' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'node_label.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # edge
		uri.setDataSource(toponame, 'edge_data', 'geom', '', 'edge_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.edge' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'edge.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # edge labels
		uri.setDataSource(toponame, 'edge_data', 'geom', '', 'edge_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.edge label' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'edge_label.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # face_left
		uri.setDataSource(toponame, 'edge_data', 'geom', '', 'edge_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.face_left' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'face_left.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # face_right
		uri.setDataSource(toponame, 'edge_data', 'geom', '', 'edge_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.face_right' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'face_right.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # next_left
		uri.setDataSource(toponame, 'edge_data', 'geom', '', 'edge_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.next_left' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'next_left.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # next_right
		uri.setDataSource(toponame, 'edge_data', 'geom', '', 'edge_id')
		layer = QgsVectorLayer(uri.uri(), u'%s.next_right' % toponame, provider)
		layer.loadNamedStyle(os.path.join(template_dir, 'next_right.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # face_seed
		layer = db.toSqlLayer(u'SELECT face_id, ST_PointOnSurface(topology.ST_GetFaceGeometry(%s, face_id)) as geom ' \
								'FROM %s.face WHERE face_id > 0' % (quoteStr(toponame), quoteId(toponame)),
								'geom', 'face_id', u'%s.face_seed' % toponame)
		layer.loadNamedStyle(os.path.join(template_dir, 'face_seed.qml'))
		registry.addMapLayer(layer)
		legend.setLayerVisible(layer, False)
		legend.setLayerExpanded(layer, False)
		legend.moveLayer(layer, group)

	  # TODO: add polygon0, polygon1 and polygon2 ?

	finally:
		# restore canvas render flag
		iface.mapCanvas().setRenderFlag( prevRenderFlagState )

	return True

