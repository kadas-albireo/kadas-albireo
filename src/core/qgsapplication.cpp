/***************************************************************************
    qgsapplication.cpp - Accessors for application-wide data
     --------------------------------------
    Date                 : 02-Jan-2006
    Copyright            : (C) 2006 by Tom Elwertowski
    Email                : telwertowski at users dot sourceforge dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsapplication.h"
#include "qgslogger.h"
#include "qgsmaplayerregistry.h"
#include "qgsproviderregistry.h"
#include "qgsexception.h"

#include <QDir>
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QPalette>
#include <QSettings>

#ifndef Q_WS_WIN
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

#include "qgsconfig.h"

#include <gdal.h>
#include <ogr_api.h>
#include <cpl_conv.h> // for setting gdal options

QObject * QgsApplication::mFileOpenEventReceiver;
QStringList QgsApplication::mFileOpenEventList;
QString QgsApplication::mPrefixPath;
QString QgsApplication::mPluginPath;
QString QgsApplication::mPkgDataPath;
QString QgsApplication::mLibraryPath;
QString QgsApplication::mLibexecPath;
QString QgsApplication::mThemeName;
QStringList QgsApplication::mDefaultSvgPaths;
QString QgsApplication::mConfigPath = QDir::homePath() + QString( "/.qgis/" );
bool QgsApplication::mRunningFromBuildDir = false;
QString QgsApplication::mBuildSourcePath;
QString QgsApplication::mBuildOutputPath;
QStringList QgsApplication::mGdalSkipList;

/*!
  \class QgsApplication
  \brief The QgsApplication class manages application-wide information.

  This is a subclass of QApplication and should be instantiated in place of
  QApplication. Most methods are static in keeping witn the design of QApplication.

  This class hides platform-specific path information and provides
  a portable way of referencing specific files and directories.
  Ideally, hard-coded paths should appear only here and not in other modules
  so that platform-conditional code is minimized and paths are easier
  to change due to centralization.
*/
QgsApplication::QgsApplication( int & argc, char ** argv, bool GUIenabled, QString customConfigPath )
    : QApplication( argc, argv, GUIenabled )
{
  init( customConfigPath ); // init can also be called directly by e.g. unit tests that don't inherit QApplication.
}
void QgsApplication::init( QString customConfigPath )
{
  // check if QGIS is run from build directory (not the install directory)
  QDir appDir( applicationDirPath() );
  if ( appDir.exists( "source_path.txt" ) )
  {
    QFile f( applicationDirPath() + "/source_path.txt" );
    if ( f.open( QIODevice::ReadOnly ) )
    {
      mRunningFromBuildDir = true;
      mBuildSourcePath = f.readAll();
#if defined(Q_WS_MACX) || defined(Q_WS_WIN32) || defined(WIN32)
      mBuildOutputPath = applicationDirPath();
#else
      mBuildOutputPath = applicationDirPath() + "/.."; // on linux
#endif
      qDebug( "Running from build directory!" );
      qDebug( "- source directory: %s", mBuildSourcePath.toAscii().data() );
      qDebug( "- output directory of the build: %s", mBuildOutputPath.toAscii().data() );
    }
  }

  if ( mRunningFromBuildDir )
  {
    // we run from source directory - not installed to destination (specified prefix)
    mPrefixPath = QString(); // set invalid path
    setPluginPath( mBuildOutputPath + "/" + QString( QGIS_PLUGIN_SUBDIR ) );
    setPkgDataPath( mBuildSourcePath ); // directly source path - used for: doc, resources, svg
    mLibraryPath = mBuildOutputPath + "/" + QGIS_LIB_SUBDIR + "/";
    mLibexecPath = mBuildOutputPath + "/" + QGIS_LIBEXEC_SUBDIR + "/";
  }
  else
  {
#if defined(Q_WS_MACX) || defined(Q_WS_WIN32) || defined(WIN32)
    setPrefixPath( applicationDirPath(), true );
#else
    QDir myDir( applicationDirPath() );
    myDir.cdUp();
    QString myPrefix = myDir.absolutePath();
    setPrefixPath( myPrefix, true );
#endif
  }

  if ( !customConfigPath.isEmpty() )
  {
    mConfigPath = customConfigPath + "/"; // make sure trailing slash is included
  }

  mDefaultSvgPaths << qgisSettingsDirPath() + QString( "svg/" );

  // set a working directory up for gdal to write .aux.xml files into
  // for cases where the raster dir is read only to the user
  // if the env var is already set it will be used preferentially
  QString myPamPath = qgisSettingsDirPath() + QString( "gdal_pam/" );
  QDir myDir( myPamPath );
  if ( !myDir.exists() )
  {
    myDir.mkpath( myPamPath ); //fail silently
  }


#if defined(Q_WS_WIN32) || defined(WIN32)
  CPLSetConfigOption( "GDAL_PAM_PROXY_DIR", myPamPath.toUtf8() );
#else
  //under other OS's we use an environment var so the user can
  //override the path if he likes
  int myChangeFlag = 0; //whether we want to force the env var to change
  setenv( "GDAL_PAM_PROXY_DIR", myPamPath.toUtf8(), myChangeFlag );
#endif
}

QgsApplication::~QgsApplication()
{
}

bool QgsApplication::event( QEvent * event )
{
  bool done = false;
  if ( event->type() == QEvent::FileOpen )
  {
    // handle FileOpen event (double clicking a file icon in Mac OS X Finder)
    if ( mFileOpenEventReceiver )
    {
      // Forward event to main window.
      done = notify( mFileOpenEventReceiver, event );
    }
    else
    {
      // Store filename because receiver has not registered yet.
      // If QGIS has been launched by double clicking a file icon, FileOpen will be
      // the first event; the main window is not yet ready to handle the event.
      mFileOpenEventList.append( static_cast<QFileOpenEvent *>( event )->file() );
      done = true;
    }
  }
  else
  {
    // pass other events to base class
    done = QApplication::event( event );
  }
  return done;
}

bool QgsApplication::notify( QObject * receiver, QEvent * event )
{
  bool done = false;
  emit preNotify( receiver, event, &done );

  if ( done )
    return true;

  // Send event to receiver and catch unhandled exceptions
  done = true;
  try
  {
    done = QApplication::notify( receiver, event );
  }
  catch ( QgsException & e )
  {
    QMessageBox::critical( activeWindow(), tr( "Exception" ), e.what() );
  }
  catch ( std::exception & e )
  {
    QMessageBox::critical( activeWindow(), tr( "Exception" ), e.what() );
  }
  catch ( ... )
  {
    QMessageBox::critical( activeWindow(), tr( "Exception" ), tr( "unknown exception" ) );
  }

  return done;
}

void QgsApplication::setFileOpenEventReceiver( QObject * receiver )
{
  // Set receiver for FileOpen events
  mFileOpenEventReceiver = receiver;
  // Propagate any events collected before the receiver has registered.
  if ( mFileOpenEventList.count() > 0 )
  {
    QStringListIterator i( mFileOpenEventList );
    while ( i.hasNext() )
    {
      QFileOpenEvent foe( i.next() );
      QgsApplication::sendEvent( mFileOpenEventReceiver, &foe );
    }
    mFileOpenEventList.clear();
  }
}

void QgsApplication::setPrefixPath( const QString thePrefixPath, bool useDefaultPaths )
{
  mPrefixPath = thePrefixPath;
#if defined(_MSC_VER)
  if ( mPrefixPath.endsWith( "/bin" ) )
  {
    mPrefixPath.chop( 4 );
  }
#endif
  if ( useDefaultPaths )
  {
    setPluginPath( mPrefixPath + "/" + QString( QGIS_PLUGIN_SUBDIR ) );
    setPkgDataPath( mPrefixPath + "/" + QString( QGIS_DATA_SUBDIR ) );
  }
  mLibraryPath = mPrefixPath + "/" + QGIS_LIB_SUBDIR + "/";
  mLibexecPath = mPrefixPath + "/" + QGIS_LIBEXEC_SUBDIR + "/";
}

void QgsApplication::setPluginPath( const QString thePluginPath )
{
  mPluginPath = thePluginPath;
}

void QgsApplication::setPkgDataPath( const QString thePkgDataPath )
{
  mPkgDataPath = thePkgDataPath;
  QString mySvgPath = thePkgDataPath + ( mRunningFromBuildDir ? "/images/svg/" : "/svg/" );
  // avoid duplicate entries
  if ( !mDefaultSvgPaths.contains( mySvgPath ) )
    mDefaultSvgPaths << mySvgPath;
}

void QgsApplication::setDefaultSvgPaths( const QStringList& pathList )
{
  mDefaultSvgPaths = pathList;
}

const QString QgsApplication::prefixPath()
{
  if ( mRunningFromBuildDir )
  {
    qWarning( "!!! prefix path was requested, but it is not valid - we do not run from installed path !!!" );
  }

  return mPrefixPath;
}
const QString QgsApplication::pluginPath()
{
  return mPluginPath;
}
const QString QgsApplication::pkgDataPath()
{
  return mPkgDataPath;
}
const QString QgsApplication::defaultThemePath()
{
  return ":/images/themes/default/";
}
const QString QgsApplication::activeThemePath()
{
  return ":/images/themes/" + mThemeName + "/";
}


QString QgsApplication::iconPath( QString iconFile )
{
  // try active theme
  QString path = activeThemePath();
  if ( QFile::exists( path + iconFile ) )
    return path + iconFile;

  // use default theme
  return defaultThemePath() + iconFile;
}

/*!
  Set the theme path to the specified theme.
*/
void QgsApplication::setThemeName( const QString theThemeName )
{
  QString myPath = ":/images/themes/" + theThemeName + "/";
  //check it exists and if not roll back to default theme
  if ( QFile::exists( myPath ) )
  {
    mThemeName = theThemeName;
  }
  else
  {
    mThemeName = "default";
  }
}
/*!
 * Get the active theme name
 */
const QString QgsApplication::themeName()
{
  return mThemeName;
}
/*!
  Returns the path to the authors file.
*/
const QString QgsApplication::authorsFilePath()
{
  return mPkgDataPath + QString( "/doc/AUTHORS" );
}
/*!
  Returns the path to the contributors file.
*/
const QString QgsApplication::contributorsFilePath()
{
  return mPkgDataPath + QString( "/doc/CONTRIBUTORS" );
}
/*!
  Returns the path to the sponsors file.
*/
const QString QgsApplication::sponsorsFilePath()
{
  return mPkgDataPath + QString( "/doc/SPONSORS" );
}

/*!
  Returns the path to the donors file.
*/
const QString QgsApplication::donorsFilePath()
{
  return mPkgDataPath + QString( "/doc/DONORS" );
}

/*!
  Returns the path to the sponsors file.
  @note Added in QGIS 1.1
*/
const QString QgsApplication::translatorsFilePath()
{
  return mPkgDataPath + QString( "/doc/TRANSLATORS" );
}

const QString QgsApplication::developerPath()
{
  return QString(); // developer images are no longer shipped!
}

/*!
  Returns the path to the help application.
*/
const QString QgsApplication::helpAppPath()
{
  QString helpAppPath;
#ifdef Q_OS_MACX
  helpAppPath = applicationDirPath() + "/bin/qgis_help.app/Contents/MacOS";
#else
  helpAppPath = libexecPath();
#endif
  helpAppPath += "/qgis_help";
  return helpAppPath;
}
/*!
  Returns the path to the translation directory.
*/
const QString QgsApplication::i18nPath()
{
  if ( mRunningFromBuildDir )
    return mBuildOutputPath + QString( "/i18n" );
  else
    return mPkgDataPath + QString( "/i18n/" );
}

/*!
  Returns the path to the master qgis.db file.
*/
const QString QgsApplication::qgisMasterDbFilePath()
{
  return mPkgDataPath + QString( "/resources/qgis.db" );
}

/*!
  Returns the path to the settings directory in user's home dir
 */
const QString QgsApplication::qgisSettingsDirPath()
{
  return mConfigPath;
}

/*!
  Returns the path to the user qgis.db file.
*/
const QString QgsApplication::qgisUserDbFilePath()
{
  return qgisSettingsDirPath() + QString( "qgis.db" );
}

/*!
  Returns the path to the splash screen image directory.
*/
const QString QgsApplication::splashPath()
{
  return QString( ":/images/splash/" );
}

/*!
  Returns the path to the icons image directory.
*/
const QString QgsApplication::iconsPath()
{
  return mPkgDataPath + QString( "/images/icons/" );
}
/*!
  Returns the path to the srs.db file.
*/
const QString QgsApplication::srsDbFilePath()
{
  return mPkgDataPath + QString( "/resources/srs.db" );
}

/*!
  Returns the paths to the svg directories.
*/
const QStringList QgsApplication::svgPaths()
{
  //local directories to search when looking for an SVG with a given basename
  //defined by user in options dialog
  QSettings settings;
  QStringList myPathList;
  QString myPaths = settings.value( "svg/searchPathsForSVG", "" ).toString();
  if ( !myPaths.isEmpty() )
  {
    myPathList = myPaths.split( "|" );
  }

  myPathList << mDefaultSvgPaths;
  return myPathList;
}

/*!
  Returns the path to the applications svg directories.
*/
const QString QgsApplication::svgPath()
{
  QString svgSubDir( mRunningFromBuildDir ? "/images/svg/" : "/svg/" );
  return mPkgDataPath + svgSubDir;
}

const QString QgsApplication::userStyleV2Path()
{
  return qgisSettingsDirPath() + QString( "symbology-ng-style.xml" );
}

const QString QgsApplication::defaultStyleV2Path()
{
  return mPkgDataPath + QString( "/resources/symbology-ng-style.xml" );
}

const QString QgsApplication::libraryPath()
{
  return mLibraryPath;
}

const QString QgsApplication::libexecPath()
{
  return mLibexecPath;
}

QgsApplication::endian_t QgsApplication::endian()
{
  return ( htonl( 1 ) == 1 ) ? XDR : NDR ;
}

void QgsApplication::initQgis()
{
  // set the provider plugin path (this creates provider registry)
  QgsProviderRegistry::instance( pluginPath() );

  // create map layer registry if doesn't exist
  QgsMapLayerRegistry::instance();
}

void QgsApplication::exitQgis()
{
  delete QgsMapLayerRegistry::instance();
  delete QgsProviderRegistry::instance();
}

QString QgsApplication::showSettings()
{
  QString myState = QString( "Application state:\n"
                             "Prefix              : %1\n"
                             "Plugin Path         : %2\n"
                             "Package Data Path   : %3\n"
                             "Active Theme Name   : %4\n"
                             "Active Theme Path   : %5\n"
                             "Default Theme Path  : %6\n"
                             "SVG Search Paths    : %7\n"
                             "User DB Path        : %8\n" )
                    .arg( mPrefixPath )
                    .arg( mPluginPath )
                    .arg( mPkgDataPath )
                    .arg( themeName() )
                    .arg( activeThemePath() )
                    .arg( defaultThemePath() )
                    .arg( svgPaths().join( "\n" ) )
                    .arg( qgisMasterDbFilePath() );
  return myState;
}

QString QgsApplication::reportStyleSheet()
{
  //
  // Make the style sheet desktop preferences aware by using qappliation
  // palette as a basis for colors where appropriate
  //
  QColor myColor1 = palette().highlight().color();
  QColor myColor2 = myColor1;
  myColor2 = myColor2.lighter( 110 ); //10% lighter
  QString myStyle;
  myStyle = ".glossy{ background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
            "stop: 0 " + myColor1.name()  + ","
            "stop: 0.1 " + myColor2.name() + ","
            "stop: 0.5 " + myColor1.name()  + ","
            "stop: 0.9 " + myColor2.name() + ","
            "stop: 1 " + myColor1.name() + ");"
            "color: white;"
            "padding-left: 4px;"
            "padding-top: 20px;"
            "padding-bottom: 8px;"
            "border: 1px solid #6c6c6c;"
            "}"
            ".overview{ font: 1.82em; font-weight: bold;}"
            "body{  background: white;"
            "  color: black;"
            "  font-family: arial,sans-serif;"
            "}"
            "h2{  background-color: #F6F6F6;"
            "  color: #8FB171; "
            "  font-size: medium;  "
            "  font-weight: normal;"
            "  font-family: luxi serif, georgia, times new roman, times, serif;"
            "  background: none;"
            "  padding: 0.75em 0 0;"
            "  margin: 0;"
            "  line-height: 1.1em;"
            "}"
            "h3{  background-color: #F6F6F6;"
            "  color: #729FCF;"
            "  font-family: luxi serif, georgia, times new roman, times, serif;"
            "  font-weight: bold;"
            "  font-size: large;"
            "  text-align: right;"
            "  border-bottom: 5px solid #DCEB5C;"
            "}"
            "h4{  background-color: #F6F6F6;"
            "  color: #729FCF;"
            "  font-family: luxi serif, georgia, times new roman, times, serif;"
            "  font-weight: bold;"
            "  font-size: medium;"
            "  text-align: right;"
            "}"
            "h5{    background-color: #F6F6F6;"
            "   color: #729FCF;"
            "   font-family: luxi serif, georgia, times new roman, times, serif;"
            "   font-weight: bold;"
            "   font-size: small;"
            "   text-align: right;"
            "}"
            "a{  color: #729FCF;"
            "  font-family: arial,sans-serif;"
            "  font-size: small;"
            "}"
            "label{  background-color: #FFFFCC;"
            "  border: 1px solid black;"
            "  margin: 1px;"
            "  padding: 0px 3px; "
            "  font-size: small;"
            "}";
  return myStyle;
}

void QgsApplication::registerOgrDrivers()
{
  if ( 0 >= OGRGetDriverCount() )
  {
    OGRRegisterAll();
  }
}

QString QgsApplication::absolutePathToRelativePath( QString aPath, QString targetPath )
{
#if defined( Q_OS_WIN )
  const Qt::CaseSensitivity cs = Qt::CaseInsensitive;

  aPath.replace( "\\", "/" );
  if ( aPath.startsWith( "//" ) )
  {
    // keep UNC prefix
    aPath = "\\\\" + aPath.mid( 2 );
  }

  targetPath.replace( "\\", "/" );
  if ( targetPath.startsWith( "//" ) )
  {
    // keep UNC prefix
    targetPath = "\\\\" + targetPath.mid( 2 );
  }
#else
  const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif

  QStringList targetElems = targetPath.split( "/", QString::SkipEmptyParts );
  QStringList aPathElems = aPath.split( "/", QString::SkipEmptyParts );

  targetElems.removeAll( "." );
  aPathElems.removeAll( "." );

  // remove common part
  int n = 0;
  while ( aPathElems.size() > 0 &&
          targetElems.size() > 0 &&
          aPathElems[0].compare( targetElems[0], cs ) == 0 )
  {
    aPathElems.removeFirst();
    targetElems.removeFirst();
    n++;
  }

  if ( n == 0 )
  {
    // no common parts; might not even be a file
    return aPath;
  }

  if ( targetElems.size() > 0 )
  {
    // go up to the common directory
    for ( int i = 0; i < targetElems.size(); i++ )
    {
      aPathElems.insert( 0, ".." );
    }
  }
  else
  {
    // let it start with . nevertheless,
    // so relative path always start with either ./ or ../
    aPathElems.insert( 0, "." );
  }

  return aPathElems.join( "/" );
}

QString QgsApplication::relativePathToAbsolutePath( QString rpath, QString targetPath )
{
  // relative path should always start with ./ or ../
  if ( !rpath.startsWith( "./" ) && !rpath.startsWith( "../" ) )
  {
    return rpath;
  }

#if defined(Q_OS_WIN)
  rpath.replace( "\\", "/" );
  targetPath.replace( "\\", "/" );

  bool uncPath = targetPath.startsWith( "//" );
#endif

  QStringList srcElems = rpath.split( "/", QString::SkipEmptyParts );
  QStringList targetElems = targetPath.split( "/", QString::SkipEmptyParts );

#if defined(Q_OS_WIN)
  if ( uncPath )
  {
    targetElems.insert( 0, "" );
    targetElems.insert( 0, "" );
  }
#endif

  // append source path elements
  targetElems << srcElems;
  targetElems.removeAll( "." );

  // resolve ..
  int pos;
  while (( pos = targetElems.indexOf( ".." ) ) > 0 )
  {
    // remove preceding element and ..
    targetElems.removeAt( pos - 1 );
    targetElems.removeAt( pos - 1 );
  }

#if !defined(Q_OS_WIN)
  // make path absolute
  targetElems.prepend( "" );
#endif

  return targetElems.join( "/" );
}

void QgsApplication::skipGdalDriver( QString theDriver )
{
  if ( mGdalSkipList.contains( theDriver ) || theDriver.isEmpty() )
  {
    return;
  }
  mGdalSkipList << theDriver;
  applyGdalSkippedDrivers();
}

void QgsApplication::restoreGdalDriver( QString theDriver )
{
  if ( !mGdalSkipList.contains( theDriver ) )
  {
    return;
  }
  int myPos = mGdalSkipList.indexOf( theDriver );
  if ( myPos >= 0 )
  {
    mGdalSkipList.removeAt( myPos );
  }
  applyGdalSkippedDrivers();
}

void QgsApplication::applyGdalSkippedDrivers()
{
  mGdalSkipList.removeDuplicates();
  QString myDriverList = mGdalSkipList.join( " " );
  QgsDebugMsg( "Gdal Skipped driver list set to:" );
  QgsDebugMsg( myDriverList );
  CPLSetConfigOption( "GDAL_SKIP", myDriverList.toUtf8() );
  GDALAllRegister(); //to update driver list and skip missing ones
}
