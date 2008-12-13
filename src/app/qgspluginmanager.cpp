/***************************************************************************
                          qgspluginmanager.cpp  -  description
                             -------------------
    begin                : Someday 2003
    copyright            : (C) 2003 by Gary E.Sherman
    email                : sherman at mrcc.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsconfig.h"

#include "qgis.h"
#include <QApplication>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QLibrary>
#include <QSettings>
#include <QStandardItem>
#include <QPushButton>
#include <QRegExp>

#include "qgspluginmanager.h"
#include <qgisplugin.h>
#include <qgslogger.h>
#include <qgspluginitem.h>
#include <qgsproviderregistry.h>
#include <qgspluginregistry.h>
#include <qgsdetaileditemdelegate.h>
#include <qgsdetaileditemwidget.h>
#include <qgsdetaileditemdata.h>

#include <qgspythonutils.h>
#include "qgslogger.h"

#define TESTLIB
#ifdef TESTLIB
// This doesn't work on WIN32 and causes problems with plugins
// on OS X (the code doesn't cause a problem but including dlfcn.h
// renders plugins unloadable)
#if !defined(WIN32) && !defined(Q_OS_MACX)
#include <dlfcn.h>
#endif
#endif


const int PLUGIN_DATA_ROLE = Qt::UserRole;
const int PLUGIN_LIBRARY_ROLE = Qt::UserRole + 1;
const int PLUGIN_BASE_NAME_ROLE = Qt::UserRole + 2;

QgsPluginManager::QgsPluginManager( QgsPythonUtils* pythonUtils, QWidget * parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );

  mPythonUtils = pythonUtils;

  // set the default lib dir to the qgis install directory/lib (this info is
  // available from the provider registry so we use it here)
  QgsProviderRegistry *pr = QgsProviderRegistry::instance();
  /*  char **argv = qApp->argv();
     QString appDir = argv[0];
     int bin = appDir.findRev("/bin", -1, false);
     QString baseDir = appDir.left(bin);
     QString libDir = baseDir + "/lib"; */

  lblPluginDir->setText( pr->libraryDirectory().path() );
  setTable();
  getPluginDescriptions();
  getPythonPluginDescriptions();
  mModelProxy->sort( 0, Qt::AscendingOrder );
  //
  // Create the select all and clear all buttons and add them to the
  // buttonBox
  //
  QPushButton * btnSelectAll = new QPushButton( tr( "&Select All" ) );
  QPushButton * btnClearAll = new QPushButton( tr( "&Clear All" ) );
  btnSelectAll->setDefault( true );
  buttonBox->addButton( btnSelectAll, QDialogButtonBox::ActionRole );
  buttonBox->addButton( btnClearAll, QDialogButtonBox::ActionRole );
  // connect the slot up to catch when a bookmark is deleted
  connect( btnSelectAll, SIGNAL( clicked() ), this, SLOT( selectAll() ) );
  // connect the slot up to catch when a bookmark is zoomed to
  connect( btnClearAll, SIGNAL( clicked() ), this, SLOT( clearAll() ) );

  qRegisterMetaType<QgsDetailedItemData>();
}


QgsPluginManager::~QgsPluginManager()
{
  delete mModelProxy;
  delete mModelPlugins;
}

void QgsPluginManager::setTable()
{
  mModelPlugins = new QStandardItemModel( 0, 1 );
  mModelProxy = new QSortFilterProxyModel( this );
  mModelProxy->setSourceModel( mModelPlugins );
  vwPlugins->setModel( mModelProxy );
  vwPlugins->setFocus();
  vwPlugins->setItemDelegateForColumn( 0, new QgsDetailedItemDelegate() );
}

void QgsPluginManager::resizeColumnsToContents()
{
  // Resize columns to contents.
  QgsDebugMsg( "entered." );
}

void QgsPluginManager::sortModel( int column )
{
  // Sort column ascending.
  mModelPlugins->sort( column );
  QgsDebugMsg( "entered." );
}

void QgsPluginManager::getPythonPluginDescriptions()
{
  if ( !mPythonUtils || !mPythonUtils->isEnabled() )
    return;

  // look for plugins systemwide
  QStringList pluginList = mPythonUtils->pluginList();

  for ( int i = 0; i < pluginList.size(); i++ )
  {
    QString packageName = pluginList[i];

    // import plugin's package - skip loading it if an error occured
    if ( !mPythonUtils->loadPlugin( packageName ) )
      continue;

    // get information from the plugin
    QString pluginName  = mPythonUtils->getPluginMetadata( packageName, "name" );
    QString description = mPythonUtils->getPluginMetadata( packageName, "description" );
    QString version     = mPythonUtils->getPluginMetadata( packageName, "version" );

    if ( pluginName == "???" || description == "???" || version == "???" ) continue;

    bool isCompatible = QgsPluginRegistry::instance()->isPythonPluginCompatible( packageName );
    QString compatibleString; // empty by default
    if ( !isCompatible )
      compatibleString = "  " + tr( "[ incompatible ]" );

    // filtering will be done on the display role so give it name and desription
    // user wont see this text since we are using a custome delegate
    QStandardItem * mypDetailItem = new QStandardItem( pluginName + " - " + description );
    QString myLibraryName = "python:" + packageName;
    mypDetailItem->setData( myLibraryName, PLUGIN_LIBRARY_ROLE ); //for loading libs later
    mypDetailItem->setData( packageName, PLUGIN_BASE_NAME_ROLE ); //for matching in registry later
    mypDetailItem->setCheckable( false );
    mypDetailItem->setEditable( false );
    mypDetailItem->setEnabled( isCompatible );
    // setData in the delegate with a variantised QgsDetailedItemData
    QgsDetailedItemData myData;
    myData.setTitle( pluginName + " (" + version + ")" + compatibleString );
    myData.setEnabled( isCompatible );
    myData.setDetail( description );
    //myData.setIcon(pixmap); //todo use a python logo here
    myData.setCheckable( true );
    myData.setRenderAsWidget( false );
    myData.setChecked( false ); //start off assuming false

    // check to see if the plugin is loaded and set the checkbox accordingly
    QgsPluginRegistry *pRegistry = QgsPluginRegistry::instance();
    if ( pRegistry->isLoaded( packageName ) && pRegistry->isPythonPlugin( packageName ) )
    {
      QgsDebugMsg( "Found plugin in the registry" );
      // set the checkbox
      myData.setChecked( true );
    }
    else
    {
      QgsDebugMsg( "Couldn't find plugin in the registry: " + packageName );
    }
    QVariant myVariant = qVariantFromValue( myData );
    mypDetailItem->setData( myVariant, PLUGIN_DATA_ROLE );
    // Add item to model
    mModelPlugins->appendRow( mypDetailItem );
  }
}


void QgsPluginManager::getPluginDescriptions()
{
  QString sharedLibExtension;
#ifdef WIN32
  sharedLibExtension = "*.dll";
#else
  sharedLibExtension = "*.so*";
#endif

// check all libs in the current plugin directory and get name and descriptions
  QDir pluginDir( lblPluginDir->text(), sharedLibExtension, QDir::Name | QDir::IgnoreCase, QDir::Files | QDir::NoSymLinks );

  if ( pluginDir.count() == 0 )
  {
    QMessageBox::information( this, tr( "No Plugins" ), tr( "No QGIS plugins found in " ) + lblPluginDir->text() );
    return;
  }

  QgsDebugMsg( "PLUGIN MANAGER:" );
  for ( uint i = 0; i < pluginDir.count(); i++ )
  {
    QString lib = QString( "%1/%2" ).arg( lblPluginDir->text() ).arg( pluginDir[i] );

#ifdef TESTLIB
    // This doesn't work on WIN32 and causes problems with plugins
    // on OS X (the code doesn't cause a problem but including dlfcn.h
    // renders plugins unloadable)
#if !defined(WIN32) && !defined(Q_OS_MACX)
    // test code to help debug loading problems
    // This doesn't work on WIN32 and causes problems with plugins
    // on OS X (the code doesn't cause a problem but including dlfcn.h
    // renders plugins unloadable)

//          void *handle = dlopen((const char *) lib, RTLD_LAZY);
    void *handle = dlopen( lib.toLocal8Bit().data(), RTLD_LAZY | RTLD_GLOBAL );
    if ( !handle )
    {
      QgsDebugMsg( "Error in dlopen: " );
      QgsDebugMsg( dlerror() );
    }
    else
    {
      QgsDebugMsg( "dlopen suceeded for " + lib );
      dlclose( handle );
    }
#endif //#ifndef WIN32 && Q_OS_MACX
#endif //#ifdef TESTLIB

    QgsDebugMsg( "Examining: " + lib );
    QLibrary *myLib = new QLibrary( lib );
    bool loaded = myLib->load();
    if ( !loaded )
    {
      QgsDebugMsg( "Failed to load: " + myLib->fileName() );
      delete myLib;
      continue;
    }

    QgsDebugMsg( "Loaded library: " + myLib->fileName() );

    // Don't bother with libraries that are providers
    //if(!myLib->resolve("isProvider"))

    //MH: Replaced to allow for plugins that are linked to providers
    //type is only used in non-provider plugins
    if ( !myLib->resolve( "type" ) )
    {
      delete myLib;
      continue;
    }

    // resolve the metadata from plugin
    name_t *pName = ( name_t * ) cast_to_fptr( myLib->resolve( "name" ) );
    description_t *pDesc = ( description_t * ) cast_to_fptr( myLib->resolve( "description" ) );
    version_t *pVersion = ( version_t * ) cast_to_fptr( myLib->resolve( "version" ) );

    // show the values (or lack of) for each function
    if ( pName )
    {
      QgsDebugMsg( "Plugin name: " + pName() );
    }
    else
    {
      QgsDebugMsg( "Plugin name not returned when queried" );
    }
    if ( pDesc )
    {
      QgsDebugMsg( "Plugin description: " + pDesc() );
    }
    else
    {
      QgsDebugMsg( "Plugin description not returned when queried" );
    }
    if ( pVersion )
    {
      QgsDebugMsg( "Plugin version: " + pVersion() );
    }
    else
    {
      QgsDebugMsg( "Plugin version not returned when queried" );
    }

    if ( !pName || !pDesc || !pVersion )
    {
      QgsDebugMsg( "Failed to get name, description, or type for " + myLib->fileName() );
      delete myLib;
      continue;
    }

    QString pluginName = pName();
    QString pluginDesc = pDesc();
    QString pluginVersion = pVersion();
    QString baseName = QFileInfo( lib ).baseName();

    QString myLibraryName = pluginDir[i];
    // filtering will be done on the display role so give it name and desription
    // user wont see this text since we are using a custome delegate
    QStandardItem * mypDetailItem = new QStandardItem( pluginName + " - " + pluginDesc );
    mypDetailItem->setData( myLibraryName, PLUGIN_LIBRARY_ROLE );
    mypDetailItem->setData( baseName, PLUGIN_BASE_NAME_ROLE ); //for matching in registry later
    QgsDetailedItemData myData;
    myData.setTitle( pluginName );
    myData.setDetail( pluginDesc );
    myData.setRenderAsWidget( false );
    myData.setCheckable( true );
    myData.setChecked( false ); //start unchecked - we will check it later if needed

    QgsDebugMsg( "Getting an instance of the QgsPluginRegistry" );

    // check to see if the plugin is loaded and set the checkbox accordingly
    QgsPluginRegistry *pRegistry = QgsPluginRegistry::instance();

    // get the library using the plugin description
    if ( !pRegistry->isLoaded( baseName ) )
    {
      QgsDebugMsg( "Couldn't find plugin in the registry" );
    }
    else
    {
      QgsDebugMsg( "Found plugin in the registry" );
      // TODO: this check shouldn't be necessary, plugin base names must be unique
      if ( pRegistry->library( baseName ) == myLib->fileName() )
      {
        // set the checkbox
        myData.setChecked( true );
      }
    }
    QVariant myVariant = qVariantFromValue( myData );
    mypDetailItem->setData( myVariant, PLUGIN_DATA_ROLE );
    // Add items to model
    mModelPlugins->appendRow( mypDetailItem );

    delete myLib;
  }
}

void QgsPluginManager::accept()
{
  unload();
  done( 1 );
}

void QgsPluginManager::unload()
{
  QSettings settings;
  QgsDebugMsg( "Checking for plugins to unload" );
  for ( int row = 0;row < mModelPlugins->rowCount();row++ )
  {
    // FPV - I want to use index. You can do evrething with item.
    QModelIndex myIndex = mModelPlugins->index( row, 0 );
    QgsDetailedItemData myData =
      qVariantValue<QgsDetailedItemData>( mModelPlugins->data( myIndex, PLUGIN_DATA_ROLE ) );
    if ( !myData.isChecked() )
    {
      // iThe plugin name without version string in its data PLUGIN_LIB [ts]
      myIndex = mModelPlugins->index( row, 0 );
      // its off -- see if it is loaded and if so, unload it
      QgsPluginRegistry *pRegistry = QgsPluginRegistry::instance();

      // is loaded?
      QString baseName = mModelPlugins->data( myIndex, PLUGIN_BASE_NAME_ROLE ).toString();
      if ( ! pRegistry->isLoaded( baseName ) )
        continue;

      if ( pRegistry->isPythonPlugin( baseName ) )
      {
        if ( mPythonUtils && mPythonUtils->isEnabled() )
        {
          mPythonUtils->unloadPlugin( baseName );
          //disable it to the qsettings file
          settings.setValue( "/PythonPlugins/" + baseName, false );
        }
      }
      else // C++ plugin
      {
        QgisPlugin *plugin = pRegistry->plugin( baseName );
        if ( plugin )
        {
          plugin->unload();
        }
        //disable it to the qsettings file [ts]
        settings.setValue( "/Plugins/" + baseName, false );
      }
      // remove the plugin from the registry
      pRegistry->removePlugin( baseName );
    }
  }
  QgsPluginRegistry::instance()->dump();
}

std::vector < QgsPluginItem > QgsPluginManager::getSelectedPlugins()
{
  std::vector < QgsPluginItem > pis;
  // FPV - I want to use item here. You can do everything with index if you want.
  for ( int row = 0;row < mModelPlugins->rowCount();row++ )
  {
    QgsDetailedItemData myData =
      qVariantValue<QgsDetailedItemData>( mModelPlugins->item( row, 0 )->data( PLUGIN_DATA_ROLE ) );

    if ( myData.isChecked() )
    {
      //QString baseName = mModelPlugins->item( row, 0 )->data( PLUGIN_BASE_NAME_ROLE ).toString();
      QString pluginName = mModelPlugins->item( row, 0 )->data( Qt::DisplayRole ).toString();
      bool pythonic = false;

      QString library = mModelPlugins->item( row, 0 )->data( PLUGIN_LIBRARY_ROLE ).toString();
      if ( library.left( 7 ) == "python:" )
      {
        library = library.mid( 7 );
        pythonic = true;
      }
      else // C++ plugin
      {
        library = lblPluginDir->text() + QDir::separator() + library;
      }
      // Ctor params for plugin item:
      //QgsPluginItem(QString name=0,
      //              QString description=0,
      //              QString fullPath=0,
      //              QString type=0,
      //              bool python=false);
      pis.push_back( QgsPluginItem( pluginName, library, 0, pythonic ) );
    }

  }
  return pis;
}

void QgsPluginManager::selectAll()
{
  // select all plugins
  for ( int row = 0;row < mModelPlugins->rowCount();row++ )
  {
    QStandardItem *mypItem = mModelPlugins->item( row, 0 );
    QgsDetailedItemData myData =
      qVariantValue<QgsDetailedItemData>( mypItem->data( PLUGIN_DATA_ROLE ) );
    myData.setChecked( true );
    QVariant myVariant = qVariantFromValue( myData );
    mypItem->setData( myVariant, PLUGIN_DATA_ROLE );

  }
}

void QgsPluginManager::clearAll()
{
  // clear all selection checkboxes
  for ( int row = 0;row < mModelPlugins->rowCount();row++ )
  {
    QStandardItem *mypItem = mModelPlugins->item( row, 0 );
    QgsDetailedItemData myData =
      qVariantValue<QgsDetailedItemData>( mypItem->data( PLUGIN_DATA_ROLE ) );
    myData.setChecked( false );
    QVariant myVariant = qVariantFromValue( myData );
    mypItem->setData( myVariant, PLUGIN_DATA_ROLE );
  }
}

void QgsPluginManager::on_vwPlugins_clicked( const QModelIndex &theIndex )
{
  if ( theIndex.column() == 0 )
  {
    //
    // If the model has been filtered, the index row in the proxy wont match
    // the index row in the underlying model so we need to jump through this
    // little hoop to get the correct item
    //

    QModelIndex realIndex = mModelProxy->mapToSource( theIndex );
    QStandardItem* mypItem = mModelPlugins->itemFromIndex( realIndex );
    QgsDetailedItemData myData =
      qVariantValue<QgsDetailedItemData>( mypItem->data( PLUGIN_DATA_ROLE ) );
    if ( myData.isEnabled() )
    {
      myData.setChecked( ! myData.isChecked() );
    }
    QVariant myVariant = qVariantFromValue( myData );
    mypItem->setData( myVariant, PLUGIN_DATA_ROLE );
  }
}

void QgsPluginManager::on_leFilter_textChanged( QString theText )
{
  QgsDebugMsg( "PluginManager filter changed to :" + theText );
  QRegExp::PatternSyntax mySyntax = QRegExp::PatternSyntax( QRegExp::RegExp );
  Qt::CaseSensitivity myCaseSensitivity = Qt::CaseInsensitive;
  QRegExp myRegExp( theText, myCaseSensitivity, mySyntax );
  mModelProxy->setFilterRegExp( myRegExp );
}
