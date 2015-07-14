/***************************************************************************
                          qgsabout.cpp  -  description
                             -------------------
    begin                : Sat Aug 10 2002
    copyright            : (C) 2002 by Gary E.Sherman
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

#include "qgsabout.h"
#include "qgsapplication.h"
#include "qgsproviderregistry.h"
#include "qgslogger.h"
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QImageReader>
#include <QSqlDatabase>
#include <QTcpSocket>

/* Uncomment this block to use preloaded images
#include <map>
std::map<QString, QPixmap> mugs;
*/
#ifdef Q_OS_MACX
QgsAbout::QgsAbout( QWidget *parent )
    : QgsOptionsDialogBase( "about", parent, Qt::WindowSystemMenuHint )  // Modeless dialog with close button only
#else
QgsAbout::QgsAbout( QWidget *parent )
    : QgsOptionsDialogBase( "about", parent )  // Normal dialog in non Mac-OS
#endif
{
  setupUi( this );
  QString title = QString( "%1 - %2 Bit" ).arg( windowTitle() ).arg( QSysInfo::WordSize );
  initOptionsBase( true, title );
  init();
}

QgsAbout::~QgsAbout()
{
}

void QgsAbout::init()
{
  setPluginInfo();
  QString myStyle = QgsApplication::reportStyleSheet();
}

void QgsAbout::setLicence()
{
  // read the DONORS file and populate the text widget
  QFile licenceFile( QgsApplication::licenceFilePath() );
#ifdef QGISDEBUG
  printf( "Reading licence file %s.............................................\n",
          licenceFile.fileName().toLocal8Bit().constData() );
#endif
  if ( licenceFile.open( QIODevice::ReadOnly ) )
  {
    QString content = licenceFile.readAll();
    //txtLicense->setText( content );
  }
}

void QgsAbout::setVersion( QString v )
{
  txtVersion->setBackgroundRole( QPalette::NoRole );
  txtVersion->setAutoFillBackground( true );
  txtVersion->setHtml( v );
}

void QgsAbout::setWhatsNew()
{
  QString myStyle = QgsApplication::reportStyleSheet();
  /*txtWhatsNew->clear();
  txtWhatsNew->document()->setDefaultStyleSheet( myStyle );
  txtWhatsNew->setSource( "file:///" + QgsApplication::pkgDataPath() + "/doc/news.html" );*/
}

void QgsAbout::setPluginInfo()
{
  QString myString;
  //provide info about the plugins available
  myString += "<b>" + tr( "Available QGIS Data Provider Plugins" ) + "</b><br>";
  myString += QgsProviderRegistry::instance()->pluginList( true );
  //qt database plugins
  myString += "<b>" + tr( "Available Qt Database Plugins" ) + "</b><br>";
  myString += "<ol>\n<li>\n";
  QStringList myDbDriverList = QSqlDatabase::drivers();
  myString += myDbDriverList.join( "</li>\n<li>" );
  myString += "</li>\n</ol>\n";
  //qt image plugins
  myString += "<b>" + tr( "Available Qt Image Plugins" ) + "</b><br>";
  myString += tr( "Qt Image Plugin Search Paths <br>" );
  myString += QApplication::libraryPaths().join( "<br>" );
  myString += "<ol>\n<li>\n";
  QList<QByteArray> myImageFormats = QImageReader::supportedImageFormats();
  QList<QByteArray>::const_iterator myIterator = myImageFormats.begin();
  while ( myIterator != myImageFormats.end() )
  {
    QString myFormat = ( *myIterator ).data();
    myString += myFormat + "</li>\n<li>";
    ++myIterator;
  }
  myString += "</li>\n</ol>\n";

  QString myStyle = QgsApplication::reportStyleSheet();
  txtProviders->clear();
  txtProviders->document()->setDefaultStyleSheet( myStyle );
  txtProviders->setText( myString );
}

void QgsAbout::on_btnQgisUser_clicked()
{
  // find a browser
  QString url = "http://lists.osgeo.org/mailman/listinfo/qgis-user";
  openUrl( url );
}

void QgsAbout::on_btnQgisHome_clicked()
{
  openUrl( "http://qgisenterprise.com" );
}

void QgsAbout::openUrl( QString url )
{
  //use the users default browser
  QDesktopServices::openUrl( url );
}

/*
 * The function below makes a name safe for using in most file system
 * Step 1: Code QString as UTF-8
 * Step 2: Replace all bytes of the UTF-8 above 0x7f with the hexcode in lower case.
 * Step 2: Replace all non [a-z][a-Z][0-9] with underscore (backward compatibility)
 */
QString QgsAbout::fileSystemSafe( QString fileName )
{
  QString result;
  QByteArray utf8 = fileName.toUtf8();

  for ( int i = 0; i < utf8.size(); i++ )
  {
    uchar c = utf8[i];

    if ( c > 0x7f )
    {
      result = result + QString( "%1" ).arg( c, 2, 16, QChar( '0' ) );
    }
    else
    {
      result = result + QString( c );
    }
  }
  result.replace( QRegExp( "[^a-z0-9A-Z]" ), "_" );
  QgsDebugMsg( result );

  return result;
}

void QgsAbout::on_developersMapView_linkClicked( const QUrl &url )
{
  QString link = url.toString();
  openUrl( link );
}

void QgsAbout::setDevelopersMap()
{
  /*developersMapView->settings()->setAttribute( QWebSettings::JavascriptEnabled, true );
  QUrl url = QUrl::fromLocalFile( QgsApplication::developersMapFilePath() );
  developersMapView->load( url );*/
}
