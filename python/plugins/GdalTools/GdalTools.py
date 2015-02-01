# -*- coding: utf-8 -*-
"""
/***************************************************************************
Name                  : GdalTools
Description          : Integrate gdal tools into qgis
Date                 : 17/Sep/09
copyright            : (C) 2009 by Lorenzo Masini (Faunalia)
email                : lorenxo86@gmail.com
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
# Import the PyQt and QGIS libraries
from PyQt4.QtCore import QObject, QCoreApplication, QSettings, QLocale, QFileInfo, QTranslator, SIGNAL
from PyQt4.QtGui import QMessageBox, QMenu, QIcon, QAction
from qgis.core import QGis
import qgis.utils

# are all dependencies satisfied?
valid = True

# Import required modules
req_mods = { "osgeo": "osgeo [python-gdal]" }
try:
  from osgeo import gdal
  from osgeo import ogr
except ImportError, e:
  valid = False

  # if the plugin is shipped with QGis catch the exception and
  # display an error message
  import os.path
  qgisUserPluginPath = qgis.utils.home_plugin_path
  if not os.path.dirname(__file__).startswith( qgisUserPluginPath ):
    title = QCoreApplication.translate( "GdalTools", "Plugin error" )
    message = QCoreApplication.translate( "GdalTools", u'Unable to load {0} plugin. \nThe required "{1}" module is missing. \nInstall it and try again.' )
    import qgis.utils
    QMessageBox.warning( qgis.utils.iface.mainWindow(), title, message.format( "GdalTools", req_mods["osgeo"] ) )
  else:
    # if a module is missing show a more friendly module's name
    error_str = e.args[0]
    error_mod = error_str.replace( "No module named ", "" )
    if error_mod in req_mods:
      error_str = error_str.replace( error_mod, req_mods[error_mod] )
    raise ImportError( error_str )


class GdalTools:

  def __init__( self, iface ):
    if not valid: return

    # Save reference to the QGIS interface
    self.iface = iface
    try:
      self.QgisVersion = unicode( QGis.QGIS_VERSION_INT )
    except:
      self.QgisVersion = unicode( QGis.qgisVersion )[ 0 ]

    if QGis.QGIS_VERSION[0:3] < "1.5":
      # For i18n support
      userPluginPath = qgis.utils.home_plugin_path + "/GdalTools"
      systemPluginPath = qgis.utils.sys_plugin_path + "/GdalTools"

      overrideLocale = QSettings().value( "locale/overrideFlag", False, type=bool )
      if not overrideLocale:
        localeFullName = QLocale.system().name()
      else:
        localeFullName = QSettings().value( "locale/userLocale", "", type=str )

      if QFileInfo( userPluginPath ).exists():
        translationPath = userPluginPath + "/i18n/GdalTools_" + localeFullName + ".qm"
      else:
        translationPath = systemPluginPath + "/i18n/GdalTools_" + localeFullName + ".qm"

      self.localePath = translationPath
      if QFileInfo( self.localePath ).exists():
        self.translator = QTranslator()
        self.translator.load( self.localePath )
        QCoreApplication.installTranslator( self.translator )

  def initGui( self ):
    if not valid: return
    if int( self.QgisVersion ) < 1:
      QMessageBox.warning(
          self.iface.getMainWindow(), "Gdal Tools",
          QCoreApplication.translate( "GdalTools", "QGIS version detected: " ) +unicode( self.QgisVersion )+".xx\n"
          + QCoreApplication.translate( "GdalTools", "This version of Gdal Tools requires at least QGIS version 1.0.0\nPlugin will not be enabled." ) )
      return None

    from tools.GdalTools_utils import GdalConfig, LayerRegistry
    self.GdalVersionNum = GdalConfig.versionNum()
    LayerRegistry.setIface( self.iface )

    # find the Raster menu
    rasterMenu = None
    menu_bar = self.iface.mainWindow().menuBar()
    actions = menu_bar.actions()

    rasterText = QCoreApplication.translate( "QgisApp", "&Raster" )

    for a in actions:
        if a.menu() is not None and a.menu().title() == rasterText:
            rasterMenu = a.menu()
            break

    if rasterMenu is None:
        # no Raster menu, create and insert it before the Help menu
        self.menu = QMenu( rasterText, self.iface.mainWindow() )
        lastAction = actions[ len( actions ) - 1 ]
        menu_bar.insertMenu( lastAction, self.menu )
    else:
        self.menu = rasterMenu
        self.menu.addSeparator()

    # projections menu (Warp (Reproject), Assign projection)
    self.projectionsMenu = QMenu( QCoreApplication.translate( "GdalTools", "Projections" ), self.iface.mainWindow() )
    self.projectionsMenu.setObjectName("projectionsMenu")

    self.warp = QAction( QIcon(":/icons/warp.png"),  QCoreApplication.translate( "GdalTools", "Warp (Reproject)..." ), self.iface.mainWindow() )
    self.warp.setObjectName("warp")
    self.warp.setStatusTip( QCoreApplication.translate( "GdalTools", "Warp an image into a new coordinate system") )
    QObject.connect( self.warp, SIGNAL( "triggered()" ), self.doWarp )

    self.projection = QAction( QIcon( ":icons/projection-add.png" ), QCoreApplication.translate( "GdalTools", "Assign Projection..." ), self.iface.mainWindow() )
    self.projection.setObjectName("projection")
    self.projection.setStatusTip( QCoreApplication.translate( "GdalTools", "Add projection info to the raster" ) )
    QObject.connect( self.projection, SIGNAL( "triggered()" ), self.doProjection )

    self.extractProj = QAction( QIcon( ":icons/projection-export.png" ), QCoreApplication.translate( "GdalTools", "Extract Projection..." ), self.iface.mainWindow() )
    self.extractProj.setObjectName("extractProj")
    self.extractProj.setStatusTip( QCoreApplication.translate( "GdalTools", "Extract projection information from raster(s)" ) )
    QObject.connect( self.extractProj, SIGNAL( "triggered()" ), self.doExtractProj )

    self.projectionsMenu.addActions( [ self.warp, self.projection, self.extractProj ] )

    # conversion menu (Rasterize (Vector to raster), Polygonize (Raster to vector), Translate, RGB to PCT, PCT to RGB)
    self.conversionMenu = QMenu( QCoreApplication.translate( "GdalTools", "Conversion" ), self.iface.mainWindow() )
    self.conversionMenu.setObjectName("conversionMenu")

    if self.GdalVersionNum >= 1300:
      self.rasterize = QAction( QIcon(":/icons/rasterize.png"), QCoreApplication.translate( "GdalTools", "Rasterize (Vector to Raster)..." ), self.iface.mainWindow() )
      self.rasterize.setObjectName("rasterize")
      self.rasterize.setStatusTip( QCoreApplication.translate( "GdalTools", "Burns vector geometries into a raster") )
      QObject.connect( self.rasterize, SIGNAL( "triggered()" ), self.doRasterize )
      self.conversionMenu.addAction( self.rasterize )

    if self.GdalVersionNum >= 1600:
      self.polygonize = QAction( QIcon(":/icons/polygonize.png"), QCoreApplication.translate( "GdalTools", "Polygonize (Raster to Vector)..." ), self.iface.mainWindow() )
      self.polygonize.setObjectName("polygonize")
      self.polygonize.setStatusTip( QCoreApplication.translate( "GdalTools", "Produces a polygon feature layer from a raster") )
      QObject.connect( self.polygonize, SIGNAL( "triggered()" ), self.doPolygonize )
      self.conversionMenu.addAction( self.polygonize )

    self.translate = QAction( QIcon(":/icons/translate.png"), QCoreApplication.translate( "GdalTools", "Translate (Convert Format)..." ), self.iface.mainWindow() )
    self.translate.setObjectName("translate")
    self.translate.setStatusTip( QCoreApplication.translate( "GdalTools", "Converts raster data between different formats") )
    QObject.connect( self.translate, SIGNAL( "triggered()" ), self.doTranslate )

    self.paletted = QAction( QIcon( ":icons/24-to-8-bits.png" ), QCoreApplication.translate( "GdalTools", "RGB to PCT..." ), self.iface.mainWindow() )
    self.paletted.setObjectName("paletted")
    self.paletted.setStatusTip( QCoreApplication.translate( "GdalTools", "Convert a 24bit RGB image to 8bit paletted" ) )
    QObject.connect( self.paletted, SIGNAL( "triggered()" ), self.doPaletted )

    self.rgb = QAction( QIcon( ":icons/8-to-24-bits.png" ), QCoreApplication.translate( "GdalTools", "PCT to RGB..." ), self.iface.mainWindow() )
    self.rgb.setObjectName("rgb")
    self.rgb.setStatusTip( QCoreApplication.translate( "GdalTools", "Convert an 8bit paletted image to 24bit RGB" ) )
    QObject.connect( self.rgb, SIGNAL( "triggered()" ), self.doRGB )

    self.conversionMenu.addActions( [ self.translate, self.paletted, self.rgb ] )

    # extraction menu (Clipper, Contour)
    self.extractionMenu = QMenu( QCoreApplication.translate( "GdalTools", "Extraction" ), self.iface.mainWindow() )
    self.extractionMenu.setObjectName("extractionMenu")

    if self.GdalVersionNum >= 1600:
      self.contour = QAction( QIcon(":/icons/contour.png"), QCoreApplication.translate( "GdalTools", "Contour..." ), self.iface.mainWindow() )
      self.contour.setObjectName("contour")
      self.contour.setStatusTip( QCoreApplication.translate( "GdalTools", "Builds vector contour lines from a DEM") )
      QObject.connect( self.contour, SIGNAL( "triggered()" ), self.doContour )
      self.extractionMenu.addAction( self.contour )

    self.clipper = QAction( QIcon( ":icons/raster-clip.png" ), QCoreApplication.translate( "GdalTools", "Clipper..." ), self.iface.mainWindow() )
    self.clipper.setObjectName("clipper")
    #self.clipper.setStatusTip( QCoreApplication.translate( "GdalTools", "Converts raster data between different formats") )
    QObject.connect( self.clipper, SIGNAL( "triggered()" ), self.doClipper )

    self.extractionMenu.addActions( [ self.clipper ] )

    # analysis menu (DEM (Terrain model), Grid (Interpolation), Near black, Proximity (Raster distance), Sieve)
    self.analysisMenu = QMenu( QCoreApplication.translate( "GdalTools", "Analysis" ), self.iface.mainWindow() )
    self.analysisMenu.setObjectName("analysisMenu")

    if self.GdalVersionNum >= 1600:
      self.sieve = QAction( QIcon(":/icons/sieve.png"), QCoreApplication.translate( "GdalTools", "Sieve..." ), self.iface.mainWindow() )
      self.sieve.setObjectName("sieve")
      self.sieve.setStatusTip( QCoreApplication.translate( "GdalTools", "Removes small raster polygons") )
      QObject.connect( self.sieve, SIGNAL( "triggered()" ), self.doSieve )
      self.analysisMenu.addAction( self.sieve )

    if self.GdalVersionNum >= 1500:
      self.nearBlack = QAction( QIcon(":/icons/nearblack.png"),  QCoreApplication.translate( "GdalTools", "Near Black..." ), self.iface.mainWindow() )
      self.nearBlack.setObjectName("nearBlack")
      self.nearBlack.setStatusTip( QCoreApplication.translate( "GdalTools", "Convert nearly black/white borders to exact value") )
      QObject.connect( self.nearBlack, SIGNAL( "triggered()" ), self.doNearBlack )
      self.analysisMenu.addAction( self.nearBlack )

    if self.GdalVersionNum >= 1700:
      self.fillNodata = QAction( QIcon(":/icons/fillnodata.png"), QCoreApplication.translate( "GdalTools", "Fill nodata..." ), self.iface.mainWindow() )
      self.fillNodata.setObjectName("fillNodata")
      self.fillNodata.setStatusTip( QCoreApplication.translate( "GdalTools", "Fill raster regions by interpolation from edges") )
      QObject.connect( self.fillNodata, SIGNAL( "triggered()" ), self.doFillNodata )
      self.analysisMenu.addAction( self.fillNodata )

    if self.GdalVersionNum >= 1600:
      self.proximity = QAction( QIcon(":/icons/proximity.png"),  QCoreApplication.translate( "GdalTools", "Proximity (Raster Distance)..." ), self.iface.mainWindow() )
      self.proximity.setObjectName("proximity")
      self.proximity.setStatusTip( QCoreApplication.translate( "GdalTools", "Produces a raster proximity map") )
      QObject.connect( self.proximity, SIGNAL( "triggered()" ), self.doProximity )
      self.analysisMenu.addAction( self.proximity )

    if self.GdalVersionNum >= 1500:
      self.grid = QAction( QIcon(":/icons/grid.png"), QCoreApplication.translate( "GdalTools", "Grid (Interpolation)..." ), self.iface.mainWindow() )
      self.grid.setObjectName("grid")
      self.grid.setStatusTip( QCoreApplication.translate( "GdalTools", "Create raster from the scattered data") )
      QObject.connect( self.grid, SIGNAL( "triggered()" ), self.doGrid )
      self.analysisMenu.addAction( self.grid )

    if self.GdalVersionNum >= 1700:
      self.dem = QAction( QIcon( ":icons/dem.png" ), QCoreApplication.translate( "GdalTools", "DEM (Terrain Models)..." ), self.iface.mainWindow() )
      self.dem.setObjectName("dem")
      self.dem.setStatusTip( QCoreApplication.translate( "GdalTools", "Tool to analyze and visualize DEMs" ) )
      QObject.connect( self.dem, SIGNAL( "triggered()" ), self.doDEM )
      self.analysisMenu.addAction( self.dem )

    #self.analysisMenu.addActions( [  ] )

    # miscellaneous menu (Build overviews (Pyramids), Tile index, Information, Merge, Build Virtual Raster (Catalog))
    self.miscellaneousMenu = QMenu( QCoreApplication.translate( "GdalTools", "Miscellaneous" ), self.iface.mainWindow() )
    self.miscellaneousMenu.setObjectName("miscellaneousMenu")

    if self.GdalVersionNum >= 1600:
      self.buildVRT = QAction( QIcon(":/icons/vrt.png"), QCoreApplication.translate( "GdalTools", "Build Virtual Raster (Catalog)..." ), self.iface.mainWindow() )
      self.buildVRT.setObjectName("buildVRT")
      self.buildVRT.setStatusTip( QCoreApplication.translate( "GdalTools", "Builds a VRT from a list of datasets") )
      QObject.connect( self.buildVRT, SIGNAL( "triggered()" ), self.doBuildVRT )
      self.miscellaneousMenu.addAction( self.buildVRT )

    self.merge = QAction( QIcon(":/icons/merge.png"), QCoreApplication.translate( "GdalTools", "Merge..." ), self.iface.mainWindow() )
    self.merge.setObjectName("merge")
    self.merge.setStatusTip( QCoreApplication.translate( "GdalTools", "Build a quick mosaic from a set of images") )
    QObject.connect( self.merge, SIGNAL( "triggered()" ), self.doMerge )

    self.info = QAction( QIcon( ":/icons/raster-info.png" ), QCoreApplication.translate( "GdalTools", "Information..." ), self.iface.mainWindow() )
    self.info.setObjectName("info")
    self.info.setStatusTip( QCoreApplication.translate( "GdalTools", "Lists information about raster dataset" ) )
    QObject.connect( self.info, SIGNAL("triggered()"), self.doInfo )

    self.overview = QAction( QIcon( ":icons/raster-overview.png" ), QCoreApplication.translate( "GdalTools", "Build Overviews (Pyramids)..." ), self.iface.mainWindow() )
    self.overview.setObjectName("overview")
    self.overview.setStatusTip( QCoreApplication.translate( "GdalTools", "Builds or rebuilds overview images" ) )
    QObject.connect( self.overview, SIGNAL( "triggered()" ), self.doOverview )

    self.tileindex = QAction( QIcon( ":icons/tiles.png" ), QCoreApplication.translate( "GdalTools", "Tile Index..." ), self.iface.mainWindow() )
    self.tileindex.setObjectName("tileindex")
    self.tileindex.setStatusTip( QCoreApplication.translate( "GdalTools", "Build a shapefile as a raster tileindex" ) )
    QObject.connect( self.tileindex, SIGNAL( "triggered()" ), self.doTileIndex )

    self.miscellaneousMenu.addActions( [ self.merge, self.info, self.overview, self.tileindex ] )

    self.menu.addMenu( self.projectionsMenu )
    self.menu.addMenu( self.conversionMenu )
    self.menu.addMenu( self.extractionMenu )

    if not self.analysisMenu.isEmpty():
      self.menu.addMenu( self.analysisMenu )

    self.menu.addMenu( self.miscellaneousMenu )

    self.settings = QAction( QCoreApplication.translate( "GdalTools", "GdalTools Settings..." ), self.iface.mainWindow() )
    self.settings.setObjectName("settings")
    self.settings.setStatusTip( QCoreApplication.translate( "GdalTools", "Various settings for Gdal Tools" ) )
    QObject.connect( self.settings, SIGNAL( "triggered()" ), self.doSettings )
    self.menu.addAction( self.settings )

  def unload( self ):
    if not valid: return
    pass

  def doBuildVRT( self ):
    from tools.doBuildVRT import GdalToolsDialog as BuildVRT
    d = BuildVRT( self.iface )
    self.runToolDialog( d )

  def doContour( self ):
    from tools.doContour import GdalToolsDialog as Contour
    d = Contour( self.iface )
    self.runToolDialog( d )

  def doRasterize( self ):
    from tools.doRasterize import GdalToolsDialog as Rasterize
    d = Rasterize( self.iface )
    self.runToolDialog( d )

  def doPolygonize( self ):
    from tools.doPolygonize import GdalToolsDialog as Polygonize
    d = Polygonize( self.iface )
    self.runToolDialog( d )

  def doMerge( self ):
    from tools.doMerge import GdalToolsDialog as Merge
    d = Merge( self.iface )
    self.runToolDialog( d )

  def doSieve( self ):
    from tools.doSieve import GdalToolsDialog as Sieve
    d = Sieve( self.iface )
    self.runToolDialog( d )

  def doProximity( self ):
    from tools.doProximity import GdalToolsDialog as Proximity
    d = Proximity( self.iface )
    self.runToolDialog( d )

  def doNearBlack( self ):
    from tools.doNearBlack import GdalToolsDialog as NearBlack
    d = NearBlack( self.iface )
    self.runToolDialog( d )

  def doFillNodata( self ):
    from tools.doFillNodata import GdalToolsDialog as FillNodata
    d = FillNodata( self.iface )
    self.runToolDialog( d )

  def doWarp( self ):
    from tools.doWarp import GdalToolsDialog as Warp
    d = Warp( self.iface )
    self.runToolDialog( d )

  def doGrid( self ):
    from tools.doGrid import GdalToolsDialog as Grid
    d = Grid( self.iface )
    self.runToolDialog( d )

  def doTranslate( self ):
    from tools.doTranslate import GdalToolsDialog as Translate
    d = Translate( self.iface )
    self.runToolDialog( d )

  def doInfo( self ):
    from tools.doInfo import GdalToolsDialog as Info
    d = Info( self.iface )
    self.runToolDialog( d )

  def doProjection( self ):
    from tools.doProjection import GdalToolsDialog as Projection
    d = Projection( self.iface )
    self.runToolDialog( d )

  def doOverview( self ):
    from tools.doOverview import GdalToolsDialog as Overview
    d = Overview( self.iface )
    self.runToolDialog( d )

  def doClipper( self ):
    from tools.doClipper import GdalToolsDialog as Clipper
    d = Clipper( self.iface )
    self.runToolDialog( d )

  def doPaletted( self ):
    from tools.doRgbPct import GdalToolsDialog as RgbPct
    d = RgbPct( self.iface )
    self.runToolDialog( d )

  def doRGB( self ):
    from tools.doPctRgb import GdalToolsDialog as PctRgb
    d = PctRgb( self.iface )
    self.runToolDialog( d )

  def doTileIndex( self ):
    from tools.doTileIndex import GdalToolsDialog as TileIndex
    d = TileIndex( self.iface )
    self.runToolDialog( d )

  def doExtractProj( self ):
    from tools.doExtractProj import GdalToolsDialog as ExtractProj
    d = ExtractProj( self.iface )
    d.exec_()

  def doDEM( self ):
    from tools.doDEM import GdalToolsDialog as DEM
    d = DEM( self.iface )
    self.runToolDialog( d )

  def runToolDialog( self, dlg ):
    dlg.show_()
    dlg.exec_()
    del dlg

  def doSettings( self ):
    from tools.doSettings import GdalToolsSettingsDialog as Settings
    d = Settings( self.iface )
    d.exec_()
