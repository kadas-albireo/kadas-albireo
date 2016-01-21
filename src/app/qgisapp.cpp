/***************************************************************************
  qgisapp.cpp  -  description
  -------------------

           begin                : Sat Jun 22 2002
           copyright            : (C) 2002 by Gary E.Sherman
           email                : sherman at mrcc.com
           Romans 3:23=>Romans 6:23=>Romans 10:9,10=>Romans 12
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//
// QT4 includes make sure to use the new <CamelCase> style!
//
#include <QAction>
#include <QApplication>
#include <QBitmap>
#include <QClipboard>
#include <QColor>
#include <QCursor>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLibrary>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QProcess>
#include <QProgressDialog>
#include <QRegExp>
#include <QSettings>
#include <QSplashScreen>
#include <QStringList>
#include <QTapAndHoldGesture>
#include <QtGlobal>
#include <QTimer>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QThread>
//
// Mac OS X Includes
// Must include before GEOS 3 due to unqualified use of 'Point'
//
#ifdef Q_OS_MACX
#include <ApplicationServices/ApplicationServices.h>

// check macro breaks QItemDelegate
#ifdef check
#undef check
#endif
#endif

//
// QGIS Specific Includes
//
#include "qgisapp.h"
#include "qgisappinterface.h"
#include "qgisappstylesheet.h"
#include "qgis.h"
#include "qgisplugin.h"
#include "qgsabout.h"
#include "qgsapplication.h"
#include "qgsattributeaction.h"
#include "qgsattributetabledialog.h"
#include "qgsbookmarks.h"
#include "qgsbrowserdockwidget.h"
#include "qgsadvanceddigitizingdockwidget.h"
#include "qgsclipboard.h"
#include "qgscomposer.h"
#include "qgscomposition.h"
#include "qgscomposermanager.h"
#include "qgscomposerview.h"
#include "qgsconfigureshortcutsdialog.h"
#include "qgscoordinatetransform.h"
#include "qgscredentialdialog.h"
#include "qgscursors.h"
#include "qgscustomization.h"
#include "qgscustomprojectiondialog.h"
#include "qgsdatasourceuri.h"
#include "qgsdatumtransformdialog.h"
#include "qgsdxfexport.h"
#include "qgsdxfexportdialog.h"
#include "qgsdecorationcopyright.h"
#include "qgsdecorationnortharrow.h"
#include "qgsdecorationscalebar.h"
#include "qgsdecorationgrid.h"
#include "qgsencodingfiledialog.h"
#include "qgserror.h"
#include "qgserrordialog.h"
#include "qgsexception.h"
#include "qgsexpressionselectiondialog.h"
#include "qgsfeature.h"
#include "qgsfieldcalculator.h"
#include "qgsgenericprojectionselector.h"
#include "qgsgeoimageannotationitem.h"
#include "qgsgpsinformationwidget.h"
#include "qgsgpsrouteeditor.h"
#include "qgsguivectorlayertools.h"
#include "qgshtmlannotationitem.h"
#include "qgskmlexport.h"
#include "qgskmlexportdialog.h"
#include "qgslabelinggui.h"
#include "qgslayerdefinition.h"
#include "qgslayertree.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgslayertreemodel.h"
#include "qgslayertreeregistrybridge.h"
#include "qgslayertreeutils.h"
#include "qgslayertreeview.h"
#include "qgsgeoimageannotationitem.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgslegendgroupproperties.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvascontextmenu.h"
#include "qgsmapcanvasmap.h"
#include "qgsmapcanvassnappingutils.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaplayerstyleguiutils.h"
#include "qgsmapoverviewcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsmapsettings.h"
#include "qgsmaptip.h"
#include "qgsmergeattributesdialog.h"
#include "qgsmessageviewer.h"
#include "qgsmessagebar.h"
#include "qgsmessagebaritem.h"
#include "qgsmimedatautils.h"
#include "qgsmessagelog.h"
#include "qgsmultibandcolorrenderer.h"
#include "qgsmultimapmanager.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsnewvectorlayerdialog.h"
#include "qgsnewmemorylayerdialog.h"
#include "qgsoptions.h"
#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"
#include "qgspluginmanager.h"
#include "qgspluginregistry.h"
#include "qgspoint.h"
#include "qgshandlebadlayers.h"
#include "qgsproject.h"
#include "qgsprojectlayergroupdialog.h"
#include "qgsprojectproperties.h"
#include "qgsproviderregistry.h"
#include "qgspythonrunner.h"
#include "qgsquerybuilder.h"
#include "qgsrastercalcdialog.h"
#include "qgsrasterfilewriter.h"
#include "qgsrasteriterator.h"
#include "qgsrasterlayer.h"
#include "qgsrasterlayerproperties.h"
#include "qgsrasternuller.h"
#include "qgsbrightnesscontrastfilter.h"
#include "qgsrasterrenderer.h"
#include "qgsrasterlayersaveasdialog.h"
#include "qgsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsrectangle.h"
#include "qgsscalecombobox.h"
#include "qgsscalevisibilitydialog.h"
#include "qgsshortcutsmanager.h"
#include "qgssinglebandgrayrenderer.h"
#include "qgssnappingdialog.h"
#include "qgssourceselectdialog.h"
#include "qgssponsors.h"
#include "qgstemporaryfile.h"
#include "qgstipgui.h"
#include "qgsundowidget.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorfilewriter.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerproperties.h"
#include "qgsvisibilitypresets.h"
#include "qgsmessagelogviewer.h"
#include "qgsdataitem.h"
#include "qgsmaplayeractionregistry.h"

#include "qgssublayersdialog.h"
#include "ogr/qgsopenvectorlayerdialog.h"
#include "ogr/qgsvectorlayersaveasdialog.h"

#include "qgsosmdownloaddialog.h"
#include "qgsosmimportdialog.h"
#include "qgsosmexportdialog.h"

#ifdef ENABLE_MODELTEST
#include "modeltest.h"
#endif

//
// GDAL/OGR includes
//
#include <ogr_api.h>
#include <proj_api.h>

//
// Other includes
//
#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iomanip>
#include <list>
#include <memory>
#include <vector>

//
// Map tools
//
#include "qgsmaptooladdfeature.h"
#include "qgsmaptooladdpart.h"
#include "qgsmaptooladdring.h"
#include "qgsmaptoolfillring.h"
#include "qgsmaptoolannotation.h"
#include "qgsmaptoolcircularstringcurvepoint.h"
#include "qgsmaptoolcircularstringradius.h"
#include "qgsmaptooldeletering.h"
#include "qgsmaptooldeletepart.h"
#include "qgsmaptoolfeatureaction.h"
#include "qgsmaptoolformannotation.h"
#include "qgsmaptoolhtmlannotation.h"
#include "qgsmaptoolidentifyaction.h"
#include "qgsmaptoolmeasureangle.h"
#include "qgsmaptoolmovefeature.h"
#include "qgsmaptoolrotatefeature.h"
#include "qgsmaptooloffsetcurve.h"
#include "qgsmaptoolpan.h"
#include "qgsmaptoolpinannotation.h"
#include "qgsmaptoolselect.h"
#include "qgsmaptoolselectrectangle.h"
#include "qgsmaptoolselectfreehand.h"
#include "qgsmaptoolselectpolygon.h"
#include "qgsmaptoolselectradius.h"
#include "qgsmaptoolsvgannotation.h"
#include "qgsmaptoolreshape.h"
#include "qgsmaptoolrotatepointsymbols.h"
#include "qgsmaptoolsplitfeatures.h"
#include "qgsmaptoolsplitparts.h"
#include "qgsmaptooltextannotation.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolsimplify.h"
#include "qgsmeasuretool.h"
#include "qgsmeasuretoolv2.h"
#include "qgsmeasurecircletool.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgsmaptoolpinlabels.h"
#include "qgsmaptoolshowhidelabels.h"
#include "qgsmaptoolmovelabel.h"
#include "qgsmaptoolrotatelabel.h"
#include "qgsmaptoolchangelabelproperties.h"
#include "qgsmaptoolslope.h"
#include "qgsmaptoolhillshade.h"
#include "qgsmaptoolviewshed.h"

#include "nodetool/qgsmaptoolnodetool.h"

// Editor widgets
#include "qgseditorwidgetregistry.h"
//
// Conditional Includes
//
#ifdef HAVE_PGCONFIG
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include <pg_config.h>
#else
#define PG_VERSION "unknown"
#endif

#include <sqlite3.h>

extern "C"
{
#include <spatialite.h>
}
#include "qgsnewspatialitelayerdialog.h"

#include "qgspythonutils.h"

#ifndef _MSC_VER
#include <dlfcn.h>
#else
#include <windows.h>
#include <DbgHelp.h>
#endif

class QTreeWidgetItem;

/** set the application title bar text

  If the current project title is null
  if the project file is null then
  set title text to just application name and version
  else
  set set title text to the project file name
  else
  set the title text to project title
  */
static void setTitleBarText_( QWidget & qgisApp )
{
  QString caption = QgisApp::tr( "QGIS Enterprise " );

  if ( QString( QGis::QGIS_VERSION ).endsWith( "Dev" ) )
  {
    caption += QString( "%1" ).arg( QGis::QGIS_DEV_VERSION );
  }
  else
  {
    caption += QGis::QGIS_VERSION;
  }

  if ( QgsProject::instance()->title().isEmpty() )
  {
    if ( QgsProject::instance()->fileName().isEmpty() )
    {
      // no project title nor file name, so just leave caption with
      // application name and version
    }
    else
    {
      QFileInfo projectFileInfo( QgsProject::instance()->fileName() );
      caption += " - " + projectFileInfo.completeBaseName();
    }
  }
  else
  {
    caption += " - " + QgsProject::instance()->title();
  }

  qgisApp.setWindowTitle( caption );
} // setTitleBarText_( QWidget * qgisApp )

/**
 Creator function for output viewer
*/
static QgsMessageOutput *messageOutputViewer_()
{
  if ( QThread::currentThread() == QApplication::instance()->thread() )
    return new QgsMessageViewer( QgisApp::instance() );
  else
    return new QgsMessageOutputConsole();
}

static void customSrsValidation_( QgsCoordinateReferenceSystem &srs )
{
  QgisApp::instance()->emitCustomSrsValidation( srs );
}

void QgisApp::emitCustomSrsValidation( QgsCoordinateReferenceSystem &srs )
{
  emit customSrsValidation( srs );
}

QgsLegendInterface* QgisApp::legendInterface() const
{
  return mQgisInterface->legendInterface();
}

void QgisApp::layerTreeViewDoubleClicked( const QModelIndex& index )
{
  Q_UNUSED( index )
  QSettings settings;
  switch ( settings.value( "/qgis/legendDoubleClickAction", 0 ).toInt() )
  {
    case 0:
      QgisApp::instance()->layerProperties();
      break;
    case 1:
      QgisApp::instance()->attributeTable();
      break;
    default:
      break;
  }
}

void QgisApp::activeLayerChanged( QgsMapLayer* layer )
{
  mapCanvas()->setCurrentLayer( layer );
}

/**
 * This function contains forced validation of CRS used in QGIS.
 * There are 3 options depending on the settings:
 * - ask for CRS using projection selecter
 * - use project's CRS
 * - use predefined global CRS
 */
void QgisApp::validateSrs( QgsCoordinateReferenceSystem &srs )
{
  static QString authid = QString::null;
  QSettings mySettings;
  QString myDefaultProjectionOption = mySettings.value( "/Projections/defaultBehaviour", "prompt" ).toString();
  if ( myDefaultProjectionOption == "prompt" )
  {
    // @note this class is not a descendent of QWidget so we can't pass
    // it in the ctor of the layer projection selector

    QgsGenericProjectionSelector *mySelector = new QgsGenericProjectionSelector();
    mySelector->setMessage( srs.validationHint() ); //shows a generic message, if not specified
    if ( authid.isNull() )
      authid = QgisApp::instance()->mapCanvas()->mapSettings().destinationCrs().authid();

    QgsCoordinateReferenceSystem defaultCrs;
    if ( defaultCrs.createFromOgcWmsCrs( authid ) )
    {
      mySelector->setSelectedCrsId( defaultCrs.srsid() );
    }

    bool waiting = QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::WaitCursor;
    if ( waiting )
      QApplication::setOverrideCursor( Qt::ArrowCursor );

    if ( mySelector->exec() )
    {
      QgsDebugMsg( "Layer srs set from dialog: " + QString::number( mySelector->selectedCrsId() ) );
      authid = mySelector->selectedAuthId();
      srs.createFromOgcWmsCrs( mySelector->selectedAuthId() );
    }

    if ( waiting )
      QApplication::restoreOverrideCursor();

    delete mySelector;
  }
  else if ( myDefaultProjectionOption == "useProject" )
  {
    // XXX TODO: Change project to store selected CS as 'projectCRS' not 'selectedWkt'
    authid = QgisApp::instance()->mapCanvas()->mapSettings().destinationCrs().authid();
    srs.createFromOgcWmsCrs( authid );
    QgsDebugMsg( "Layer srs set from project: " + authid );
    messageBar()->pushMessage( tr( "CRS was undefined" ), tr( "defaulting to project CRS %1 - %2" ).arg( authid ).arg( srs.description() ), QgsMessageBar::WARNING, messageTimeout() );
  }
  else ///Projections/defaultBehaviour==useGlobal
  {
    authid = mySettings.value( "/Projections/layerDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
    srs.createFromOgcWmsCrs( authid );
    QgsDebugMsg( "Layer srs set from default: " + authid );
    messageBar()->pushMessage( tr( "CRS was undefined" ), tr( "defaulting to CRS %1 - %2" ).arg( authid ).arg( srs.description() ), QgsMessageBar::WARNING, messageTimeout() );
  }
}

QgisApp *QgisApp::smInstance = 0;

// constructor starts here
QgisApp::QgisApp( QSplashScreen *splash, QWidget * parent, Qt::WindowFlags fl )
    : QMainWindow( parent, fl )
#ifdef Q_OS_WIN
    , mSkipNextContextMenuEvent( 0 )
#endif
    , mLayerTreeCanvasBridge( 0 )
    , mSplash( splash )
    , mMousePrecisionDecimalPlaces( 0 )
    , mInternalClipboard( 0 )
    , mShowProjectionTab( false )
    , mPythonUtils( 0 )
    , mComposerManager( 0 )
    , mpTileScaleWidget( 0 )
    , mpGpsWidget( 0 )
    , mSnappingUtils( 0 )
    , mRedlining( 0 )
    , mGpsRouteEditor( 0 )
{
  if ( smInstance )
  {
    QMessageBox::critical(
      this,
      tr( "Multiple Instances of QgisApp" ),
      tr( "Multiple instances of QGIS application object detected.\nPlease contact the developers.\n" ) );
    abort();
  }

  smInstance = this;
}


void QgisApp::init( bool restorePlugins )
{
  namSetup();

  //////////

  mSplash->showMessage( tr( "Checking database" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();
  // Do this early on before anyone else opens it and prevents us copying it
  QString dbError;
  if ( !QgsApplication::createDB( &dbError ) )
  {
    QMessageBox::critical( this, tr( "Private qgis.db" ), dbError );
  }

  mSplash->showMessage( tr( "Reading settings" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();

  mSplash->showMessage( tr( "Setting up the GUI" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();

  QSettings settings;

  // set up stylesheet builder and apply saved or default style options
  mStyleSheetBuilder = new QgisAppStyleSheet( this );
  connect( mStyleSheetBuilder, SIGNAL( appStyleSheetChanged( const QString& ) ),
           this, SLOT( setAppStyleSheet( const QString& ) ) );
  mStyleSheetBuilder->buildStyleSheet( mStyleSheetBuilder->defaultOptions() );

  // "theMapCanvas" used to find this canonical instance later
  mapCanvas()->setObjectName( "theMapCanvas" );
  mapCanvas()->setWhatsThis( tr( "Map canvas. This is where raster and vector "
                                 "layers are displayed when added to the map" ) );

  // set canvas color right away
  int myRed = settings.value( "/qgis/default_canvas_color_red", 255 ).toInt();
  int myGreen = settings.value( "/qgis/default_canvas_color_green", 255 ).toInt();
  int myBlue = settings.value( "/qgis/default_canvas_color_blue", 255 ).toInt();
  int myAlpha = settings.value( "/qgis/default_canvas_color_alpha", 0 ).toInt();
  mapCanvas()->setCanvasColor( QColor( myRed, myGreen, myBlue, myAlpha ) );

  //set the focus to the map canvas
  mapCanvas()->setFocus();

  layerTreeView()->setObjectName( "theLayerTreeView" ); // "theLayerTreeView" used to find this canonical instance later

  // create undo widget
  mUndoWidget = new QgsUndoWidget( NULL, mapCanvas() );
  mUndoWidget->setObjectName( "Undo" );

  // Advanced Digitizing dock
  mAdvancedDigitizingDockWidget = new QgsAdvancedDigitizingDockWidget( mapCanvas(), this );
  mAdvancedDigitizingDockWidget->setObjectName( "AdvancedDigitizingTools" );

  mSnappingUtils = new QgsMapCanvasSnappingUtils( mapCanvas(), this );
  mapCanvas()->setSnappingUtils( mSnappingUtils );
  connect( QgsProject::instance(), SIGNAL( snapSettingsChanged() ), mSnappingUtils, SLOT( readConfigFromProject() ) );
  connect( this, SIGNAL( projectRead() ), mSnappingUtils, SLOT( readConfigFromProject() ) );

  connect( printComposersMenu(), SIGNAL( aboutToShow() ), this, SLOT( preparePrintComposersMenu() ) );

  createCanvasTools();
  mapCanvas()->freeze();
  createOverview();
  createMapTips();
  createDecorations();
  readSettings();
  updateRecentProjectPaths();
  updateProjectFromTemplates();
  mSaveRollbackInProgress = false;

  // Multi map manager
  mMultiMapManager = new QgsMultiMapManager( mapCanvas(), this );
  connect( this, SIGNAL( newProject() ), mMultiMapManager, SLOT( clearMapWidgets() ) );

  // initialize the plugin manager
  mPluginManager = new QgsPluginManager( this, restorePlugins );

  addDockWidget( Qt::LeftDockWidgetArea, mUndoWidget );
  mUndoWidget->hide();

  mSnappingDialog = new QgsSnappingDialog( this, mapCanvas() );
  mSnappingDialog->setObjectName( "SnappingOption" );

  mBrowserWidget = new QgsBrowserDockWidget( tr( "Browser" ), this );
  mBrowserWidget->setObjectName( "Browser" );
  addDockWidget( Qt::LeftDockWidgetArea, mBrowserWidget );
  mBrowserWidget->hide();

  mBrowserWidget2 = new QgsBrowserDockWidget( tr( "Browser (2)" ), this );
  mBrowserWidget2->setObjectName( "Browser2" );
  addDockWidget( Qt::LeftDockWidgetArea, mBrowserWidget2 );
  mBrowserWidget2->hide();

  addDockWidget( Qt::LeftDockWidgetArea, mAdvancedDigitizingDockWidget );
  mAdvancedDigitizingDockWidget->hide();

  // create the GPS tool on starting QGIS - this is like the browser
  mpGpsWidget = new QgsGPSInformationWidget( mapCanvas() );
  //create the dock widget
  mpGpsDock = new QDockWidget( tr( "GPS Information" ), this );
  mpGpsDock->setObjectName( "GPSInformation" );
  mpGpsDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
  addDockWidget( Qt::LeftDockWidgetArea, mpGpsDock );
  // add to the Panel submenu
  // now add our widget to the dock - ownership of the widget is passed to the dock
  mpGpsDock->setWidget( mpGpsWidget );
  mpGpsDock->hide();

  mLastMapToolMessage = 0;

  mLogViewer = new QgsMessageLogViewer( statusBar(), this );

  mLogDock = new QDockWidget( tr( "Log Messages" ), this );
  mLogDock->setObjectName( "MessageLog" );
  mLogDock->setAllowedAreas( Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea );
  addDockWidget( Qt::BottomDockWidgetArea, mLogDock );
  mLogDock->setWidget( mLogViewer );
  mLogDock->hide();
  mVectorLayerTools = new QgsGuiVectorLayerTools();

  // Init the editor widget types
  QgsEditorWidgetRegistry::initEditors( mapCanvas(), messageBar() );

  mInternalClipboard = new QgsClipboard; // create clipboard
  connect( mInternalClipboard, SIGNAL( changed() ), this, SLOT( clipboardChanged() ) );
  mQgisInterface = new QgisAppInterface( this ); // create the interfce

#ifdef Q_OS_MAC
  // action for Window menu (create before generating WindowTitleChange event))
  mWindowAction = new QAction( this );
  connect( mWindowAction, SIGNAL( triggered() ), this, SLOT( activate() ) );

  // add this window to Window menu
  addWindow( mWindowAction );
#endif

  activateDeactivateLayerRelatedActions( NULL ); // after members were created

  // set application's caption
  QString caption = tr( "QGIS - %1 ('%2')" ).arg( QGis::QGIS_VERSION ).arg( QGis::QGIS_RELEASE_NAME );
  setWindowTitle( caption );

  QgsMessageLog::logMessage( tr( "QGIS starting..." ), QString::null, QgsMessageLog::INFO );

  // set QGIS specific srs validation
  connect( this, SIGNAL( customSrsValidation( QgsCoordinateReferenceSystem& ) ),
           this, SLOT( validateSrs( QgsCoordinateReferenceSystem& ) ) );
  QgsCoordinateReferenceSystem::setCustomSrsValidation( customSrsValidation_ );

  // set graphical message output
  QgsMessageOutput::setMessageOutputCreator( messageOutputViewer_ );

  // set graphical credential requester
  new QgsCredentialDialog( this );

  qApp->processEvents();

  // load providers
  mSplash->showMessage( tr( "Checking provider plugins" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();
  QgsApplication::initQgis();

  mSplash->showMessage( tr( "Starting Python" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();
  loadPythonSupport();

  // Create the plugin registry and load plugins
  // load any plugins that were running in the last session
  mSplash->showMessage( tr( "Restoring loaded plugins" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();
  QgsPluginRegistry::instance()->setQgisInterface( mQgisInterface );
  if ( restorePlugins )
  {
    // Restoring of plugins can be disabled with --noplugins command line option
    // because some plugins may cause QGIS to crash during startup
    QgsPluginRegistry::instance()->restoreSessionPlugins( QgsApplication::pluginPath() );

    // Also restore plugins from user specified plugin directories
    QString myPaths = settings.value( "plugins/searchPathsForPlugins", "" ).toString();
    if ( !myPaths.isEmpty() )
    {
      QStringList myPathList = myPaths.split( "|" );
      QgsPluginRegistry::instance()->restoreSessionPlugins( myPathList );
    }
  }

  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    // initialize the plugin installer to start fetching repositories in background
    QgsPythonRunner::run( "import pyplugin_installer" );
    QgsPythonRunner::run( "pyplugin_installer.initPluginInstaller()" );
    // enable Python in the Plugin Manager and pass the PythonUtils to it
    mPluginManager->setPythonUtils( mPythonUtils );
  }

  mSplash->showMessage( tr( "Initializing file filters" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();

  // now build vector and raster file filters
  mVectorFileFilter = QgsProviderRegistry::instance()->fileVectorFilters();
  mRasterFileFilter = QgsProviderRegistry::instance()->fileRasterFilters();

  // set handler for missing layers (will be owned by QgsProject)
  QgsProject::instance()->setBadLayerHandler( new QgsHandleBadLayersHandler() );

#if 0
  // Set the background color for toolbox and overview as they default to
  // white instead of the window color
  QPalette myPalette = toolBox->palette();
  myPalette.setColor( QPalette::Button, myPalette.window().color() );
  toolBox->setPalette( myPalette );
  //do the same for legend control
  myPalette = toolBox->palette();
  myPalette.setColor( QPalette::Button, myPalette.window().color() );
  mMapLegend->setPalette( myPalette );
  //and for overview control this is done in createOverView method
#endif
  // Do this last in the ctor to ensure that all members are instantiated properly
  setupConnections();
  //
  // Please make sure this is the last thing the ctor does so that we can ensure the
  // widgets are all initialised before trying to restore their state.
  //
  mSplash->showMessage( tr( "Restoring window state" ), Qt::AlignHCenter | Qt::AlignBottom );
  qApp->processEvents();
  restoreWindowState();

  mSplash->showMessage( tr( "QGIS Ready!" ), Qt::AlignHCenter | Qt::AlignBottom );

  QgsMessageLog::logMessage( QgsApplication::showSettings(), QString::null, QgsMessageLog::INFO );

  QgsMessageLog::logMessage( tr( "QGIS Ready!" ), QString::null, QgsMessageLog::INFO );

  mMapTipsVisible = false;
  mTrustedMacros = false;

  // setup drag drop
  setAcceptDrops( true );

  mFullScreenMode = false;
  mPrevScreenModeMaximized = false;
  show();
  qApp->processEvents();

  mapCanvas()->freeze( false );
  mapCanvas()->clearExtentHistory(); // reset zoomnext/zoomlast
  mLastComposerId = 0;

  // Show a nice tip of the day
  /*if ( settings.value( QString( "/qgis/showTips%1" ).arg( QGis::QGIS_VERSION_INT / 100 ), true ).toBool() )
  {
    mSplash->hide();
    QgsTipGui myTip;
    myTip.exec();
  }
  else
  {
    QgsDebugMsg( "Tips are disabled" );
  }*/

  //add reacting to long click in touch
#pragma message("WARNING: disabled because it interferes with drag and drop as well as context menu right click")
//  grabGesture( Qt::TapAndHoldGesture );

  // supposedly all actions have been added, now register them to the shortcut manager
  QgsShortcutsManager::instance()->registerAllChildrenActions( this );

  QgsProviderRegistry::instance()->registerGuis( this );

  // update windows
  qApp->processEvents();

  fileNewBlank(); // prepare empty project, also skips any default templates from loading

  // request notification of FileOpen events (double clicking a file icon in Mac OS X Finder)
  // should come after fileNewBlank to ensure project is properly set up to receive any data source files
  QgsApplication::setFileOpenEventReceiver( this );

#ifdef ANDROID
  toggleFullScreen();
#endif

} // QgisApp ctor

QgisApp::QgisApp()
    : QMainWindow( 0, 0 )
    , mLayerTreeCanvasBridge( 0 )
    , mMapLayerOrder( 0 )
    , mOverviewMapCursor( 0 )
    , mMapWindow( 0 )
    , mQgisInterface( 0 )
    , mSplash( 0 )
    , mMousePrecisionDecimalPlaces( 0 )
    , mInternalClipboard( 0 )
    , mShowProjectionTab( false )
    , mpMapTipsTimer( 0 )
    , mDizzyTimer( 0 )
    , mpMaptip( 0 )
    , mMapTipsVisible( false )
    , mFullScreenMode( false )
    , mPrevScreenModeMaximized( false )
    , mSaveRollbackInProgress( false )
    , mPythonUtils( 0 )
    , mBrowserWidget( 0 )
    , mBrowserWidget2( 0 )
    , mAdvancedDigitizingDockWidget( 0 )
    , mSnappingDialog( 0 )
    , mPluginManager( 0 )
    , mComposerManager( 0 )
    , mpTileScaleWidget( 0 )
    , mLastComposerId( 0 )
    , mpGpsWidget( 0 )
    , mLastMapToolMessage( 0 )
    , mLogViewer( 0 )
    , mTrustedMacros( false )
    , mMacrosWarn( 0 )
    , mVectorLayerTools( 0 )
    , mBtnFilterLegend( 0 )
    , mSnappingUtils( 0 )
{
  smInstance = this;
  mInternalClipboard = new QgsClipboard;
  // More tests may need more members to be initialized
}

void QgisApp::destroy()
{
  QgsPluginRegistry::instance()->unloadAll();

  mapCanvas()->stopRendering();
  mapCanvas()->setMapTool( 0 );
  disconnect( mapCanvas(), SIGNAL( mapToolSet( QgsMapTool *, QgsMapTool * ) ),
              this, SLOT( mapToolChanged( QgsMapTool *, QgsMapTool * ) ) );

  delete mInternalClipboard;
  delete mQgisInterface;
  delete mStyleSheetBuilder;

  delete mMapTools.mZoomIn;
  delete mMapTools.mZoomOut;
  delete mMapTools.mPan;
  delete mMapTools.mAddFeature;
  delete mMapTools.mAddPart;
  delete mMapTools.mAddRing;
  delete mMapTools.mFillRing;
  delete mMapTools.mChangeLabelProperties;
  delete mMapTools.mDeletePart;
  delete mMapTools.mDeleteRing;
  delete mMapTools.mFeatureAction;
  delete mMapTools.mFormAnnotation;
  delete mMapTools.mHtmlAnnotation;
  delete mMapTools.mIdentify;
  delete mMapTools.mMeasureAngle;
  delete mMapTools.mMeasureArea;
  delete mMapTools.mMeasureDist;
  delete mMapTools.mMeasureCircle;
  delete mMapTools.mMeasureHeightProfile;
  delete mMapTools.mMoveFeature;
  delete mMapTools.mMoveLabel;
  delete mMapTools.mNodeTool;
  delete mMapTools.mOffsetCurve;
  delete mMapTools.mPinLabels;
  delete mMapTools.mReshapeFeatures;
  delete mMapTools.mRotateFeature;
  delete mMapTools.mRotateLabel;
  delete mMapTools.mRotatePointSymbolsTool;
  delete mMapTools.mSelectFreehand;
  delete mMapTools.mSelectPolygon;
  delete mMapTools.mSelectRadius;
  delete mMapTools.mSelectFeatures;
  delete mMapTools.mShowHideLabels;
  delete mMapTools.mSimplifyFeature;
  delete mMapTools.mSplitFeatures;
  delete mMapTools.mSplitParts;
  delete mMapTools.mSvgAnnotation;
  delete mMapTools.mTextAnnotation;
  delete mMapTools.mPinAnnotation;
  delete mMapTools.mSlope;
  delete mMapTools.mHillshade;
  delete mMapTools.mViewshed;
  delete mMapTools.mViewshedSector;

  delete mpMaptip;

  delete mpGpsWidget;

  delete mOverviewMapCursor;

  delete mComposerManager;

  delete mVectorLayerTools;

  delete mRedlining;
  delete mGpsRouteEditor;

  deletePrintComposers();
  removeAnnotationItems();

  // cancel request for FileOpen events
  QgsApplication::setFileOpenEventReceiver( 0 );

  QgsApplication::exitQgis();

  delete QgsProject::instance();

  delete mPythonUtils;

  QgsMapLayerStyleGuiUtils::cleanup();
}

void QgisApp::dragEnterEvent( QDragEnterEvent *event )
{
  if ( event->mimeData()->hasUrls() || event->mimeData()->hasFormat( "application/x-vnd.qgis.qgis.uri" ) )
  {
    event->acceptProposedAction();
  }
}

void QgisApp::dropEvent( QDropEvent *event )
{
  mapCanvas()->freeze();
  // get the file list
  QList<QUrl>::iterator i;
  QList<QUrl>urls = event->mimeData()->urls();
  for ( i = urls.begin(); i != urls.end(); ++i )
  {
    QString fileName = i->toLocalFile();
    // seems that some drag and drop operations include an empty url
    // so we test for length to make sure we have something
    if ( !fileName.isEmpty() )
    {
      openFile( fileName );
    }
  }

  if ( QgsMimeDataUtils::isUriList( event->mimeData() ) )
  {
    QgsMimeDataUtils::UriList lst = QgsMimeDataUtils::decodeUriList( event->mimeData() );
    foreach ( const QgsMimeDataUtils::Uri& u, lst )
    {
      QString uri = crsAndFormatAdjustedLayerUri( u.uri, u.supportedCrs, u.supportedFormats );

      if ( u.layerType == "vector" )
      {
        addVectorLayer( uri, u.name, u.providerKey );
      }
      else if ( u.layerType == "raster" )
      {
        addRasterLayer( uri, u.name, u.providerKey );
      }
    }
  }
  mapCanvas()->freeze( false );
  mapCanvas()->refresh();
  event->acceptProposedAction();
}

bool QgisApp::event( QEvent * event )
{
  bool done = false;
  if ( event->type() == QEvent::FileOpen )
  {
    // handle FileOpen event (double clicking a file icon in Mac OS X Finder)
    QFileOpenEvent *foe = static_cast<QFileOpenEvent *>( event );
    openFile( foe->file() );
    done = true;
  }
  else if ( event->type() == QEvent::Gesture )
  {
    done = gestureEvent( static_cast<QGestureEvent*>( event ) );
  }
  else
  {
    // pass other events to base class
    done = QMainWindow::event( event );
  }
  return done;
}

QgisAppStyleSheet* QgisApp::styleSheetBuilder()
{
  Q_ASSERT( mStyleSheetBuilder );
  return mStyleSheetBuilder;
}

// restore any application settings stored in QSettings
void QgisApp::readSettings()
{
  QSettings settings;
  // get the users theme preference from the settings
  // 'gis' theme is new /themes/default directory (2013-04-15)
  setTheme( settings.value( "/Themes", "default" ).toString() );

  // Add the recently accessed project file paths to the Project menu
  mRecentProjectPaths = settings.value( "/UI/recentProjectsList" ).toStringList();

  // this is a new session! reset enable macros value to "ask"
  // whether set to "just for this session"
  if ( settings.value( "/qgis/enableMacros", 1 ).toInt() == 2 )
  {
    settings.setValue( "/qgis/enableMacros", 1 );
  }
}


//////////////////////////////////////////////////////////////////////
//            Set Up the gui toolbars, menus, statusbar etc
//////////////////////////////////////////////////////////////////////

#include "qgsstylev2.h"
#include "qgsstylev2managerdialog.h"

void QgisApp::showStyleManagerV2()
{
  QgsStyleV2ManagerDialog dlg( QgsStyleV2::defaultStyle(), this );
  dlg.exec();
}

void QgisApp::writeAnnotationItemsToProject( QDomDocument& doc )
{
  QList<QgsAnnotationItem*> items = annotationItems();
  QList<QgsAnnotationItem*>::const_iterator itemIt = items.constBegin();
  for ( ; itemIt != items.constEnd(); ++itemIt )
  {
    if ( ! *itemIt )
    {
      continue;
    }
    ( *itemIt )->writeXML( doc );
  }
}

void QgisApp::showPythonDialog()
{
  if ( !mPythonUtils || !mPythonUtils->isEnabled() )
    return;

  bool res = mPythonUtils->runString(
               "import console\n"
               "console.show_console()\n", tr( "Failed to open Python console:" ), false );

  if ( !res )
  {
    QString className, text;
    mPythonUtils->getError( className, text );
    messageBar()->pushMessage( tr( "Error" ), tr( "Failed to open Python console:" ) + "\n" + className + ": " + text, QgsMessageBar::WARNING );
  }
#ifdef Q_OS_MAC
  else
  {
    addWindow( mActionShowPythonDialog );
  }
#endif
}

void QgisApp::setAppStyleSheet( const QString& stylesheet )
{
  setStyleSheet( stylesheet );

  // cascade styles to any current project composers
  foreach ( QgsComposer *c, mPrintComposers )
  {
    c->setStyleSheet( stylesheet );
  }
}

int QgisApp::messageTimeout()
{
  QSettings settings;
  return settings.value( "/qgis/messageTimeout", 5 ).toInt();
}

QAction* QgisApp::findAction( const QString& name ) const
{
  return findChild<QAction*>( name );
}

void QgisApp::setIconSizes( int size )
{
  //Set the icon size of for all the toolbars created in the future.
  setIconSize( QSize( size, size ) );

  //Change all current icon sizes.
  QList<QToolBar *> toolbars = findChildren<QToolBar *>();
  foreach ( QToolBar * toolbar, toolbars )
  {
    toolbar->setIconSize( QSize( size, size ) );
  }

  foreach ( QgsComposer *c, mPrintComposers )
  {
    c->setIconSizes( size );
  }
}

void QgisApp::setupConnections()
{
  // connect the "cleanup" slot
  connect( qApp, SIGNAL( aboutToQuit() ), this, SLOT( saveWindowState() ) );

  // signal when mouse moved over window (coords display in status bar)
  connect( mapCanvas(), SIGNAL( scaleChanged( double ) ),
           this, SLOT( updateMouseCoordinatePrecision() ) );
  connect( mapCanvas(), SIGNAL( mapToolSet( QgsMapTool *, QgsMapTool * ) ),
           this, SLOT( mapToolChanged( QgsMapTool *, QgsMapTool * ) ) );
  connect( mapCanvas(), SIGNAL( selectionChanged( QgsMapLayer * ) ),
           this, SLOT( selectionChanged( QgsMapLayer * ) ) );
  connect( mapCanvas(), SIGNAL( extentsChanged() ),
           this, SLOT( markDirty() ) );
  connect( mapCanvas(), SIGNAL( layersChanged() ),
           this, SLOT( markDirty() ) );

  // connect MapCanvas keyPress event so we can check if selected feature collection must be deleted
  connect( mapCanvas(), SIGNAL( keyPressed( QKeyEvent * ) ),
           this, SLOT( mapCanvas_keyPressed( QKeyEvent * ) ) );

  // connect renderer
  connect( mapCanvas(), SIGNAL( hasCrsTransformEnabledChanged( bool ) ),
           this, SLOT( hasCrsTransformEnabled( bool ) ) );
  connect( mapCanvas(), SIGNAL( destinationCrsChanged() ),
           this, SLOT( destinationCrsChanged() ) );

  // connect legend signals
  connect( layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer * ) ),
           this, SLOT( activateDeactivateLayerRelatedActions( QgsMapLayer * ) ) );
  connect( layerTreeView()->layerTreeModel()->rootGroup(), SIGNAL( addedChildren( QgsLayerTreeNode*, int, int ) ),
           this, SLOT( markDirty() ) );
  connect( layerTreeView()->layerTreeModel()->rootGroup(), SIGNAL( addedChildren( QgsLayerTreeNode*, int, int ) ),
           this, SLOT( updateNewLayerInsertionPoint() ) );
  connect( layerTreeView()->layerTreeModel()->rootGroup(), SIGNAL( removedChildren( QgsLayerTreeNode*, int, int ) ),
           this, SLOT( markDirty() ) );
  connect( layerTreeView()->layerTreeModel()->rootGroup(), SIGNAL( removedChildren( QgsLayerTreeNode*, int, int ) ),
           this, SLOT( updateNewLayerInsertionPoint() ) );
  connect( layerTreeView()->layerTreeModel()->rootGroup(), SIGNAL( visibilityChanged( QgsLayerTreeNode*, Qt::CheckState ) ),
           this, SLOT( markDirty() ) );
  connect( layerTreeView()->layerTreeModel()->rootGroup(), SIGNAL( customPropertyChanged( QgsLayerTreeNode*, QString ) ),
           this, SLOT( markDirty() ) );

  // connect map layer registry
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer *> ) ),
           this, SLOT( layersWereAdded( QList<QgsMapLayer *> ) ) );
  connect( QgsMapLayerRegistry::instance(),
           SIGNAL( layersWillBeRemoved( QStringList ) ),
           this, SLOT( removingLayers( QStringList ) ) );

  // connect initialization signal
  connect( this, SIGNAL( initializationCompleted() ),
           this, SLOT( fileOpenAfterLaunch() ) );

  // Connect warning dialog from project reading
  connect( QgsProject::instance(), SIGNAL( oldProjectVersionWarning( QString ) ),
           this, SLOT( oldProjectVersionWarning( QString ) ) );
  connect( QgsProject::instance(), SIGNAL( layerLoaded( int, int ) ),
           this, SIGNAL( progress( int, int ) ) );
  connect( QgsProject::instance(), SIGNAL( loadingLayer( QString ) ),
           this, SLOT( showStatusMessage( QString ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument & ) ),
           this, SLOT( readProject( const QDomDocument & ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument & ) ),
           this, SLOT( writeProject( QDomDocument & ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ),
           this, SLOT( writeAnnotationItemsToProject( QDomDocument& ) ) );

  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument & ) ), this, SLOT( loadComposersFromProject( const QDomDocument& ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument & ) ), this, SLOT( loadAnnotationItemsFromProject( const QDomDocument& ) ) );

  connect( this, SIGNAL( projectRead() ),
           this, SLOT( fileOpenedOKAfterLaunch() ) );

  // handle deprecated labels in project for QGIS 2.0
  connect( this, SIGNAL( newProject() ),
           this, SLOT( checkForDeprecatedLabelsInProject() ) );
  connect( this, SIGNAL( projectRead() ),
           this, SLOT( checkForDeprecatedLabelsInProject() ) );
}

void QgisApp::createCanvasTools()
{
  // create tools
  mMapTools.mZoomIn = new QgsMapToolZoom( mapCanvas(), false /* zoomIn */ );
  mMapTools.mZoomOut = new QgsMapToolZoom( mapCanvas(), true /* zoomOut */ );
  mMapTools.mPan = new QgsMapToolPan( mapCanvas() );
  connect( mMapTools.mPan, SIGNAL( contextMenuRequested( QPoint, QgsPoint ) ),
           this, SLOT( showCanvasContextMenu( QPoint, QgsPoint ) ) );
  connect( mMapTools.mPan, SIGNAL( featurePicked( QgsFeature, QgsVectorLayer* ) ),
           this, SLOT( handleFeaturePicked( QgsFeature, QgsVectorLayer* ) ) );
  connect( mMapTools.mPan, SIGNAL( labelPicked( QgsLabelPosition ) ),
           this, SLOT( handleLabelPicked( QgsLabelPosition ) ) );
  mMapTools.mIdentify = new QgsMapToolIdentifyAction( mapCanvas() );
  connect( mMapTools.mIdentify, SIGNAL( copyToClipboard( QgsFeatureStore & ) ),
           this, SLOT( copyFeatures( QgsFeatureStore & ) ) );
  mMapTools.mFeatureAction = new QgsMapToolFeatureAction( mapCanvas() );
  mMapTools.mMeasureDist = new QgsMeasureToolV2( mapCanvas(), QgsMeasureToolV2::MeasureLine );
  mMapTools.mMeasureArea = new QgsMeasureToolV2( mapCanvas(), QgsMeasureToolV2::MeasurePolygon );
  mMapTools.mMeasureCircle = new QgsMeasureToolV2( mapCanvas(), QgsMeasureToolV2::MeasureCircle );
  mMapTools.mMeasureHeightProfile = new QgsMeasureHeightProfileTool( mapCanvas() );
  mMapTools.mMeasureAngle = new QgsMeasureToolV2( mapCanvas(), QgsMeasureToolV2::MeasureAngle );
  mMapTools.mTextAnnotation = new QgsMapToolTextAnnotation( mapCanvas() );
  mMapTools.mPinAnnotation = new QgsMapToolPinAnnotation( mapCanvas() );
  mMapTools.mFormAnnotation = new QgsMapToolFormAnnotation( mapCanvas() );
  mMapTools.mHtmlAnnotation = new QgsMapToolHtmlAnnotation( mapCanvas() );
  mMapTools.mSvgAnnotation = new QgsMapToolSvgAnnotation( mapCanvas() );
  mMapTools.mAddFeature = new QgsMapToolAddFeature( mapCanvas() );
  mMapTools.mCircularStringCurvePoint = new QgsMapToolCircularStringCurvePoint( dynamic_cast<QgsMapToolAddFeature*>( mMapTools.mAddFeature ), mapCanvas() );
  mMapTools.mCircularStringRadius = new QgsMapToolCircularStringRadius( dynamic_cast<QgsMapToolAddFeature*>( mMapTools.mAddFeature ), mapCanvas() );
  mMapTools.mMoveFeature = new QgsMapToolMoveFeature( mapCanvas() );
  mMapTools.mRotateFeature = new QgsMapToolRotateFeature( mapCanvas() );
//need at least geos 3.3 for OffsetCurve tool
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
((GEOS_VERSION_MAJOR>3) || ((GEOS_VERSION_MAJOR==3) && (GEOS_VERSION_MINOR>=3)))
  mMapTools.mOffsetCurve = new QgsMapToolOffsetCurve( mapCanvas() );
#else
  mMapTools.mOffsetCurve = 0;
#endif //GEOS_VERSION
  mMapTools.mReshapeFeatures = new QgsMapToolReshape( mapCanvas() );
  mMapTools.mSplitFeatures = new QgsMapToolSplitFeatures( mapCanvas() );
  mMapTools.mSplitParts = new QgsMapToolSplitParts( mapCanvas() );
  mMapTools.mSelectFeatures = new QgsMapToolSelectFeatures( mapCanvas() );
  mMapTools.mSelectPolygon = new QgsMapToolSelectPolygon( mapCanvas() );
  mMapTools.mSelectFreehand = new QgsMapToolSelectFreehand( mapCanvas() );
  mMapTools.mSelectRadius = new QgsMapToolSelectRadius( mapCanvas() );
  mMapTools.mAddRing = new QgsMapToolAddRing( mapCanvas() );
  mMapTools.mFillRing = new QgsMapToolFillRing( mapCanvas() );
  mMapTools.mAddPart = new QgsMapToolAddPart( mapCanvas() );
  mMapTools.mSimplifyFeature = new QgsMapToolSimplify( mapCanvas() );
  mMapTools.mDeleteRing = new QgsMapToolDeleteRing( mapCanvas() );
  mMapTools.mDeletePart = new QgsMapToolDeletePart( mapCanvas() );
  mMapTools.mNodeTool = new QgsMapToolNodeTool( mapCanvas() );
  mMapTools.mRotatePointSymbolsTool = new QgsMapToolRotatePointSymbols( mapCanvas() );
  mMapTools.mPinLabels = new QgsMapToolPinLabels( mapCanvas() );
  mMapTools.mShowHideLabels = new QgsMapToolShowHideLabels( mapCanvas() );
  mMapTools.mMoveLabel = new QgsMapToolMoveLabel( mapCanvas() );
  mMapTools.mSlope = new QgsMapToolSlope( mapCanvas() );
  mMapTools.mHillshade = new QgsMapToolHillshade( mapCanvas() );
  mMapTools.mViewshed = new QgsMapToolViewshed( mapCanvas() );
  mMapTools.mViewshedSector = new QgsViewshedSectorTool( mapCanvas() );

  mMapTools.mRotateLabel = new QgsMapToolRotateLabel( mapCanvas() );
  mMapTools.mChangeLabelProperties = new QgsMapToolChangeLabelProperties( mapCanvas() );
}

void QgisApp::createOverview()
{
  // overview canvas
  QgsMapOverviewCanvas* overviewCanvas = new QgsMapOverviewCanvas( NULL, mapCanvas() );
  overviewCanvas->setWhatsThis( tr( "Map overview canvas. This canvas can be used to display a locator map that shows the current extent of the map canvas. The current extent is shown as a red rectangle. Any layer on the map can be added to the overview canvas." ) );

  QBitmap overviewPanBmp = QBitmap::fromData( QSize( 16, 16 ), pan_bits );
  QBitmap overviewPanBmpMask = QBitmap::fromData( QSize( 16, 16 ), pan_mask_bits );
  mOverviewMapCursor = new QCursor( overviewPanBmp, overviewPanBmpMask, 0, 0 ); //set upper left corner as hot spot - this is better when extent marker is small; hand won't cover the marker
  overviewCanvas->setCursor( *mOverviewMapCursor );
//  QVBoxLayout *myOverviewLayout = new QVBoxLayout;
//  myOverviewLayout->addWidget(overviewCanvas);
//  overviewFrame->setLayout(myOverviewLayout);
  mOverviewDock = new QDockWidget( tr( "Overview" ), this );
  mOverviewDock->setObjectName( "Overview" );
  mOverviewDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
  mOverviewDock->setWidget( overviewCanvas );
  addDockWidget( Qt::LeftDockWidgetArea, mOverviewDock );
  // add to the Panel submenu
  panelMenu()->addAction( mOverviewDock->toggleViewAction() );

  mapCanvas()->enableOverviewMode( overviewCanvas );

  // moved here to set anti aliasing to both map canvas and overview
  QSettings mySettings;
  // Anti Aliasing enabled by default as of QGIS 1.7
  mapCanvas()->enableAntiAliasing( mySettings.value( "/qgis/enable_anti_aliasing", true ).toBool() );

  int action = mySettings.value( "/qgis/wheel_action", 2 ).toInt();
  double zoomFactor = mySettings.value( "/qgis/zoom_factor", 2 ).toDouble();
  mapCanvas()->setWheelAction(( QgsMapCanvas::WheelAction ) action, zoomFactor );

  mapCanvas()->setCachingEnabled( mySettings.value( "/qgis/enable_render_caching", true ).toBool() );

  mapCanvas()->setParallelRenderingEnabled( mySettings.value( "/qgis/parallel_rendering", false ).toBool() );

  mapCanvas()->setMapUpdateInterval( mySettings.value( "/qgis/map_update_interval", 250 ).toInt() );
}

void QgisApp::addDockWidget( Qt::DockWidgetArea theArea, QDockWidget * thepDockWidget )
{
  QMainWindow::addDockWidget( theArea, thepDockWidget );
  // Make the right and left docks consume all vertical space and top
  // and bottom docks nest between them
  setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );
  setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
  setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
  setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );
  // add to the Panel submenu
  panelMenu()->addAction( thepDockWidget->toggleViewAction() );

  thepDockWidget->show();

  // refresh the map canvas
  mapCanvas()->refresh();
}

void QgisApp::removeDockWidget( QDockWidget * thepDockWidget )
{
  QMainWindow::removeDockWidget( thepDockWidget );
  panelMenu()->removeAction( thepDockWidget->toggleViewAction() );
}

QgsPluginManager *QgisApp::pluginManager()
{
  Q_ASSERT( mPluginManager );
  return mPluginManager;
}

void QgisApp::setupLayerTreeViewFromSettings()
{
  QSettings s;

  QgsLayerTreeModel* model = layerTreeView()->layerTreeModel();
  model->setFlag( QgsLayerTreeModel::ShowRasterPreviewIcon, s.value( "/qgis/createRasterLegendIcons", false ).toBool() );

  QFont fontLayer, fontGroup;
  fontLayer.setBold( s.value( "/qgis/legendLayersBold", true ).toBool() );
  fontGroup.setBold( s.value( "/qgis/legendGroupsBold", false ).toBool() );
  model->setLayerTreeNodeFont( QgsLayerTreeNode::NodeLayer, fontLayer );
  model->setLayerTreeNodeFont( QgsLayerTreeNode::NodeGroup, fontGroup );
}


void QgisApp::updateNewLayerInsertionPoint()
{
  // defaults
  QgsLayerTreeGroup* parentGroup = layerTreeView()->layerTreeModel()->rootGroup();
  int index = 0;
  QModelIndex current = layerTreeView()->currentIndex();

  if ( current.isValid() )
  {
    if ( QgsLayerTreeNode* currentNode = layerTreeView()->currentNode() )
    {
      // if the insertion point is actually a group, insert new layers into the group
      if ( QgsLayerTree::isGroup( currentNode ) )
      {
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTree::toGroup( currentNode ), 0 );
        return;
      }

      // otherwise just set the insertion point in front of the current node
      QgsLayerTreeNode* parentNode = currentNode->parent();
      if ( QgsLayerTree::isGroup( parentNode ) )
        parentGroup = QgsLayerTree::toGroup( parentNode );
    }

    index = current.row();
  }

  QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( parentGroup, index );
}

void QgisApp::autoSelectAddedLayer( QList<QgsMapLayer*> layers )
{
  if ( layers.count() )
  {
    QgsLayerTreeLayer* nodeLayer = QgsProject::instance()->layerTreeRoot()->findLayer( layers[0]->id() );

    if ( !nodeLayer )
      return;

    QModelIndex index = layerTreeView()->layerTreeModel()->node2index( nodeLayer );
    layerTreeView()->setCurrentIndex( index );
  }
}

void QgisApp::createMapTips()
{
  // Set up the timer for maptips. The timer is reset everytime the mouse is moved
  mpMapTipsTimer = new QTimer( mapCanvas() );
  // connect the timer to the maptips slot
  connect( mpMapTipsTimer, SIGNAL( timeout() ), this, SLOT( showMapTip() ) );
  // set the interval to 0.850 seconds - timer will be started next time the mouse moves
  mpMapTipsTimer->setInterval( 850 );
  // Create the maptips object
  mpMaptip = new QgsMapTip();
}

void QgisApp::createDecorations()
{
  mDecorationCopyright = new QgsDecorationCopyright( this );
  mDecorationNorthArrow = new QgsDecorationNorthArrow( this );
  mDecorationScaleBar = new QgsDecorationScaleBar( this );
  mDecorationGrid = new QgsDecorationGrid( mapCanvas(), this );

  // add the decorations in a particular order so they are rendered in that order
  addDecorationItem( mDecorationGrid );
  addDecorationItem( mDecorationCopyright );
  addDecorationItem( mDecorationNorthArrow );
  addDecorationItem( mDecorationScaleBar );
  connect( mapCanvas(), SIGNAL( renderComplete( QPainter * ) ), this, SLOT( renderDecorationItems( QPainter * ) ) );
  connect( this, SIGNAL( newProject() ), this, SLOT( projectReadDecorationItems() ) );
  connect( this, SIGNAL( projectRead() ), this, SLOT( projectReadDecorationItems() ) );
}

void QgisApp::renderDecorationItems( QPainter *p )
{
  foreach ( QgsDecorationItem* item, mDecorationItems )
  {
    item->render( p );
  }
}

void QgisApp::projectReadDecorationItems()
{
  foreach ( QgsDecorationItem* item, mDecorationItems )
  {
    item->projectRead();
  }
}

// Update project menu with the current list of recently accessed projects
void QgisApp::updateRecentProjectPaths()
{
  // Remove existing paths from the recent projects menu
  int i;

  int menusize = recentProjectsMenu()->actions().size();

  for ( i = menusize; i < mRecentProjectPaths.size(); i++ )
  {
    recentProjectsMenu()->addAction( "Dummy text" );
  }

  QList<QAction *> menulist = recentProjectsMenu()->actions();

  assert( menulist.size() == mRecentProjectPaths.size() );

  for ( i = 0; i < mRecentProjectPaths.size(); i++ )
  {
    menulist.at( i )->setText( mRecentProjectPaths.at( i ) );

    // Disable this menu item if the file has been removed, if not enable it
    menulist.at( i )->setEnabled( QFile::exists(( mRecentProjectPaths.at( i ) ) ) );

  }
} // QgisApp::updateRecentProjectPaths

// add this file to the recently opened/saved projects list
void QgisApp::saveRecentProjectPath( QString projectPath, QSettings & settings )
{
  // Get canonical absolute path
  QFileInfo myFileInfo( projectPath );
  projectPath = myFileInfo.absoluteFilePath();

  // If this file is already in the list, remove it
  mRecentProjectPaths.removeAll( projectPath );

  // Prepend this file to the list
  mRecentProjectPaths.prepend( projectPath );

  // Keep the list to 8 items by trimming excess off the bottom
  while ( mRecentProjectPaths.count() > 8 )
  {
    mRecentProjectPaths.pop_back();
  }

  // Persist the list
  settings.setValue( "/UI/recentProjectsList", mRecentProjectPaths );


  // Update menu list of paths
  updateRecentProjectPaths();

} // QgisApp::saveRecentProjectPath

// Update project menu with the project templates
void QgisApp::updateProjectFromTemplates()
{
  // get list of project files in template dir
  QSettings settings;
  QString templateDirName = settings.value( "/qgis/projectTemplateDir",
                            QgsApplication::qgisSettingsDirPath() + "project_templates" ).toString();
  QDir templateDir( templateDirName );
  QStringList filters( "*.qgs" );
  templateDir.setNameFilters( filters );
  QStringList templateFiles = templateDir.entryList( filters );

  // Remove existing entries
  projectFromTemplateMenu()->clear();

  // Add entries
  foreach ( QString templateFile, templateFiles )
  {
    projectFromTemplateMenu()->addAction( templateFile );
  }

  // add <blank> entry, which loads a blank template (regardless of "default template")
  if ( settings.value( "/qgis/newProjectDefault", QVariant( false ) ).toBool() )
    projectFromTemplateMenu()->addAction( tr( "< Blank >" ) );

} // QgisApp::updateProjectFromTemplates

void QgisApp::saveWindowState()
{
  // store window and toolbar positions
  QSettings settings;
  // store the toolbar/dock widget settings using Qt4 settings API
  settings.setValue( "/UI/state" + mWindowStateSuffix, saveState() );

  // store window geometry
  settings.setValue( "/UI/geometry" + mWindowStateSuffix, saveGeometry() );
}

#include "ui_defaults.h"

void QgisApp::restoreWindowState()
{
  // restore the toolbar and dock widgets positions using Qt4 settings API
  QSettings settings;

  if ( !restoreState( settings.value( "/UI/state" + mWindowStateSuffix, QByteArray::fromRawData(( char * )defaultUIstate, sizeof defaultUIstate ) ).toByteArray() ) )
  {
    QgsDebugMsg( "restore of UI state failed" );
  }

  // restore window geometry
  if ( !restoreGeometry( settings.value( "/UI/geometry" + mWindowStateSuffix, QByteArray::fromRawData(( char * )defaultUIgeometry, sizeof defaultUIgeometry ) ).toByteArray() ) )
  {
    QgsDebugMsg( "restore of UI geometry failed" );
  }

}
///////////// END OF GUI SETUP ROUTINES ///////////////
void QgisApp::sponsors()
{
  QgsSponsors * sponsors = new QgsSponsors( this );
  sponsors->show();
  sponsors->raise();
  sponsors->activateWindow();
}

void QgisApp::about()
{
  static QgsAbout *abt = NULL;
  if ( !abt )
  {
    QApplication::setOverrideCursor( Qt::WaitCursor );
    abt = new QgsAbout( this );
    QString versionString = "<html><body><div align='center'><table width='100%'>";

    versionString += "<tr>";
    versionString += "<td>" + tr( "QGIS version" )       + "</td><td>QGIS Enterprise</td>";
    versionString += "<td>" + tr( "QGIS code revision" ) + "</td><td>" + QGis::QGIS_DEV_VERSION + "</td>";

    versionString += "</tr><tr>";

    versionString += "<td>" + tr( "Compiled against Qt" ) + "</td><td>" + QT_VERSION_STR + "</td>";
    versionString += "<td>" + tr( "Running against Qt" )  + "</td><td>" + qVersion() + "</td>";

    versionString += "</tr><tr>";

    versionString += "<td>" + tr( "Compiled against GDAL/OGR" ) + "</td><td>" + GDAL_RELEASE_NAME + "</td>";
    versionString += "<td>" + tr( "Running against GDAL/OGR" )  + "</td><td>" + GDALVersionInfo( "RELEASE_NAME" ) + "</td>";

    versionString += "</tr><tr>";

    versionString += "<td>" + tr( "Compiled against GEOS" ) + "</td><td>" + GEOS_CAPI_VERSION + "</td>";
    versionString += "<td>" + tr( "Running against GEOS" ) + "</td><td>" + GEOSversion() + "</td>";

    versionString += "</tr><tr>";

    versionString += "<td>" + tr( "PostgreSQL Client Version" ) + "</td><td>";
#ifdef HAVE_POSTGRESQL
    versionString += PG_VERSION;
#else
    versionString += tr( "No support." );
#endif
    versionString += "</td>";

    versionString += "<td>" +  tr( "SpatiaLite Version" ) + "</td><td>";
    versionString += spatialite_version();
    versionString += "</td>";

    versionString += "</tr><tr>";

    versionString += "<td>" + tr( "QWT Version" ) + "</td><td>" + QWT_VERSION_STR + "</td>";
    versionString += "<td>" + tr( "PROJ.4 Version" ) + "</td><td>" + QString::number( PJ_VERSION ) + "</td>";

    versionString += "</tr><tr>";

    versionString += "<td>" + tr( "QScintilla2 Version" ) + "</td><td>" + QSCINTILLA_VERSION_STR + "</td>";

#ifdef QGISDEBUG
    versionString += "<td colspan=2>" + tr( "This copy of QGIS writes debugging output." ) + "</td>";
#endif

    versionString += "</tr></table></div></body></html>";

    abt->setVersion( versionString );

    QApplication::restoreOverrideCursor();
  }
  abt->show();
  abt->raise();
  abt->activateWindow();
}

void QgisApp::addLayerDefinition()
{
  QString path = QFileDialog::getOpenFileName( this, "Add Layer Definition File", QDir::home().path(), "*.qlr" );
  if ( path.isEmpty() )
    return;

  openLayerDefinition( path );
}

QString QgisApp::crsAndFormatAdjustedLayerUri( const QString &uri, const QStringList &supportedCrs, const QStringList &supportedFormats ) const
{
  QString newuri = uri;

  // Adjust layer CRS to project CRS
  QgsCoordinateReferenceSystem testCrs;
  foreach ( QString c, supportedCrs )
  {
    testCrs.createFromOgcWmsCrs( c );
    if ( testCrs == mapCanvas()->mapSettings().destinationCrs() )
    {
      newuri.replace( QRegExp( "crs=[^&]+" ), "crs=" + c );
      QgsDebugMsg( QString( "Changing layer crs to %1, new uri: %2" ).arg( c, uri ) );
      break;
    }
  }

  // Use the last used image format
  QString lastImageEncoding = QSettings().value( "/qgis/lastWmsImageEncoding", "image/png" ).toString();
  foreach ( QString fmt, supportedFormats )
  {
    if ( fmt == lastImageEncoding )
    {
      newuri.replace( QRegExp( "format=[^&]+" ), "format=" + fmt );
      QgsDebugMsg( QString( "Changing layer format to %1, new uri: %2" ).arg( fmt, uri ) );
      break;
    }
  }
  return newuri;
}

/**
  This method prompts the user for a list of vector file names  with a dialog.
  */
void QgisApp::addVectorLayer()
{
  mapCanvas()->freeze();
  QgsOpenVectorLayerDialog *ovl = new QgsOpenVectorLayerDialog( this );

  if ( ovl->exec() )
  {
    QStringList selectedSources = ovl->dataSources();
    QString enc = ovl->encoding();
    if ( !selectedSources.isEmpty() )
    {
      addVectorLayers( selectedSources, enc, ovl->dataSourceType() );
    }
  }

  mapCanvas()->freeze( false );
  mapCanvas()->refresh();

  delete ovl;
}


bool QgisApp::addVectorLayers( const QStringList &theLayerQStringList, const QString &enc, const QString &dataSourceType )
{
  bool wasfrozen = mapCanvas()->isFrozen();
  QList<QgsMapLayer *> myList;
  foreach ( QString src, theLayerQStringList )
  {
    src = src.trimmed();
    QString base;
    if ( dataSourceType == "file" )
    {
      QFileInfo fi( src );
      base = fi.completeBaseName();

      // if needed prompt for zipitem layers
      QString vsiPrefix = QgsZipItem::vsiPrefix( src );
      if ( ! src.startsWith( "/vsi", Qt::CaseInsensitive ) &&
           ( vsiPrefix == "/vsizip/" || vsiPrefix == "/vsitar/" ) )
      {
        if ( askUserForZipItemLayers( src ) )
          continue;
      }
    }
    else if ( dataSourceType == "database" )
    {
      base = src;
    }
    else //directory //protocol
    {
      QFileInfo fi( src );
      base = fi.completeBaseName();
    }

    QgsDebugMsg( "completeBaseName: " + base );

    // create the layer

    QgsVectorLayer *layer = new QgsVectorLayer( src, base, "ogr", false );
    Q_CHECK_PTR( layer );

    if ( ! layer )
    {
      mapCanvas()->freeze( false );

      // Let render() do its own cursor management
      //      QApplication::restoreOverrideCursor();

      // XXX insert meaningful whine to the user here
      return false;
    }

    if ( layer->isValid() )
    {
      layer->setProviderEncoding( enc );

      QStringList sublayers = layer->dataProvider()->subLayers();
      QgsDebugMsg( QString( "got valid layer with %1 sublayers" ).arg( sublayers.count() ) );

      // If the newly created layer has more than 1 layer of data available, we show the
      // sublayers selection dialog so the user can select the sublayers to actually load.
      if ( sublayers.count() > 1 )
      {
        askUserForOGRSublayers( layer );

        // The first layer loaded is not useful in that case. The user can select it in
        // the list if he wants to load it.
        delete layer;

      }
      else if ( sublayers.count() > 0 ) // there is 1 layer of data available
      {
        //set friendly name for datasources with only one layer
        QStringList sublayers = layer->dataProvider()->subLayers();
        QStringList elements = sublayers.at( 0 ).split( ":" );
        if ( layer->storageType() != "GeoJSON" )
        {
          while ( elements.size() > 4 )
          {
            elements[1] += ":" + elements[2];
            elements.removeAt( 2 );
          }

          layer->setLayerName( elements.at( 1 ) );
        }
        myList << layer;
      }
      else
      {
        QString msg = tr( "%1 doesn't have any layers" ).arg( src );
        messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, QgsMessageBar::CRITICAL, messageTimeout() );
        delete layer;
      }
    }
    else
    {
      QString msg = tr( "%1 is not a valid or recognized data source" ).arg( src );
      messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, QgsMessageBar::CRITICAL, messageTimeout() );

      // since the layer is bad, stomp on it
      delete layer;
    }

  }

  // make sure at least one layer was successfully added
  if ( myList.count() == 0 )
  {
    return false;
  }

  // Register this layer with the layers registry
  QgsMapLayerRegistry::instance()->addMapLayers( myList );
  foreach ( QgsMapLayer *l, myList )
  {
    bool ok;
    l->loadDefaultStyle( ok );
  }

  // Only update the map if we frozen in this method
  // Let the caller do it otherwise
  if ( !wasfrozen )
  {
    mapCanvas()->freeze( false );
    mapCanvas()->refresh();
  }
// Let render() do its own cursor management
//  QApplication::restoreOverrideCursor();

  // statusBar()->showMessage( mapCanvas()->extent().toString( 2 ) );

  return true;
} // QgisApp::addVectorLayer()

bool QgisApp::cmpByText_( QAction* a, QAction* b )
{
  return QString::localeAwareCompare( a->text(), b->text() ) < 0;
}

// present a dialog to choose zipitem layers
bool QgisApp::askUserForZipItemLayers( QString path )
{
  bool ok = false;
  QVector<QgsDataItem*> childItems;
  QgsZipItem *zipItem = 0;
  QSettings settings;
  int promptLayers = settings.value( "/qgis/promptForRasterSublayers", 1 ).toInt();

  QgsDebugMsg( "askUserForZipItemLayers( " + path + ")" );

  // if scanZipBrowser == no: skip to the next file
  if ( settings.value( "/qgis/scanZipInBrowser2", "basic" ).toString() == "no" )
  {
    return false;
  }

  zipItem = new QgsZipItem( 0, path, path );
  if ( ! zipItem )
    return false;

  zipItem->populate();
  QgsDebugMsg( QString( "Path= %1 got zipitem with %2 children" ).arg( path ).arg( zipItem->rowCount() ) );

  // if 1 or 0 child found, exit so a normal item is created by gdal or ogr provider
  if ( zipItem->rowCount() <= 1 )
  {
    delete zipItem;
    return false;
  }

  // if promptLayers=Load all, load all layers without prompting
  if ( promptLayers == 3 )
  {
    childItems = zipItem->children();
  }
  // exit if promptLayers=Never
  else if ( promptLayers == 2 )
  {
    delete zipItem;
    return false;
  }
  else
  {
    // We initialize a selection dialog and display it.
    QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Vsifile, "vsi", this );

    QStringList layers;
    for ( int i = 0; i < zipItem->children().size(); i++ )
    {
      QgsDataItem *item = zipItem->children()[i];
      QgsLayerItem *layerItem = dynamic_cast<QgsLayerItem *>( item );
      if ( layerItem )
      {
        QgsDebugMsgLevel( QString( "item path=%1 provider=%2" ).arg( item->path() ).arg( layerItem->providerKey() ), 2 );
      }
      if ( layerItem && layerItem->providerKey() == "gdal" )
      {
        layers << QString( "%1|%2|%3" ).arg( i ).arg( item->name() ).arg( "Raster" );
      }
      else if ( layerItem && layerItem->providerKey() == "ogr" )
      {
        layers << QString( "%1|%2|%3" ).arg( i ).arg( item->name() ).arg( tr( "Vector" ) );
      }
    }

    chooseSublayersDialog.populateLayerTable( layers, "|" );

    if ( chooseSublayersDialog.exec() )
    {
      foreach ( int i, chooseSublayersDialog.selectionIndexes() )
      {
        childItems << zipItem->children()[i];
      }
    }
  }

  if ( childItems.isEmpty() )
  {
    // return true so dialog doesn't popup again (#6225) - hopefully this doesn't create other trouble
    ok = true;
  }

  // add childItems
  foreach ( QgsDataItem* item, childItems )
  {
    QgsLayerItem *layerItem = dynamic_cast<QgsLayerItem *>( item );
    if ( !layerItem )
      continue;

    QgsDebugMsg( QString( "item path=%1 provider=%2" ).arg( item->path() ).arg( layerItem->providerKey() ) );
    if ( layerItem->providerKey() == "gdal" )
    {
      if ( addRasterLayer( item->path(), QFileInfo( item->name() ).completeBaseName() ) )
        ok = true;
    }
    else if ( layerItem->providerKey() == "ogr" )
    {
      if ( addVectorLayers( QStringList( item->path() ), "System", "file" ) )
        ok = true;
    }
  }

  delete zipItem;
  return ok;
}

// present a dialog to choose GDAL raster sublayers
void QgisApp::askUserForGDALSublayers( QgsRasterLayer *layer )
{
  if ( !layer )
    return;

  QStringList sublayers = layer->subLayers();
  QgsDebugMsg( QString( "raster has %1 sublayers" ).arg( layer->subLayers().size() ) );

  if ( sublayers.size() < 1 )
    return;

  // if promptLayers=Load all, load all sublayers without prompting
  QSettings settings;
  if ( settings.value( "/qgis/promptForRasterSublayers", 1 ).toInt() == 3 )
  {
    loadGDALSublayers( layer->source(), sublayers );
    return;
  }

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Gdal, "gdal", this );

  QStringList layers;
  QStringList names;
  for ( int i = 0; i < sublayers.size(); i++ )
  {
    // simplify raster sublayer name - should add a function in gdal provider for this?
    // code is copied from QgsGdalLayerItem::createChildren
    QString name = sublayers[i];
    QString path = layer->source();
    // if netcdf/hdf use all text after filename
    // for hdf4 it would be best to get description, because the subdataset_index is not very practical
    if ( name.startsWith( "netcdf", Qt::CaseInsensitive ) ||
         name.startsWith( "hdf", Qt::CaseInsensitive ) )
      name = name.mid( name.indexOf( path ) + path.length() + 1 );
    else
    {
      // remove driver name and file name
      name.replace( name.split( ":" )[0], "" );
      name.replace( path, "" );
    }
    // remove any : or " left over
    if ( name.startsWith( ":" ) )
      name.remove( 0, 1 );

    if ( name.startsWith( "\"" ) )
      name.remove( 0, 1 );

    if ( name.endsWith( ":" ) )
      name.chop( 1 );

    if ( name.endsWith( "\"" ) )
      name.chop( 1 );

    names << name;
    layers << QString( "%1|%2" ).arg( i ).arg( name );
  }

  chooseSublayersDialog.populateLayerTable( layers, "|" );

  if ( chooseSublayersDialog.exec() )
  {
    foreach ( int i, chooseSublayersDialog.selectionIndexes() )
    {
      QgsRasterLayer *rlayer = new QgsRasterLayer( sublayers[i], names[i] );
      if ( rlayer && rlayer->isValid() )
      {
        addRasterLayer( rlayer );
      }
    }
  }
}

// should the GDAL sublayers dialog should be presented to the user?
bool QgisApp::shouldAskUserForGDALSublayers( QgsRasterLayer *layer )
{
  // return false if layer is empty or raster has no sublayers
  if ( !layer || layer->providerType() != "gdal" || layer->subLayers().size() < 1 )
    return false;

  QSettings settings;
  int promptLayers = settings.value( "/qgis/promptForRasterSublayers", 1 ).toInt();
  // 0 = Always -> always ask (if there are existing sublayers)
  // 1 = If needed -> ask if layer has no bands, but has sublayers
  // 2 = Never -> never prompt, will not load anything
  // 3 = Load all -> never prompt, but load all sublayers

  return promptLayers == 0 || promptLayers == 3 || ( promptLayers == 1 && layer->bandCount() == 0 );
}

// This method will load with GDAL the layers in parameter.
// It is normally triggered by the sublayer selection dialog.
void QgisApp::loadGDALSublayers( QString uri, QStringList list )
{
  QString path, name;
  QgsRasterLayer *subLayer = NULL;

  //add layers in reverse order so they appear in the right order in the layer dock
  for ( int i = list.size() - 1; i >= 0 ; i-- )
  {
    path = list[i];
    // shorten name by replacing complete path with filename
    name = path;
    name.replace( uri, QFileInfo( uri ).completeBaseName() );
    subLayer = new QgsRasterLayer( path, name );
    if ( subLayer )
    {
      if ( subLayer->isValid() )
        addRasterLayer( subLayer );
      else
        delete subLayer;
    }

  }
}

// This method is the method that does the real job. If the layer given in
// parameter is NULL, then the method tries to act on the activeLayer.
void QgisApp::askUserForOGRSublayers( QgsVectorLayer *layer )
{
  if ( !layer )
  {
    layer = qobject_cast<QgsVectorLayer *>( activeLayer() );
    if ( !layer || layer->dataProvider()->name() != "ogr" )
      return;
  }

  QStringList sublayers = layer->dataProvider()->subLayers();
  QString layertype = layer->dataProvider()->storageType();

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Ogr, "ogr", this );
  chooseSublayersDialog.populateLayerTable( sublayers );

  if ( chooseSublayersDialog.exec() )
  {
    QString uri = layer->source();
    //the separator char & was changed to | to be compatible
    //with url for protocol drivers
    if ( uri.contains( '|', Qt::CaseSensitive ) )
    {
      // If we get here, there are some options added to the filename.
      // A valid uri is of the form: filename&option1=value1&option2=value2,...
      // We want only the filename here, so we get the first part of the split.
      QStringList theURIParts = uri.split( "|" );
      uri = theURIParts.at( 0 );
    }
    QgsDebugMsg( "Layer type " + layertype );
    // the user has done his choice
    loadOGRSublayers( layertype, uri, chooseSublayersDialog.selectionNames() );
  }
}

// This method will load with OGR the layers  in parameter.
// This method has been conceived to use the new URI
// format of the ogrprovider so as to give precisions about which
// sublayer to load into QGIS. It is normally triggered by the
// sublayer selection dialog.
void QgisApp::loadOGRSublayers( QString layertype, QString uri, QStringList list )
{
  // The uri must contain the actual uri of the vectorLayer from which we are
  // going to load the sublayers.
  QString fileName = QFileInfo( uri ).baseName();
  QList<QgsMapLayer *> myList;
  for ( int i = 0; i < list.size(); i++ )
  {
    QString composedURI;
    QStringList elements = list.at( i ).split( ":" );
    while ( elements.size() > 2 )
    {
      elements[0] += ":" + elements[1];
      elements.removeAt( 1 );
    }

    QString layerName = elements.value( 0 );
    QString layerType = elements.value( 1 );
    if ( layerType == "any" )
    {
      layerType = "";
      elements.removeAt( 1 );
    }

    if ( layertype != "GRASS" )
    {
      composedURI = uri + "|layername=" + layerName;
    }
    else
    {
      composedURI = uri + "|layerindex=" + layerName;
    }

    if ( !layerType.isEmpty() )
    {
      composedURI += "|geometrytype=" + layerType;
    }

    // addVectorLayer( composedURI, list.at( i ), "ogr" );

    QgsDebugMsg( "Creating new vector layer using " + composedURI );
    QString name = list.at( i );
    name.replace( ":", " " );
    QgsVectorLayer *layer = new QgsVectorLayer( composedURI, name, "ogr", false );
    if ( layer && layer->isValid() )
    {
      myList << layer;
    }
    else
    {
      QString msg = tr( "%1 is not a valid or recognized data source" ).arg( composedURI );
      messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, QgsMessageBar::CRITICAL, messageTimeout() );
      if ( layer )
        delete layer;
    }
  }

  if ( ! myList.isEmpty() )
  {
    // Register layer(s) with the layers registry
    QgsMapLayerRegistry::instance()->addMapLayers( myList );
    foreach ( QgsMapLayer *l, myList )
    {
      bool ok;
      l->loadDefaultStyle( ok );
    }
  }
}

void QgisApp::addDatabaseLayer()
{
#ifdef HAVE_POSTGRESQL
  // Fudge for now
  QgsDebugMsg( "about to addRasterLayer" );

  // TODO: QDialog for now, switch to QWidget in future
  QDialog *dbs = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( "postgres", this ) );
  if ( !dbs )
  {
    QMessageBox::warning( this, tr( "PostgreSQL" ), tr( "Cannot get PostgreSQL select dialog from provider." ) );
    return;
  }
  connect( dbs, SIGNAL( addDatabaseLayers( QStringList const &, QString const & ) ),
           this, SLOT( addDatabaseLayers( QStringList const &, QString const & ) ) );
  connect( dbs, SIGNAL( progress( int, int ) ),
           this, SIGNAL( progress( int, int ) ) );
  connect( dbs, SIGNAL( progressMessage( QString ) ),
           this, SLOT( showStatusMessage( QString ) ) );
  dbs->exec();
  delete dbs;
#endif
} // QgisApp::addDatabaseLayer()

void QgisApp::addDatabaseLayers( QStringList const & layerPathList, QString const & providerKey )
{
  QList<QgsMapLayer *> myList;

  if ( layerPathList.empty() )
  {
    // no layers to add so bail out, but
    // allow mMapCanvas to handle events
    // first
    mapCanvas()->freeze( false );
    return;
  }

  mapCanvas()->freeze( true );

  QApplication::setOverrideCursor( Qt::WaitCursor );

  foreach ( QString layerPath, layerPathList )
  {
    // create the layer
    QgsDataSourceURI uri( layerPath );

    QgsVectorLayer *layer = new QgsVectorLayer( uri.uri(), uri.table(), providerKey, false );
    Q_CHECK_PTR( layer );

    if ( ! layer )
    {
      mapCanvas()->freeze( false );
      QApplication::restoreOverrideCursor();

      // XXX insert meaningful whine to the user here
      return;
    }

    if ( layer->isValid() )
    {
      // add to list of layers to register
      //with the central layers registry
      myList << layer;
    }
    else
    {
      QgsMessageLog::logMessage( tr( "%1 is an invalid layer - not loaded" ).arg( layerPath ) );
      QLabel *msgLabel = new QLabel( tr( "%1 is an invalid layer and cannot be loaded. Please check the <a href=\"#messageLog\">message log</a> for further info." ).arg( layerPath ), messageBar() );
      msgLabel->setWordWrap( true );
      connect( msgLabel, SIGNAL( linkActivated( QString ) ), mLogDock, SLOT( show() ) );
      QgsMessageBarItem *item = new QgsMessageBarItem( msgLabel, QgsMessageBar::WARNING );
      messageBar()->pushItem( item );
      delete layer;
    }
    //qWarning("incrementing iterator");
  }

  QgsMapLayerRegistry::instance()->addMapLayers( myList );

  // load default style after adding to process readCustomSymbology signals
  foreach ( QgsMapLayer *l, myList )
  {
    bool ok;
    l->loadDefaultStyle( ok );
  }

  // draw the map
  mapCanvas()->freeze( false );
  mapCanvas()->refresh();

  QApplication::restoreOverrideCursor();
}


void QgisApp::addSpatiaLiteLayer()
{
  // show the SpatiaLite dialog
  QDialog *dbs = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( "spatialite", this ) );
  if ( !dbs )
  {
    QMessageBox::warning( this, tr( "SpatiaLite" ), tr( "Cannot get SpatiaLite select dialog from provider." ) );
    return;
  }
  connect( dbs, SIGNAL( addDatabaseLayers( QStringList const &, QString const & ) ),
           this, SLOT( addDatabaseLayers( QStringList const &, QString const & ) ) );
  dbs->exec();
  delete dbs;
} // QgisApp::addSpatiaLiteLayer()

void QgisApp::addDelimitedTextLayer()
{
  // show the Delimited text dialog
  QDialog *dts = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( "delimitedtext", this ) );
  if ( !dts )
  {
    QMessageBox::warning( this, tr( "Delimited Text" ), tr( "Cannot get Delimited Text select dialog from provider." ) );
    return;
  }
  connect( dts, SIGNAL( addVectorLayer( QString, QString, QString ) ),
           this, SLOT( addSelectedVectorLayer( QString, QString, QString ) ) );
  dts->exec();
  delete dts;
} // QgisApp::addDelimitedTextLayer()

void QgisApp::addSelectedVectorLayer( QString uri, QString layerName, QString provider )
{
  addVectorLayer( uri, layerName, provider );
} // QgisApp:addSelectedVectorLayer

void QgisApp::addMssqlLayer()
{
#ifdef HAVE_MSSQL
  // show the MSSQL dialog
  QDialog *dbs = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( "mssql", this ) );
  if ( !dbs )
  {
    QMessageBox::warning( this, tr( "MSSQL" ), tr( "Cannot get MSSQL select dialog from provider." ) );
    return;
  }
  connect( dbs, SIGNAL( addDatabaseLayers( QStringList const &, QString const & ) ),
           this, SLOT( addDatabaseLayers( QStringList const &, QString const & ) ) );
  dbs->exec();
  delete dbs;
#endif
} // QgisApp::addMssqlLayer()

void QgisApp::addOracleLayer()
{
#ifdef HAVE_ORACLE
  // show the Oracle dialog
  QDialog *dbs = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( "oracle", this ) );
  if ( !dbs )
  {
    QMessageBox::warning( this, tr( "Oracle" ), tr( "Cannot get Oracle select dialog from provider." ) );
    return;
  }
  connect( dbs, SIGNAL( addDatabaseLayers( QStringList const &, QString const & ) ),
           this, SLOT( addDatabaseLayers( QStringList const &, QString const & ) ) );
  connect( dbs, SIGNAL( progress( int, int ) ),
           this, SIGNAL( progress( int, int ) ) );
  connect( dbs, SIGNAL( progressMessage( QString ) ),
           this, SLOT( showStatusMessage( QString ) ) );
  dbs->exec();
  delete dbs;
#endif
} // QgisApp::addOracleLayer()

void QgisApp::addWmsLayer()
{
  // Fudge for now
  QgsDebugMsg( "about to addRasterLayer" );

  // TODO: QDialog for now, switch to QWidget in future
  QDialog *wmss = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( QString( "wms" ), this ) );
  if ( !wmss )
  {
    QMessageBox::warning( this, tr( "WMS" ), tr( "Cannot get WMS select dialog from provider." ) );
    return;
  }
  connect( wmss, SIGNAL( addRasterLayer( QString const &, QString const &, QString const & ) ),
           this, SLOT( addRasterLayer( QString const &, QString const &, QString const & ) ) );
  wmss->exec();
  delete wmss;
}

void QgisApp::addWcsLayer()
{
  QgsDebugMsg( "about to addWcsLayer" );

  // TODO: QDialog for now, switch to QWidget in future
  QDialog *wcss = dynamic_cast<QDialog*>( QgsProviderRegistry::instance()->selectWidget( QString( "wcs" ), this ) );
  if ( !wcss )
  {
    QMessageBox::warning( this, tr( "WCS" ), tr( "Cannot get WCS select dialog from provider." ) );
    return;
  }
  connect( wcss, SIGNAL( addRasterLayer( QString const &, QString const &, QString const & ) ),
           this, SLOT( addRasterLayer( QString const &, QString const &, QString const & ) ) );
  wcss->exec();
  delete wcss;
}

void QgisApp::addWfsLayer()
{
  if ( !mapCanvas() )
  {
    return;
  }

  QgsDebugMsg( "about to addWfsLayer" );

  // TODO: QDialog for now, switch to QWidget in future
  QgsSourceSelectDialog *wfss = dynamic_cast<QgsSourceSelectDialog*>( QgsProviderRegistry::instance()->selectWidget( QString( "WFS" ), this ) );
  if ( !wfss )
  {
    QMessageBox::warning( this, tr( "WFS" ), tr( "Cannot get WFS select dialog from provider." ) );
    return;
  }
  wfss->setCurrentExtentAndCrs( mapCanvas()->extent(), mapCanvas()->mapSettings().destinationCrs() );
  connect( wfss, SIGNAL( addLayer( QString, QString ) ),
           this, SLOT( addWfsLayer( QString, QString ) ) );

  bool bkRenderFlag = mapCanvas()->renderFlag();
  mapCanvas()->setRenderFlag( false );
  wfss->exec();
  mapCanvas()->setRenderFlag( bkRenderFlag );
  delete wfss;
}

void QgisApp::addWfsLayer( QString uri, QString typeName )
{
  // TODO: this should be eventually moved to a more reasonable place
  addVectorLayer( uri, typeName, "WFS" );
}

void QgisApp::addAfsLayer()
{
  if ( !mapCanvas() )
  {
    return;
  }

  QgsDebugMsg( "about to addAfsLayer" );

  // TODO: QDialog for now, switch to QWidget in future
  QgsSourceSelectDialog *afss = dynamic_cast<QgsSourceSelectDialog*>( QgsProviderRegistry::instance()->selectWidget( "arcgisfeatureserver", this ) );
  if ( !afss )
  {
    QMessageBox::warning( this, tr( "ArcGIS Feature Server" ), tr( "Cannot get ArcGIS Feature Server select dialog from provider." ) );
    return;
  }
  afss->setCurrentExtentAndCrs( mapCanvas()->extent(), mapCanvas()->mapSettings().destinationCrs() );
  connect( afss, SIGNAL( addLayer( QString, QString ) ),
           this, SLOT( addAfsLayer( QString, QString ) ) );

  bool bkRenderFlag = mapCanvas()->renderFlag();
  mapCanvas()->setRenderFlag( false );
  afss->exec();
  mapCanvas()->setRenderFlag( bkRenderFlag );
  delete afss;
}

void QgisApp::addAfsLayer( QString uri, QString typeName )
{
  // TODO: this should be eventually moved to a more reasonable place
  addVectorLayer( uri, typeName, "arcgisfeatureserver" );
}

void QgisApp::addAmsLayer()
{
  if ( !mapCanvas() )
  {
    return;
  }

  QgsDebugMsg( "about to addAmsLayer" );

  // TODO: QDialog for now, switch to QWidget in future
  QgsSourceSelectDialog *amss = dynamic_cast<QgsSourceSelectDialog*>( QgsProviderRegistry::instance()->selectWidget( "arcgismapserver", this ) );
  if ( !amss )
  {
    QMessageBox::warning( this, tr( "ArcGIS Map Server" ), tr( "Cannot get ArcGIS Map Server select dialog from provider." ) );
    return;
  }
  amss->setCurrentExtentAndCrs( mapCanvas()->extent(), mapCanvas()->mapSettings().destinationCrs() );
  connect( amss, SIGNAL( addLayer( QString, QString ) ),
           this, SLOT( addAmsLayer( QString, QString ) ) );

  bool bkRenderFlag = mapCanvas()->renderFlag();
  mapCanvas()->setRenderFlag( false );
  amss->exec();
  mapCanvas()->setRenderFlag( bkRenderFlag );
  delete amss;
}

void QgisApp::addAmsLayer( QString uri, QString typeName )
{
  // TODO: this should be eventually moved to a more reasonable place
  addRasterLayer( uri, typeName, QString( "arcgismapserver" ) );
}


void QgisApp::fileExit()
{
  if ( saveDirty() )
  {
    closeProject();
    qApp->exit( 0 );
  }
}


void QgisApp::fileNew()
{
  fileNew( true ); // prompts whether to save project
} // fileNew()


void QgisApp::fileNewBlank()
{
  fileNew( true, true );
}


//as file new but accepts flags to indicate whether we should prompt to save
void QgisApp::fileNew( bool thePromptToSaveFlag, bool forceBlank )
{
  if ( thePromptToSaveFlag )
  {
    if ( !saveDirty() )
    {
      return; //cancel pressed
    }
  }

  QSettings settings;

  closeProject();

  QgsProject* prj = QgsProject::instance();
  prj->clear();

  prj->layerTreeRegistryBridge()->setNewLayersVisible( settings.value( "/qgis/new_layers_visible", true ).toBool() );

  mLayerTreeCanvasBridge->clear();

  //set the color for selections
  //the default can be set in qgisoptions
  //use project properties to override the color on a per project basis
  int myRed = settings.value( "/qgis/default_selection_color_red", 255 ).toInt();
  int myGreen = settings.value( "/qgis/default_selection_color_green", 255 ).toInt();
  int myBlue = settings.value( "/qgis/default_selection_color_blue", 0 ).toInt();
  int myAlpha = settings.value( "/qgis/default_selection_color_alpha", 255 ).toInt();
  prj->writeEntry( "Gui", "/SelectionColorRedPart", myRed );
  prj->writeEntry( "Gui", "/SelectionColorGreenPart", myGreen );
  prj->writeEntry( "Gui", "/SelectionColorBluePart", myBlue );
  prj->writeEntry( "Gui", "/SelectionColorAlphaPart", myAlpha );
  mapCanvas()->setSelectionColor( QColor( myRed, myGreen, myBlue, myAlpha ) );

  //set the canvas to the default background color
  //the default can be set in qgisoptions
  //use project properties to override the color on a per project basis
  myRed = settings.value( "/qgis/default_canvas_color_red", 255 ).toInt();
  myGreen = settings.value( "/qgis/default_canvas_color_green", 255 ).toInt();
  myBlue = settings.value( "/qgis/default_canvas_color_blue", 255 ).toInt();
  myAlpha = settings.value( "/qgis/default_canvas_color_alpha", 0 ).toInt();
  prj->writeEntry( "Gui", "/CanvasColorRedPart", myRed );
  prj->writeEntry( "Gui", "/CanvasColorGreenPart", myGreen );
  prj->writeEntry( "Gui", "/CanvasColorBluePart", myBlue );
  prj->writeEntry( "Gui", "/CanvasColorAlphaPart", myAlpha );
  mapCanvas()->setCanvasColor( QColor( myRed, myGreen, myBlue, myAlpha ) );

  prj->dirty( false );

  setTitleBarText_( *this );

  //QgsDebugMsg("emiting new project signal");

  // emit signal so listeners know we have a new project
  emit newProject();

  mapCanvas()->freeze( false );
  mapCanvas()->refresh();
  mapCanvas()->clearExtentHistory();
  mapCanvas()->setRotation( 0.0 );
  emit projectScalesChanged( QStringList() );

  // set project CRS
  QString defCrs = settings.value( "/Projections/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
  QgsCoordinateReferenceSystem srs;
  srs.createFromOgcWmsCrs( defCrs );
  mapCanvas()->setDestinationCrs( srs );
  // write the projections _proj string_ to project settings
  prj->writeEntry( "SpatialRefSys", "/ProjectCRSProj4String", srs.toProj4() );
  prj->writeEntry( "SpatialRefSys", "/ProjectCrs", srs.authid() );
  prj->writeEntry( "SpatialRefSys", "/ProjectCRSID", ( int ) srs.srsid() );
  prj->dirty( false );
  if ( srs.mapUnits() != QGis::UnknownUnit )
  {
    mapCanvas()->setMapUnits( srs.mapUnits() );
  }

  // enable OTF CRS transformation if necessary
  mapCanvas()->setCrsTransformEnabled( settings.value( "/Projections/otfTransformEnabled", 0 ).toBool() );

  /** New Empty Project Created
      (before attempting to load custom project templates/filepaths) */

  // load default template
  /* NOTE: don't open default template on launch until after initialization,
           in case a project was defined via command line */

  // don't open template if last auto-opening of a project failed
  if ( ! forceBlank )
  {
    forceBlank = ! settings.value( "/qgis/projOpenedOKAtLaunch", QVariant( true ) ).toBool();
  }

  if ( ! forceBlank && settings.value( "/qgis/newProjectDefault", QVariant( false ) ).toBool() )
  {
    fileNewFromDefaultTemplate();
  }

  QgsVisibilityPresets::instance()->clear();

  // set the initial map tool
  mapCanvas()->setMapTool( mMapTools.mPan );
} // QgisApp::fileNew(bool thePromptToSaveFlag)

bool QgisApp::fileNewFromTemplate( QString fileName )
{
  if ( !saveDirty() )
  {
    return false; //cancel pressed
  }

  QgsDebugMsg( QString( "loading project template: %1" ).arg( fileName ) );
  if ( addProject( fileName ) )
  {
    // set null filename so we don't override the template
    QgsProject::instance()->setFileName( QString() );
    return true;
  }
  return false;
}

void QgisApp::fileNewFromDefaultTemplate()
{
  QString projectTemplate = QgsApplication::qgisSettingsDirPath() + QString( "project_default.qgs" );
  QString msgTxt;
  if ( !projectTemplate.isEmpty() && QFile::exists( projectTemplate ) )
  {
    if ( fileNewFromTemplate( projectTemplate ) )
    {
      return;
    }
    msgTxt = tr( "Default failed to open: %1" );
  }
  else
  {
    msgTxt = tr( "Default not found: %1" );
  }
  messageBar()->pushMessage( tr( "Open Template Project" ),
                             msgTxt.arg( projectTemplate ),
                             QgsMessageBar::WARNING );
}

void QgisApp::fileOpenAfterLaunch()
{
  // TODO: move auto-open project options to enums

  // check if a project is already loaded via command line or filesystem
  if ( !QgsProject::instance()->fileName().isNull() )
  {
    return;
  }

  // check if a data source is already loaded via command line or filesystem
  // empty project with layer loaded, but may not trigger a dirty project at this point
  if ( QgsProject::instance() && QgsMapLayerRegistry::instance()->count() > 0 )
  {
    return;
  }

  // fileNewBlank() has already been called in QgisApp constructor
  // loaded project is either a new blank one, or one from command line/filesystem
  QSettings settings;
  QString autoOpenMsgTitle = tr( "Auto-open Project" );

  // what type of project to auto-open
  int projOpen = settings.value( "/qgis/projOpenAtLaunch", 0 ).toInt();

  // get path of project file to open, or was attempted
  QString projPath = QString();
  if ( projOpen == 1 && mRecentProjectPaths.size() > 0 ) // most recent project
  {
    projPath = mRecentProjectPaths.at( 0 );
  }
  if ( projOpen == 2 ) // specific project
  {
    projPath = settings.value( "/qgis/projOpenAtLaunchPath" ).toString();
  }

  // whether last auto-opening of a project failed
  bool projOpenedOK = settings.value( "/qgis/projOpenedOKAtLaunch", QVariant( true ) ).toBool();

  // notify user if last attempt at auto-opening a project failed
  /** NOTE: Notification will not show if last auto-opened project failed but
      next project opened is from command line (minor issue) */
  /** TODO: Keep projOpenedOKAtLaunch from being reset to true after
      reading command line project (which happens before initialization signal) */
  if ( !projOpenedOK )
  {
    // only show the following 'auto-open project failed' message once, at launch
    settings.setValue( "/qgis/projOpenedOKAtLaunch", QVariant( true ) );

    // set auto-open project back to 'New' to avoid re-opening bad project
    settings.setValue( "/qgis/projOpenAtLaunch", QVariant( 0 ) );

    messageBar()->pushMessage( autoOpenMsgTitle,
                               tr( "Failed to open: %1" ).arg( projPath ),
                               QgsMessageBar::CRITICAL );
    return;
  }

  if ( projOpen == 0 ) // new project (default)
  {
    // open default template, if defined
    if ( settings.value( "/qgis/newProjectDefault", QVariant( false ) ).toBool() )
    {
      fileNewFromDefaultTemplate();
    }
    return;
  }

  if ( projPath.isEmpty() ) // projPath required from here
  {
    return;
  }

  if ( !projPath.endsWith( QString( "qgs" ), Qt::CaseInsensitive ) )
  {
    messageBar()->pushMessage( autoOpenMsgTitle,
                               tr( "Not valid project file: %1" ).arg( projPath ),
                               QgsMessageBar::WARNING );
    return;
  }

  if ( QFile::exists( projPath ) )
  {
    // set flag to check on next app launch if the following project opened OK
    settings.setValue( "/qgis/projOpenedOKAtLaunch", QVariant( false ) );

    if ( !addProject( projPath ) )
    {
      messageBar()->pushMessage( autoOpenMsgTitle,
                                 tr( "Project failed to open: %1" ).arg( projPath ),
                                 QgsMessageBar::WARNING );
    }

    if ( projPath.endsWith( QString( "project_default.qgs" ) ) )
    {
      messageBar()->pushMessage( autoOpenMsgTitle,
                                 tr( "Default template has been reopened: %1" ).arg( projPath ),
                                 QgsMessageBar::INFO );
    }
  }
  else
  {
    messageBar()->pushMessage( autoOpenMsgTitle,
                               tr( "File not found: %1" ).arg( projPath ),
                               QgsMessageBar::WARNING );
  }
}

void QgisApp::fileOpenedOKAfterLaunch()
{
  QSettings settings;
  settings.setValue( "/qgis/projOpenedOKAtLaunch", QVariant( true ) );
}

void QgisApp::fileNewFromTemplateAction( QAction * qAction )
{
  if ( ! qAction )
    return;

  if ( qAction->text() == tr( "< Blank >" ) )
  {
    fileNewBlank();
  }
  else
  {
    QSettings settings;
    QString templateDirName = settings.value( "/qgis/projectTemplateDir",
                              QgsApplication::qgisSettingsDirPath() + "project_templates" ).toString();
    fileNewFromTemplate( templateDirName + QDir::separator() + qAction->text() );
  }
}


void QgisApp::newVectorLayer()
{
  QString enc;
  QString fileName = QgsNewVectorLayerDialog::runAndCreateLayer( this, &enc );

  if ( !fileName.isEmpty() )
  {
    //then add the layer to the view
    QStringList fileNames;
    fileNames.append( fileName );
    //todo: the last parameter will change accordingly to layer type
    addVectorLayers( fileNames, enc, "file" );
  }
  else if ( fileName.isNull() )
  {
    QLabel *msgLabel = new QLabel( tr( "Layer creation failed. Please check the <a href=\"#messageLog\">message log</a> for further information." ), messageBar() );
    msgLabel->setWordWrap( true );
    connect( msgLabel, SIGNAL( linkActivated( QString ) ), mLogDock, SLOT( show() ) );
    QgsMessageBarItem *item = new QgsMessageBarItem( msgLabel, QgsMessageBar::WARNING );
    messageBar()->pushItem( item );
  }
}

void QgisApp::newMemoryLayer()
{
  QgsVectorLayer* newLayer = QgsNewMemoryLayerDialog::runAndCreateLayer( this );

  if ( newLayer )
  {
    //then add the layer to the view
    QList< QgsMapLayer* > layers;
    layers << newLayer;

    QgsMapLayerRegistry::instance()->addMapLayers( layers );
    newLayer->startEditing();
  }
}

void QgisApp::newSpatialiteLayer()
{
  QgsNewSpatialiteLayerDialog spatialiteDialog( this );
  spatialiteDialog.exec();
}

void QgisApp::showRasterCalculator()
{
  QgsRasterCalcDialog d( this );
  if ( d.exec() == QDialog::Accepted )
  {
    //invoke analysis library
    //extent and output resolution will come later...
    QgsRasterCalculator rc( d.formulaString(), d.outputFile(), d.outputFormat(), d.outputRectangle(), d.numberOfColumns(), d.numberOfRows(), d.rasterEntries() );

    QProgressDialog p( tr( "Calculating..." ), tr( "Abort..." ), 0, 0 );
    p.setWindowModality( Qt::WindowModal );
    if ( rc.processCalculation( &p ) == 0 )
    {
      if ( d.addLayerToProject() )
      {
        addRasterLayer( d.outputFile(), QFileInfo( d.outputFile() ).baseName() );
      }
    }
  }
}

void QgisApp::fileOpen()
{
  // possibly save any pending work before opening a new project
  if ( saveDirty() )
  {
    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();
    QString fullPath = QFileDialog::getOpenFileName( this,
                       tr( "Choose a QGIS project file to open" ),
                       lastUsedDir,
                       tr( "QGIS files" ) + " (*.qgs *.QGS)" );
    if ( fullPath.isNull() )
    {
      return;
    }

    // Fix by Tim - getting the dirPath from the dialog
    // directly truncates the last node in the dir path.
    // This is a workaround for that
    QFileInfo myFI( fullPath );
    QString myPath = myFI.path();
    // Persist last used project dir
    settings.setValue( "/UI/lastProjectDir", myPath );

    // open the selected project
    addProject( fullPath );
  }
} // QgisApp::fileOpen

void QgisApp::enableProjectMacros()
{
  mTrustedMacros = true;

  // load macros
  QgsPythonRunner::run( "qgis.utils.reloadProjectMacros()" );
}

/**
  adds a saved project to qgis, usually called on startup by specifying a
  project file on the command line
  */
bool QgisApp::addProject( QString projectFile )
{
  QFileInfo pfi( projectFile );
  statusBar()->showMessage( tr( "Loading project: %1" ).arg( pfi.fileName() ) );
  qApp->processEvents();

  QApplication::setOverrideCursor( Qt::WaitCursor );

  // close the previous opened project if any
  closeProject();

  if ( ! QgsProject::instance()->read( projectFile ) )
  {
    QApplication::restoreOverrideCursor();
    statusBar()->clearMessage();

    QMessageBox::critical( this,
                           tr( "Unable to open project" ),
                           QgsProject::instance()->error() );


    mapCanvas()->freeze( false );
    mapCanvas()->refresh();
    return false;
  }

  setTitleBarText_( *this );
  int  myRedInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorRedPart", 255 );
  int  myGreenInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorGreenPart", 255 );
  int  myBlueInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorBluePart", 255 );
  int  myAlphaInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorAlphaPart", 0 );
  QColor myColor = QColor( myRedInt, myGreenInt, myBlueInt, myAlphaInt );
  mapCanvas()->setCanvasColor( myColor ); //this is fill color before rendering starts
  QgsDebugMsg( "Canvas background color restored..." );

  //load project scales
  bool projectScales = QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" );
  if ( projectScales )
  {
    emit projectScalesChanged( QgsProject::instance()->readListEntry( "Scales", "/ScalesList" ) );
  }

  mapCanvas()->updateScale();
  QgsDebugMsg( "Scale restored..." );

  setFilterLegendByMapEnabled( QgsProject::instance()->readBoolEntry( "Legend", "filterByMap" ) );

  QSettings settings;

  // does the project have any macros?
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    if ( !QgsProject::instance()->readEntry( "Macros", "/pythonCode", QString::null ).isEmpty() )
    {
      int enableMacros = settings.value( "/qgis/enableMacros", 1 ).toInt();
      // 0 = never, 1 = ask, 2 = just for this session, 3 = always

      if ( enableMacros == 3 || enableMacros == 2 )
      {
        enableProjectMacros();
      }
      else if ( enableMacros == 1 ) // ask
      {
        // create the notification widget for macros


        QToolButton *btnEnableMacros = new QToolButton();
        btnEnableMacros->setText( tr( "Enable macros" ) );
        btnEnableMacros->setStyleSheet( "background-color: rgba(255, 255, 255, 0); color: black; text-decoration: underline;" );
        btnEnableMacros->setCursor( Qt::PointingHandCursor );
        btnEnableMacros->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
        connect( btnEnableMacros, SIGNAL( clicked() ), messageBar(), SLOT( popWidget() ) );
        connect( btnEnableMacros, SIGNAL( clicked() ), this, SLOT( enableProjectMacros() ) );

        QgsMessageBarItem *macroMsg = new QgsMessageBarItem(
          tr( "Security warning" ),
          tr( "project macros have been disabled." ),
          btnEnableMacros,
          QgsMessageBar::WARNING,
          0,
          messageBar() );
        // display the macros notification widget
        messageBar()->pushItem( macroMsg );
      }
    }
  }

  emit projectRead(); // let plug-ins know that we've read in a new
  // project so that they can check any project
  // specific plug-in state

  // add this to the list of recently used project files
  saveRecentProjectPath( projectFile, settings );

  QApplication::restoreOverrideCursor();

  mapCanvas()->freeze( false );
  mapCanvas()->refresh();

  statusBar()->showMessage( tr( "Project loaded" ), 3000 );
  return true;
} // QgisApp::addProject(QString projectFile)



bool QgisApp::fileSave()
{
  // if we don't have a file name, then obviously we need to get one; note
  // that the project file name is reset to null in fileNew()
  QFileInfo fullPath;

  // we need to remember if this is a new project so that we know to later
  // update the "last project dir" settings; we know it's a new project if
  // the current project file name is empty
  bool isNewProject = false;

  if ( QgsProject::instance()->fileName().isNull() )
  {
    isNewProject = true;

    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

    QString path = QFileDialog::getSaveFileName(
                     this,
                     tr( "Choose a QGIS project file" ),
                     lastUsedDir + "/" + QgsProject::instance()->title(),
                     tr( "QGIS files" ) + " (*.qgs *.QGS)" );
    if ( path.isEmpty() )
      return false;

    fullPath.setFile( path );

    // make sure we have the .qgs extension in the file name
    if ( "qgs" != fullPath.suffix().toLower() )
    {
      fullPath.setFile( fullPath.filePath() + ".qgs" );
    }


    QgsProject::instance()->setFileName( fullPath.filePath() );
  }
  else
  {
    QFileInfo fi( QgsProject::instance()->fileName() );
    if ( fi.exists() && ! fi.isWritable() )
    {
      messageBar()->pushMessage( tr( "Insufficient permissions" ),
                                 tr( "The project file is not writable." ),
                                 QgsMessageBar::WARNING );
      return false;
    }
  }

  if ( QgsProject::instance()->write() )
  {
    setTitleBarText_( *this ); // update title bar
    statusBar()->showMessage( tr( "Saved project to: %1" ).arg( QgsProject::instance()->fileName() ), 5000 );

    if ( isNewProject )
    {
      // add this to the list of recently used project files
      QSettings settings;
      saveRecentProjectPath( fullPath.filePath(), settings );
    }
  }
  else
  {
    QMessageBox::critical( this,
                           tr( "Unable to save project %1" ).arg( QgsProject::instance()->fileName() ),
                           QgsProject::instance()->error() );
    return false;
  }

  // run the saved project macro
  if ( mTrustedMacros )
  {
    QgsPythonRunner::run( "qgis.utils.saveProjectMacro();" );
  }

  return true;
} // QgisApp::fileSave

void QgisApp::fileSaveAs()
{
  // Retrieve last used project dir from persistent settings
  QSettings settings;
  QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

  QString path = QFileDialog::getSaveFileName( this,
                 tr( "Choose a file name to save the QGIS project file as" ),
                 lastUsedDir + "/" + QgsProject::instance()->title(),
                 tr( "QGIS files" ) + " (*.qgs *.QGS)" );
  if ( path.isEmpty() )
    return;

  QFileInfo fullPath( path );

  settings.setValue( "/UI/lastProjectDir", fullPath.path() );

  // make sure the .qgs extension is included in the path name. if not, add it...
  if ( "qgs" != fullPath.suffix().toLower() )
  {
    fullPath.setFile( fullPath.filePath() + ".qgs" );
  }

  QgsProject::instance()->setFileName( fullPath.filePath() );

  if ( QgsProject::instance()->write() )
  {
    setTitleBarText_( *this ); // update title bar
    statusBar()->showMessage( tr( "Saved project to: %1" ).arg( QgsProject::instance()->fileName() ), 5000 );
    // add this to the list of recently used project files
    saveRecentProjectPath( fullPath.filePath(), settings );
  }
  else
  {
    QMessageBox::critical( this,
                           tr( "Unable to save project %1" ).arg( QgsProject::instance()->fileName() ),
                           QgsProject::instance()->error(),
                           QMessageBox::Ok,
                           Qt::NoButton );
  }
} // QgisApp::fileSaveAs

void QgisApp::dxfExport()
{
  QgsDxfExportDialog d;
  if ( d.exec() == QDialog::Accepted )
  {
    QgsDxfExport dxfExport;
    dxfExport.addLayers( d.layers() );
    dxfExport.setSymbologyScaleDenominator( d.symbologyScale() );
    dxfExport.setSymbologyExport( d.symbologyMode() );
    if ( mapCanvas() )
    {
      dxfExport.setMapUnits( mapCanvas()->mapUnits() );
      //extent
      if ( d.exportMapExtent() )
      {
        dxfExport.setExtent( mapCanvas()->extent() );
      }
    }

    QString fileName( d.saveFile() );
    if ( !fileName.endsWith( ".dxf", Qt::CaseInsensitive ) )
      fileName += ".dxf";
    QFile dxfFile( fileName );
    QApplication::setOverrideCursor( Qt::BusyCursor );
    if ( dxfExport.writeToFile( &dxfFile, d.encoding() ) == 0 )
    {
      messageBar()->pushMessage( tr( "DXF export completed" ), QgsMessageBar::INFO, 4 );
    }
    else
    {
      messageBar()->pushMessage( tr( "DXF export failed" ), QgsMessageBar::CRITICAL, 4 );
    }
    QApplication::restoreOverrideCursor();
  }
}

void QgisApp::kmlExport()
{
  QgsKMLExportDialog d( mapCanvas()->mapSettings().layers() );
  if ( d.exec() == QDialog::Accepted )
  {
    QgsKMLExport kmlExport;
    kmlExport.setLayers( d.selectedLayers() );

    QString fileName = d.saveFile();
    QFile kmlFile( fileName );

    QApplication::setOverrideCursor( Qt::BusyCursor );
    if ( kmlExport.writeToDevice( &kmlFile, mapCanvas()->mapSettings(), d.visibleExtentOnly() ) == 0 )
    {
      messageBar()->pushMessage( tr( "KML export completed" ), QgsMessageBar::INFO, 4 );
    }
    else
    {
      messageBar()->pushMessage( tr( "KML export failed" ), QgsMessageBar::CRITICAL, 4 );
    }
    QApplication::restoreOverrideCursor();
  }
}

void QgisApp::openLayerDefinition( const QString & path )
{
  QString errorMessage;
  bool loaded = QgsLayerDefinition::loadLayerDefinition( path, QgsProject::instance()->layerTreeRoot(), errorMessage );
  if ( !loaded )
  {
    QgsDebugMsg( errorMessage );
    messageBar()->pushMessage( tr( "Error loading layer definition" ), errorMessage, QgsMessageBar::WARNING );
  }
}

// Open the project file corresponding to the
// path at the given index in mRecentProjectPaths
void QgisApp::openProject( QAction *action )
{
  // possibly save any pending work before opening a different project
  QString debugme;
  assert( action != NULL );

  debugme = action->text();

  if ( saveDirty() )
  {
    addProject( debugme );
  }

  //set the projections enabled icon in the status bar
  int myProjectionEnabledFlag =
    QgsProject::instance()->readNumEntry( "SpatialRefSys", "/ProjectionsEnabled", 0 );
  mapCanvas()->setCrsTransformEnabled( myProjectionEnabledFlag );
}

void QgisApp::runScript( const QString &filePath )
{
  if ( !mPythonUtils || !mPythonUtils->isEnabled() )
    return;

  mPythonUtils->runString(
    QString( "import sys\n"
             "execfile(\"%1\".replace(\"\\\\\", \"/\").encode(sys.getfilesystemencoding()))\n" ).arg( filePath )
    , tr( "Failed to run Python script:" ), false );
}


/**
  Open the specified project file; prompt to save previous project if necessary.
  Used to process a commandline argument or OpenDocument AppleEvent.
  */
void QgisApp::openProject( const QString & fileName )
{
  // possibly save any pending work before opening a different project
  if ( saveDirty() )
  {
    // error handling and reporting is in addProject() function
    addProject( fileName );
  }
  return;
}

/**
  Open a raster or vector file; ignore other files.
  Used to process a commandline argument or OpenDocument AppleEvent.
  @returns true if the file is successfully opened
  */
bool QgisApp::openLayer( const QString & fileName, bool allowInteractive )
{
  QFileInfo fileInfo( fileName );
  bool ok( false );

  CPLPushErrorHandler( CPLQuietErrorHandler );

  // if needed prompt for zipitem layers
  QString vsiPrefix = QgsZipItem::vsiPrefix( fileName );
  if ( vsiPrefix == "/vsizip/" || vsiPrefix == "/vsitar/" )
  {
    if ( askUserForZipItemLayers( fileName ) )
    {
      CPLPopErrorHandler();
      return true;
    }
  }

  // Handle georeferenced images (with EXIF tags)
  if ( QgsGeoImageAnnotationItem::create( mapCanvas(), fileName ) )
  {
    return true;
  }

  // try to load it as raster
  if ( QgsRasterLayer::isValidRasterFileName( fileName ) )
  {
    ok  = addRasterLayer( fileName, fileInfo.completeBaseName() );
  }
  // TODO - should we really call isValidRasterFileName() before addRasterLayer()
  //        this results in 2 calls to GDALOpen()
  // I think (Radim) that it is better to test only first if valid,
  // addRasterLayer() is really trying to add layer and gives error if fails
  //
  // if ( addRasterLayer( fileName, fileInfo.completeBaseName() ) )
  // {
  //   ok  = true );
  // }
  else // nope - try to load it as a shape/ogr
  {
    if ( allowInteractive )
    {
      ok = addVectorLayers( QStringList( fileName ), "System", "file" );
    }
    else
    {
      ok = addVectorLayer( fileName, fileInfo.completeBaseName(), "ogr" );
    }
  }

  CPLPopErrorHandler();

  if ( !ok )
  {
    // we have no idea what this file is...
    QgsMessageLog::logMessage( tr( "Unable to load %1" ).arg( fileName ) );
  }

  return ok;
}


// Open a file specified by a commandline argument, Drop or FileOpen event.
void QgisApp::openFile( const QString & fileName )
{
  // check to see if we are opening a project file
  QFileInfo fi( fileName );
  if ( fi.completeSuffix() == "qgs" )
  {
    QgsDebugMsg( "Opening project " + fileName );
    openProject( fileName );
  }
  else if ( fi.completeSuffix() == "qlr" )
  {
    openLayerDefinition( fileName );
  }
  else if ( fi.completeSuffix() == "py" )
  {
    runScript( fileName );
  }
  else
  {
    QgsDebugMsg( "Adding " + fileName + " to the map canvas" );
    openLayer( fileName, true );
  }
}


void QgisApp::newPrintComposer()
{
  QString title = uniqueComposerTitle( this, true );
  if ( title.isNull() )
  {
    return;
  }
  createNewComposer( title );
}

void QgisApp::showComposerManager()
{
  if ( !mComposerManager )
  {
    mComposerManager = new QgsComposerManager( 0, Qt::Window );
    connect( mComposerManager, SIGNAL( finished( int ) ), this, SLOT( deleteComposerManager() ) );
  }
  mComposerManager->show();
  mComposerManager->activate();
}

void QgisApp::deleteComposerManager()
{
  mComposerManager->deleteLater();
  mComposerManager = 0;
}

void QgisApp::disablePreviewMode()
{
  mapCanvas()->setPreviewModeEnabled( false );
}

void QgisApp::activateGrayscalePreview()
{
  mapCanvas()->setPreviewModeEnabled( true );
  mapCanvas()->setPreviewMode( QgsPreviewEffect::PreviewGrayscale );
}

void QgisApp::activateMonoPreview()
{
  mapCanvas()->setPreviewModeEnabled( true );
  mapCanvas()->setPreviewMode( QgsPreviewEffect::PreviewMono );
}

void QgisApp::activateProtanopePreview()
{
  mapCanvas()->setPreviewModeEnabled( true );
  mapCanvas()->setPreviewMode( QgsPreviewEffect::PreviewProtanope );
}

void QgisApp::activateDeuteranopePreview()
{
  mapCanvas()->setPreviewModeEnabled( true );
  mapCanvas()->setPreviewMode( QgsPreviewEffect::PreviewDeuteranope );
}

void QgisApp::toggleFilterLegendByMap()
{
  bool enabled = layerTreeView()->layerTreeModel()->legendFilterByMap();
  setFilterLegendByMapEnabled( !enabled );
}

void QgisApp::setFilterLegendByMapEnabled( bool enabled )
{
  QgsLayerTreeModel* model = layerTreeView()->layerTreeModel();
  bool wasEnabled = model->legendFilterByMap();
  if ( wasEnabled == enabled )
    return; // no change

  mBtnFilterLegend->setChecked( enabled );

  if ( enabled )
  {
    connect( mapCanvas(), SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateFilterLegendByMap() ) );
    model->setLegendFilterByMap( &mapCanvas()->mapSettings() );
  }
  else
  {
    disconnect( mapCanvas(), SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateFilterLegendByMap() ) );
    model->setLegendFilterByMap( 0 );
  }
}

void QgisApp::updateFilterLegendByMap()
{
  layerTreeView()->layerTreeModel()->setLegendFilterByMap( &mapCanvas()->mapSettings() );
}

void QgisApp::saveMapAsImage()
{
  QPair< QString, QString> myFileNameAndFilter = QgisGui::getSaveAsImageName( this, tr( "Choose a file name to save the map image as" ) );
  if ( myFileNameAndFilter.first != "" )
  {
    //save the mapview to the selected file
    mapCanvas()->saveAsImage( myFileNameAndFilter.first, NULL, myFileNameAndFilter.second );
    statusBar()->showMessage( tr( "Saved map image to %1" ).arg( myFileNameAndFilter.first ) );
  }

} // saveMapAsImage

//overloaded version of the above function
void QgisApp::saveMapAsImage( QString theImageFileNameQString, QPixmap * theQPixmap )
{
  if ( theImageFileNameQString == "" )
  {
    //no fileName chosen
    return;
  }
  else
  {
    //force the size of the canvase
    mapCanvas()->resize( theQPixmap->width(), theQPixmap->height() );
    //save the mapview to the selected file
    mapCanvas()->saveAsImage( theImageFileNameQString, theQPixmap );
  }
} // saveMapAsImage

void QgisApp::saveMapToClipboard()
{
  QImage image( mapCanvas()->size(), QImage::Format_ARGB32 );
  image.fill( Qt::transparent );
  QPainter painter( &image );
  mapCanvas()->render( &painter );
  QApplication::clipboard()->setImage( image );
  messageBar()->pushMessage( tr( "Map saved to clipboard" ), QString(), QgsMessageBar::INFO, 5 );
}

//reimplements method from base (gui) class
void QgisApp::addAllToOverview()
{
  if ( layerTreeView() )
  {
    foreach ( QgsLayerTreeLayer* nodeL, layerTreeView()->layerTreeModel()->rootGroup()->findLayers() )
      nodeL->setCustomProperty( "overview", 1 );
  }

  markDirty();
}

//reimplements method from base (gui) class
void QgisApp::removeAllFromOverview()
{
  if ( layerTreeView() )
  {
    foreach ( QgsLayerTreeLayer* nodeL, layerTreeView()->layerTreeModel()->rootGroup()->findLayers() )
      nodeL->setCustomProperty( "overview", 0 );
  }

  markDirty();
}

void QgisApp::toggleFullScreen()
{
  if ( mFullScreenMode )
  {
    if ( mPrevScreenModeMaximized )
    {
      // Change to maximized state. Just calling showMaximized() results in
      // the window going to the normal state. Calling showNormal() then
      // showMaxmized() is a work-around. Turn off rendering for this as it
      // would otherwise cause two re-renders of the map, which can take a
      // long time.
      bool renderFlag = mapCanvas()->renderFlag();
      if ( renderFlag )
        mapCanvas()->setRenderFlag( false );
      showNormal();
      showMaximized();
      if ( renderFlag )
        mapCanvas()->setRenderFlag( true );
      mPrevScreenModeMaximized = false;
    }
    else
    {
      showNormal();
    }
    mFullScreenMode = false;
  }
  else
  {
    if ( isMaximized() )
    {
      mPrevScreenModeMaximized = true;
    }
    showFullScreen();
    mFullScreenMode = true;
  }
}

void QgisApp::showActiveWindowMinimized()
{
  QWidget *window = QApplication::activeWindow();
  if ( window )
  {
    window->showMinimized();
  }
}

void QgisApp::toggleActiveWindowMaximized()
{
  QWidget *window = QApplication::activeWindow();
  if ( window )
  {
    if ( window->isMaximized() )
      window->showNormal();
    else
      window->showMaximized();
  }
}

void QgisApp::activate()
{
  raise();
  setWindowState( windowState() & ~Qt::WindowMinimized );
  activateWindow();
}

void QgisApp::bringAllToFront()
{
#ifdef Q_OS_MAC
  // Bring forward all open windows while maintaining layering order
  ProcessSerialNumber psn;
  GetCurrentProcess( &psn );
  SetFrontProcess( &psn );
#endif
}

void QgisApp::addWindow( QAction *action )
{
#ifdef Q_OS_MAC
  mWindowActions->addAction( action );
  mWindowMenu->addAction( action );
  action->setCheckable( true );
  action->setChecked( true );
#else
  Q_UNUSED( action );
#endif
}

void QgisApp::removeWindow( QAction *action )
{
#ifdef Q_OS_MAC
  mWindowActions->removeAction( action );
  mWindowMenu->removeAction( action );
#else
  Q_UNUSED( action );
#endif
}

void QgisApp::stopRendering()
{
  if ( mapCanvas() )
    mapCanvas()->stopRendering();
}

//reimplements method from base (gui) class
void QgisApp::hideAllLayers()
{
  QgsDebugMsg( "hiding all layers!" );

  foreach ( QgsLayerTreeLayer* nodeLayer, layerTreeView()->layerTreeModel()->rootGroup()->findLayers() )
    nodeLayer->setVisible( Qt::Unchecked );
}


// reimplements method from base (gui) class
void QgisApp::showAllLayers()
{
  QgsDebugMsg( "Showing all layers!" );

  foreach ( QgsLayerTreeLayer* nodeLayer, layerTreeView()->layerTreeModel()->rootGroup()->findLayers() )
    nodeLayer->setVisible( Qt::Checked );
}

//reimplements method from base (gui) class
void QgisApp::hideSelectedLayers()
{
  QgsDebugMsg( "hiding selected layers!" );

  foreach ( QgsLayerTreeNode* node, layerTreeView()->selectedNodes() )
  {
    if ( QgsLayerTree::isGroup( node ) )
      QgsLayerTree::toGroup( node )->setVisible( Qt::Unchecked );
    else if ( QgsLayerTree::isLayer( node ) )
      QgsLayerTree::toLayer( node )->setVisible( Qt::Unchecked );
  }
}


// reimplements method from base (gui) class
void QgisApp::showSelectedLayers()
{
  QgsDebugMsg( "show selected layers!" );

  foreach ( QgsLayerTreeNode* node, layerTreeView()->selectedNodes() )
  {
    if ( QgsLayerTree::isGroup( node ) )
      QgsLayerTree::toGroup( node )->setVisible( Qt::Checked );
    else if ( QgsLayerTree::isLayer( node ) )
      QgsLayerTree::toLayer( node )->setVisible( Qt::Checked );
  }
}


void QgisApp::zoomIn()
{
  QgsDebugMsg( "Setting map tool to zoomIn" );

  mapCanvas()->setMapTool( mMapTools.mZoomIn );
}


void QgisApp::zoomOut()
{
  mapCanvas()->setMapTool( mMapTools.mZoomOut );
}

void QgisApp::zoomToSelected()
{
  mapCanvas()->zoomToSelected();
}

void QgisApp::panToSelected()
{
  mapCanvas()->panToSelected();
}

void QgisApp::pan()
{
  mapCanvas()->setMapTool( mMapTools.mPan );
}

void QgisApp::zoomFull()
{
  mapCanvas()->zoomToFullExtent();
}

void QgisApp::zoomToPrevious()
{
  mapCanvas()->zoomToPreviousExtent();
}

void QgisApp::zoomToNext()
{
  mapCanvas()->zoomToNextExtent();
}

void QgisApp::zoomActualSize()
{
  legendLayerZoomNative();
}

void QgisApp::identify()
{
  mapCanvas()->setMapTool( mMapTools.mIdentify );
}

void QgisApp::doFeatureAction()
{
  mapCanvas()->setMapTool( mMapTools.mFeatureAction );
}

void QgisApp::updateDefaultFeatureAction( QAction *action )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !vlayer )
    return;

  featureActionMenu()->setActiveAction( action );

  int index = featureActionMenu()->actions().indexOf( action );

  if ( vlayer->actions()->size() > 0 && index < vlayer->actions()->size() )
  {
    vlayer->actions()->setDefaultAction( index );
    QgsMapLayerActionRegistry::instance()->setDefaultActionForLayer( vlayer, 0 );
  }
  else
  {
    //action is from QgsMapLayerActionRegistry
    vlayer->actions()->setDefaultAction( -1 );

    QgsMapLayerAction * mapLayerAction = dynamic_cast<QgsMapLayerAction *>( action );
    if ( mapLayerAction )
    {
      QgsMapLayerActionRegistry::instance()->setDefaultActionForLayer( vlayer, mapLayerAction );
    }
    else
    {
      QgsMapLayerActionRegistry::instance()->setDefaultActionForLayer( vlayer, 0 );
    }
  }

  doFeatureAction();
}

void QgisApp::refreshFeatureActions()
{
  featureActionMenu()->clear();

  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !vlayer )
    return;

  QgsAttributeAction *actions = vlayer->actions();
  for ( int i = 0; i < actions->size(); i++ )
  {
    QAction *action = featureActionMenu()->addAction( actions->at( i ).name() );
    if ( i == actions->defaultAction() )
    {
      featureActionMenu()->setActiveAction( action );
    }
  }

  //add actions registered in QgsMapLayerActionRegistry
  QList<QgsMapLayerAction *> registeredActions = QgsMapLayerActionRegistry::instance()->mapLayerActions( vlayer );
  if ( actions->size() > 0 && registeredActions.size() > 0 )
  {
    //add a separator between user defined and standard actions
    featureActionMenu()->addSeparator();
  }

  for ( int i = 0; i < registeredActions.size(); i++ )
  {
    featureActionMenu()->addAction( registeredActions.at( i ) );
    if ( registeredActions.at( i ) == QgsMapLayerActionRegistry::instance()->defaultActionForLayer( vlayer ) )
    {
      featureActionMenu()->setActiveAction( registeredActions.at( i ) );
    }
  }

}

void QgisApp::toggleTool( QgsMapTool* tool, bool active )
{
  if ( active )
    mapCanvas()->setMapTool( tool );
  else
    mapCanvas()->unsetMapTool( tool );
}

void QgisApp::measure( bool active )
{
  toggleTool( mMapTools.mMeasureDist, active );
}

void QgisApp::measureArea( bool active )
{
  toggleTool( mMapTools.mMeasureArea, active );
}

void QgisApp::measureCircle( bool active )
{
  toggleTool( mMapTools.mMeasureCircle, active );
}

void QgisApp::measureHeightProfile( bool active )
{
  toggleTool( mMapTools.mMeasureHeightProfile, active );
}

void QgisApp::measureAngle( bool active )
{
  toggleTool( mMapTools.mMeasureAngle, active );
}

void QgisApp::slope( bool active )
{
  toggleTool( mMapTools.mSlope, active );
}

void QgisApp::hillshade( bool active )
{
  toggleTool( mMapTools.mHillshade, active );
}

void QgisApp::viewshed( bool active )
{
  toggleTool( mMapTools.mViewshed, active );
}

void QgisApp::viewshedSector( bool active )
{
  toggleTool( mMapTools.mViewshedSector, active );
}

void QgisApp::addFormAnnotation( bool active )
{
  toggleTool( mMapTools.mFormAnnotation, active );
}

void QgisApp::addHtmlAnnotation( bool active )
{
  toggleTool( mMapTools.mHtmlAnnotation, active );
}

void QgisApp::addTextAnnotation( bool active )
{
  toggleTool( mMapTools.mTextAnnotation, active );
}

void QgisApp::addPinAnnotation( bool active )
{
  toggleTool( mMapTools.mPinAnnotation, active );
}

void QgisApp::addSvgAnnotation( bool active )
{
  toggleTool( mMapTools.mSvgAnnotation, active );
}

void QgisApp::labelingFontNotFound( QgsVectorLayer* vlayer, const QString& fontfamily )
{
  // TODO: update when pref for how to resolve missing family (use matching algorithm or just default font) is implemented
  QString substitute = tr( "Default system font substituted." );

  QToolButton* btnOpenPrefs = new QToolButton();
  btnOpenPrefs->setStyleSheet( "QToolButton{ background-color: rgba(255, 255, 255, 0); color: black; text-decoration: underline; }" );
  btnOpenPrefs->setCursor( Qt::PointingHandCursor );
  btnOpenPrefs->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  btnOpenPrefs->setToolButtonStyle( Qt::ToolButtonTextOnly );

  // store pointer to vlayer in data of QAction
  QAction* act = new QAction( btnOpenPrefs );
  act->setData( QVariant( QMetaType::QObjectStar, &vlayer ) );
  act->setText( tr( "Open labeling dialog" ) );
  btnOpenPrefs->addAction( act );
  btnOpenPrefs->setDefaultAction( act );
  btnOpenPrefs->setToolTip( "" );
  connect( btnOpenPrefs, SIGNAL( triggered( QAction* ) ), this, SLOT( labelingDialogFontNotFound( QAction* ) ) );

  // no timeout set, since notice needs attention and is only shown first time layer is labeled
  QgsMessageBarItem* fontMsg = new QgsMessageBarItem(
    tr( "Labeling" ),
    tr( "Font for layer <b><u>%1</u></b> was not found (<i>%2</i>). %3" ).arg( vlayer->name() ).arg( fontfamily ).arg( substitute ),
    btnOpenPrefs,
    QgsMessageBar::WARNING,
    0,
    messageBar() );
  messageBar()->pushItem( fontMsg );
}

void QgisApp::commitError( QgsVectorLayer* vlayer )
{
  QgsMessageViewer *mv = new QgsMessageViewer();
  mv->setWindowTitle( tr( "Commit errors" ) );
  mv->setMessageAsPlainText( tr( "Could not commit changes to layer %1" ).arg( vlayer->name() )
                             + "\n\n"
                             + tr( "Errors: %1\n" ).arg( vlayer->commitErrors().join( "\n  " ) )
                           );

  QToolButton *showMore = new QToolButton();
  // store pointer to vlayer in data of QAction
  QAction *act = new QAction( showMore );
  act->setData( QVariant( QMetaType::QObjectStar, &vlayer ) );
  act->setText( tr( "Show more" ) );
  showMore->setStyleSheet( "background-color: rgba(255, 255, 255, 0); color: black; text-decoration: underline;" );
  showMore->setCursor( Qt::PointingHandCursor );
  showMore->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  showMore->addAction( act );
  showMore->setDefaultAction( act );
  connect( showMore, SIGNAL( triggered( QAction* ) ), mv, SLOT( exec() ) );
  connect( showMore, SIGNAL( triggered( QAction* ) ), showMore, SLOT( deleteLater() ) );

  // no timeout set, since notice needs attention and is only shown first time layer is labeled
  QgsMessageBarItem *errorMsg = new QgsMessageBarItem(
    tr( "Commit errors" ),
    tr( "Could not commit changes to layer %1" ).arg( vlayer->name() ),
    showMore,
    QgsMessageBar::WARNING,
    0,
    messageBar() );
  messageBar()->pushItem( errorMsg );
}

void QgisApp::labelingDialogFontNotFound( QAction* act )
{
  if ( !act )
  {
    return;
  }

  // get base pointer to layer
  QObject* obj = qvariant_cast<QObject*>( act->data() );

  // remove calling messagebar widget
  messageBar()->popWidget();

  if ( !obj )
  {
    return;
  }

  QgsMapLayer* layer = qobject_cast<QgsMapLayer*>( obj );
  if ( layer && setActiveLayer( layer ) )
  {
    labeling();
  }
}

void QgisApp::labeling()
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>( activeLayer() );
  if ( !vlayer )
  {
    messageBar()->pushMessage( tr( "Labeling Options" ),
                               tr( "Please select a vector layer first" ),
                               QgsMessageBar::INFO,
                               messageTimeout() );
    return;
  }


  QDialog *dlg = new QDialog( this );
  dlg->setWindowTitle( tr( "Layer labeling settings" ) );
  QgsLabelingGui *labelingGui = new QgsLabelingGui( vlayer, mapCanvas(), dlg );
  labelingGui->init(); // load QgsPalLayerSettings for layer
  labelingGui->layout()->setContentsMargins( 0, 0, 0, 0 );
  QVBoxLayout *layout = new QVBoxLayout( dlg );
  layout->addWidget( labelingGui );

  QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, Qt::Horizontal, dlg );
  layout->addWidget( buttonBox );

  dlg->setLayout( layout );

  QSettings settings;
  dlg->restoreGeometry( settings.value( "/Windows/Labeling/geometry" ).toByteArray() );

  connect( buttonBox->button( QDialogButtonBox::Ok ), SIGNAL( clicked() ), dlg, SLOT( accept() ) );
  connect( buttonBox->button( QDialogButtonBox::Cancel ), SIGNAL( clicked() ), dlg, SLOT( reject() ) );
  connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked() ), labelingGui, SLOT( apply() ) );

  if ( dlg->exec() )
  {
    labelingGui->apply();

    settings.setValue( "/Windows/Labeling/geometry", dlg->saveGeometry() );

    // alter labeling - save the changes
    labelingGui->layerSettings().writeToLayer( vlayer );

    // trigger refresh
    if ( mapCanvas() )
    {
      mapCanvas()->refresh();
    }
  }

  delete dlg;

  activateDeactivateLayerRelatedActions( vlayer );
}

void QgisApp::setCadDockVisible( bool visible )
{
  mAdvancedDigitizingDockWidget->setVisible( visible );
}

void QgisApp::fieldCalculator()
{
  QgsVectorLayer *myLayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !myLayer )
  {
    return;
  }

  QgsFieldCalculator calc( myLayer );
  if ( calc.exec() )
  {
    mapCanvas()->refresh();
  }
}

void QgisApp::attributeTable()
{
  QgsVectorLayer *myLayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !myLayer )
  {
    return;
  }

  QgsAttributeTableDialog *mDialog = new QgsAttributeTableDialog( myLayer );
  mDialog->show();
  // the dialog will be deleted by itself on close
}

void QgisApp::saveAsRasterFile()
{
  QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer *>( activeLayer() );
  if ( !rasterLayer )
  {
    return;
  }

  QgsRasterLayerSaveAsDialog d( rasterLayer, rasterLayer->dataProvider(),
                                mapCanvas()->extent(), rasterLayer->crs(),
                                mapCanvas()->mapSettings().destinationCrs(),
                                this );
  if ( d.exec() == QDialog::Accepted )
  {
    QSettings settings;
    settings.setValue( "/UI/lastRasterFileDir", QFileInfo( d.outputFileName() ).absolutePath() );

    QgsRasterFileWriter fileWriter( d.outputFileName() );
    if ( d.tileMode() )
    {
      fileWriter.setTiledMode( true );
      fileWriter.setMaxTileWidth( d.maximumTileSizeX() );
      fileWriter.setMaxTileHeight( d.maximumTileSizeY() );
    }

    QProgressDialog pd( 0, tr( "Abort..." ), 0, 0 );
    // Show the dialo immediately because cloning pipe can take some time (WCS)
    pd.setLabelText( tr( "Reading raster" ) );
    pd.show();
    pd.setWindowModality( Qt::WindowModal );

    // TODO: show error dialogs
    // TODO: this code should go somewhere else, but probably not into QgsRasterFileWriter
    // clone pipe/provider is not really necessary, ready for threads
    QScopedPointer<QgsRasterPipe> pipe( 0 );

    if ( d.mode() == QgsRasterLayerSaveAsDialog::RawDataMode )
    {
      QgsDebugMsg( "Writing raw data" );
      //QgsDebugMsg( QString( "Writing raw data" ).arg( pos ) );
      pipe.reset( new QgsRasterPipe() );
      if ( !pipe->set( rasterLayer->dataProvider()->clone() ) )
      {
        QgsDebugMsg( "Cannot set pipe provider" );
        return;
      }

      QgsRasterNuller *nuller = new QgsRasterNuller();
      for ( int band = 1; band <= rasterLayer->dataProvider()->bandCount(); band ++ )
      {
        nuller->setNoData( band, d.noData() );
      }
      if ( !pipe->insert( 1, nuller ) )
      {
        QgsDebugMsg( "Cannot set pipe nuller" );
        return;
      }

      // add projector if necessary
      if ( d.outputCrs() != rasterLayer->crs() )
      {
        QgsRasterProjector * projector = new QgsRasterProjector;
        projector->setCRS( rasterLayer->crs(), d.outputCrs() );
        if ( !pipe->insert( 2, projector ) )
        {
          QgsDebugMsg( "Cannot set pipe projector" );
          return;
        }
      }
    }
    else // RenderedImageMode
    {
      // clone the whole pipe
      QgsDebugMsg( "Writing rendered image" );
      pipe.reset( new QgsRasterPipe( *rasterLayer->pipe() ) );
      QgsRasterProjector *projector = pipe->projector();
      if ( !projector )
      {
        QgsDebugMsg( "Cannot get pipe projector" );
        return;
      }
      projector->setCRS( rasterLayer->crs(), d.outputCrs() );
    }

    if ( !pipe->last() )
    {
      return;
    }
    fileWriter.setCreateOptions( d.createOptions() );

    fileWriter.setBuildPyramidsFlag( d.buildPyramidsFlag() );
    fileWriter.setPyramidsList( d.pyramidsList() );
    fileWriter.setPyramidsResampling( d.pyramidsResamplingMethod() );
    fileWriter.setPyramidsFormat( d.pyramidsFormat() );
    fileWriter.setPyramidsConfigOptions( d.pyramidsConfigOptions() );

    QgsRasterFileWriter::WriterError err = fileWriter.writeRaster( pipe.data(), d.nColumns(), d.nRows(), d.outputRectangle(), d.outputCrs(), &pd );
    if ( err != QgsRasterFileWriter::NoError )
    {
      QMessageBox::warning( this, tr( "Error" ),
                            tr( "Cannot write raster error code: %1" ).arg( err ),
                            QMessageBox::Ok );

    }
    else
    {
      emit layerSavedAs( rasterLayer, d.outputFileName() );
    }
  }
}

void QgisApp::saveAsFile()
{
  QgsMapLayer* layer = activeLayer();
  if ( !layer )
    return;

  QgsMapLayer::LayerType layerType = layer->type();
  if ( layerType == QgsMapLayer::RasterLayer )
  {
    saveAsRasterFile();
  }
  else if ( layerType == QgsMapLayer::VectorLayer )
  {
    saveAsVectorFileGeneral();
  }
}

void QgisApp::saveAsLayerDefinition()
{

  QString path = QFileDialog::getSaveFileName( this, "Save as Layer Definition File", QDir::home().path(), "*.qlr" );
  QgsDebugMsg( path );
  if ( path.isEmpty() )
    return;

  QString errorMessage;
  bool saved = QgsLayerDefinition::exportLayerDefinition( path, layerTreeView()->selectedNodes(), errorMessage );
  if ( !saved )
  {
    messageBar()->pushMessage( tr( "Error saving layer definintion file" ), errorMessage, QgsMessageBar::WARNING );
  }
}

void QgisApp::saveAsVectorFileGeneral( QgsVectorLayer* vlayer, bool symbologyOption )
{
  if ( !vlayer )
  {
    vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() ); // FIXME: output of multiple layers at once?
  }

  if ( !vlayer )
    return;

  QgsCoordinateReferenceSystem destCRS;

  int options = QgsVectorLayerSaveAsDialog::AllOptions;
  if ( !symbologyOption )
  {
    options &= ~QgsVectorLayerSaveAsDialog::Symbology;
  }

  QgsVectorLayerSaveAsDialog *dialog = new QgsVectorLayerSaveAsDialog( vlayer->crs().srsid(), vlayer->extent(), vlayer->selectedFeatureCount() != 0, options, this );

  dialog->setCanvasExtent( mapCanvas()->mapSettings().visibleExtent(), mapCanvas()->mapSettings().destinationCrs() );

  if ( dialog->exec() == QDialog::Accepted )
  {
    QString encoding = dialog->encoding();
    QString vectorFilename = dialog->filename();
    QString format = dialog->format();
    QStringList datasourceOptions = dialog->datasourceOptions();

    QgsCoordinateTransform* ct = 0;
    destCRS = QgsCoordinateReferenceSystem( dialog->crs(), QgsCoordinateReferenceSystem::InternalCrsId );

    if ( destCRS.isValid() && destCRS != vlayer->crs() )
    {
      ct = new QgsCoordinateTransform( vlayer->crs(), destCRS );

      //ask user about datum transformation
      QSettings settings;
      QList< QList< int > > dt = QgsCoordinateTransform::datumTransformations( vlayer->crs(), destCRS );
      if ( dt.size() > 1 && settings.value( "/Projections/showDatumTransformDialog", false ).toBool() )
      {
        QgsDatumTransformDialog d( vlayer->name(), dt );
        if ( d.exec() == QDialog::Accepted )
        {
          QList< int > sdt = d.selectedDatumTransform();
          if ( sdt.size() > 0 )
          {
            ct->setSourceDatumTransform( sdt.at( 0 ) );
          }
          if ( sdt.size() > 1 )
          {
            ct->setDestinationDatumTransform( sdt.at( 1 ) );
          }
          ct->initialise();
        }
      }
    }

    // ok if the file existed it should be deleted now so we can continue...
    QApplication::setOverrideCursor( Qt::WaitCursor );

    QgsVectorFileWriter::WriterError error;
    QString errorMessage;
    QString newFilename;
    QgsRectangle filterExtent = dialog->filterExtent();
    error = QgsVectorFileWriter::writeAsVectorFormat(
              vlayer, vectorFilename, encoding, ct, format,
              dialog->onlySelected(),
              &errorMessage,
              datasourceOptions, dialog->layerOptions(),
              dialog->skipAttributeCreation(),
              &newFilename,
              ( QgsVectorFileWriter::SymbologyExport )( dialog->symbologyExport() ),
              dialog->scaleDenominator(),
              dialog->hasFilterExtent() ? &filterExtent : 0 );

    delete ct;

    QApplication::restoreOverrideCursor();

    if ( error == QgsVectorFileWriter::NoError )
    {
      if ( dialog->addToCanvas() )
      {
        addVectorLayers( QStringList( newFilename ), encoding, "file" );
      }
      emit layerSavedAs( vlayer, vectorFilename );
      messageBar()->pushMessage( tr( "Saving done" ),
                                 tr( "Export to vector file has been completed" ),
                                 QgsMessageBar::INFO, messageTimeout() );
    }
    else
    {
      QgsMessageViewer *m = new QgsMessageViewer( 0 );
      m->setWindowTitle( tr( "Save error" ) );
      m->setMessageAsPlainText( tr( "Export to vector file failed.\nError: %1" ).arg( errorMessage ) );
      m->exec();
    }
  }

  delete dialog;
}

void QgisApp::checkForDeprecatedLabelsInProject()
{
  bool ok = false;
  QgsProject::instance()->readBoolEntry( "DeprecatedLabels", "/Enabled", false, &ok );
  if ( ok ) // project already flagged (regardless of project property value)
  {
    return;
  }

  if ( QgsMapLayerRegistry::instance()->count() > 0 )
  {
    bool depLabelsUsed = false;
    QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
    for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
    {
      QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() );
      if ( !vl )
      {
        continue;
      }

      depLabelsUsed = vl->hasLabelsEnabled();
      if ( depLabelsUsed )
      {
        break;
      }
    }
    if ( depLabelsUsed )
    {
      QgsProject::instance()->writeEntry( "DeprecatedLabels", "/Enabled", true );
    }
  }
}

void QgisApp::layerProperties()
{
  showLayerProperties( activeLayer() );
}

void QgisApp::groupProperties()
{
  if ( !layerTreeView() )
  {
    return;
  }

  QModelIndex idx = layerTreeView()->currentIndex();
  if ( !idx.isValid() )
  {
    return;
  }

  QgsLayerTreeNode* node = layerTreeView()->layerTreeModel()->index2node( idx );
  if ( !node || !QgsLayerTree::isGroup( node ) )
  {
    return;
  }

  QgsLayerTreeGroup* gnode = static_cast<QgsLayerTreeGroup*>( node );
  QgsLegendGroupProperties p( gnode, this );
  p.exec();
}

void QgisApp::deleteSelected( QgsMapLayer *layer, QWidget* parent, bool promptConfirmation )
{
  if ( !layer )
  {
    layer = layerTreeView()->currentLayer();
  }

  if ( !parent )
  {
    parent = this;
  }

  if ( !layer )
  {
    messageBar()->pushMessage( tr( "No Layer Selected" ),
                               tr( "To delete features, you must select a vector layer in the legend" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer )
  {
    messageBar()->pushMessage( tr( "No Vector Layer Selected" ),
                               tr( "Deleting features only works on vector layers" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  if ( !( vlayer->dataProvider()->capabilities() & QgsVectorDataProvider::DeleteFeatures ) )
  {
    messageBar()->pushMessage( tr( "Provider does not support deletion" ),
                               tr( "Data provider does not support deleting features" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  if ( !vlayer->isEditable() )
  {
    messageBar()->pushMessage( tr( "Layer not editable" ),
                               tr( "The current layer is not editable. Choose 'Start editing' in the digitizing toolbar." ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  //validate selection
  int numberOfSelectedFeatures = vlayer->selectedFeaturesIds().size();
  if ( numberOfSelectedFeatures == 0 )
  {
    messageBar()->pushMessage( tr( "No Features Selected" ),
                               tr( "The current layer has no selected features" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }
  //display a warning
  if ( promptConfirmation && QMessageBox::warning( parent, tr( "Delete features" ), tr( "Delete %n feature(s)?", "number of features to delete", numberOfSelectedFeatures ), QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
  {
    return;
  }

  vlayer->beginEditCommand( tr( "Features deleted" ) );
  int deletedCount = 0;
  if ( !vlayer->deleteSelectedFeatures( &deletedCount ) )
  {
    messageBar()->pushMessage( tr( "Problem deleting features" ),
                               tr( "A problem occured during deletion of %1 feature(s)" ).arg( numberOfSelectedFeatures - deletedCount ),
                               QgsMessageBar::WARNING );
  }
  else
  {
    showStatusMessage( tr( "%n feature(s) deleted.", "number of features deleted", numberOfSelectedFeatures ) );
  }

  vlayer->endEditCommand();
}

void QgisApp::moveFeature()
{
  mapCanvas()->setMapTool( mMapTools.mMoveFeature );
}

void QgisApp::offsetCurve()
{
  mapCanvas()->setMapTool( mMapTools.mOffsetCurve );
}

void QgisApp::simplifyFeature()
{
  mapCanvas()->setMapTool( mMapTools.mSimplifyFeature );
}

void QgisApp::deleteRing()
{
  mapCanvas()->setMapTool( mMapTools.mDeleteRing );
}

void QgisApp::deletePart()
{
  mapCanvas()->setMapTool( mMapTools.mDeletePart );
}

QgsGeometry* QgisApp::unionGeometries( const QgsVectorLayer* vl, QgsFeatureList& featureList, bool& canceled )
{
  canceled = false;
  if ( !vl || featureList.size() < 2 )
  {
    return 0;
  }

  QgsGeometry* unionGeom = featureList[0].geometry();
  QgsGeometry* backupPtr = 0; //pointer to delete intermediate results
  if ( !unionGeom )
  {
    return 0;
  }

  QProgressDialog progress( tr( "Merging features..." ), tr( "Abort" ), 0, featureList.size(), this );
  progress.setWindowModality( Qt::WindowModal );

  QApplication::setOverrideCursor( Qt::WaitCursor );

  for ( int i = 1; i < featureList.size(); ++i )
  {
    if ( progress.wasCanceled() )
    {
      delete unionGeom;
      QApplication::restoreOverrideCursor();
      canceled = true;
      return 0;
    }
    progress.setValue( i );
    QgsGeometry* currentGeom = featureList[i].geometry();
    if ( currentGeom )
    {
      backupPtr = unionGeom;
      unionGeom = unionGeom->combine( currentGeom );
      if ( i > 1 ) //delete previous intermediate results
      {
        delete backupPtr;
        backupPtr = 0;
      }
      if ( !unionGeom )
      {
        QApplication::restoreOverrideCursor();
        return 0;
      }
    }
  }

  //convert unionGeom to a multipart geometry in case it is necessary to match the layer type
  if ( QGis::isMultiType( vl->wkbType() ) && !unionGeom->isMultipart() )
  {
    unionGeom->convertToMultiType();
  }

  QApplication::restoreOverrideCursor();
  progress.setValue( featureList.size() );
  return unionGeom;
}

QString QgisApp::uniqueComposerTitle( QWidget* parent, bool acceptEmpty, const QString& currentName )
{
  if ( !parent )
  {
    parent = this;
  }
  bool ok = false;
  bool titleValid = false;
  QString newTitle = QString( currentName );
  QString chooseMsg = tr( "Create unique print composer title" );
  if ( acceptEmpty )
  {
    chooseMsg += "\n" + tr( "(title generated if left empty)" );
  }
  QString titleMsg = chooseMsg;

  QStringList cNames;
  cNames << newTitle;
  foreach ( QgsComposer* c, printComposers() )
  {
    cNames << c->title();
  }

  while ( !titleValid )
  {
    newTitle = QInputDialog::getItem( parent,
                                      tr( "Composer title" ),
                                      titleMsg,
                                      cNames,
                                      cNames.indexOf( newTitle ),
                                      true,
                                      &ok );
    if ( !ok )
    {
      return QString::null;
    }

    if ( newTitle.isEmpty() )
    {
      if ( !acceptEmpty )
      {
        titleMsg = chooseMsg + "\n\n" + tr( "Title can not be empty!" );
      }
      else
      {
        newTitle = QString( "" );
        titleValid = true;
      }
    }
    else if ( cNames.indexOf( newTitle, 1 ) >= 0 )
    {
      cNames[0] = QString( "" ); // clear non-unique name
      titleMsg = chooseMsg + "\n\n" + tr( "Title already exists!" );
    }
    else
    {
      titleValid = true;
    }
  }

  return newTitle;
}

QgsComposer* QgisApp::createNewComposer( QString title )
{
  //ask user about name
  mLastComposerId++;
  if ( title.isEmpty() )
  {
    title = tr( "Composer %1" ).arg( mLastComposerId );
  }
  //create new composer object
  QgsComposer* newComposerObject = new QgsComposer( this, title );

  //add it to the map of existing print composers
  mPrintComposers.insert( newComposerObject );
  //and place action into print composers menu
  printComposersMenu()->addAction( newComposerObject->windowAction() );
  newComposerObject->open();
  emit composerAdded( newComposerObject->view() );
  connect( newComposerObject, SIGNAL( composerAdded( QgsComposerView* ) ), this, SIGNAL( composerAdded( QgsComposerView* ) ) );
  connect( newComposerObject, SIGNAL( composerWillBeRemoved( QgsComposerView* ) ), this, SIGNAL( composerWillBeRemoved( QgsComposerView* ) ) );
  connect( newComposerObject, SIGNAL( atlasPreviewFeatureChanged() ), this, SLOT( refreshMapCanvas() ) );
  markDirty();
  return newComposerObject;
}

void QgisApp::deleteComposer( QgsComposer* c )
{
  emit composerWillBeRemoved( c->view() );
  mPrintComposers.remove( c );
  printComposersMenu()->removeAction( c->windowAction() );
  markDirty();
  emit composerRemoved( c->view() );

  //save a reference to the composition
  QgsComposition* composition = c->composition();

  //first, delete the composer. This must occur before deleting the composition as some of the cleanup code in
  //composer or in composer item widgets may require the composition to still be around
  delete c;

  //next, delete the composition
  if ( composition )
  {
    delete composition;
  }
}

QgsComposer* QgisApp::duplicateComposer( QgsComposer* currentComposer, QString title )
{
  QgsComposer* newComposer = 0;

  // test that current composer template write is valid
  QDomDocument currentDoc;
  currentComposer->templateXML( currentDoc );
  QDomElement compositionElem = currentDoc.documentElement().firstChildElement( "Composition" );
  if ( compositionElem.isNull() )
  {
    QgsDebugMsg( "selected composer could not be stored as temporary template" );
    return newComposer;
  }

  if ( title.isEmpty() )
  {
    // TODO: inject a bit of randomness in auto-titles?
    title = currentComposer->title() + tr( " copy" );
  }

  newComposer = createNewComposer( title );
  if ( !newComposer )
  {
    QgsDebugMsg( "could not create new composer" );
    return newComposer;
  }

  // hiding composer until template is loaded is much faster, provide feedback to user
  newComposer->hide();
  QApplication::setOverrideCursor( Qt::BusyCursor );
  if ( !newComposer->composition()->loadFromTemplate( currentDoc, 0, false ) )
  {
    deleteComposer( newComposer );
    newComposer = 0;
    QgsDebugMsg( "Error, composer could not be duplicated" );
    return newComposer;
  }
  newComposer->activate();
  QApplication::restoreOverrideCursor();

  return newComposer;
}

bool QgisApp::loadComposersFromProject( const QDomDocument& doc )
{
  if ( doc.isNull() )
  {
    return false;
  }

  //restore each composer
  QDomNodeList composerNodes = doc.elementsByTagName( "Composer" );
  for ( int i = 0; i < composerNodes.size(); ++i )
  {
    ++mLastComposerId;
    QgsComposer* composer = new QgsComposer( this, tr( "Composer %1" ).arg( mLastComposerId ) );
    composer->readXML( composerNodes.at( i ).toElement(), doc );
    mPrintComposers.insert( composer );
    printComposersMenu()->addAction( composer->windowAction() );
#ifndef Q_OS_MACX
    composer->setWindowState( Qt::WindowMinimized );
    composer->show();
#endif
    composer->zoomFull();
    QgsComposerView* composerView = composer->view();
    if ( composerView )
    {
      composerView->updateRulers();
    }
    if ( composerNodes.at( i ).toElement().attribute( "visible", "1" ).toInt() < 1 )
    {
      composer->close();
    }
    emit composerAdded( composer->view() );
    connect( composer, SIGNAL( composerAdded( QgsComposerView* ) ), this, SIGNAL( composerAdded( QgsComposerView* ) ) );
    connect( composer, SIGNAL( composerWillBeRemoved( QgsComposerView* ) ), this, SIGNAL( composerWillBeRemoved( QgsComposerView* ) ) );
    connect( composer, SIGNAL( atlasPreviewFeatureChanged() ), this, SLOT( refreshMapCanvas() ) );
  }
  return true;
}

void QgisApp::deletePrintComposers()
{
  QSet<QgsComposer*>::iterator it = mPrintComposers.begin();
  while ( it != mPrintComposers.end() )
  {
    QgsComposer* c = ( *it );
    emit composerWillBeRemoved( c->view() );
    it = mPrintComposers.erase( it );
    emit composerRemoved( c->view() );

    //save a reference to the composition
    QgsComposition* composition = c->composition();

    //first, delete the composer. This must occur before deleting the composition as some of the cleanup code in
    //composer or in composer item widgets may require the composition to still be around
    delete( c );

    //next, delete the composition
    if ( composition )
    {
      delete composition;
    }
  }
  mLastComposerId = 0;
  markDirty();
}

void QgisApp::preparePrintComposersMenu()
{
  QList<QAction*> acts = printComposersMenu()->actions();
  printComposersMenu()->clear();
  if ( acts.size() > 1 )
  {
    // sort actions by text
    qSort( acts.begin(), acts.end(), cmpByText_ );
  }
  printComposersMenu()->addActions( acts );
}

bool QgisApp::loadAnnotationItemsFromProject( const QDomDocument& doc )
{
  if ( !mapCanvas() )
  {
    return false;
  }

  removeAnnotationItems();

  if ( doc.isNull() )
  {
    return false;
  }

  foreach ( const QgsAnnotationItem::AnnotationRegistryItem& item, QgsAnnotationItem::registeredAnnotations() )
  {
    QDomNodeList itemList = doc.elementsByTagName( item.first );
    for ( int i = 0; i < itemList.size(); ++i )
    {
      QgsAnnotationItem* newItem = item.second( mapCanvas() );
      newItem->readXML( doc, itemList.at( i ).toElement() );
    }
  }
  return true;
}

void QgisApp::showPinnedLabels( bool show )
{
  qobject_cast<QgsMapToolPinLabels*>( mMapTools.mPinLabels )->showPinnedLabels( show );
}

void QgisApp::pinLabels()
{
  mapCanvas()->setMapTool( mMapTools.mPinLabels );
}

void QgisApp::showHideLabels()
{
  mapCanvas()->setMapTool( mMapTools.mShowHideLabels );
}

void QgisApp::moveLabel()
{
  mapCanvas()->setMapTool( mMapTools.mMoveLabel );
}

void QgisApp::rotateFeature()
{
  mapCanvas()->setMapTool( mMapTools.mRotateFeature );
}

void QgisApp::rotateLabel()
{
  mapCanvas()->setMapTool( mMapTools.mRotateLabel );
}

void QgisApp::changeLabelProperties()
{
  mapCanvas()->setMapTool( mMapTools.mChangeLabelProperties );
}

QList<QgsAnnotationItem*> QgisApp::annotationItems()
{
  QList<QgsAnnotationItem*> itemList;

  if ( !mapCanvas() )
  {
    return itemList;
  }

  QList<QGraphicsItem*> graphicsItems = mapCanvas()->items();
  QList<QGraphicsItem*>::iterator gIt = graphicsItems.begin();
  for ( ; gIt != graphicsItems.end(); ++gIt )
  {
    QgsAnnotationItem* currentItem = dynamic_cast<QgsAnnotationItem*>( *gIt );
    if ( currentItem )
    {
      itemList.push_back( currentItem );
    }
  }
  return itemList;
}

void QgisApp::removeAnnotationItems()
{
  if ( !mapCanvas() )
  {
    return;
  }
  QGraphicsScene* scene = mapCanvas()->scene();
  if ( !scene )
  {
    return;
  }
  QList<QgsAnnotationItem*> itemList = annotationItems();
  QList<QgsAnnotationItem*>::iterator itemIt = itemList.begin();
  for ( ; itemIt != itemList.end(); ++itemIt )
  {
    if ( *itemIt )
    {
      scene->removeItem( *itemIt );
      delete *itemIt;
    }
  }
}

void QgisApp::mergeAttributesOfSelectedFeatures()
{
  //get active layer (hopefully vector)
  QgsMapLayer *activeMapLayer = activeLayer();
  if ( !activeMapLayer )
  {
    messageBar()->pushMessage( tr( "No active layer" ),
                               tr( "No active layer found. Please select a layer in the layer list" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( activeMapLayer );
  if ( !vl )
  {
    QMessageBox::information( 0, tr( "Active layer is not vector" ), tr( "The merge features tool only works on vector layers. Please select a vector layer from the layer list" ) );
    return;
  }

  if ( !vl->isEditable() )
  {
    QMessageBox::information( 0, tr( "Layer not editable" ), tr( "Merging features can only be done for layers in editing mode. To use the merge tool, go to Layer->Toggle editing" ) );
    return;
  }

  //get selected feature ids (as a QSet<int> )
  const QgsFeatureIds& featureIdSet = vl->selectedFeaturesIds();
  if ( featureIdSet.size() < 2 )
  {
    QMessageBox::information( 0, tr( "Not enough features selected" ), tr( "The merge tool requires at least two selected features" ) );
    return;
  }

  //get initial selection (may be altered by attribute merge dialog later)
  QgsFeatureList featureList = vl->selectedFeatures();  //get QList<QgsFeature>

  //merge the attributes together
  QgsMergeAttributesDialog d( featureList, vl, mapCanvas() );
  if ( d.exec() == QDialog::Rejected )
  {
    return;
  }

  vl->beginEditCommand( tr( "Merged feature attributes" ) );

  const QgsAttributes &merged = d.mergedAttributes();

  foreach ( QgsFeatureId fid, vl->selectedFeaturesIds() )
  {
    for ( int i = 0; i < merged.count(); ++i )
    {
      vl->changeAttributeValue( fid, i, merged.at( i ) );
    }
  }

  vl->endEditCommand();

  if ( mapCanvas() )
  {
    mapCanvas()->refresh();
  }
}

void QgisApp::mergeSelectedFeatures()
{
  //get active layer (hopefully vector)
  QgsMapLayer* activeMapLayer = activeLayer();
  if ( !activeMapLayer )
  {
    QMessageBox::information( 0, tr( "No active layer" ), tr( "No active layer found. Please select a layer in the layer list" ) );
    return;
  }
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( activeMapLayer );
  if ( !vl )
  {
    QMessageBox::information( 0, tr( "Active layer is not vector" ), tr( "The merge features tool only works on vector layers. Please select a vector layer from the layer list" ) );
    return;
  }
  if ( !vl->isEditable() )
  {
    QMessageBox::information( 0, tr( "Layer not editable" ), tr( "Merging features can only be done for layers in editing mode. To use the merge tool, go to Layer->Toggle editing" ) );
    return;
  }

  QgsVectorDataProvider* dp = vl->dataProvider();
  bool providerChecksTypeStrictly = true;
  if ( dp )
  {
    providerChecksTypeStrictly = dp->doesStrictFeatureTypeCheck();
  }

  //get selected feature ids (as a QSet<int> )
  const QgsFeatureIds& featureIdSet = vl->selectedFeaturesIds();
  if ( featureIdSet.size() < 2 )
  {
    QMessageBox::information( 0, tr( "Not enough features selected" ), tr( "The merge tool requires at least two selected features" ) );
    return;
  }

  //get initial selection (may be altered by attribute merge dialog later)
  QgsFeatureIds featureIds = vl->selectedFeaturesIds();
  QgsFeatureList featureList = vl->selectedFeatures();  //get QList<QgsFeature>
  bool canceled;
  QgsGeometry* unionGeom = unionGeometries( vl, featureList, canceled );
  if ( !unionGeom )
  {
    if ( !canceled )
    {
      QMessageBox::critical( 0, tr( "Merge failed" ), tr( "An error occured during the merge operation" ) );
    }
    return;
  }

  //make a first geometry union and notify the user straight away if the union geometry type does not match the layer one
  if ( providerChecksTypeStrictly && unionGeom->wkbType() != vl->wkbType() )
  {
    QMessageBox::critical( 0, tr( "Union operation canceled" ), tr( "The union operation would result in a geometry type that is not compatible with the current layer and therefore is canceled" ) );
    delete unionGeom;
    return;
  }

  //merge the attributes together
  QgsMergeAttributesDialog d( featureList, vl, mapCanvas() );
  if ( d.exec() == QDialog::Rejected )
  {
    delete unionGeom;
    return;
  }

  QgsFeatureIds featureIdsAfter = vl->selectedFeaturesIds();

  if ( featureIdsAfter.size() < 2 )
  {
    QMessageBox::information( 0, tr( "Not enough features selected" ), tr( "The merge tool requires at least two selected features" ) );
    delete unionGeom;
    return;
  }

  //if the user changed the feature selection in the merge dialog, we need to repeat the union and check the type
  if ( featureIds.size() != featureIdsAfter.size() )
  {
    delete unionGeom;
    bool canceled;
    QgsFeatureList featureListAfter = vl->selectedFeatures();
    unionGeom = unionGeometries( vl, featureListAfter, canceled );
    if ( !unionGeom )
    {
      if ( !canceled )
      {
        QMessageBox::critical( 0, tr( "Merge failed" ), tr( "An error occured during the merge operation" ) );
      }
      return;
    }

    if ( providerChecksTypeStrictly && unionGeom->wkbType() != vl->wkbType() )
    {
      QMessageBox::critical( 0, "Union operation canceled", tr( "The union operation would result in a geometry type that is not compatible with the current layer and therefore is canceled" ) );
      delete unionGeom;
      return;
    }
  }

  vl->beginEditCommand( tr( "Merged features" ) );

  //create new feature
  QgsFeature newFeature;
  newFeature.setGeometry( unionGeom );
  newFeature.setAttributes( d.mergedAttributes() );

  QgsFeatureIds::const_iterator feature_it = featureIdsAfter.constBegin();
  for ( ; feature_it != featureIdsAfter.constEnd(); ++feature_it )
  {
    vl->deleteFeature( *feature_it );
  }

  vl->addFeature( newFeature, false );

  vl->endEditCommand();

  if ( mapCanvas() )
  {
    mapCanvas()->refresh();
  }
}

void QgisApp::nodeTool()
{
  mapCanvas()->setMapTool( mMapTools.mNodeTool );
}

void QgisApp::rotatePointSymbols()
{
  mapCanvas()->setMapTool( mMapTools.mRotatePointSymbolsTool );
}

void QgisApp::snappingOptions()
{
  mSnappingDialog->show();
}

void QgisApp::splitFeatures()
{
  mapCanvas()->setMapTool( mMapTools.mSplitFeatures );
}

void QgisApp::splitParts()
{
  mapCanvas()->setMapTool( mMapTools.mSplitParts );
}

void QgisApp::reshapeFeatures()
{
  mapCanvas()->setMapTool( mMapTools.mReshapeFeatures );
}

void QgisApp::addFeature()
{
  mapCanvas()->setMapTool( mMapTools.mAddFeature );
}

void QgisApp::circularStringCurvePoint()
{
  mapCanvas()->setMapTool( mMapTools.mCircularStringCurvePoint );
}

void QgisApp::circularStringRadius()
{
  mapCanvas()->setMapTool( mMapTools.mCircularStringRadius );
}

void QgisApp::selectFeatures()
{
  mapCanvas()->setMapTool( mMapTools.mSelectFeatures );
}

void QgisApp::selectByPolygon()
{
  mapCanvas()->setMapTool( mMapTools.mSelectPolygon );
}

void QgisApp::selectByFreehand()
{
  mapCanvas()->setMapTool( mMapTools.mSelectFreehand );
}

void QgisApp::selectByRadius()
{
  mapCanvas()->setMapTool( mMapTools.mSelectRadius );
}

void QgisApp::deselectAll()
{
  // Turn off rendering to improve speed.
  bool renderFlagState = mapCanvas()->renderFlag();
  if ( renderFlagState )
    mapCanvas()->setRenderFlag( false );

  QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
  for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
  {
    QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() );
    if ( !vl )
      continue;

    vl->removeSelection();
  }

  // Turn on rendering (if it was on previously)
  if ( renderFlagState )
    mapCanvas()->setRenderFlag( true );
}

void QgisApp::selectByExpression()
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( mapCanvas()->currentLayer() );
  if ( !vlayer )
  {
    messageBar()->pushMessage(
      tr( "No active vector layer" ),
      tr( "To select features, choose a vector layer in the legend" ),
      QgsMessageBar::INFO,
      messageTimeout() );
    return;
  }

  QgsExpressionSelectionDialog* dlg = new QgsExpressionSelectionDialog( vlayer );
  dlg->setAttribute( Qt::WA_DeleteOnClose );
  dlg->show();
}

void QgisApp::addRing()
{
  mapCanvas()->setMapTool( mMapTools.mAddRing );
}

void QgisApp::fillRing()
{
  mapCanvas()->setMapTool( mMapTools.mFillRing );
}


void QgisApp::addPart()
{
  mapCanvas()->setMapTool( mMapTools.mAddPart );
}


void QgisApp::editCut( QgsMapLayer * layerContainingSelection )
{
  // Test for feature support in this layer
  QgsVectorLayer* selectionVectorLayer = qobject_cast<QgsVectorLayer *>( layerContainingSelection ? layerContainingSelection : activeLayer() );
  if ( !selectionVectorLayer )
    return;

  clipboard()->replaceWithCopyOf( selectionVectorLayer );

  selectionVectorLayer->beginEditCommand( tr( "Features cut" ) );
  selectionVectorLayer->deleteSelectedFeatures();
  selectionVectorLayer->endEditCommand();
}

void QgisApp::editCopy( QgsMapLayer * layerContainingSelection )
{
  QgsVectorLayer* selectionVectorLayer = qobject_cast<QgsVectorLayer *>( layerContainingSelection ? layerContainingSelection : activeLayer() );
  if ( !selectionVectorLayer )
    return;

  // Test for feature support in this layer
  clipboard()->replaceWithCopyOf( selectionVectorLayer );
}

void QgisApp::clipboardChanged()
{
  activateDeactivateLayerRelatedActions( activeLayer() );
}

void QgisApp::editPaste( QgsMapLayer *destinationLayer )
{
  QgsVectorLayer *pasteVectorLayer = qobject_cast<QgsVectorLayer *>( destinationLayer ? destinationLayer : activeLayer() );
  if ( !pasteVectorLayer )
    return;

  pasteVectorLayer->beginEditCommand( tr( "Features pasted" ) );
  QgsFeatureList features;
  if ( mapCanvas()->mapSettings().hasCrsTransformEnabled() )
  {
    features = clipboard()->transformedCopyOf( pasteVectorLayer->crs(), pasteVectorLayer->pendingFields() );
  }
  else
  {
    features = clipboard()->copyOf( pasteVectorLayer->pendingFields() );
  }
  int nTotalFeatures = features.count();

  QHash<int, int> remap;
  const QgsFields &fields = clipboard()->fields();
  QgsAttributeList pkAttrList = pasteVectorLayer->pendingPkAttributesList();
  for ( int idx = 0; idx < fields.count(); ++idx )
  {
    int dst = pasteVectorLayer->fieldNameIndex( fields[idx].name() );
    if ( dst < 0 )
      continue;

    remap.insert( idx, dst );
  }

  int dstAttrCount = pasteVectorLayer->pendingFields().count();

  QgsFeatureList::iterator featureIt = features.begin();
  while ( featureIt != features.end() )
  {
    const QgsAttributes &srcAttr = featureIt->attributes();
    QgsAttributes dstAttr( dstAttrCount );

    for ( int src = 0; src < srcAttr.count(); ++src )
    {
      int dst = remap.value( src, -1 );
      if ( dst < 0 )
        continue;

      // use default value for primary key fields if it's NOT NULL
      if ( pkAttrList.contains( dst ) )
      {
        dstAttr[ dst ] = pasteVectorLayer->dataProvider()->defaultValue( dst );
        if ( !dstAttr[ dst ].isNull() )
          continue;
        else if ( pasteVectorLayer->providerType() == "spatialite" )
          continue;
      }

      dstAttr[ dst ] = srcAttr[ src ];
    }

    featureIt->setAttributes( dstAttr );

    if ( featureIt->geometry() )
    {
      // convert geometry to match destination layer
      QGis::GeometryType destType = pasteVectorLayer->geometryType();
      bool destIsMulti = QGis::isMultiType( pasteVectorLayer->wkbType() );
      if ( pasteVectorLayer->dataProvider() &&
           !pasteVectorLayer->dataProvider()->doesStrictFeatureTypeCheck() )
      {
        // force destination to multi if provider doesn't do a feature strict check
        destIsMulti = true;
      }

      if ( destType != QGis::UnknownGeometry )
      {
        QgsGeometry* newGeometry = featureIt->geometry()->convertToType( destType, destIsMulti );
        if ( !newGeometry )
        {
          featureIt = features.erase( featureIt );
          continue;
        }
        featureIt->setGeometry( newGeometry );
      }
      // avoid intersection if enabled in digitize settings
      featureIt->geometry()->avoidIntersections();
    }

    ++featureIt;
  }

  if ( pasteVectorLayer->type() == QgsMapLayer::RedliningLayer )
  {
    static_cast<QgsRedliningLayer*>( pasteVectorLayer )->pasteFeatures( features );
  }
  else
  {
    pasteVectorLayer->addFeatures( features );
  }
  pasteVectorLayer->endEditCommand();

  int nCopiedFeatures = features.count();
  if ( nCopiedFeatures == 0 )
  {
    messageBar()->pushMessage( tr( "Paste features" ),
                               tr( "no features could be successfully pasted." ),
                               QgsMessageBar::WARNING, messageTimeout() );

  }
  else if ( nCopiedFeatures == nTotalFeatures )
  {
    messageBar()->pushMessage( tr( "Paste features" ),
                               tr( "%1 features were successfully pasted." ).arg( nCopiedFeatures ),
                               QgsMessageBar::INFO, messageTimeout() );
  }
  else
  {
    messageBar()->pushMessage( tr( "Paste features" ),
                               tr( "%1 of %2 features could be successfully pasted." ).arg( nCopiedFeatures ).arg( nTotalFeatures ),
                               QgsMessageBar::WARNING, messageTimeout() );
  }

  mapCanvas()->refresh();
}

bool QgisApp::editCanPaste()
{
  return clipboard()->hasFeatures();
}

void QgisApp::pasteAsNewVector()
{

  QgsVectorLayer *layer = pasteToNewMemoryVector();
  if ( !layer )
    return;

  saveAsVectorFileGeneral( layer, false );

  delete layer;
}

QgsVectorLayer *QgisApp::pasteAsNewMemoryVector( const QString & theLayerName )
{

  QString layerName = theLayerName;

  if ( layerName.isEmpty() )
  {
    bool ok;
    QString defaultName = tr( "Pasted" );
    layerName = QInputDialog::getText( this, tr( "New memory layer name" ),
                                       tr( "Layer name" ), QLineEdit::Normal,
                                       defaultName, &ok );
    if ( !ok )
      return 0;

    if ( layerName.isEmpty() )
    {
      layerName = defaultName;
    }
  }

  QgsVectorLayer *layer = pasteToNewMemoryVector();
  if ( !layer )
    return 0;

  layer->setLayerName( layerName );

  mapCanvas()->freeze();

  QgsMapLayerRegistry::instance()->addMapLayer( layer );

  mapCanvas()->freeze( false );
  mapCanvas()->refresh();

  return layer;
}

QgsVectorLayer *QgisApp::pasteToNewMemoryVector()
{
  // Decide geometry type from features, switch to multi type if at least one multi is found
  QMap<QGis::WkbType, int> typeCounts;
  QgsFeatureList features = clipboard()->copyOf();
  for ( int i = 0; i < features.size(); i++ )
  {
    QgsFeature &feature = features[i];
    if ( !feature.geometry() )
      continue;

    QGis::WkbType type = QGis::flatType( feature.geometry()->wkbType() );

    if ( type == QGis::WKBUnknown || type == QGis::WKBNoGeometry )
      continue;

    if ( QGis::isSingleType( type ) )
    {
      if ( typeCounts.contains( QGis::multiType( type ) ) )
      {
        typeCounts[ QGis::multiType( type )] = typeCounts[ QGis::multiType( type )] + 1;
      }
      else
      {
        typeCounts[ type ] = typeCounts[ type ] + 1;
      }
    }
    else if ( QGis::isMultiType( type ) )
    {
      if ( typeCounts.contains( QGis::singleType( type ) ) )
      {
        // switch to multi type
        typeCounts[type] = typeCounts[ QGis::singleType( type )];
        typeCounts.remove( QGis::singleType( type ) );
      }
      typeCounts[type] = typeCounts[type] + 1;
    }
  }

  QGis::WkbType wkbType = typeCounts.size() > 0 ? typeCounts.keys().value( 0 ) : QGis::WKBPoint;

  QString typeName = QString( QGis::featureType( wkbType ) ).replace( "WKB", "" );

  typeName += QString( "?memoryid=%1" ).arg( QUuid::createUuid().toString() );

  QgsDebugMsg( QString( "output wkbType = %1 typeName = %2" ).arg( wkbType ).arg( typeName ) );

  QString message;

  if ( features.size() == 0 )
  {
    message = tr( "No features in clipboard." ); // should not happen
  }
  else if ( typeCounts.size() == 0 )
  {
    message = tr( "No features with geometry found, point type layer will be created." );
  }
  else if ( typeCounts.size() > 1 )
  {
    message = tr( "Multiple geometry types found, features with geometry different from %1 will be created without geometry." ).arg( typeName );
  }

  if ( !message.isEmpty() )
  {
    QMessageBox::warning( this, tr( "Warning" ), message, QMessageBox::Ok );
    return 0;
  }

  QgsVectorLayer *layer = new QgsVectorLayer( typeName, "pasted_features", "memory" );

  if ( !layer->isValid() || !layer->dataProvider() )
  {
    delete layer;
    QMessageBox::warning( this, tr( "Warning" ), tr( "Cannot create new layer" ), QMessageBox::Ok );
    return 0;
  }

  layer->startEditing();
  layer->setCrs( clipboard()->crs(), false );

  foreach ( QgsField f, clipboard()->fields().toList() )
  {
    QgsDebugMsg( QString( "field %1 (%2)" ).arg( f.name() ).arg( QVariant::typeToName( f.type() ) ) );
    if ( !layer->addAttribute( f ) )
    {
      QMessageBox::warning( this, tr( "Warning" ),
                            tr( "Cannot create field %1 (%2,%3)" ).arg( f.name() ).arg( f.typeName() ).arg( QVariant::typeToName( f.type() ) ),
                            QMessageBox::Ok );
      delete layer;
      return 0;
    }
  }

  // Convert to multi if necessary
  for ( int i = 0; i < features.size(); i++ )
  {
    QgsFeature &feature = features[i];
    if ( !feature.geometry() )
      continue;

    QGis::WkbType type = QGis::flatType( feature.geometry()->wkbType() );
    if ( type == QGis::WKBUnknown || type == QGis::WKBNoGeometry )
      continue;

    if ( QGis::singleType( wkbType ) != QGis::singleType( type ) )
    {
      feature.setGeometry( 0 );
    }

    if ( QGis::isMultiType( wkbType ) &&  QGis::isSingleType( type ) )
    {
      feature.geometry()->convertToMultiType();
    }
  }
  if ( ! layer->addFeatures( features ) || !layer->commitChanges() )
  {
    QgsDebugMsg( "Cannot add features or commit changes" );
    delete layer;
    return 0;
  }
  layer->removeSelection();

  QgsDebugMsg( QString( "%1 features pasted to memory layer" ).arg( layer->featureCount() ) );
  return layer;
}

void QgisApp::copyStyle( QgsMapLayer * sourceLayer )
{
  QgsMapLayer *selectionLayer = sourceLayer ? sourceLayer : activeLayer();
  if ( selectionLayer )
  {
    QDomImplementation DomImplementation;
    QDomDocumentType documentType =
      DomImplementation.createDocumentType(
        "qgis", "http://mrcc.com/qgis.dtd", "SYSTEM" );
    QDomDocument doc( documentType );
    QDomElement rootNode = doc.createElement( "qgis" );
    rootNode.setAttribute( "version", QString( "%1" ).arg( QGis::QGIS_VERSION ) );
    doc.appendChild( rootNode );
    QString errorMsg;
    if ( !selectionLayer->writeSymbology( rootNode, doc, errorMsg ) )
    {
      QMessageBox::warning( this,
                            tr( "Error" ),
                            tr( "Cannot copy style: %1" )
                            .arg( errorMsg ),
                            QMessageBox::Ok );
      return;
    }
    // Copies data in text form as well, so the XML can be pasted into a text editor
    clipboard()->setData( QGSCLIPBOARD_STYLE_MIME, doc.toByteArray(), doc.toString() );
  }
}

void QgisApp::pasteStyle( QgsMapLayer * destinationLayer )
{
  QgsMapLayer *selectionLayer = destinationLayer ? destinationLayer : activeLayer();
  if ( selectionLayer )
  {
    if ( clipboard()->hasFormat( QGSCLIPBOARD_STYLE_MIME ) )
    {
      QDomDocument doc( "qgis" );
      QString errorMsg;
      int errorLine, errorColumn;
      if ( !doc.setContent( clipboard()->data( QGSCLIPBOARD_STYLE_MIME ), false, &errorMsg, &errorLine, &errorColumn ) )
      {
        QMessageBox::information( this,
                                  tr( "Error" ),
                                  tr( "Cannot parse style: %1:%2:%3" )
                                  .arg( errorMsg )
                                  .arg( errorLine )
                                  .arg( errorColumn ),
                                  QMessageBox::Ok );
        return;
      }
      QDomElement rootNode = doc.firstChildElement( "qgis" );
      if ( !selectionLayer->readSymbology( rootNode, errorMsg ) )
      {
        QMessageBox::information( this,
                                  tr( "Error" ),
                                  tr( "Cannot read style: %1" )
                                  .arg( errorMsg ),
                                  QMessageBox::Ok );
        return;
      }

      layerTreeView()->refreshLayerSymbology( selectionLayer->id() );
      mapCanvas()->clearCache( selectionLayer->id() );
      mapCanvas()->refresh();
    }
  }
}

void QgisApp::copyFeatures( QgsFeatureStore & featureStore )
{
  clipboard()->replaceWithCopyOf( featureStore );
}

void QgisApp::refreshMapCanvas()
{
  //stop any current rendering
  mapCanvas()->stopRendering();

  //reload cached provider data
  QgsMapLayerRegistry::instance()->reloadAllLayers();

  mapCanvas()->clearCache();
  //then refresh
  mapCanvas()->refresh();
}

void QgisApp::canvasRefreshStarted()
{
  emit progress( -1, 0 ); // trick to make progress bar show busy indicator
}

void QgisApp::canvasRefreshFinished()
{
  emit progress( 0, 0 ); // stop the busy indicator
}

void QgisApp::toggleMapTips()
{
  mMapTipsVisible = !mMapTipsVisible;
  // if off, stop the timer
  if ( !mMapTipsVisible )
  {
    mpMapTipsTimer->stop();
  }
}

void QgisApp::toggleEditing()
{
  QgsVectorLayer *currentLayer = qobject_cast<QgsVectorLayer*>( activeLayer() );
  if ( currentLayer )
  {
    toggleEditing( currentLayer, true );
  }
}

bool QgisApp::toggleEditing( QgsMapLayer *layer, bool allowCancel )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer )
  {
    return false;
  }

  bool res = true;

  if ( !vlayer->isEditable() && !vlayer->isReadOnly() )
  {
    if ( !( vlayer->dataProvider()->capabilities() & QgsVectorDataProvider::EditingCapabilities ) )
    {
      messageBar()->pushMessage( tr( "Start editing failed" ),
                                 tr( "Provider cannot be opened for editing" ),
                                 QgsMessageBar::INFO, messageTimeout() );
      return false;
    }

    vlayer->startEditing();

    QSettings settings;
    QString markerType = settings.value( "/qgis/digitizing/marker_style", "Cross" ).toString();
    bool markSelectedOnly = settings.value( "/qgis/digitizing/marker_only_for_selected", false ).toBool();

    // redraw only if markers will be drawn
    if (( !markSelectedOnly || vlayer->selectedFeatureCount() > 0 ) &&
        ( markerType == "Cross" || markerType == "SemiTransparentCircle" ) )
    {
      vlayer->triggerRepaint();
    }
  }
  else if ( vlayer->isModified() && vlayer->type() == QgsMapLayer::RedliningLayer )
  {
    vlayer->commitChanges();
  }
  else if ( vlayer->isModified() )
  {
    QMessageBox::StandardButtons buttons = QMessageBox::Save | QMessageBox::Discard;
    if ( allowCancel )
      buttons |= QMessageBox::Cancel;

    switch ( QMessageBox::information( 0,
                                       tr( "Stop editing" ),
                                       tr( "Do you want to save the changes to layer %1?" ).arg( vlayer->name() ),
                                       buttons ) )
    {
      case QMessageBox::Cancel:
        res = false;
        break;

      case QMessageBox::Save:
        QApplication::setOverrideCursor( Qt::WaitCursor );

        if ( !vlayer->commitChanges() )
        {
          commitError( vlayer );
          // Leave the in-memory editing state alone,
          // to give the user a chance to enter different values
          // and try the commit again later
          res = false;
        }

        vlayer->triggerRepaint();

        QApplication::restoreOverrideCursor();
        break;

      case QMessageBox::Discard:
        QApplication::setOverrideCursor( Qt::WaitCursor );

        mapCanvas()->freeze( true );
        if ( !vlayer->rollBack() )
        {
          messageBar()->pushMessage( tr( "Error" ),
                                     tr( "Problems during roll back" ),
                                     QgsMessageBar::CRITICAL );
          res = false;
        }
        mapCanvas()->freeze( false );

        vlayer->triggerRepaint();

        QApplication::restoreOverrideCursor();
        break;

      default:
        break;
    }
  }
  else //layer not modified
  {
    mapCanvas()->freeze( true );
    vlayer->rollBack();
    mapCanvas()->freeze( false );
    res = true;
    vlayer->triggerRepaint();
  }

  if ( !res && layer == activeLayer() )
  {
    // while also called when layer sends editingStarted/editingStopped signals,
    // this ensures correct restoring of gui state if toggling was canceled
    // or layer commit/rollback functions failed
    activateDeactivateLayerRelatedActions( layer );
  }
  if ( res )
  {
    emit editingToggled();
  }
  return res;
}

void QgisApp::saveActiveLayerEdits()
{
  saveEdits( activeLayer(), true, true );
}

void QgisApp::saveEdits( QgsMapLayer *layer, bool leaveEditable, bool triggerRepaint )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer || !vlayer->isEditable() || !vlayer->isModified() )
    return;

  if ( vlayer == activeLayer() )
    mSaveRollbackInProgress = true;

  if ( !vlayer->commitChanges() )
  {
    mSaveRollbackInProgress = false;
    commitError( vlayer );
  }

  if ( leaveEditable )
  {
    vlayer->startEditing();
  }
  if ( triggerRepaint )
  {
    vlayer->triggerRepaint();
  }
}

void QgisApp::cancelEdits( QgsMapLayer *layer, bool leaveEditable, bool triggerRepaint )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer || !vlayer->isEditable() )
    return;

  if ( vlayer == activeLayer() && leaveEditable )
    mSaveRollbackInProgress = true;

  mapCanvas()->freeze( true );
  if ( !vlayer->rollBack( !leaveEditable ) )
  {
    mSaveRollbackInProgress = false;
    QMessageBox::information( 0,
                              tr( "Error" ),
                              tr( "Could not %1 changes to layer %2\n\nErrors: %3\n" )
                              .arg( leaveEditable ? tr( "rollback" ) : tr( "cancel" ) )
                              .arg( vlayer->name() )
                              .arg( vlayer->commitErrors().join( "\n  " ) ) );
  }
  mapCanvas()->freeze( false );

  if ( leaveEditable )
  {
    vlayer->startEditing();
  }
  if ( triggerRepaint )
  {
    vlayer->triggerRepaint();
  }
}

void QgisApp::saveEdits()
{
  foreach ( QgsMapLayer * layer, layerTreeView()->selectedLayers() )
  {
    saveEdits( layer, true, false );
  }
  mapCanvas()->refresh();
  activateDeactivateLayerRelatedActions( activeLayer() );
}

void QgisApp::saveAllEdits( bool verifyAction )
{
  if ( verifyAction )
  {
    if ( !verifyEditsActionDialog( tr( "Save" ), tr( "all" ) ) )
      return;
  }

  foreach ( QgsMapLayer * layer, editableLayers( true ) )
  {
    saveEdits( layer, true, false );
  }
  mapCanvas()->refresh();
  activateDeactivateLayerRelatedActions( activeLayer() );
}

void QgisApp::rollbackEdits()
{
  foreach ( QgsMapLayer * layer, layerTreeView()->selectedLayers() )
  {
    cancelEdits( layer, true, false );
  }
  mapCanvas()->refresh();
  activateDeactivateLayerRelatedActions( activeLayer() );
}

void QgisApp::rollbackAllEdits( bool verifyAction )
{
  if ( verifyAction )
  {
    if ( !verifyEditsActionDialog( tr( "Rollback" ), tr( "all" ) ) )
      return;
  }

  foreach ( QgsMapLayer * layer, editableLayers( true ) )
  {
    cancelEdits( layer, true, false );
  }
  mapCanvas()->refresh();
  activateDeactivateLayerRelatedActions( activeLayer() );
}

void QgisApp::cancelEdits()
{
  foreach ( QgsMapLayer * layer, layerTreeView()->selectedLayers() )
  {
    cancelEdits( layer, false, false );
  }
  mapCanvas()->refresh();
  activateDeactivateLayerRelatedActions( activeLayer() );
}

void QgisApp::cancelAllEdits( bool verifyAction )
{
  if ( verifyAction )
  {
    if ( !verifyEditsActionDialog( tr( "Cancel" ), tr( "all" ) ) )
      return;
  }

  foreach ( QgsMapLayer * layer, editableLayers() )
  {
    cancelEdits( layer, false, false );
  }
  mapCanvas()->refresh();
  activateDeactivateLayerRelatedActions( activeLayer() );
}

bool QgisApp::verifyEditsActionDialog( const QString& act, const QString& upon )
{
  bool res = false;
  switch ( QMessageBox::information( 0,
                                     tr( "Current edits" ),
                                     tr( "%1 current changes for %2 layer(s)?" )
                                     .arg( act )
                                     .arg( upon ),
                                     QMessageBox::Cancel | QMessageBox::Ok ) )
  {
    case QMessageBox::Ok:
      res = true;
      break;
    default:
      break;
  }
  return res;
}

QList<QgsMapLayer *> QgisApp::editableLayers( bool modified ) const
{
  QList<QgsMapLayer*> editLayers;
  // use legend layers (instead of registry) so QList mirrors its order
  foreach ( QgsLayerTreeLayer* nodeLayer, layerTreeView()->layerTreeModel()->rootGroup()->findLayers() )
  {
    if ( !nodeLayer->layer() )
      continue;

    QgsVectorLayer *vl = qobject_cast<QgsVectorLayer*>( nodeLayer->layer() );
    if ( !vl )
      continue;

    if ( vl->isEditable() && ( !modified || ( modified && vl->isModified() ) ) )
      editLayers << vl;
  }
  return editLayers;
}

void QgisApp::layerSubsetString()
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !vlayer )
    return;

  // launch the query builder
  QgsQueryBuilder *qb = new QgsQueryBuilder( vlayer, this );
  QString subsetBefore = vlayer->subsetString();

  // Set the sql in the query builder to the same in the prop dialog
  // (in case the user has already changed it)
  qb->setSql( vlayer->subsetString() );
  // Open the query builder
  if ( qb->exec() )
  {
    if ( subsetBefore != qb->sql() )
    {
      mapCanvas()->refresh();
      if ( layerTreeView() )
      {
        layerTreeView()->refreshLayerSymbology( vlayer->id() );
      }
    }
  }

  // delete the query builder object
  delete qb;
}

void QgisApp::dizzy()
{
  // constants should go to options so that people can customize them to their taste
  int d = 10; // max. translational dizziness offset
  int r = 4;  // max. rotational dizzines angle
  QRectF rect = mapCanvas()->sceneRect();
  if ( rect.x() < -d || rect.x() > d || rect.y() < -d || rect.y() > d )
    return; // do not affect panning
  rect.moveTo(( qrand() % ( 2 * d ) ) - d, ( qrand() % ( 2 * d ) ) - d );
  mapCanvas()->setSceneRect( rect );
  QTransform matrix;
  matrix.rotate(( qrand() % ( 2 * r ) ) - r );
  mapCanvas()->setTransform( matrix );
}

void QgisApp::showCanvasContextMenu( QPoint screenPos, QgsPoint mapPos )
{
  QgsMapCanvasContextMenu( mapPos ).exec( screenPos );
}

void QgisApp::handleFeaturePicked( const QgsFeature &feature, QgsVectorLayer *layer )
{
  if ( mRedlining && mRedlining->getLayer() && layer == mRedlining->getLayer() )
  {
    mRedlining->editFeature( feature );
  }
  else if ( mGpsRouteEditor && mGpsRouteEditor->getLayer() && layer == mGpsRouteEditor->getLayer() )
  {
    mGpsRouteEditor->editFeature( feature );
  }
}

void QgisApp::handleLabelPicked( const QgsLabelPosition &labelPos )
{
  if ( mRedlining && mRedlining->getLayer() && mRedlining->getLayer()->id() == labelPos.layerID )
  {
    mRedlining->editLabel( labelPos );
  }
}

// toggle overview status
void QgisApp::isInOverview()
{
  layerTreeView()->defaultActions()->showInOverview();
}

void QgisApp::removingLayers( QStringList theLayers )
{
  foreach ( const QString &layerId, theLayers )
  {
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>(
                               QgsMapLayerRegistry::instance()->mapLayer( layerId ) );
    if ( !vlayer || !vlayer->isEditable() )
      return;

    toggleEditing( vlayer, false );
  }
}

void QgisApp::removeAllLayers()
{
  QgsMapLayerRegistry::instance()->removeAllMapLayers();
}

void QgisApp::removeLayer()
{
  if ( !layerTreeView() )
  {
    return;
  }

  foreach ( QgsMapLayer * layer, layerTreeView()->selectedLayers() )
  {
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>( layer );
    if ( vlayer && vlayer->isEditable() && !toggleEditing( vlayer, true ) )
      return;
  }

  QList<QgsLayerTreeNode*> selectedNodes = layerTreeView()->selectedNodes( true );

  //validate selection
  if ( selectedNodes.isEmpty() )
  {
    messageBar()->pushMessage( tr( "No legend entries selected" ),
                               tr( "Select the layers and groups you want to remove in the legend." ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  bool promptConfirmation = QSettings().value( "qgis/askToDeleteLayers", true ).toBool();
  //display a warning
  if ( promptConfirmation && QMessageBox::warning( this, tr( "Remove layers and groups" ), tr( "Remove %n legend entries?", "number of legend items to remove", selectedNodes.count() ), QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
  {
    return;
  }

  foreach ( QgsLayerTreeNode* node, selectedNodes )
  {
    QgsLayerTreeGroup* parentGroup = qobject_cast<QgsLayerTreeGroup*>( node->parent() );
    if ( parentGroup )
      parentGroup->removeChildNode( node );
  }

  showStatusMessage( tr( "%n legend entries removed.", "number of removed legend entries", selectedNodes.count() ) );

  mapCanvas()->refresh();
}

void QgisApp::duplicateLayers( QList<QgsMapLayer *> lyrList )
{
  if ( !layerTreeView() )
  {
    return;
  }

  const QList<QgsMapLayer *> selectedLyrs = lyrList.empty() ? layerTreeView()->selectedLayers() : lyrList;
  if ( selectedLyrs.empty() )
  {
    return;
  }

  mapCanvas()->freeze();
  QgsMapLayer *dupLayer;
  QString layerDupName, unSppType;
  QList<QgsMessageBarItem *> msgBars;

  foreach ( QgsMapLayer * selectedLyr, selectedLyrs )
  {
    dupLayer = 0;
    unSppType = QString( "" );
    layerDupName = selectedLyr->name() + " " + tr( "copy" );

    if ( selectedLyr->type() == QgsMapLayer::PluginLayer )
    {
      unSppType = tr( "Plugin layer" );
    }

    // duplicate the layer's basic parameters

    if ( unSppType.isEmpty() )
    {
      QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>( selectedLyr );
      // TODO: add other layer types that can be duplicated
      // currently memory and plugin layers are skipped
      if ( vlayer && vlayer->storageType() == "Memory storage" )
      {
        unSppType = tr( "Memory layer" );
      }
      else if ( vlayer )
      {
        QgsVectorLayer *dupVLayer = new QgsVectorLayer( vlayer->source(), layerDupName, vlayer->providerType() );
        if ( vlayer->dataProvider() )
        {
          dupVLayer->setProviderEncoding( vlayer->dataProvider()->encoding() );
        }
        dupLayer = dupVLayer;
      }
    }

    if ( unSppType.isEmpty() && !dupLayer )
    {
      QgsRasterLayer *rlayer = qobject_cast<QgsRasterLayer*>( selectedLyr );
      if ( rlayer )
      {
        dupLayer = new QgsRasterLayer( rlayer->source(), layerDupName );
      }
    }

    if ( unSppType.isEmpty() && dupLayer && !dupLayer->isValid() )
    {
      msgBars.append( new QgsMessageBarItem(
                        tr( "Duplicate layer: " ),
                        tr( "%1 (duplication resulted in invalid layer)" ).arg( selectedLyr->name() ),
                        QgsMessageBar::WARNING,
                        0,
                        messageBar() ) );
      continue;
    }

    if ( !unSppType.isEmpty() || !dupLayer )
    {
      msgBars.append( new QgsMessageBarItem(
                        tr( "Duplicate layer: " ),
                        tr( "%1 (%2 type unsupported)" )
                        .arg( selectedLyr->name() )
                        .arg( !unSppType.isEmpty() ? QString( "'" ) + unSppType + "' " : "" ),
                        QgsMessageBar::WARNING,
                        0,
                        messageBar() ) );
      continue;
    }

    // add layer to layer registry and legend
    QList<QgsMapLayer *> myList;
    myList << dupLayer;
    QgsProject::instance()->layerTreeRegistryBridge()->setEnabled( false );
    QgsMapLayerRegistry::instance()->addMapLayers( myList );
    QgsProject::instance()->layerTreeRegistryBridge()->setEnabled( true );

    QgsLayerTreeLayer* nodeSelectedLyr = layerTreeView()->layerTreeModel()->rootGroup()->findLayer( selectedLyr->id() );
    Q_ASSERT( nodeSelectedLyr );
    Q_ASSERT( QgsLayerTree::isGroup( nodeSelectedLyr->parent() ) );
    QgsLayerTreeGroup* parentGroup = QgsLayerTree::toGroup( nodeSelectedLyr->parent() );

    QgsLayerTreeLayer* nodeDupLayer = parentGroup->insertLayer( parentGroup->children().indexOf( nodeSelectedLyr ) + 1, dupLayer );

    // always set duplicated layers to not visible so layer can be configured before being turned on
    nodeDupLayer->setVisible( Qt::Unchecked );

    // duplicate the layer style
    copyStyle( selectedLyr );
    pasteStyle( dupLayer );

    QgsVectorLayer* vLayer = dynamic_cast<QgsVectorLayer*>( selectedLyr );
    QgsVectorLayer* vDupLayer = dynamic_cast<QgsVectorLayer*>( dupLayer );
    if ( vLayer && vDupLayer )
    {
      foreach ( const QgsVectorJoinInfo join, vLayer->vectorJoins() )
      {
        vDupLayer->addJoin( join );
      }
    }
  }

  dupLayer = 0;

  mapCanvas()->freeze( false );

  // display errors in message bar after duplication of layers
  foreach ( QgsMessageBarItem * msgBar, msgBars )
  {
    messageBar()->pushItem( msgBar );
  }
}

void QgisApp::setLayerScaleVisibility()
{
  if ( !layerTreeView() )
    return;

  QList<QgsMapLayer*> layers = layerTreeView()->selectedLayers();

  if ( layers.length() < 1 )
    return;

  QgsScaleVisibilityDialog* dlg = new QgsScaleVisibilityDialog( this, tr( "Set scale visibility for selected layers" ), mapCanvas() );
  QgsMapLayer* layer = layerTreeView()->currentLayer();
  if ( layer )
  {
    dlg->setScaleVisiblity( layer->hasScaleBasedVisibility() );
    dlg->setMinimumScale( 1.0 / layer->maximumScale() );
    dlg->setMaximumScale( 1.0 / layer->minimumScale() );
  }
  if ( dlg->exec() )
  {
    mapCanvas()->freeze();
    foreach ( QgsMapLayer* layer, layers )
    {
      layer->setScaleBasedVisibility( dlg->hasScaleVisibility() );
      layer->setMinimumScale( 1.0 / dlg->maximumScale() );
      layer->setMaximumScale( 1.0 / dlg->minimumScale() );
    }
    mapCanvas()->freeze( false );
    mapCanvas()->refresh();
  }
  delete dlg;
}

void QgisApp::setLayerCRS()
{
  if ( !( layerTreeView() && layerTreeView()->currentLayer() ) )
  {
    return;
  }

  QgsGenericProjectionSelector mySelector( this );
  mySelector.setSelectedCrsId( layerTreeView()->currentLayer()->crs().srsid() );
  mySelector.setMessage();
  if ( !mySelector.exec() )
  {
    QApplication::restoreOverrideCursor();
    return;
  }

  QgsCoordinateReferenceSystem crs( mySelector.selectedCrsId(), QgsCoordinateReferenceSystem::InternalCrsId );

  foreach ( QgsLayerTreeNode* node, layerTreeView()->selectedNodes() )
  {
    if ( QgsLayerTree::isGroup( node ) )
    {
      foreach ( QgsLayerTreeLayer* child, QgsLayerTree::toGroup( node )->findLayers() )
      {
        if ( child->layer() )
        {
          child->layer()->setCrs( crs );
          child->layer()->triggerRepaint();
        }
      }
    }
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer* nodeLayer = QgsLayerTree::toLayer( node );
      if ( nodeLayer->layer() )
      {
        nodeLayer->layer()->setCrs( crs );
        nodeLayer->layer()->triggerRepaint();
      }
    }
  }

  mapCanvas()->refresh();
}

void QgisApp::setProjectCRSFromLayer()
{
  if ( !( layerTreeView() && layerTreeView()->currentLayer() ) )
  {
    return;
  }

  QgsCoordinateReferenceSystem crs = layerTreeView()->currentLayer()->crs();
  mapCanvas()->freeze();
  mapCanvas()->setDestinationCrs( crs );
  if ( crs.mapUnits() != QGis::UnknownUnit )
  {
    mapCanvas()->setMapUnits( crs.mapUnits() );
  }
  mapCanvas()->freeze( false );
  mapCanvas()->refresh();
}


void QgisApp::legendLayerZoomNative()
{
  if ( !layerTreeView() )
    return;

  //find current Layer
  QgsMapLayer* currentLayer = layerTreeView()->currentLayer();
  if ( !currentLayer )
    return;

  QgsRasterLayer *layer =  qobject_cast<QgsRasterLayer *>( currentLayer );
  if ( layer )
  {
    QgsDebugMsg( "Raster units per pixel  : " + QString::number( layer->rasterUnitsPerPixelX() ) );
    QgsDebugMsg( "MapUnitsPerPixel before : " + QString::number( mapCanvas()->mapUnitsPerPixel() ) );

    if ( mapCanvas()->hasCrsTransformEnabled() )
    {
      // get legth of central canvas pixel width in source raster crs
      QgsRectangle e = mapCanvas()->extent();
      QSize s = mapCanvas()->mapSettings().outputSize();
      QgsPoint p1( e.center().x(), e.center().y() );
      QgsPoint p2( e.center().x() + e.width() / s.width(), e.center().y() + e.height() / s.height() );
      QgsCoordinateTransform ct( mapCanvas()->mapSettings().destinationCrs(), layer->crs() );
      p1 = ct.transform( p1 );
      p2 = ct.transform( p2 );
      double width = sqrt( p1.sqrDist( p2 ) ); // width of reprojected pixel
      // This is not perfect of course, we use the resolution in just one direction
      mapCanvas()->zoomByFactor( qAbs( layer->rasterUnitsPerPixelX() / width ) );
    }
    else
    {
      mapCanvas()->zoomByFactor( qAbs( layer->rasterUnitsPerPixelX() / mapCanvas()->mapUnitsPerPixel() ) );
    }
    mapCanvas()->refresh();
    QgsDebugMsg( "MapUnitsPerPixel after  : " + QString::number( mapCanvas()->mapUnitsPerPixel() ) );
  }
}

void QgisApp::legendLayerStretchUsingCurrentExtent()
{
  if ( !layerTreeView() )
    return;

  //find current Layer
  QgsMapLayer* currentLayer = layerTreeView()->currentLayer();
  if ( !currentLayer )
    return;

  QgsRasterLayer *layer =  qobject_cast<QgsRasterLayer *>( currentLayer );
  if ( layer )
  {
    QgsContrastEnhancement::ContrastEnhancementAlgorithm contrastEnhancementAlgorithm = QgsContrastEnhancement::StretchToMinimumMaximum;

    QgsRectangle myRectangle;
    myRectangle = mapCanvas()->mapSettings().outputExtentToLayerExtent( layer, mapCanvas()->extent() );
    layer->setContrastEnhancement( contrastEnhancementAlgorithm, QgsRaster::ContrastEnhancementMinMax, myRectangle );

    layerTreeView()->refreshLayerSymbology( layer->id() );
    mapCanvas()->refresh();
  }
}



void QgisApp::legendGroupSetCRS()
{
  if ( !mapCanvas() )
  {
    return;
  }

  QgsLayerTreeGroup* currentGroup = layerTreeView()->currentGroupNode();
  if ( !currentGroup )
    return;

  QgsGenericProjectionSelector mySelector( this );
  mySelector.setMessage();
  if ( !mySelector.exec() )
  {
    QApplication::restoreOverrideCursor();
    return;
  }

  QgsCoordinateReferenceSystem crs( mySelector.selectedCrsId(), QgsCoordinateReferenceSystem::InternalCrsId );
  foreach ( QgsLayerTreeLayer* nodeLayer, currentGroup->findLayers() )
  {
    if ( nodeLayer->layer() )
    {
      nodeLayer->layer()->setCrs( crs );
      nodeLayer->layer()->triggerRepaint();
    }
  }
}

void QgisApp::zoomToLayerExtent()
{
  layerTreeView()->defaultActions()->zoomToLayer( mapCanvas() );
}

void QgisApp::showPluginManager()
{

  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    // Call pluginManagerInterface()->showPluginManager() as soon as the plugin installer says the remote data is fetched.
    QgsPythonRunner::run( "pyplugin_installer.instance().showPluginManagerWhenReady()" );
  }
  else
  {
    // Call the pluginManagerInterface directly
    mQgisInterface->pluginManagerInterface()->showPluginManager();
  }
}

// implementation of the python runner
class QgsPythonRunnerImpl : public QgsPythonRunner
{
  public:
    QgsPythonRunnerImpl( QgsPythonUtils* pythonUtils ) : mPythonUtils( pythonUtils ) {}

    virtual bool runCommand( QString command, QString messageOnError = QString() ) override
    {
      if ( mPythonUtils && mPythonUtils->isEnabled() )
      {
        return mPythonUtils->runString( command, messageOnError, false );
      }
      return false;
    }

    virtual bool evalCommand( QString command, QString &result ) override
    {
      if ( mPythonUtils && mPythonUtils->isEnabled() )
      {
        return mPythonUtils->evalString( command, result );
      }
      return false;
    }

  protected:
    QgsPythonUtils* mPythonUtils;
};

void QgisApp::loadPythonSupport()
{
  QString pythonlibName( "qgispython" );
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
  pythonlibName.prepend( QgsApplication::libraryPath() );
#endif
#ifdef __MINGW32__
  pythonlibName.prepend( "lib" );
#endif
  QString version = QString( "%1.%2.%3" ).arg( QGis::QGIS_VERSION_INT / 10000 ).arg( QGis::QGIS_VERSION_INT / 100 % 100 ).arg( QGis::QGIS_VERSION_INT % 100 );
  QgsDebugMsg( QString( "load library %1 (%2)" ).arg( pythonlibName ).arg( version ) );
  QLibrary pythonlib( pythonlibName, version );
  // It's necessary to set these two load hints, otherwise Python library won't work correctly
  // see http://lists.kde.org/?l=pykde&m=117190116820758&w=2
  pythonlib.setLoadHints( QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint );
  if ( !pythonlib.load() )
  {
    pythonlib.setFileName( pythonlibName );
    if ( !pythonlib.load() )
    {
      QgsMessageLog::logMessage( tr( "Couldn't load Python support library: %1" ).arg( pythonlib.errorString() ) );
      return;
    }
  }

  //QgsDebugMsg("Python support library loaded successfully.");
  typedef QgsPythonUtils*( *inst )();
  inst pythonlib_inst = ( inst ) cast_to_fptr( pythonlib.resolve( "instance" ) );
  if ( !pythonlib_inst )
  {
    //using stderr on purpose because we want end users to see this [TS]
    QgsMessageLog::logMessage( tr( "Couldn't resolve python support library's instance() symbol." ) );
    return;
  }

  //QgsDebugMsg("Python support library's instance() symbol resolved.");
  mPythonUtils = pythonlib_inst();
  if ( mPythonUtils )
  {
    mPythonUtils->initPython( mQgisInterface );
  }

  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    QgsPluginRegistry::instance()->setPythonUtils( mPythonUtils );

    // init python runner
    QgsPythonRunner::setInstance( new QgsPythonRunnerImpl( mPythonUtils ) );

    QgsMessageLog::logMessage( tr( "Python support ENABLED :-) " ), QString::null, QgsMessageLog::INFO );
  }
}

void QgisApp::checkQgisVersion()
{
  QApplication::setOverrideCursor( Qt::WaitCursor );

  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( QNetworkRequest( QUrl( "http://qgis.org/version.txt" ) ) );
  connect( reply, SIGNAL( finished() ), this, SLOT( versionReplyFinished() ) );
}

void QgisApp::versionReplyFinished()
{
  QApplication::restoreOverrideCursor();

  QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );
  if ( !reply )
    return;

  QNetworkReply::NetworkError error = reply->error();

  if ( error == QNetworkReply::NoError )
  {
    QString versionMessage = reply->readAll();
    QgsDebugMsg( QString( "version message: %1" ).arg( versionMessage ) );

    // strip the header
    QString contentFlag = "#QGIS Version";
    int pos = versionMessage.indexOf( contentFlag );
    if ( pos > -1 )
    {
      pos += contentFlag.length();
      QgsDebugMsg( QString( "Pos is %1" ).arg( pos ) );

      versionMessage = versionMessage.mid( pos );
      QStringList parts = versionMessage.split( "|", QString::SkipEmptyParts );
      // check the version from the  server against our version
      QString versionInfo;
      int currentVersion = parts[0].toInt();
      if ( currentVersion > QGis::QGIS_VERSION_INT )
      {
        // show version message from server
        versionInfo = tr( "There is a new version of QGIS available" ) + "\n";
      }
      else if ( QGis::QGIS_VERSION_INT > currentVersion )
      {
        versionInfo = tr( "You are running a development version of QGIS" ) + "\n";
      }
      else
      {
        versionInfo = tr( "You are running the current version of QGIS" ) + "\n";
      }

      if ( parts.count() > 1 )
      {
        versionInfo += parts[1] + "\n\n" + tr( "Would you like more information?" );

        QMessageBox::StandardButton result = QMessageBox::information( this,
                                             tr( "QGIS Version Information" ), versionInfo, QMessageBox::Ok |
                                             QMessageBox::Cancel );
        if ( result == QMessageBox::Ok )
        {
          // show more info
          QgsMessageViewer *mv = new QgsMessageViewer( this );
          mv->setWindowTitle( tr( "QGIS - Changes since last release" ) );
          mv->setMessageAsHtml( parts[2] );
          mv->exec();
        }
      }
      else
      {
        QMessageBox::information( this, tr( "QGIS Version Information" ), versionInfo );
      }
    }
    else
    {
      QMessageBox::warning( this, tr( "QGIS Version Information" ), tr( "Unable to get current version information from server" ) );
    }
  }
  else
  {
    // get error type
    QString detail;
    switch ( error )
    {
      case QNetworkReply::ConnectionRefusedError:
        detail = tr( "Connection refused - server may be down" );
        break;
      case QNetworkReply::HostNotFoundError:
        detail = tr( "QGIS server was not found" );
        break;
      default:
        detail = tr( "Unknown network socket error: %1" ).arg( error );
        break;
    }

    // show version message from server
    QMessageBox::critical( this, tr( "QGIS Version Information" ), tr( "Unable to communicate with QGIS Version server\n%1" ).arg( detail ) );
  }

  reply->deleteLater();
}

void QgisApp::configureShortcuts()
{
  QgsConfigureShortcutsDialog dlg;
  dlg.exec();
}

void QgisApp::customize()
{
  QgsCustomization::instance()->openDialog( this );
}

void QgisApp::options()
{
  showOptionsDialog( this );
}

void QgisApp::showOptionsDialog( QWidget *parent, QString currentPage )
{
  QSettings mySettings;
  QString oldScales = mySettings.value( "Map/scales", PROJECT_SCALES ).toString();

  bool oldCapitalise = mySettings.value( "/qgis/capitaliseLayerName", QVariant( false ) ).toBool();

  QgsOptions *optionsDialog = new QgsOptions( parent );
  if ( !currentPage.isEmpty() )
  {
    optionsDialog->setCurrentPage( currentPage );
  }

  if ( optionsDialog->exec() )
  {
    // set the theme if it changed
    setTheme( optionsDialog->theme() );

    QgsProject::instance()->layerTreeRegistryBridge()->setNewLayersVisible( mySettings.value( "/qgis/new_layers_visible", true ).toBool() );

    setupLayerTreeViewFromSettings();

    mapCanvas()->enableAntiAliasing( mySettings.value( "/qgis/enable_anti_aliasing" ).toBool() );

    int action = mySettings.value( "/qgis/wheel_action", 2 ).toInt();
    double zoomFactor = mySettings.value( "/qgis/zoom_factor", 2 ).toDouble();
    mapCanvas()->setWheelAction(( QgsMapCanvas::WheelAction ) action, zoomFactor );

    mapCanvas()->setCachingEnabled( mySettings.value( "/qgis/enable_render_caching", true ).toBool() );

    mapCanvas()->setParallelRenderingEnabled( mySettings.value( "/qgis/parallel_rendering", false ).toBool() );

    mapCanvas()->setMapUpdateInterval( mySettings.value( "/qgis/map_update_interval", 250 ).toInt() );

    if ( oldCapitalise != mySettings.value( "/qgis/capitaliseLayerName", QVariant( false ) ).toBool() )
    {
      // if the layer capitalization has changed, we need to update all layer names
      foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers() )
        layer->setLayerName( layer->originalName() );
    }

    //update any open compositions so they reflect new composer settings
    //we have to push the changes to the compositions here, because compositions
    //have no access to qgisapp and accordingly can't listen in to changes
    QSet<QgsComposer*> composers = instance()->printComposers();
    QSet<QgsComposer*>::iterator composer_it = composers.begin();
    for ( ; composer_it != composers.end(); ++composer_it )
    {
      QgsComposition* composition = ( *composer_it )->composition();
      composition->updateSettings();
    }

    //do we need this? TS
    mapCanvas()->refresh();

    mRasterFileFilter = QgsProviderRegistry::instance()->fileRasterFilters();

    if ( oldScales != mySettings.value( "Map/scales", PROJECT_SCALES ).toString() )
    {
      emit projectScalesChanged( QStringList() );
    }

    bool otfTransformAutoEnable = mySettings.value( "/Projections/otfTransformAutoEnable", true ).toBool();
    mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );
  }

  delete optionsDialog;
}

void QgisApp::fullHistogramStretch()
{
  histogramStretch( false, QgsRaster::ContrastEnhancementMinMax );
}

void QgisApp::localHistogramStretch()
{
  histogramStretch( true, QgsRaster::ContrastEnhancementMinMax );
}

void QgisApp::fullCumulativeCutStretch()
{
  histogramStretch( false, QgsRaster::ContrastEnhancementCumulativeCut );
}

void QgisApp::localCumulativeCutStretch()
{
  histogramStretch( true, QgsRaster::ContrastEnhancementCumulativeCut );
}

void QgisApp::histogramStretch( bool visibleAreaOnly, QgsRaster::ContrastEnhancementLimits theLimits )
{
  QgsMapLayer * myLayer = layerTreeView()->currentLayer();

  if ( !myLayer )
  {
    messageBar()->pushMessage( tr( "No Layer Selected" ),
                               tr( "To perform a full histogram stretch, you need to have a raster layer selected." ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  QgsRasterLayer* myRasterLayer = qobject_cast<QgsRasterLayer *>( myLayer );
  if ( !myRasterLayer )
  {
    messageBar()->pushMessage( tr( "No Layer Selected" ),
                               tr( "To perform a full histogram stretch, you need to have a raster layer selected." ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  QgsRectangle myRectangle;
  if ( visibleAreaOnly )
    myRectangle = mapCanvas()->mapSettings().outputExtentToLayerExtent( myRasterLayer, mapCanvas()->extent() );

  myRasterLayer->setContrastEnhancement( QgsContrastEnhancement::StretchToMinimumMaximum, theLimits, myRectangle );

  mapCanvas()->refresh();
}

void QgisApp::increaseBrightness()
{
  int step = 1;
  if ( QgsApplication::keyboardModifiers() == Qt::ShiftModifier )
  {
    step = 10;
  }
  adjustBrightnessContrast( step );
}

void QgisApp::decreaseBrightness()
{
  int step = -1;
  if ( QgsApplication::keyboardModifiers() == Qt::ShiftModifier )
  {
    step = -10;
  }
  adjustBrightnessContrast( step );
}

void QgisApp::increaseContrast()
{
  int step = 1;
  if ( QgsApplication::keyboardModifiers() == Qt::ShiftModifier )
  {
    step = 10;
  }
  adjustBrightnessContrast( step, false );
}

void QgisApp::decreaseContrast()
{
  int step = -1;
  if ( QgsApplication::keyboardModifiers() == Qt::ShiftModifier )
  {
    step = -10;
  }
  adjustBrightnessContrast( step, false );
}

void QgisApp::adjustBrightnessContrast( int delta, bool updateBrightness )
{
  foreach ( QgsMapLayer * layer, layerTreeView()->selectedLayers() )
  {
    if ( !layer )
    {
      messageBar()->pushMessage( tr( "No Layer Selected" ),
                                 tr( "To change brightness or contrast, you need to have a raster layer selected." ),
                                 QgsMessageBar::INFO, messageTimeout() );
      return;
    }

    QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer *>( layer );
    if ( !rasterLayer )
    {
      messageBar()->pushMessage( tr( "No Layer Selected" ),
                                 tr( "To change brightness or contrast, you need to have a raster layer selected." ),
                                 QgsMessageBar::INFO, messageTimeout() );
      return;
    }

    QgsBrightnessContrastFilter* brightnessFilter = rasterLayer->brightnessFilter();

    if ( updateBrightness )
    {
      brightnessFilter->setBrightness( brightnessFilter->brightness() + delta );
    }
    else
    {
      brightnessFilter->setContrast( brightnessFilter->contrast() + delta );
    }

    rasterLayer->triggerRepaint();
  }
}

void QgisApp::helpContents()
{
  // We should really ship the HTML version of the docs local too.
  openURL( QString( "http://docs.qgis.org/%1.%2/%3/docs/user_manual/" )
           .arg( QGis::QGIS_VERSION_INT / 10000 )
           .arg( QGis::QGIS_VERSION_INT / 100 % 100 )
           .arg( tr( "en", "documentation language" ) ),
           false );
}

void QgisApp::apiDocumentation()
{
  if ( QFileInfo( QgsApplication::pkgDataPath() + "/doc/api/index.html" ).exists() )
  {
    openURL( "api/index.html" );
  }
  else
  {
    openURL( "http://qgis.org/api/", false );
  }
}

void QgisApp::supportProviders()
{
  openURL( tr( "http://qgis.org/en/site/forusers/commercial_support.html" ), false );
}

void QgisApp::helpQgisHomePage()
{
  openURL( "http://qgis.org", false );
}

void QgisApp::openURL( QString url, bool useQgisDocDirectory )
{
  // open help in user browser
  if ( useQgisDocDirectory )
  {
    url = "file://" + QgsApplication::pkgDataPath() + "/doc/" + url;
  }
#ifdef Q_OS_MACX
  /* Use Mac OS X Launch Services which uses the user's default browser
   * and will just open a new window if that browser is already running.
   * QProcess creates a new browser process for each invocation and expects a
   * commandline application rather than a bundled application.
   */
  CFURLRef urlRef = CFURLCreateWithBytes( kCFAllocatorDefault,
                                          reinterpret_cast<const UInt8*>( url.toUtf8().data() ), url.length(),
                                          kCFStringEncodingUTF8, NULL );
  OSStatus status = LSOpenCFURLRef( urlRef, NULL );
  status = 0; //avoid compiler warning
  CFRelease( urlRef );
#elif defined(Q_OS_WIN)
  if ( url.startsWith( "file://", Qt::CaseInsensitive ) )
    ShellExecute( 0, 0, url.mid( 7 ).toLocal8Bit().constData(), 0, 0, SW_SHOWNORMAL );
  else
    QDesktopServices::openUrl( url );
#else
  QDesktopServices::openUrl( url );
#endif
}

QgsRedliningLayer* QgisApp::redliningLayer() const
{
  return mRedlining->getOrCreateLayer();
}

/** Get a pointer to the currently selected map layer */
QgsMapLayer *QgisApp::activeLayer()
{
  return layerTreeView() ? layerTreeView()->currentLayer() : 0;
}

/** set the current layer */
bool QgisApp::setActiveLayer( QgsMapLayer *layer )
{
  if ( !layer )
    return false;

  if ( !layerTreeView()->layerTreeModel()->rootGroup()->findLayer( layer->id() ) )
    return false;

  layerTreeView()->setCurrentLayer( layer );
  return true;
}

/** Add a vector layer directly without prompting user for location
  The caller must provide information compatible with the provider plugin
  using the vectorLayerPath and baseName. The provider can use these
  parameters in any way necessary to initialize the layer. The baseName
  parameter is used in the Map Legend so it should be formed in a meaningful
  way.
  */
QgsVectorLayer* QgisApp::addVectorLayer( QString vectorLayerPath, QString baseName, QString providerKey )
{
  bool wasfrozen = mapCanvas()->isFrozen();

  mapCanvas()->freeze();

// Let render() do its own cursor management
//  QApplication::setOverrideCursor(Qt::WaitCursor);

  /* Eliminate the need to instantiate the layer based on provider type.
     The caller is responsible for cobbling together the needed information to
     open the layer
     */
  QgsDebugMsg( "Creating new vector layer using " + vectorLayerPath
               + " with baseName of " + baseName
               + " and providerKey of " + providerKey );

  // create the layer
  QgsVectorLayer *layer = new QgsVectorLayer( vectorLayerPath, baseName, providerKey, false );

  if ( layer && layer->isValid() )
  {
    QStringList sublayers = layer->dataProvider()->subLayers();
    QgsDebugMsg( QString( "got valid layer with %1 sublayers" ).arg( sublayers.count() ) );

    // If the newly created layer has more than 1 layer of data available, we show the
    // sublayers selection dialog so the user can select the sublayers to actually load.
    if ( sublayers.count() > 1 )
    {
      askUserForOGRSublayers( layer );

      // The first layer loaded is not useful in that case. The user can select it in
      // the list if he wants to load it.
      delete layer;
      layer = 0;
    }
    else
    {
      // Register this layer with the layers registry
      QList<QgsMapLayer *> myList;
      myList << layer;
      QgsMapLayerRegistry::instance()->addMapLayers( myList );
      bool ok;
      layer->loadDefaultStyle( ok );
    }
  }
  else
  {
    QString msg = tr( "The layer %1 is not a valid layer and can not be added to the map" ).arg( vectorLayerPath );
    messageBar()->pushMessage( tr( "Layer is not valid" ), msg, QgsMessageBar::CRITICAL, messageTimeout() );

    delete layer;
    mapCanvas()->freeze( false );
    return NULL;
  }

  // Only update the map if we frozen in this method
  // Let the caller do it otherwise
  if ( !wasfrozen )
  {
    mapCanvas()->freeze( false );
    mapCanvas()->refresh();
  }

// Let render() do its own cursor management
//  QApplication::restoreOverrideCursor();

  return layer;

} // QgisApp::addVectorLayer



void QgisApp::addMapLayer( QgsMapLayer *theMapLayer )
{
  mapCanvas()->freeze();

// Let render() do its own cursor management
//  QApplication::setOverrideCursor(Qt::WaitCursor);

  if ( theMapLayer->isValid() )
  {
    // Register this layer with the layers registry
    QList<QgsMapLayer *> myList;
    myList << theMapLayer;
    QgsMapLayerRegistry::instance()->addMapLayers( myList );
    // add it to the mapcanvas collection
    // not necessary since adding to registry adds to canvas mapCanvas()->addLayer(theMapLayer);
  }
  else
  {
    QString msg = tr( "The layer is not a valid layer and can not be added to the map" );
    messageBar()->pushMessage( tr( "Layer is not valid" ), msg, QgsMessageBar::CRITICAL, messageTimeout() );
  }

  // draw the map
  mapCanvas()->freeze( false );
  mapCanvas()->refresh();

// Let render() do its own cursor management
//  QApplication::restoreOverrideCursor();

}

void QgisApp::embedLayers()
{
  //dialog to select groups/layers from other project files
  QgsProjectLayerGroupDialog d( this );
  if ( d.exec() == QDialog::Accepted && d.isValid() )
  {
    mapCanvas()->freeze( true );

    QString projectFile = d.selectedProjectFile();

    //groups
    QStringList groups = d.selectedGroups();
    QStringList::const_iterator groupIt = groups.constBegin();
    for ( ; groupIt != groups.constEnd(); ++groupIt )
    {
      QgsLayerTreeGroup* newGroup = QgsProject::instance()->createEmbeddedGroup( *groupIt, projectFile, QStringList() );

      if ( newGroup )
        QgsProject::instance()->layerTreeRoot()->addChildNode( newGroup );
    }

    //layer ids
    QList<QDomNode> brokenNodes;
    QList< QPair< QgsVectorLayer*, QDomElement > > vectorLayerList;
    QStringList layerIds = d.selectedLayerIds();
    QStringList::const_iterator layerIt = layerIds.constBegin();
    for ( ; layerIt != layerIds.constEnd(); ++layerIt )
    {
      QgsProject::instance()->createEmbeddedLayer( *layerIt, projectFile, brokenNodes, vectorLayerList );
    }

    mapCanvas()->freeze( false );
    if ( groups.size() > 0 || layerIds.size() > 0 )
    {
      mapCanvas()->refresh();
    }
  }
}

void QgisApp::setExtent( QgsRectangle theRect )
{
  mapCanvas()->setExtent( theRect );
}

/**
  Prompt and save if project has been modified.
  @return true if saved or discarded, false if cancelled
 */
bool QgisApp::saveDirty()
{
  QString whyDirty = "";
  bool hasUnsavedEdits = false;
  // extra check to see if there are any vector layers with unsaved provider edits
  // to ensure user has opportunity to save any editing
  if ( QgsMapLayerRegistry::instance()->count() > 0 )
  {
    QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
    for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
    {
      QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() );
      if ( !vl )
      {
        continue;
      }

      hasUnsavedEdits = ( vl->isEditable() && vl->isModified() );
      if ( hasUnsavedEdits )
      {
        break;
      }
    }

    if ( hasUnsavedEdits )
    {
      markDirty();
      whyDirty = "<p style='color:darkred;'>";
      whyDirty += tr( "Project has layer(s) in edit mode with unsaved edits, which will NOT be saved!" );
      whyDirty += "</p>";
    }
  }

  QMessageBox::StandardButton answer( QMessageBox::Discard );
  mapCanvas()->freeze( true );

  //QgsDebugMsg(QString("Layer count is %1").arg(mapCanvas()->layerCount()));
  //QgsDebugMsg(QString("Project is %1dirty").arg( QgsProject::instance()->isDirty() ? "" : "not "));
  //QgsDebugMsg(QString("Map canvas is %1dirty").arg(mapCanvas()->isDirty() ? "" : "not "));

  QSettings settings;
  bool askThem = settings.value( "qgis/askToSaveProjectChanges", true ).toBool();

  if ( askThem && QgsProject::instance()->isDirty() && QgsMapLayerRegistry::instance()->count() > 0 )
  {
    // flag project as dirty since dirty state of canvas is reset if "dirty"
    // is based on a zoom or pan
    markDirty();

    // old code: mProjectIsDirtyFlag = true;

    // prompt user to save
    answer = QMessageBox::information( this, tr( "Save?" ),
                                       tr( "Do you want to save the current project? %1" )
                                       .arg( whyDirty ),
                                       QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard,
                                       hasUnsavedEdits ? QMessageBox::Cancel : QMessageBox::Save );
    if ( QMessageBox::Save == answer )
    {
      if ( !fileSave() )
        answer = QMessageBox::Cancel;
    }
  }

  mapCanvas()->freeze( false );

  return answer != QMessageBox::Cancel;
} // QgisApp::saveDirty()

void QgisApp::closeProject()
{
  // unload the project macros before changing anything
  if ( mTrustedMacros )
  {
    QgsPythonRunner::run( "qgis.utils.unloadProjectMacros();" );
  }

  // remove any message widgets from the message bar
  messageBar()->clearWidgets();

  mTrustedMacros = false;

  setFilterLegendByMapEnabled( false );

  deletePrintComposers();
  removeAnnotationItems();
  // clear out any stuff from project
  mapCanvas()->freeze( true );
  QList<QgsMapCanvasLayer> emptyList;
  mapCanvas()->setLayerSet( emptyList );
  mapCanvas()->clearCache();
  removeAllLayers();
  QgsTemporaryFile::clear();
}


void QgisApp::changeEvent( QEvent* event )
{
  QMainWindow::changeEvent( event );
#ifdef Q_OS_MAC
  switch ( event->type() )
  {
    case QEvent::ActivationChange:
      if ( QApplication::activeWindow() == this )
      {
        mWindowAction->setChecked( true );
      }
      // this should not be necessary since the action is part of an action group
      // however this check is not cleared if PrintComposer is closed and reopened
      else
      {
        mWindowAction->setChecked( false );
      }
      break;

    case QEvent::WindowTitleChange:
      mWindowAction->setText( windowTitle() );
      break;

    default:
      break;
  }
#endif
}

void QgisApp::closeEvent( QCloseEvent* event )
{
  // We'll close in our own good time, thank you very much
  event->ignore();
  // Do the usual checks and ask if they want to save, etc
  fileExit();
}


void QgisApp::whatsThis()
{
  QWhatsThis::enterWhatsThisMode();
} // QgisApp::whatsThis()

void QgisApp::destinationCrsChanged()
{
  // save this information to project
  long srsid = mapCanvas()->mapSettings().destinationCrs().srsid();
  QgsProject::instance()->writeEntry( "SpatialRefSys", "/ProjectCRSID", ( int )srsid );
}

void QgisApp::hasCrsTransformEnabled( bool theFlag )
{
  // save this information to project
  QgsProject::instance()->writeEntry( "SpatialRefSys", "/ProjectionsEnabled", ( theFlag ? 1 : 0 ) );
}

void QgisApp::mapToolChanged( QgsMapTool *newTool, QgsMapTool *oldTool )
{
  if ( oldTool )
  {
    disconnect( oldTool, SIGNAL( messageEmitted( QString ) ), this, SLOT( displayMapToolMessage( QString ) ) );
    disconnect( oldTool, SIGNAL( messageEmitted( QString, QgsMessageBar::MessageLevel ) ), this, SLOT( displayMapToolMessage( QString, QgsMessageBar::MessageLevel ) ) );
    disconnect( oldTool, SIGNAL( messageDiscarded() ), this, SLOT( removeMapToolMessage() ) );
  }
  // Automatically return to pan tool if no tool is active
  if ( !newTool && oldTool != mMapTools.mPan )
  {
    mapCanvas()->setMapTool( mMapTools.mPan );
    return;
  }

  if ( newTool )
  {
    connect( newTool, SIGNAL( messageEmitted( QString ) ), this, SLOT( displayMapToolMessage( QString ) ) );
    connect( newTool, SIGNAL( messageEmitted( QString, QgsMessageBar::MessageLevel ) ), this, SLOT( displayMapToolMessage( QString, QgsMessageBar::MessageLevel ) ) );
    connect( newTool, SIGNAL( messageDiscarded() ), this, SLOT( removeMapToolMessage() ) );
  }
}

void QgisApp::markDirty()
{
  // notify the project that there was a change
  QgsProject::instance()->dirty( true );
}

void QgisApp::layersWereAdded( QList<QgsMapLayer *> theLayers )
{
  for ( int i = 0; i < theLayers.size(); ++i )
  {
    QgsMapLayer * layer = theLayers.at( i );
    QgsDataProvider *provider = 0;

    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
    if ( vlayer )
    {
      // notify user about any font family substitution, but only when rendering labels (i.e. not when opening settings dialog)
      connect( vlayer, SIGNAL( labelingFontNotFound( QgsVectorLayer*, QString ) ), this, SLOT( labelingFontNotFound( QgsVectorLayer*, QString ) ) );

      QgsVectorDataProvider* vProvider = vlayer->dataProvider();
      if ( vProvider && vProvider->capabilities() & QgsVectorDataProvider::EditingCapabilities )
      {
        connect( vlayer, SIGNAL( layerModified() ), this, SLOT( updateLayerModifiedActions() ) );
        connect( vlayer, SIGNAL( editingStarted() ), this, SLOT( layerEditStateChanged() ) );
        connect( vlayer, SIGNAL( editingStopped() ), this, SLOT( layerEditStateChanged() ) );
      }
      provider = vProvider;
    }

    QgsRasterLayer *rlayer = qobject_cast<QgsRasterLayer *>( layer );
    if ( rlayer )
    {
      // connect up any request the raster may make to update the app progress
      connect( rlayer, SIGNAL( drawingProgress( int, int ) ), this, SIGNAL( progress( int, int ) ) );

      // connect up any request the raster may make to update the statusbar message
      connect( rlayer, SIGNAL( statusChanged( QString ) ), this, SLOT( showStatusMessage( QString ) ) );

      provider = rlayer->dataProvider();
    }

    if ( provider )
    {
      connect( provider, SIGNAL( dataChanged() ), layer, SLOT( triggerRepaint() ) );
      connect( provider, SIGNAL( dataChanged() ), mapCanvas(), SLOT( refresh() ) );
    }
  }
}

void QgisApp::updateMouseCoordinatePrecision()
{
  // Work out what mouse display precision to use. This only needs to
  // be when the settings change or the zoom level changes. This
  // function needs to be called every time one of the above happens.

  // Get the display precision from the project settings
  bool automatic = QgsProject::instance()->readBoolEntry( "PositionPrecision", "/Automatic" );
  int dp = 0;

  if ( automatic )
  {
    // Work out a suitable number of decimal places for the mouse
    // coordinates with the aim of always having enough decimal places
    // to show the difference in position between adjacent pixels.
    // Also avoid taking the log of 0.
    if ( mapCanvas()->mapUnitsPerPixel() != 0.0 )
      dp = static_cast<int>( ceil( -1.0 * log10( mapCanvas()->mapUnitsPerPixel() ) ) );
  }
  else
    dp = QgsProject::instance()->readNumEntry( "PositionPrecision", "/DecimalPlaces" );

  // Keep dp sensible
  if ( dp < 0 )
    dp = 0;

  mMousePrecisionDecimalPlaces = dp;
}

void QgisApp::showStatusMessage( QString theMessage )
{
  statusBar()->showMessage( theMessage );
}

void QgisApp::displayMapToolMessage( QString message, QgsMessageBar::MessageLevel level )
{
  // remove previous message
  messageBar()->popWidget( mLastMapToolMessage );

  QgsMapTool* tool = mapCanvas()->mapTool();

  if ( tool )
  {
    mLastMapToolMessage = new QgsMessageBarItem( tool->toolName(), message, level, messageTimeout() );
    messageBar()->pushItem( mLastMapToolMessage );
  }
}

void QgisApp::removeMapToolMessage()
{
  // remove previous message
  messageBar()->popWidget( mLastMapToolMessage );
}


// Show the maptip using tooltip
void QgisApp::showMapTip()
{
  // Stop the timer while we look for a maptip
  mpMapTipsTimer->stop();

  // Only show tooltip if the mouse is over the canvas
  if ( mapCanvas()->underMouse() )
  {
    QPoint myPointerPos = mapCanvas()->mouseLastXY();

    //  Make sure there is an active layer before proceeding
    QgsMapLayer* mypLayer = mapCanvas()->currentLayer();
    if ( mypLayer )
    {
      //QgsDebugMsg("Current layer for maptip display is: " + mypLayer->source());
      // only process vector layers
      if ( mypLayer->type() == QgsMapLayer::VectorLayer )
      {
        // Show the maptip if the maptips button is depressed
        if ( mMapTipsVisible )
        {
          mpMaptip->showMapTip( mypLayer, mLastMapPosition, myPointerPos, mapCanvas() );
        }
      }
    }
    else
    {
      showStatusMessage( tr( "Maptips require an active layer" ) );
    }
  }
}

void QgisApp::projectPropertiesProjections()
{
  // Driver to display the project props dialog and switch to the
  // projections tab
  mShowProjectionTab = true;
  projectProperties();
}

void QgisApp::projectProperties()
{
  /* Display the property sheet for the Project */
  // set wait cursor since construction of the project properties
  // dialog results in the construction of the spatial reference
  // system QMap
  QApplication::setOverrideCursor( Qt::WaitCursor );
  QgsProjectProperties *pp = new QgsProjectProperties( mapCanvas(), this );
  // if called from the status bar, show the projection tab
  if ( mShowProjectionTab )
  {
    pp->showProjectionsTab();
    mShowProjectionTab = false;
  }
  qApp->processEvents();
  // Be told if the mouse display precision may have changed by the user
  // changing things in the project properties dialog box
  connect( pp, SIGNAL( displayPrecisionChanged() ), this,
           SLOT( updateMouseCoordinatePrecision() ) );

  connect( pp, SIGNAL( scalesChanged( const QStringList & ) ), this,
           SIGNAL( projectScalesChanged( QStringList ) ) );
  QApplication::restoreOverrideCursor();

  //pass any refresh signals off to canvases
  // Line below was commented out by wonder three years ago (r4949).
  // It is needed to refresh scale bar after changing display units.
  connect( pp, SIGNAL( refresh() ), mapCanvas(), SLOT( refresh() ) );

  // Display the modal dialog box.
  pp->exec();

  int  myRedInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorRedPart", 255 );
  int  myGreenInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorGreenPart", 255 );
  int  myBlueInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorBluePart", 255 );
  int  myAlphaInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorAlphaPart", 0 );
  QColor myColor = QColor( myRedInt, myGreenInt, myBlueInt, myAlphaInt );
  mapCanvas()->setCanvasColor( myColor ); // this is fill color before rendering onto canvas

  // Set the window title.
  setTitleBarText_( *this );

  // delete the property sheet object
  delete pp;
} // QgisApp::projectProperties


QgsClipboard * QgisApp::clipboard()
{
  return mInternalClipboard;
}

void QgisApp::selectionChanged( QgsMapLayer *layer )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( vlayer )
  {
    showStatusMessage( tr( "%n feature(s) selected on layer %1.", "number of selected features", vlayer->selectedFeatureCount() ).arg( vlayer->name() ) );
  }
  if ( layer == activeLayer() )
  {
    activateDeactivateLayerRelatedActions( layer );
  }
}

void QgisApp::layerEditStateChanged()
{
  QgsMapLayer* layer = qobject_cast<QgsMapLayer *>( sender() );
  if ( layer && layer == activeLayer() )
  {
    activateDeactivateLayerRelatedActions( layer );
    mSaveRollbackInProgress = false;
  }
}

/////////////////////////////////////////////////////////////////
//
//
// Only functions relating to raster layer management in this
// section (look for a similar comment block to this to find
// the end of this section).
//
// Tim Sutton
//
//
/////////////////////////////////////////////////////////////////


// this is a slot for action from GUI to add raster layer
void QgisApp::addRasterLayer()
{
  QStringList selectedFiles;
  QString e;//only for parameter correctness
  QString title = tr( "Open a GDAL Supported Raster Data Source" );
  QgisGui::openFilesRememberingFilter( "lastRasterFileFilter", mRasterFileFilter, selectedFiles, e,
                                       title );

  if ( selectedFiles.isEmpty() )
  {
    // no files were selected, so just bail
    return;
  }

  // Handle georeferenced images (with EXIF tags)
  QStringList rasterFiles;
  foreach ( const QString& file, selectedFiles )
  {
    if ( !QgsGeoImageAnnotationItem::create( mapCanvas(), file ) )
    {
      rasterFiles.append( file );
    }
  }

  addRasterLayers( rasterFiles );

}// QgisApp::addRasterLayer()

//
// This is the method that does the actual work of adding a raster layer - the others
// here simply create a raster layer object and delegate here. It is the responsibility
// of the calling method to manage things such as the frozen state of the mapcanvas and
// using waitcursors etc. - this method won't and SHOULDN'T do it
//
bool QgisApp::addRasterLayer( QgsRasterLayer *theRasterLayer )
{
  Q_CHECK_PTR( theRasterLayer );

  if ( ! theRasterLayer )
  {
    // XXX insert meaningful whine to the user here; although be
    // XXX mindful that a null layer may mean exhausted memory resources
    return false;
  }

  if ( !theRasterLayer->isValid() )
  {
    delete theRasterLayer;
    return false;
  }

  // register this layer with the central layers registry
  QList<QgsMapLayer *> myList;
  myList << theRasterLayer;
  QgsMapLayerRegistry::instance()->addMapLayers( myList );

  return true;
}


// Open a raster layer - this is the generic function which takes all parameters
// this method is a blend of addRasterLayer() functions (with and without provider)
// and addRasterLayers()
QgsRasterLayer* QgisApp::addRasterLayerPrivate(
  const QString & uri, const QString & baseName, const QString & providerKey,
  bool guiWarning, bool guiUpdate )
{
  if ( guiUpdate )
  {
    // let the user know we're going to possibly be taking a while
    // QApplication::setOverrideCursor( Qt::WaitCursor );
    mapCanvas()->freeze( true );
  }

  QgsDebugMsg( "Creating new raster layer using " + uri
               + " with baseName of " + baseName );

  QgsRasterLayer *layer = 0;
  // XXX ya know QgsRasterLayer can snip out the basename on its own;
  // XXX why do we have to pass it in for it?
  // ET : we may not be getting "normal" files here, so we still need the baseName argument
  if ( providerKey.isEmpty() )
    layer = new QgsRasterLayer( uri, baseName ); // fi.completeBaseName());
  else
    layer = new QgsRasterLayer( uri, baseName, providerKey );

  QgsDebugMsg( "Constructed new layer" );

  QgsError error;
  QString title;
  bool ok = false;

  if ( !layer->isValid() )
  {
    error = layer->error();
    title = tr( "Invalid Layer" );

    if ( shouldAskUserForGDALSublayers( layer ) )
    {
      askUserForGDALSublayers( layer );
      ok = true;

      // The first layer loaded is not useful in that case. The user can select it in
      // the list if he wants to load it.
      delete layer;
      layer = NULL;
    }
  }
  else
  {
    ok = addRasterLayer( layer );
    if ( !ok )
    {
      error.append( QGS_ERROR_MESSAGE( tr( "Error adding valid layer to map canvas" ),
                                       tr( "Raster layer" ) ) );
      title = tr( "Error" );
    }
  }

  if ( !ok )
  {
    if ( guiUpdate )
      mapCanvas()->freeze( false );

    // don't show the gui warning if we are loading from command line
    if ( guiWarning )
    {
      QgsErrorDialog::show( error, title );
    }

    if ( layer )
    {
      delete layer;
      layer = NULL;
    }
  }

  if ( guiUpdate )
  {
    // draw the map
    mapCanvas()->freeze( false );
    mapCanvas()->refresh();
    // Let render() do its own cursor management
    //    QApplication::restoreOverrideCursor();
  }

  return layer;

} // QgisApp::addRasterLayer


//create a raster layer object and delegate to addRasterLayer(QgsRasterLayer *)
QgsRasterLayer* QgisApp::addRasterLayer(
  QString const & rasterFile, QString const & baseName, bool guiWarning )
{
  return addRasterLayerPrivate( rasterFile, baseName, QString(), guiWarning, true );
}


/** Add a raster layer directly without prompting user for location
  The caller must provide information compatible with the provider plugin
  using the uri and baseName. The provider can use these
  parameters in any way necessary to initialize the layer. The baseName
  parameter is used in the Map Legend so it should be formed in a meaningful
  way.

  \note   Copied from the equivalent addVectorLayer function in this file
  */
QgsRasterLayer* QgisApp::addRasterLayer(
  QString const &uri, QString const &baseName, QString const &providerKey )
{
  return addRasterLayerPrivate( uri, baseName, providerKey, true, true );
}


//create a raster layer object and delegate to addRasterLayer(QgsRasterLayer *)
bool QgisApp::addRasterLayers( QStringList const &theFileNameQStringList, bool guiWarning )
{
  if ( theFileNameQStringList.empty() )
  {
    // no files selected so bail out, but
    // allow mMapCanvas to handle events
    // first
    mapCanvas()->freeze( false );
    return false;
  }

  mapCanvas()->freeze( true );

  // this is messy since some files in the list may be rasters and others may
  // be ogr layers. We'll set returnValue to false if one or more layers fail
  // to load.
  bool returnValue = true;
  for ( QStringList::ConstIterator myIterator = theFileNameQStringList.begin();
        myIterator != theFileNameQStringList.end();
        ++myIterator )
  {
    QString errMsg;
    bool ok = false;

    // if needed prompt for zipitem layers
    QString vsiPrefix = QgsZipItem::vsiPrefix( *myIterator );
    if ( ! myIterator->startsWith( "/vsi", Qt::CaseInsensitive ) &&
         ( vsiPrefix == "/vsizip/" || vsiPrefix == "/vsitar/" ) )
    {
      if ( askUserForZipItemLayers( *myIterator ) )
        continue;
    }

    if ( QgsRasterLayer::isValidRasterFileName( *myIterator, errMsg ) )
    {
      QFileInfo myFileInfo( *myIterator );
      // get the directory the .adf file was in
      QString myDirNameQString = myFileInfo.path();
      //extract basename
      QString myBaseNameQString = myFileInfo.completeBaseName();
      //only allow one copy of a ai grid file to be loaded at a
      //time to prevent the user selecting all adfs in 1 dir which
      //actually represent 1 coverage,

      // try to create the layer
      QgsRasterLayer *layer = addRasterLayerPrivate( *myIterator, myBaseNameQString,
                              QString(), guiWarning, true );
      if ( layer && layer->isValid() )
      {
        //only allow one copy of a ai grid file to be loaded at a
        //time to prevent the user selecting all adfs in 1 dir which
        //actually represent 1 coverate,

        if ( myBaseNameQString.toLower().endsWith( ".adf" ) )
        {
          break;
        }
      }
      // if layer is invalid addRasterLayerPrivate() will show the error

    } // valid raster filename
    else
    {
      ok = false;

      // Issue message box warning unless we are loading from cmd line since
      // non-rasters are passed to this function first and then successfully
      // loaded afterwards (see main.cpp)
      if ( guiWarning )
      {
        QgsError error;
        QString msg;

        msg = tr( "%1 is not a supported raster data source" ).arg( *myIterator );
        if ( errMsg.size() > 0 )
          msg += "\n" + errMsg;
        error.append( QGS_ERROR_MESSAGE( msg, tr( "Raster layer" ) ) );

        QgsErrorDialog::show( error, tr( "Unsupported Data Source" ) );
      }
    }
    if ( ! ok )
    {
      returnValue = false;
    }
  }

  mapCanvas()->freeze( false );
  mapCanvas()->refresh();

// Let render() do its own cursor management
//  QApplication::restoreOverrideCursor();

  return returnValue;

}// QgisApp::addRasterLayer()




///////////////////////////////////////////////////////////////////
//
//
//
//
//    RASTER ONLY RELATED FUNCTIONS BLOCK ENDS....
//
//
//
//
///////////////////////////////////////////////////////////////////

#ifdef ANDROID
void QgisApp::keyReleaseEvent( QKeyEvent *event )
{
  static bool accepted = true;
  if ( event->key() == Qt::Key_Close )
  {
    // do something useful here
    int ret = QMessageBox::question( this, tr( "Exit QGIS" ),
                                     tr( "Do you really want to quit QGIS?" ),
                                     QMessageBox::Yes | QMessageBox::No );
    switch ( ret )
    {
      case QMessageBox::Yes:
        this->close();
        break;

      case QMessageBox::No:
        break;
    }
    event->setAccepted( accepted ); // don't close my Top Level Widget !
    accepted = false;// close the app next time when the user press back button
  }
}
#endif

void QgisApp::keyPressEvent( QKeyEvent * e )
{
  // The following statement causes a crash on WIN32 and should be
  // enclosed in an #ifdef QGISDEBUG if its really necessary. Its
  // commented out for now. [gsherman]
  // QgsDebugMsg( QString( "%1 (keypress received)" ).arg( e->text() ) );
  emit keyPressed( e );

  //cancel rendering progress with esc key
  if ( e->key() == Qt::Key_Escape )
  {
    stopRendering();
  }
#if defined(_MSC_VER) && defined(QGISDEBUG)
  else if ( e->key() == Qt::Key_Backslash && e->modifiers() & Qt::ControlModifier )
  {
    qgisCrashDump( 0 );
  }
#endif
  else
  {
    e->ignore();
  }
}

void QgisApp::mapCanvas_keyPressed( QKeyEvent *e )
{
  // Delete selected features when it is possible and KeyEvent was not managed by current MapTool
  if (( e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete ) && e->isAccepted() )
  {
    deleteSelected();
  }
}

#ifdef Q_OS_WIN
// hope your wearing your peril sensitive sunglasses.
void QgisApp::contextMenuEvent( QContextMenuEvent *e )
{
  if ( mSkipNextContextMenuEvent )
  {
    mSkipNextContextMenuEvent--;
    e->ignore();
    return;
  }

  QMainWindow::contextMenuEvent( e );
}

void QgisApp::skipNextContextMenuEvent()
{
  mSkipNextContextMenuEvent++;
}
#endif

// Debug hook - used to output diagnostic messages when evoked (usually from the menu)
/* Temporarily disabled...
   void QgisApp::debugHook()
   {
   QgsDebugMsg("Hello from debug hook");
// show the map canvas extent
QgsDebugMsg(mapCanvas()->extent());
}
*/
void QgisApp::customProjection()
{
  // Create an instance of the Custom Projection Designer modeless dialog.
  // Autodelete the dialog when closing since a pointer is not retained.
  QgsCustomProjectionDialog * myDialog = new QgsCustomProjectionDialog( this );
  myDialog->setAttribute( Qt::WA_DeleteOnClose );
  myDialog->show();
}

void QgisApp::newBookmark()
{
  QgsBookmarks::newBookmark();
}

void QgisApp::showBookmarks()
{
  QgsBookmarks::showBookmarks();
}

// Slot that gets called when the project file was saved with an older
// version of QGIS

void QgisApp::oldProjectVersionWarning( QString oldVersion )
{
  QSettings settings;

  if ( settings.value( "/qgis/warnOldProjectVersion", QVariant( true ) ).toBool() )
  {
    QString smalltext = tr( "This project file was saved by an older version of QGIS."
                            " When saving this project file, QGIS will update it to the latest version, "
                            "possibly rendering it useless for older versions of QGIS." );

    QString text =  tr( "<p>This project file was saved by an older version of QGIS."
                        " When saving this project file, QGIS will update it to the latest version, "
                        "possibly rendering it useless for older versions of QGIS."
                        "<p>Even though QGIS developers try to maintain backwards "
                        "compatibility, some of the information from the old project "
                        "file might be lost."
                        " To improve the quality of QGIS, we appreciate "
                        "if you file a bug report at %3."
                        " Be sure to include the old project file, and state the version of "
                        "QGIS you used to discover the error."
                        "<p>To remove this warning when opening an older project file, "
                        "uncheck the box '%5' in the %4 menu."
                        "<p>Version of the project file: %1<br>Current version of QGIS: %2" )
                    .arg( oldVersion )
                    .arg( QGis::QGIS_VERSION )
                    .arg( "<a href=\"http://hub.qgis.org/projects/quantum-gis\">http://hub.qgis.org/projects/quantum-gis</a> " )
                    .arg( tr( "<tt>Settings:Options:General</tt>", "Menu path to setting options" ) )
                    .arg( tr( "Warn me when opening a project file saved with an older version of QGIS" ) );
    QString title =  tr( "Project file is older" );

#ifdef ANDROID
    //this is needed to deal with http://hub.qgis.org/issues/4573
    QMessageBox box( QMessageBox::Warning, title, tr( "This project file was saved by an older version of QGIS" ), QMessageBox::Ok, NULL );
    box.setDetailedText(
      text.remove( 0, 3 )
      .replace( QString( "<p>" ), QString( "\n\n" ) )
      .replace( QString( "<br>" ), QString( "\n" ) )
      .replace( QString( "<a href=\"http://hub.qgis.org/projects/quantum-gis\">http://hub.qgis.org/projects/quantum-gis</a> " ), QString( "\nhttp://hub.qgis.org/projects/quantum-gis" ) )
      .replace( QRegExp( "</?tt>" ), QString( "" ) )
    );
    box.exec();
#else
    messageBar()->pushMessage( title, smalltext );
#endif
  }
  return;
}

// add project directory to python path
void QgisApp::projectChanged( const QDomDocument &doc )
{
  Q_UNUSED( doc );
  QgsProject *project = qobject_cast<QgsProject*>( sender() );
  if ( !project )
    return;

  QFileInfo fi( project->fileName() );
  if ( !fi.exists() )
    return;

  static QString prevProjectDir = QString::null;

  if ( prevProjectDir == fi.canonicalPath() )
    return;

  QString expr;
  if ( !prevProjectDir.isNull() )
  {
    QString prev = prevProjectDir;
    expr = QString( "sys.path.remove('%1'); " ).arg( prev.replace( "'", "\\'" ) );
  }

  prevProjectDir = fi.canonicalPath();

  QString prev = prevProjectDir;
  expr += QString( "sys.path.append('%1')" ).arg( prev.replace( "'", "\\'" ) );

  QgsPythonRunner::run( expr );
}

void QgisApp::writeProject( QDomDocument &doc )
{
  // QGIS server does not use QgsProject for loading of QGIS project.
  // In order to allow reading of new projects, let's also write the original <legend> tag to the project.
  // Ideally the server should be ported to new layer tree implementation, but that requires
  // non-trivial changes to the server components.
  // The <legend> tag is ignored by QGIS application in >= 2.4 and this way also the new project files
  // can be opened in older versions of QGIS without loosing information about layer groups.

  QgsLayerTreeNode* clonedRoot = QgsProject::instance()->layerTreeRoot()->clone();
  QgsLayerTreeUtils::replaceChildrenOfEmbeddedGroups( QgsLayerTree::toGroup( clonedRoot ) );
  QgsLayerTreeUtils::updateEmbeddedGroupsProjectPath( QgsLayerTree::toGroup( clonedRoot ) ); // convert absolute paths to relative paths if required
  QDomElement oldLegendElem = QgsLayerTreeUtils::writeOldLegend( doc, QgsLayerTree::toGroup( clonedRoot ),
                              mLayerTreeCanvasBridge->hasCustomLayerOrder(), mLayerTreeCanvasBridge->customLayerOrder() );
  delete clonedRoot;
  doc.firstChildElement( "qgis" ).appendChild( oldLegendElem );

  QgsProject::instance()->writeEntry( "Legend", "filterByMap", ( bool ) layerTreeView()->layerTreeModel()->legendFilterByMap() );

  projectChanged( doc );
}

void QgisApp::readProject( const QDomDocument &doc )
{
  projectChanged( doc );

  // force update of canvas, without automatic changes to extent and OTF projections
  bool autoEnableCrsTransform = mLayerTreeCanvasBridge->autoEnableCrsTransform();
  bool autoSetupOnFirstLayer = mLayerTreeCanvasBridge->autoSetupOnFirstLayer();
  mLayerTreeCanvasBridge->setAutoEnableCrsTransform( false );
  mLayerTreeCanvasBridge->setAutoSetupOnFirstLayer( false );

  mLayerTreeCanvasBridge->setCanvasLayers();

  if ( autoEnableCrsTransform )
    mLayerTreeCanvasBridge->setAutoEnableCrsTransform( true );

  if ( autoSetupOnFirstLayer )
    mLayerTreeCanvasBridge->setAutoSetupOnFirstLayer( true );
}

void QgisApp::showLayerProperties( QgsMapLayer *ml )
{
  /*
  TODO: Consider reusing the property dialogs again.
  Sometimes around mid 2005, the property dialogs were saved for later reuse;
  this resulted in a time savings when reopening the dialog. The code below
  cannot be used as is, however, simply by saving the dialog pointer here.
  Either the map layer needs to be passed as an argument to sync or else
  a separate copy of the dialog pointer needs to be stored with each layer.
  */

  if ( !ml )
    return;

  if ( !QgsProject::instance()->layerIsEmbedded( ml->id() ).isEmpty() )
  {
    return; //don't show properties of embedded layers
  }

  if ( ml->type() == QgsMapLayer::RasterLayer )
  {
#if 0 // See note above about reusing this
    QgsRasterLayerProperties *rlp = NULL;
    if ( rlp )
    {
      rlp->sync();
    }
    else
    {
      rlp = new QgsRasterLayerProperties( ml, mMapCanvas, this );
      // handled by rendererChanged() connect( rlp, SIGNAL( refreshLegend( QString, bool ) ), mLayerTreeView, SLOT( refreshLayerSymbology( QString ) ) );
    }
#else
    QgsRasterLayerProperties *rlp = new QgsRasterLayerProperties( ml, mapCanvas(), this );
#endif

    rlp->exec();
    delete rlp; // delete since dialog cannot be reused without updating code
  }
  else if ( ml->type() == QgsMapLayer::VectorLayer ) // VECTOR
  {
    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( ml );

#if 0 // See note above about reusing this
    QgsVectorLayerProperties *vlp = NULL;
    if ( vlp )
    {
      vlp->syncToLayer();
    }
    else
    {
      vlp = new QgsVectorLayerProperties( vlayer, this );
      // handled by rendererChanged() connect( vlp, SIGNAL( refreshLegend( QString ) ), mLayerTreeView, SLOT( refreshLayerSymbology( QString ) ) );
    }
#else
    QgsVectorLayerProperties *vlp = new QgsVectorLayerProperties( vlayer, this );
#endif

    if ( vlp->exec() )
    {
      activateDeactivateLayerRelatedActions( ml );
    }
    delete vlp; // delete since dialog cannot be reused without updating code
  }
  else if ( ml->type() == QgsMapLayer::PluginLayer )
  {
    QgsPluginLayer* pl = qobject_cast<QgsPluginLayer *>( ml );
    if ( !pl )
      return;

    QgsPluginLayerType* plt = QgsPluginLayerRegistry::instance()->pluginLayerType( pl->pluginLayerType() );
    if ( !plt )
      return;

    if ( !plt->showLayerProperties( pl ) )
    {
      messageBar()->pushMessage( tr( "Warning" ),
                                 tr( "This layer doesn't have a properties dialog." ),
                                 QgsMessageBar::INFO, messageTimeout() );
    }
  }
}

void QgisApp::namSetup()
{
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();

  namUpdate();

#ifndef QT_NO_OPENSSL
  connect( nam, SIGNAL( sslErrorsConformationRequired( QUrl, QList<QSslError>, bool* ) ),
           this, SLOT( namConfirmSslErrors( QUrl, QList<QSslError>, bool* ) ) );
#endif
  connect( nam, SIGNAL( requestTimedOut( QNetworkReply* ) ),
           this, SLOT( namRequestTimedOut( QNetworkReply* ) ) );
}

#ifndef QT_NO_OPENSSL
void QgisApp::namConfirmSslErrors( const QUrl& url, const QList<QSslError> &errors, bool *ok )
{
  QString msg = tr( "SSL errors occured accessing URL %1:" ).arg( url.toString() );
  foreach ( QSslError error, errors )
  {
    msg += "\n" + error.errorString();
  }
  msg += tr( "\n\nAlways ignore these errors?" );
  *ok = QMessageBox::warning( this,
                              tr( "%1 SSL errors occured", "number of errors", errors.size() ),
                              msg,
                              QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes;
}
#endif

void QgisApp::namRequestTimedOut( QNetworkReply *reply )
{
  Q_UNUSED( reply );
  QLabel *msgLabel = new QLabel( tr( "A network request timed out, any data received is likely incomplete." ) +
                                 tr( " Please check the <a href=\"#messageLog\">message log</a> for further info." ), messageBar() );
  msgLabel->setWordWrap( true );
  connect( msgLabel, SIGNAL( linkActivated( QString ) ), mLogDock, SLOT( show() ) );
  messageBar()->pushItem( new QgsMessageBarItem( msgLabel, QgsMessageBar::WARNING, messageTimeout() ) );
}

void QgisApp::namUpdate()
{
  QgsNetworkAccessManager::instance()->setupDefaultProxyAndCache();
}

void QgisApp::completeInitialization()
{
  emit initializationCompleted();
}

QMenu* QgisApp::createPopupMenu()
{
  QMenu* menu = QMainWindow::createPopupMenu();
  QList< QAction* > al = menu->actions();
  QList< QAction* > panels, toolbars;

  if ( !al.isEmpty() )
  {
    bool found = false;
    for ( int i = 0; i < al.size(); ++i )
    {
      if ( al[ i ]->isSeparator() )
      {
        found = true;
        continue;
      }

      if ( !found )
      {
        panels.append( al[ i ] );
      }
      else
      {
        toolbars.append( al[ i ] );
      }
    }

    qSort( panels.begin(), panels.end(), cmpByText_ );
    foreach ( QAction* a, panels )
    {
      menu->addAction( a );
    }
    menu->addSeparator();
    qSort( toolbars.begin(), toolbars.end(), cmpByText_ );
    foreach ( QAction* a, toolbars )
    {
      menu->addAction( a );
    }
  }

  return menu;
}

void QgisApp::osmDownloadDialog()
{
  QgsOSMDownloadDialog dlg;
  dlg.exec();
}

void QgisApp::osmImportDialog()
{
  QgsOSMImportDialog dlg;
  dlg.exec();
}

void QgisApp::osmExportDialog()
{
  QgsOSMExportDialog dlg;
  dlg.exec();
}


bool QgisApp::gestureEvent( QGestureEvent *event )
{
  QTapAndHoldGesture *tapAndHold = static_cast<QTapAndHoldGesture *>( event->gesture( Qt::TapAndHoldGesture ) );
  if ( tapAndHold && tapAndHold->state() == Qt::GestureFinished )
  {
    QPoint pos = tapAndHold->position().toPoint();
    QWidget * receiver = QApplication::widgetAt( pos );
    qDebug() << "tapAndHoldTriggered: LONG CLICK gesture happened at " << pos;
    qDebug() << "widget under point of click: " << receiver;

    QApplication::postEvent( receiver, new QMouseEvent( QEvent::MouseButtonPress, receiver->mapFromGlobal( pos ), Qt::RightButton, Qt::RightButton, Qt::NoModifier ) );
    QApplication::postEvent( receiver, new QMouseEvent( QEvent::MouseButtonRelease, receiver->mapFromGlobal( pos ), Qt::RightButton, Qt::RightButton, Qt::NoModifier ) );
  }
  return true;
}

QList<double> QgisApp::wmtsResolutions() const
{
  QList<double> resolutionList;
  if ( !mapCanvas() )
  {
    return resolutionList;
  }

  QgsRasterLayer* currentLayer = 0;
  QgsRasterDataProvider* currentProvider = 0;
  QList<QgsMapLayer*> layerList = mapCanvas()->layers();
  for ( int i = 0; i < layerList.size(); ++i )
  {
    currentLayer = dynamic_cast<QgsRasterLayer*>( layerList.at( i ) );
    if ( !currentLayer )
    {
      continue;
    }

    currentProvider = currentLayer->dataProvider();
    if ( !currentProvider || currentProvider->name().compare( "wms", Qt::CaseInsensitive ) != 0 )
    {
      continue;
    }

    //wmts must not be reprojected
    if ( currentProvider->crs() != mapCanvas()->mapSettings().destinationCrs() )
    {
      continue;
    }

    //property 'resolutions' for wmts layers
    QVariant resolutionVariant = currentProvider->property( "resolutions" );
    if ( !resolutionVariant.isValid() )
    {
      continue;
    }

    QList<QVariant> resolutions = resolutionVariant.toList();
    QList<QVariant>::const_iterator resIt = resolutions.constBegin();
    for ( ; resIt != resolutions.constEnd(); ++resIt )
    {
      resolutionList.append( resIt->toDouble() );
    }

    if ( resolutions.size() > 0 )
    {
      break;
    }
  }

  return resolutionList;
}

int QgisApp::nextWMTSZoomLevel( const QList<double>& resolutions, bool zoomIn ) const
{
  if ( !mapCanvas() || resolutions.size() < 1 )
  {
    return -1;
  }

  int resolutionLevel = 0;
  double currentResolution = mapCanvas()->mapUnitsPerPixel();

  for ( int i = 0; i < resolutions.size(); ++i )
  {
    if ( qgsDoubleNear( resolutions.at( i ), currentResolution, 0.0001 ) )
    {
      resolutionLevel = zoomIn ? ( i - 1 ) : ( i + 1 );
      break;
    }
    else if ( currentResolution <= resolutions.at( i ) )
    {
      resolutionLevel = zoomIn ? ( i - 1 ) : i;
      break;
    }
  }

  if ( resolutionLevel < 0 )
  {
    resolutionLevel = 0;
  }
  else if ( resolutionLevel >= resolutions.size() )
  {
    resolutionLevel = ( resolutions.size() - 1 );
  }
  return resolutionLevel;
}

#ifdef _MSC_VER
LONG WINAPI QgisApp::qgisCrashDump( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
  QString dumpName = QDir::toNativeSeparators(
                       QString( "%1\\qgis-%2-%3-%4-%5.dmp" )
                       .arg( QDir::tempPath() )
                       .arg( QDateTime::currentDateTime().toString( "yyyyMMdd-hhmmss" ) )
                       .arg( GetCurrentProcessId() )
                       .arg( GetCurrentThreadId() )
                       .arg( QGis::QGIS_DEV_VERSION )
                     );

  QString msg;
  HANDLE hDumpFile = CreateFile( dumpName.toLocal8Bit(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0 );
  if ( hDumpFile != INVALID_HANDLE_VALUE )
  {
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;
    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = ExceptionInfo;
    ExpParam.ClientPointers = TRUE;

    if ( MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, ExceptionInfo ? &ExpParam : NULL, NULL, NULL ) )
    {
      msg = QObject::tr( "minidump written to %1" ).arg( dumpName );
    }
    else
    {
      msg = QObject::tr( "writing of minidump to %1 failed (%2)" ).arg( dumpName ).arg( GetLastError(), 0, 16 );
    }

    CloseHandle( hDumpFile );
  }
  else
  {
    msg = QObject::tr( "creation of minidump to %1 failed (%2)" ).arg( dumpName ).arg( GetLastError(), 0, 16 );
  }

  QMessageBox::critical( 0, QObject::tr( "Crash dumped" ), msg );

  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

