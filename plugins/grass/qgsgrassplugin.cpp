/***************************************************************************
    qgsgrassplugin.cpp  -  GRASS menu
                             -------------------
    begin                : March, 2004
    copyright            : (C) 2004 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id$ */

// includes
#include "qgisapp.h"
#include "qgsmaplayer.h"
#include "qgisiface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsdataprovider.h"
#include "qgsfeatureattribute.h"
#include "qgsproviderregistry.h"
#include <qgsrasterlayer.h>

#include <qtoolbar.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <q3popupmenu.h>
#include <qlineedit.h>
#include <qaction.h>
#include <qapplication.h>
#include <qcursor.h>
#include <QFileDialog>
#include <qfileinfo.h>
#include <qsettings.h>
#include <qregexp.h>
#include <qglobal.h>
#include <qinputdialog.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3PointArray>

//non qt includes
#include <iostream>

extern "C" {
#include <gis.h>
#include <Vect.h>
}

#include "qgsgrassplugin.h"
#include "../../providers/grass/qgsgrass.h"
#include "../../providers/grass/qgsgrassprovider.h"

//the gui subclass
#include "qgsgrassattributes.h"
#include "qgsgrassselect.h"
#include "qgsgrassedit.h"
#include "qgsgrasstools.h"
#include "qgsgrassregion.h"
#include "qgsgrassnewmapset.h"

// xpm for creating the toolbar icon
#include "add_vector.xpm"
#include "add_raster.xpm"
#include "grass_tools.xpm"
#include "grass_edit.xpm"
#include "grass_region.xpm"
#include "grass_region_edit.xpm"
static const char *pluginVersion = "0.1";

/**
 * Constructor for the plugin. The plugin is passed a pointer to the main app
 * and an interface object that provides access to exposed functions in QGIS.
 * @param theQGisApp Pointer to the QGIS main window
 * @param theQgisInterFace Pointer to the QGIS interface object
 */
QgsGrassPlugin::QgsGrassPlugin(QgisApp * theQGisApp, QgisIface * theQgisInterFace):
  qgisMainWindowPointer(theQGisApp), qGisInterface(theQgisInterFace)
{
  /** Initialize the plugin and set the required attributes */
  pluginNameQString = "GrassVector";
  pluginVersionQString = "0.1";
  pluginDescriptionQString = "GRASS layer";
}

QgsGrassPlugin::~QgsGrassPlugin()
{
    QString err = QgsGrass::closeMapset();
}

/* Following functions return name, description, version, and type for the plugin */
QString QgsGrassPlugin::name()
{
  return pluginNameQString;
}

QString QgsGrassPlugin::version()
{
  return pluginVersionQString;

}

QString QgsGrassPlugin::description()
{
  return pluginDescriptionQString;

}

void QgsGrassPlugin::help()
{
  //TODO
}

int QgsGrassPlugin::type()
{
  return QgisPlugin::UI;
}

/*
 * Initialize the GUI interface for the plugin 
 */
void QgsGrassPlugin::initGui()
{
  toolBarPointer = 0;
  mTools = 0;

  QSettings settings("QuantumGIS", "qgis");

  // Require GISBASE to be set. This should point to the location of
  // the GRASS installation. The GRASS libraries use it to know
  // where to look for things.

  // Look first to see if GISBASE env var is already set.
  // This is set when QGIS is run from within GRASS
  // or when set explicitly by the user.
  // This value should always take precedence.
  QString gisBase = getenv("GISBASE");
#ifdef QGISDEBUG
  qDebug( "%s:%d GRASS gisBase from GISBASE env var is: %s", __FILE__, __LINE__, (const char*)gisBase );
#endif
  if ( !isValidGrassBaseDir(gisBase) ) {
    // Look for gisbase in QSettings
    gisBase = settings.readEntry("/GRASS/gisbase", "");
#ifdef QGISDEBUG
    qDebug( "%s:%d GRASS gisBase from QSettings is: %s", __FILE__, __LINE__, (const char*)gisBase );
#endif
  }

  if ( !isValidGrassBaseDir(gisBase) ) {
    // Use the location specified --with-grass during configure
    gisBase = GRASS_BASE;
#ifdef QGISDEBUG
    qDebug( "%s:%d GRASS gisBase from configure is: %s", __FILE__, __LINE__, (const char*)gisBase );
#endif
  }

  while ( !isValidGrassBaseDir(gisBase) ) {
    // Keep asking user for GISBASE until we get a valid one
    //QMessageBox::warning( 0, "Warning", "QGIS can't find your GRASS installation,\nGRASS data "
    //    "cannot be used.\nPlease select your GISBASE.\nGISBASE is full path to the\n"
    //    "directory where GRASS is installed." );
    // XXX Need to subclass this and add explantory message above to left side
    gisBase = QFileDialog::getExistingDirectory(
        qgisMainWindowPointer, "Choose GISBASE ...", gisBase);
    if (gisBase == QString::null)
    {
      // User pressed cancel. No GRASS for you!
      return;
    }
  }

#ifdef QGISDEBUG
  qDebug( "%s:%d Valid GRASS gisBase is: %s", __FILE__, __LINE__, (const char*)gisBase );
#endif
  QString gisBaseEnv = "GISBASE=" + gisBase;
  /* _Correct_ putenv() implementation is not making copy! */ 
  char *gisBaseEnvChar = new char[gisBaseEnv.length()+1];
  strcpy ( gisBaseEnvChar, const_cast<char *>(gisBaseEnv.ascii()) ); 
  putenv( gisBaseEnvChar );
  settings.writeEntry("/GRASS/gisbase", gisBase);

  mCanvas = qGisInterface->getMapCanvas();

  // Create the action for tool
#if QT_VERSION < 0x040000
  mOpenMapsetAction = new QAction( "Open mapset", 0, this );
  mNewMapsetAction = new QAction( "New mapset", 0, this );
  mCloseMapsetAction = new QAction( "Close mapset", 0, this );
  mAddVectorAction = new QAction("Add GRASS vector layer", QIcon(icon_add_vector), 
      "Add GRASS vector layer",0, this, "addVector");
  mAddRasterAction = new QAction("Add GRASS raster layer", QIcon(icon_add_raster), 
      "Add GRASS raster layer",0, this, "addRaster");
  mOpenToolsAction = new QAction("Open GRASS tools", QIcon(icon_grass_tools), 
      "Open GRASS tools",0, this, "openTools");
  mRegionAction = new QAction("Display Current Grass Region", QIcon(icon_grass_region), 
      "Display Current Grass Region",0, this, "region", true);
  mEditRegionAction = new QAction("Edit Current Grass Region", QIcon(icon_grass_region_edit), 
      "Edit Current Grass Region",0, this, "editRegion");
  mEditAction = new QAction("Edit Grass Vector layer", QIcon(icon_grass_edit), 
      "Edit Grass Vector layer",0, this, "edit");
  mNewVectorAction = new QAction("Create new Grass Vector", 0, this);
#else
  mOpenMapsetAction = new QAction( "Open mapset", this );
  mNewMapsetAction = new QAction( "New mapset", this );
  mCloseMapsetAction = new QAction( "Close mapset", this );
  mAddVectorAction = new QAction(QIcon(icon_add_vector),
      "Add GRASS vector layer", this);
  mAddRasterAction = new QAction(QIcon(icon_add_raster),
      "Add GRASS raster layer", this);
  mOpenToolsAction = new QAction(QIcon(icon_grass_tools),
      "Open GRASS tools", this);
  mRegionAction = new QAction(QIcon(icon_grass_region),
      "Display Current Grass Region", this);
  mEditRegionAction = new QAction(QIcon(icon_grass_region_edit),
      "Edit Current Grass Region", this);
  mEditAction = new QAction(QIcon(icon_grass_edit),
      "Edit Grass Vector layer", this);
  mNewVectorAction = new QAction("Create new Grass Vector", this);
#endif

  mAddVectorAction->setWhatsThis("Adds a GRASS vector layer to the map canvas");
  mAddRasterAction->setWhatsThis("Adds a GRASS raster layer to the map canvas");
  mOpenToolsAction->setWhatsThis("Open GRASS tools");
  mRegionAction->setWhatsThis("Displays the current GRASS region as a rectangle on the map canvas");
  mEditRegionAction->setWhatsThis("Edit the current GRASS region");
  mEditAction->setWhatsThis("Edit the currently selected GRASS vector layer.");

  // Connect the action 
  connect(mAddVectorAction, SIGNAL(activated()), this, SLOT(addVector()));
  connect(mAddRasterAction, SIGNAL(activated()), this, SLOT(addRaster()));
  connect(mOpenToolsAction, SIGNAL(activated()), this, SLOT(openTools()));
  connect(mEditAction, SIGNAL(activated()), this, SLOT(edit()));
  connect(mNewVectorAction, SIGNAL(activated()), this, SLOT(newVector()));
  connect(mRegionAction, SIGNAL(toggled(bool)), this, SLOT(switchRegion(bool)));
  connect(mEditRegionAction, SIGNAL(activated()), this, SLOT(changeRegion()));
  connect(mOpenMapsetAction, SIGNAL(activated()), this, SLOT(openMapset()));
  connect(mNewMapsetAction, SIGNAL(activated()), this, SLOT(newMapset()));
  connect(mCloseMapsetAction, SIGNAL(activated()), this, SLOT(closeMapset()));

  // Create GRASS plugin menu entry
  QMenu *pluginMenu = qGisInterface->getPluginMenu("&GRASS");

  // Add actions to the menu
  mOpenMapsetAction->addTo(pluginMenu);
  mNewMapsetAction->addTo(pluginMenu);
  mCloseMapsetAction->addTo(pluginMenu);
  mAddVectorAction->addTo(pluginMenu);
  mAddRasterAction->addTo(pluginMenu);
  mOpenToolsAction->addTo(pluginMenu);
  mRegionAction->addTo(pluginMenu);
  mEditRegionAction->addTo(pluginMenu);
  mEditAction->addTo(pluginMenu);
  mNewVectorAction->addTo(pluginMenu);

  // Add the toolbar to the main window
  toolBarPointer = qgisMainWindowPointer->addToolBar("GRASS"); 
  toolBarPointer->setLabel(tr("GRASS"));
  toolBarPointer->setIconSize(QSize(24,24));

  // Add to the toolbar
  mAddVectorAction->addTo(toolBarPointer);
  mAddRasterAction->addTo(toolBarPointer);
  mOpenToolsAction->addTo(toolBarPointer);
  mRegionAction->addTo(toolBarPointer);
  mEditRegionAction->addTo(toolBarPointer);
  mEditAction->addTo(toolBarPointer);

  // Connect display region
  connect( mCanvas, SIGNAL(renderComplete(QPainter *)), this, SLOT(postRender(QPainter *)));

  // Init Region symbology
  mRegionPen.setColor( QColor ( settings.readEntry ("/GRASS/region/color", "#ff0000" ) ) );
  mRegionPen.setWidth( settings.readNumEntry ("/GRASS/region/width", 0 ) );

  mapsetChanged();
}

void QgsGrassPlugin::mapsetChanged ()
{
    if ( !QgsGrass::activeMode() )  {
        mOpenToolsAction->setEnabled(false);
	mRegionAction->setEnabled(false);
	mEditRegionAction->setEnabled(false);
        mCloseMapsetAction->setEnabled(false);
        mNewVectorAction->setEnabled(false);

        if ( mTools ) 
        {
            mTools->hide();
            delete mTools;
            mTools = 0;
        }    
    } else {
	mOpenToolsAction->setEnabled(true);
	mRegionAction->setEnabled(true);
	mEditRegionAction->setEnabled(true);
        mCloseMapsetAction->setEnabled(true);
        mNewVectorAction->setEnabled(true);
  
        QSettings settings("QuantumGIS", "qgis");
	bool on = settings.readBoolEntry ("/GRASS/region/on", true );
	mRegionAction->setOn(on);

        if ( mTools ) 
        {
            mTools->mapsetChanged();
        }    
    }
}

/*
 * Check if given directory contains a GRASS installation
 */
bool QgsGrassPlugin::isValidGrassBaseDir(QString const gisBase)
{
  if ( gisBase.isEmpty() )
  {
    return FALSE;
  }

  QFileInfo gbi ( gisBase + "/etc/element_list" );
  if ( gbi.exists() ) {
    return TRUE;
  } else {
    return FALSE;
  }
}

// Slot called when the "Add GRASS vector layer" menu item is activated
void QgsGrassPlugin::addVector()
{
#ifdef QGISDEBUG
  std::cerr << "QgsGrassPlugin::addVector()" << std::endl;
#endif
  QString uri;

  QgsGrassSelect *sel = new QgsGrassSelect(QgsGrassSelect::VECTOR );
  if ( sel->exec() ) {
    uri = sel->gisdbase + "/" + sel->location + "/" + sel->mapset + "/" + sel->map + "/" + sel->layer;
  }
#ifdef QGISDEBUG
  std::cerr << "plugin URI: " << uri.toLocal8Bit().data() << std::endl;
#endif
  if ( uri.length() == 0 ) {
    std::cerr << "Nothing was selected" << std::endl;
    return;
  } else {
#ifdef QGISDEBUG
    std::cout << "Add new vector layer" << std::endl;
#endif

    // create vector name: vector layer
    QString name = sel->map;

    QString field; 
    QString type; 

    QRegExp rx ( "(\\d+)_(.+)" );
    if ( rx.search ( sel->layer ) != -1 ) 
    {
      field = rx.cap(1);
      type = rx.cap(2);
    }

    // Set location
    QgsGrass::setLocation ( sel->gisdbase, sel->location );

    /* Open vector */
    QgsGrass::resetError();
    Vect_set_open_level (2);
    struct Map_info map;
    int level = Vect_open_old_head (&map, (char *) sel->map.ascii(),
        (char *) sel->mapset.ascii());

    if ( QgsGrass::getError() != QgsGrass::FATAL )
    {
      if ( level >= 2 ) 
      {
        // Count layers
        int cnt = 0; 
        int ncidx = Vect_cidx_get_num_fields ( &map );

        for ( int i = 0; i < ncidx; i++ ) 
        {
          int field = Vect_cidx_get_field_number ( &map, i);

          if ( Vect_cidx_get_type_count( &map, field, GV_POINT|GV_LINE|GV_AREA) > 0 ||
              (field > 1 && Vect_cidx_get_type_count( &map, field, GV_BOUNDARY) ) )
          {
            cnt++;
          }
        }

        if( cnt > 1 ) 
        {
          name.append ( " " + field );

          // No need to ad type, the type is obvious from the legend 
        }
      }

      Vect_close ( &map );
    } else {
      std::cerr << "Cannot open GRASS vector: " << QgsGrass::getErrorMessage().toLocal8Bit().data() << std::endl;
    }

    qGisInterface->addVectorLayer( uri, name, "grass");
  }
}

// Slot called when the "Add GRASS raster layer" menu item is activated
void QgsGrassPlugin::addRaster()
{
#ifdef QGISDEBUG
  std::cerr << "QgsGrassPlugin::addRaster()" << std::endl;
#endif
  QString uri;

  std::cerr << "QgsGrassPlugin::addRaster" << std::endl;

  QgsGrassSelect *sel = new QgsGrassSelect(QgsGrassSelect::RASTER );
  if ( sel->exec() ) {
    QString element;
    if ( sel->selectedType == QgsGrassSelect::RASTER ) {
      element = "cellhd";
    } else { // GROUP
      element = "group";
    }

    uri = sel->gisdbase + "/" + sel->location + "/" + sel->mapset + "/" + element + "/" + sel->map;
  }
#ifdef QGISDEBUG
  std::cerr << "plugin URI: " << uri.toLocal8Bit().data() << std::endl;
#endif
  if ( uri.length() == 0 ) {
    std::cerr << "Nothing was selected" << std::endl;
    return;
  } else {
#ifdef QGISDEBUG
    std::cout << "Add new raster layer" << std::endl;
#endif
    // create raster name
    int pos = uri.findRev('/');
    pos = uri.findRev('/', pos-1);
    QString name = uri.right( uri.length() - pos - 1 );
    name.replace('/', ' ');

    qGisInterface->addRasterLayer( uri );
  }
}

// Open tools
void QgsGrassPlugin::openTools()
{
  if ( !mTools ) 
    mTools = new QgsGrassTools ( qgisMainWindowPointer, qGisInterface, qgisMainWindowPointer, 0, Qt::WType_Dialog );

  mTools->show();
}


// Start vector editing
void QgsGrassPlugin::edit()
{
  if ( QgsGrassEdit::isRunning() ) {
    QMessageBox::warning( 0, "Warning", "GRASS Edit is already running." );
    return;
  }

  QgsGrassEdit *ed = new QgsGrassEdit( qgisMainWindowPointer, qGisInterface, qgisMainWindowPointer, 0, Qt::WType_Dialog );

  if ( ed->isValid() ) {
    ed->show();
    mCanvas->refresh();
  } else {
    delete ed;
  }
}

void QgsGrassPlugin::newVector()
{
#ifdef QGISDEBUG
    std::cout << "QgsGrassPlugin::newVector()" << std::endl;
#endif
  
    if ( QgsGrassEdit::isRunning() ) {
        QMessageBox::warning( 0, "Warning", "GRASS Edit is already running." );
        return;
    }

    bool ok;
    QString name;

    while (1)
    {
        name = QInputDialog::getText( "New GRASS vector",
                "Enter new GRASS vector name:", QLineEdit::Normal,
                name, &ok);

        if ( !ok ) return;

        // Check if the name is valid
        name = name.stripWhiteSpace();

        if ( name.isEmpty() ) {
            QMessageBox::warning( 0, "Warning", "Enter vector name" );
            continue;
        }

        if ( Vect_legal_filename ( (char *) name.ascii() ) != 1 )
        {
	    QMessageBox::warning( 0, "Warning", "The name is not valid. "
                        "A vector name can contain letters, digits and underscores "
                        "and it must start with letter." );
            continue;
        } 

        // Check if exists
        QString head = QgsGrass::getDefaultGisdbase() + "/" 
                       + QgsGrass::getDefaultLocation() + "/"
		       + QgsGrass::getDefaultMapset() + "/vector/" 
		       + name + "/head"; 

        QFile file(head);
        if ( file.exists() )
        {
             int ret = QMessageBox::question ( 0, "Warning", "The vector exists. "
			  "Overwrite? ",  QMessageBox::Yes,  QMessageBox::No );
             
             if ( ret == QMessageBox::No ) continue;
        }

        break;
    }
    
    // Create new map
    QgsGrass::setMapset ( QgsGrass::getDefaultGisdbase(), 
                          QgsGrass::getDefaultLocation(),
                          QgsGrass::getDefaultMapset() );

    QgsGrass::resetError();
    struct Map_info Map;
    Vect_open_new ( &Map, (char *) name.ascii(), 0);

    if ( QgsGrass::getError() == QgsGrass::FATAL ) 
    {
        QMessageBox::warning( 0, "Warning", "Cannot create new vector: "
                      + QgsGrass::getErrorMessage() );
        return;
    }
 
    Vect_build ( &Map, stderr );
    Vect_set_release_support ( &Map );
    Vect_close ( &Map );
 
    // Open in GRASS vector provider
    QgsProviderRegistry *pr = QgsProviderRegistry::instance();

    QString uri = QgsGrass::getDefaultGisdbase() + "/" 
                 + QgsGrass::getDefaultLocation() + "/" 
                 + QgsGrass::getDefaultMapset() + "/" 
                 + name + "/0_point";

    QgsGrassProvider *provider = (QgsGrassProvider *) 
                       pr->getProvider ( "grass", uri );

    if ( !provider ) 
    {
        QMessageBox::warning( 0, "Warning", "New vector created "
                  "but cannot beopened by data provider." );
        return;
    }

    QgsGrassEdit *ed = new QgsGrassEdit( qgisMainWindowPointer, 
               qGisInterface, provider, qgisMainWindowPointer, 
               0, Qt::WType_Dialog );

    if ( ed->isValid() ) {
        ed->show();
        mCanvas->refresh();
    } else {
        QMessageBox::warning( 0, "Warning", "Cannot start editing." );
        delete ed;
    }
/*
    if ( !(mProvider->startEdit()) ) {
        QMessageBox::warning( 0, "Warning", "Cannot open vector for update." );
        return;
    }
*/
    
}

void QgsGrassPlugin::postRender(QPainter *painter)
{
#ifdef QGISDEBUG
  std::cout << "QgsGrassPlugin::postRender()" << std::endl;
#endif

  if ( QgsGrass::activeMode() && mRegionAction->isEnabled() && mRegionAction->isOn() ) {
    displayRegion(painter);
  }
}

void QgsGrassPlugin::displayRegion(QPainter *painter)
{
#ifdef QGISDEBUG
  std::cout << "QgsGrassPlugin::displayRegion()" << std::endl;
#endif


  // Display region of current mapset if in active mode
  if ( !QgsGrass::activeMode() ) return;

  QString gisdbase = QgsGrass::getDefaultGisdbase();
  QString location = QgsGrass::getDefaultLocation();
  QString mapset   = QgsGrass::getDefaultMapset();

  if ( gisdbase.isEmpty() || location.isEmpty() || mapset.isEmpty() ) {
    QMessageBox::warning( 0, "Warning", "GISDBASE, LOCATION_NAME or MAPSET is not set, "
        "cannot display current region." );
    return;
  }

  QgsGrass::setLocation ( gisdbase, location );

  struct Cell_head window;
  char *err = G__get_window ( &window, "", "WIND", (char *) mapset.latin1() );

  if ( err ) {
    QMessageBox::warning( 0, "Warning", "Cannot read current region: " + QString(err) );
    return;
  }

  std::vector<QgsPoint> points;
  points.resize(5);

  points[0].setX(window.west); points[0].setY(window.south);
  points[1].setX(window.east); points[1].setY(window.south);
  points[2].setX(window.east); points[2].setY(window.north);
  points[3].setX(window.west); points[3].setY(window.north);
  points[4].setX(window.west); points[4].setY(window.south);

  QgsMapToPixel *transform = mCanvas->getCoordinateTransform();
  Q3PointArray pointArray(5);

  for ( int i = 0; i < 5; i++ ) {
    transform->transform( &(points[i]) );
    pointArray.setPoint( i, 
        static_cast<int>(points[i].x()), 
        static_cast<int>(points[i].y()) );
  }

  painter->setPen ( mRegionPen );
  painter->drawPolyline ( pointArray );
}

void QgsGrassPlugin::switchRegion(bool on)
{
#ifdef QGISDEBUG
  std::cout << "QgsGrassPlugin::switchRegion()" << std::endl;
#endif

  QSettings settings("QuantumGIS", "qgis");
  settings.writeEntry ("/GRASS/region/on", on );

  QPixmap *pixmap = mCanvas->canvasPixmap();
  QPainter p;
  p.begin(pixmap);

  if ( on ) {
    displayRegion(&p);
  } else {
    // This is not perfect, but user can see reaction and it is fast
    QPen pen = mRegionPen;
    mRegionPen.setColor( QColor(255,255,255) ); // TODO: background color
    displayRegion(&p);
    mRegionPen = pen;
  }

  p.end();
  mCanvas->repaint(false);
}

void QgsGrassPlugin::changeRegion(void)
{
#ifdef QGISDEBUG
  std::cout << "QgsGrassPlugin::changeRegion()" << std::endl;
#endif

  if ( QgsGrassRegion::isRunning() ) {
    QMessageBox::warning( 0, "Warning", "The Region tool is already running." );
    return;
  }

  QgsGrassRegion *reg = new QgsGrassRegion(this, qgisMainWindowPointer, qGisInterface, 
      qgisMainWindowPointer, 0, 
      Qt::WType_Dialog );

  reg->show();
}

QPen & QgsGrassPlugin::regionPen()
{
  return mRegionPen;
}

void QgsGrassPlugin::setRegionPen(QPen & pen)
{
  mRegionPen = pen;

  QSettings settings("QuantumGIS", "qgis");
  settings.writeEntry ("/GRASS/region/color", mRegionPen.color().name() );
  settings.writeEntry ("/GRASS/region/width", (int) mRegionPen.width() );
}

void QgsGrassPlugin::openMapset()
{
#ifdef QGISDEBUG
    std::cerr << "QgsGrassPlugin::openMapset()" << std::endl;
#endif

    QString element;

    QgsGrassSelect *sel = new QgsGrassSelect( QgsGrassSelect::MAPSET );

    if ( !sel->exec() ) return;

    QString err = QgsGrass::openMapset ( sel->gisdbase, 
	                           sel->location, sel->mapset );

    if ( !err.isNull() )
    {
        QMessageBox::warning( 0, "Warning", "Cannot open the mapset. " + err );
        return;
    }
        
    mapsetChanged();
}

void QgsGrassPlugin::closeMapset()
{
#ifdef QGISDEBUG
    std::cerr << "QgsGrassPlugin::closeMapset()" << std::endl;
#endif

    QString err = QgsGrass::closeMapset ();

    if ( !err.isNull() )
    {
        QMessageBox::warning( 0, "Warning", "Cannot close mapset. " + err );
        return;
    }
        
    mapsetChanged();
}

void QgsGrassPlugin::newMapset()
{
    if ( QgsGrassNewMapset::isRunning() )
    {
        QMessageBox::warning( 0, "Warning", 
           "New GRASS Mapset wizard is already running." );
        return;
    }

    QgsGrassNewMapset *nm = new QgsGrassNewMapset ( 
	qgisMainWindowPointer, qGisInterface, this, qgisMainWindowPointer );
    nm->show();
}

// Unload the plugin by cleaning up the GUI
void QgsGrassPlugin::unload()
{
  // Close mapset
  QString err = QgsGrass::closeMapset();

  // remove the GUI
  for (int i = 0; i < menuId.size(); ++i)
    qGisInterface->removePluginMenuItem("&GRASS", menuId[i]);

  if ( toolBarPointer )
    delete toolBarPointer;
}
/** 
 * Required extern functions needed  for every plugin 
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
extern "C" QgisPlugin * classFactory(QgisApp * theQGisAppPointer, QgisIface * theQgisInterfacePointer)
{
  return new QgsGrassPlugin(theQGisAppPointer, theQgisInterfacePointer);
}

// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
extern "C" QString name()
{
  return QString("GRASS");
}

// Return the description
extern "C" QString description()
{
  return QString("GRASS layer");
}

// Return the type (either UI or MapLayer plugin)
extern "C" int type()
{
  return QgisPlugin::UI;
}

// Return the version number for the plugin
extern "C" QString version()
{
  return pluginVersion;
}

// Delete ourself
extern "C" void unload(QgisPlugin * thePluginPointer)
{
  delete thePluginPointer;
}
