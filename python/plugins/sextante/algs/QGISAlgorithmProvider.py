# -*- coding: utf-8 -*-

"""
***************************************************************************
    SextanteAlgorithmProvider.py
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

from sextante.algs.ftools.PointsInPolygon import PointsInPolygon
from sextante.algs.ftools.PointsInPolygonUnique import PointsInPolygonUnique
from sextante.algs.ftools.PointsInPolygonWeighted import PointsInPolygonWeighted
from sextante.algs.ftools.SumLines import SumLines
from sextante.algs.ftools.BasicStatisticsNumbers import BasicStatisticsNumbers
from sextante.algs.ftools.BasicStatisticsStrings import BasicStatisticsStrings
from sextante.algs.ftools.NearestNeighbourAnalysis import NearestNeighbourAnalysis
from sextante.algs.ftools.LinesIntersection import LinesIntersection
from sextante.algs.ftools.MeanCoords import MeanCoords
from sextante.algs.ftools.PointDistance import PointDistance
from sextante.algs.ftools.UniqueValues import UniqueValues
from sextante.algs.ftools.ReprojectLayer import ReprojectLayer
from sextante.algs.ftools.ExportGeometryInfo import ExportGeometryInfo
from sextante.algs.ftools.Centroids import Centroids
from sextante.algs.ftools.Delaunay import Delaunay
from sextante.algs.ftools.VoronoiPolygons import VoronoiPolygons
from sextante.algs.ftools.DensifyGeometries import DensifyGeometries
from sextante.algs.ftools.MultipartToSingleparts import MultipartToSingleparts
from sextante.algs.ftools.SimplifyGeometries import SimplifyGeometries
from sextante.algs.ftools.LinesToPolygons import LinesToPolygons
from sextante.algs.ftools.PolygonsToLines import PolygonsToLines
from sextante.algs.ftools.SinglePartsToMultiparts import SinglePartsToMultiparts
from sextante.algs.ftools.ExtractNodes import ExtractNodes
from sextante.algs.ftools.ConvexHull import ConvexHull
from sextante.algs.ftools.FixedDistanceBuffer import FixedDistanceBuffer
from sextante.algs.ftools.VariableDistanceBuffer import VariableDistanceBuffer
from sextante.algs.ftools.Clip import Clip
from sextante.algs.ftools.Difference import Difference
from sextante.algs.ftools.Dissolve import Dissolve
from sextante.algs.ftools.Intersection import Intersection
from sextante.algs.ftools.ExtentFromLayer import ExtentFromLayer
from sextante.algs.ftools.RandomSelection import RandomSelection
from sextante.algs.ftools.RandomSelectionWithinSubsets import RandomSelectionWithinSubsets
from sextante.algs.ftools.SelectByLocation import SelectByLocation
from sextante.algs.ftools.Union import Union
from sextante.algs.ftools.DensifyGeometriesInterval import DensifyGeometriesInterval
from sextante.algs.mmqgisx.MMQGISXAlgorithms import mmqgisx_attribute_export_algorithm,\
    mmqgisx_attribute_join_algorithm, mmqgisx_delete_columns_algorithm,\
    mmqgisx_delete_duplicate_geometries_algorithm,\
    mmqgisx_geocode_google_algorithm, mmqgisx_geometry_convert_algorithm,\
    mmqgisx_geometry_export_algorithm, mmqgisx_geometry_import_algorithm,\
    mmqgisx_grid_algorithm, mmqgisx_gridify_algorithm,\
    mmqgisx_hub_distance_algorithm, mmqgisx_hub_lines_algorithm,\
    mmqgisx_label_point_algorithm, mmqgisx_merge_algorithm,\
    mmqgisx_select_algorithm, mmqgisx_sort_algorithm,\
    mmqgisx_text_to_float_algorithm, mmqgisx_voronoi_algorithm

from sextante.algs.EquivalentNumField import EquivalentNumField
from sextante.core.AlgorithmProvider import AlgorithmProvider
from sextante.algs.AddTableField import AddTableField
from PyQt4 import QtGui
import os
from sextante.algs.FieldsCalculator import FieldsCalculator
from sextante.algs.SaveSelectedFeatures import SaveSelectedFeatures
from sextante.algs.Explode import Explode
from sextante.algs.AutoincrementalField import AutoincrementalField
from sextante.algs.FieldPyculator import FieldsPyculator

class QGISAlgorithmProvider(AlgorithmProvider):

    def __init__(self):
        AlgorithmProvider.__init__(self)
        self.alglist = [AddTableField(), FieldsCalculator(), SaveSelectedFeatures(),
                        AutoincrementalField(), Explode(), FieldsPyculator(), EquivalentNumField(),
                        #FTOOLS
                        SumLines(), PointsInPolygon(), PointsInPolygonWeighted(), PointsInPolygonUnique(),
                        BasicStatisticsStrings(), BasicStatisticsNumbers(), NearestNeighbourAnalysis(),
                        MeanCoords(), LinesIntersection(), UniqueValues(), PointDistance(),
                        # data management
                        ReprojectLayer(),
                        # geometry
                        ExportGeometryInfo(), Centroids(), Delaunay(), VoronoiPolygons(),
                        SimplifyGeometries(), DensifyGeometries(), DensifyGeometriesInterval(),
                        MultipartToSingleparts(), SinglePartsToMultiparts(), PolygonsToLines(), 
                        LinesToPolygons(), ExtractNodes(),
                        # geoprocessing
                        ConvexHull(), FixedDistanceBuffer(), VariableDistanceBuffer(),
                        Dissolve(), Difference(), Intersection(), Union(), Clip(),
                        # research
                        ExtentFromLayer(), RandomSelection(), RandomSelectionWithinSubsets(),
                        SelectByLocation(),
                        #MMQGISX
                        mmqgisx_attribute_export_algorithm(),
                        mmqgisx_attribute_join_algorithm(),
                        mmqgisx_delete_columns_algorithm(),
                        mmqgisx_delete_duplicate_geometries_algorithm(),
                        mmqgisx_geocode_google_algorithm(),
                        mmqgisx_geometry_convert_algorithm(),
                        mmqgisx_geometry_export_algorithm(),
                        #mmqgisx_geometry_import_algorithm(),
                        mmqgisx_grid_algorithm(),
                        mmqgisx_gridify_algorithm(),
                        mmqgisx_hub_distance_algorithm(),
                        mmqgisx_hub_lines_algorithm(),
                        mmqgisx_label_point_algorithm(),
                        mmqgisx_merge_algorithm(),
                        mmqgisx_select_algorithm(),
                        mmqgisx_sort_algorithm(),
                        mmqgisx_text_to_float_algorithm()]

    def initializeSettings(self):
        AlgorithmProvider.initializeSettings(self)


    def unload(self):
        AlgorithmProvider.unload(self)


    def getName(self):
        return "qgis"

    def getDescription(self):
        return "QGIS geoalgorithms"

    def getIcon(self):
        return QtGui.QIcon(os.path.dirname(__file__) + "/../images/qgis.png")

    def _loadAlgorithms(self):
        self.algs = self.alglist

    def supportsNonFileBasedOutput(self):
        return True