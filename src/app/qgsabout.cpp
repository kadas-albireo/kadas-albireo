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
/* $Id$ */

#include "qgsabout.h"
#include "qgsapplication.h"
#include "qgsproviderregistry.h"
#include "qgslogger.h"
#include <QDesktopServices>
#include <QFile>
#include <QTextStream>
#include <QImageReader>
#include <QSqlDatabase>

/* Uncomment this block to use preloaded images
#include <map>
#include "qgslogger.h"
std::map<QString, QPixmap> mugs;
*/
#ifdef Q_OS_MACX
QgsAbout::QgsAbout()
    : QDialog( NULL, Qt::WindowSystemMenuHint )  // Modeless dialog with close button only
#else
QgsAbout::QgsAbout()
    : QDialog( NULL )  // Normal dialog in non Mac-OS
#endif
{
  setupUi( this );
  init();
}

QgsAbout::~QgsAbout()
{
}

void QgsAbout::init()
{
  setPluginInfo();

  // set the 60x60 icon pixmap
  QPixmap icon( QgsApplication::iconsPath() + "qgis-icon-60x60.png" );
  qgisIcon->setPixmap( icon );

  //read the authors file to populate the contributors list
  QStringList lines;

  QFile file( QgsApplication::authorsFilePath() );
#ifdef QGISDEBUG
  printf( "Reading authors file %s.............................................\n",
          file.fileName().toLocal8Bit().constData() );
#endif
  if ( file.open( QIODevice::ReadOnly ) )
  {
    QTextStream stream( &file );
    // Always use UTF-8
    stream.setCodec( "UTF-8" );
    QString line;
#ifdef QGISDEBUG
    int i = 1;
#endif

    while ( !stream.atEnd() )
    {
      line = stream.readLine(); // line of text excluding '\n'
      //ignore the line if it starts with a hash....
      if ( line.left( 1 ) == "#" ) continue;
#ifdef QGISDEBUG
      printf( "Contributor: %3d: %s\n", i++, line.toLocal8Bit().constData() );
#endif
      QStringList myTokens = line.split( "\t", QString::SkipEmptyParts );
      //printf ("Added contributor name to listbox: %s ",myTokens[0]);
      lines += myTokens[0];

      // add the image to the map
      /* Uncomment this block to preload the images (takes time at initial startup)
      QString authorName = myTokens[0].replace(" ","_");

      QString myString =QString(appPath + "/images/developers/") + authorName + QString(".jpg");
      printf ("Loading mug: %s\n", myString.toLocal8Bit().constData());
      QPixmap *pixmap = new QPixmap(myString);
      mugs[myTokens[0]] = *pixmap;
      */
    }
    file.close();
    listBox1->clear();
    listBox1->insertItems( 0, lines );

    // Load in the image for the first author
    if ( listBox1->count() > 0 )
      listBox1->setCurrentRow( 0 );
  }

  // read the SPONSORS file and populate the text widget
  QFile sponsorFile( QgsApplication::sponsorsFilePath() );
#ifdef QGISDEBUG
  printf( "Reading sponsors file %s.............................................\n",
          sponsorFile.fileName().toLocal8Bit().constData() );
#endif
  if ( sponsorFile.open( QIODevice::ReadOnly ) )
  {
    QString sponsorHTML = ""
                          + tr( "<p>The following have sponsored QGIS by contributing "
                                "money to fund development and other project costs</p>" )
                          + "<hr>"
                          "<table width='100%'>"
                          "<tr><th>" + tr( "Name" ) + "</th>"
                          "<th>" + tr( "Website" ) + "</th></tr>";
    QString website;
    QTextStream sponsorStream( &sponsorFile );
    // Always use UTF-8
    sponsorStream.setCodec( "UTF-8" );
    QString sline;
    while ( !sponsorStream.atEnd() )
    {
      sline = sponsorStream.readLine(); // line of text excluding '\n'
      //ignore the line if it starts with a hash....
      if ( sline.left( 1 ) == "#" ) continue;
      QStringList myTokens = sline.split( "|", QString::SkipEmptyParts );
      if ( myTokens.size() > 1 )
      {
        website = myTokens[1];
      }
      else
      {
        website = "&nbsp;";
      }
      sponsorHTML += "<tr>";
      sponsorHTML += "<td>" + myTokens[0] + "</td><td>" + website + "</td>";
      // close the row
      sponsorHTML += "</tr>";
    }
    sponsorHTML += "</table>";

    QString myStyle = QgsApplication::reportStyleSheet();
    txtSponsors->clear();
    txtSponsors->document()->setDefaultStyleSheet( myStyle );
    txtSponsors->setHtml( sponsorHTML );
    QgsDebugMsg( QString( "sponsorHTML:%1" ).arg( sponsorHTML.toAscii().constData() ) );
    QgsDebugMsg( QString( "txtSponsors:%1" ).arg( txtSponsors->toHtml().toAscii().constData() ) );
  }
}

void QgsAbout::setVersion( QString v )
{
  lblVersion->setText( v );
}

void QgsAbout::setWhatsNew( QString txt )
{
  QString myStyle = QgsApplication::reportStyleSheet();
  txtWhatsNew->clear();
  txtWhatsNew->document()->setDefaultStyleSheet( myStyle );
  txtWhatsNew->setHtml( txt );
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
  txtBrowserPlugins->clear();
  txtBrowserPlugins->document()->setDefaultStyleSheet( myStyle );
  txtBrowserPlugins->setText( myString );
}

void QgsAbout::on_buttonCancel_clicked()
{
  reject();
}

void QgsAbout::on_listBox1_currentItemChanged( QListWidgetItem *theItem )
{
  //replace spaces in author name
#ifdef QGISDEBUG
  printf( "Loading mug: " );
#endif
  QString myString = listBox1->currentItem()->text();
  myString = myString.replace( " ", "_" );
  myString = QgsAbout::fileSystemSafe( myString );
#ifdef QGISDEBUG
  printf( "Loading mug: %s", myString.toLocal8Bit().constData() );
#endif
  myString = QgsApplication::developerPath() + myString + QString( ".jpg" );
#ifdef QGISDEBUG
  printf( "Loading mug: %s\n", myString.toLocal8Bit().constData() );
#endif

  /* Uncomment this block to use preloaded images
  pixAuthorMug->setPixmap(mugs[myString]);
  */
}

void QgsAbout::on_btnQgisUser_clicked()
{
  // find a browser
  QString url = "http://lists.osgeo.org/mailman/listinfo/qgis-user";
  openUrl( url );
}

void QgsAbout::on_btnQgisHome_clicked()
{
  openUrl( "http://qgis.org" );
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
