/***************************************************************************
     testqgswcsprovider.cpp
     --------------------------------------
    Date                 : July 2012
    Copyright            : (C) 2012 by Radim Blazek
    Email                : radim dot blazek at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <cmath>

#include <QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QApplication>

#include <qgsdatasourceuri.h>
#include <qgsrasterlayer.h>
#include <qgsrasterdataprovider.h>
#include <qgsrasterchecker.h>
#include <qgsproviderregistry.h>
#include <qgsapplication.h>

#define TINY_VALUE  std::numeric_limits<double>::epsilon() * 20

/** \ingroup UnitTests
 * This is a unit test for the QgsRasterLayer class.
 */
class TestQgsWcsProvider: public QObject
{
    Q_OBJECT;
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    void read();
  private:
    bool read( QString theIdentifier, QString theWcsUri, QString theFilePath, QString & theReport );
    QString mTestDataDir;
    QString mReport;
    QString mUrl;
};

//runs before all tests
void TestQgsWcsProvider::initTestCase()
{
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  QString mySettings = QgsApplication::showSettings();
  mySettings = mySettings.replace( "\n", "<br />" );
  mReport += "<h1>WCS provider tests</h1>\n";
  mReport += "<p>" + mySettings + "</p>";
  // Style is now inlined by QgsRasterChecker
#if 0
  mReport += "<style>";
  mReport += ".tab { border-spacing: 0px; border-width: 1px 1px 0 0; border-style: solid; }";
  mReport += ".cell { border-width: 0 0 1px 1px; border-style: solid; font-size: smaller; text-align: center}";
  mReport += ".ok { background: #00ff00; }";
  mReport += ".err { background: #ff0000; }";
  mReport += ".errmsg { color: #ff0000; }";
  mReport += "</style>";
#endif
  //create some objects that will be used in all tests...
  //create a raster layer that will be used in all tests...
  mTestDataDir = QString( TEST_DATA_DIR ) + "/raster";
  qDebug() << "mTestDataDir = " << mTestDataDir;

  mUrl =  QString( TEST_SERVER_URL ) + "/wcs";
}

//runs after all tests
void TestQgsWcsProvider::cleanupTestCase()
{
  QString myReportFile = QDir::tempPath() + "/qgistest.html";
  QFile myFile( myReportFile );
  if ( myFile.open( QIODevice::WriteOnly | QIODevice::Append ) )
  {
    QTextStream myQTextStream( &myFile );
    myQTextStream << mReport;
    myFile.close();
  }
}

void TestQgsWcsProvider::read( )
{
  bool ok = true;
  QStringList versions;

  versions << "1.0" << "1.1";

  QStringList identifiers;

  // identifiers in mapfile have the same name as files without .tif extension
  identifiers << "band1_byte_noct_epsg4326";
  identifiers << "band1_int16_noct_epsg4326";
  identifiers << "band1_float32_noct_epsg4326";
  identifiers << "band3_byte_noct_epsg4326";
  identifiers << "band3_int16_noct_epsg4326";
  identifiers << "band3_float32_noct_epsg4326";

  // How to reasonably log multiple fails within this loop?
  foreach ( QString version, versions )
  {
    foreach ( QString identifier, identifiers )
    {
      foreach ( QString ext, QStringList() << ".tif" << ".tif.aux.xml" )
      {
        QFile::remove( QDir::tempPath() + "/" + identifier + ext );
        QVERIFY( QFile::copy( mTestDataDir + "/" + identifier + ext, QDir::tempPath() + "/" + identifier + ext ) );
      }

      QgsDataSourceURI uri;
      uri.setParam( "url", mUrl );
      uri.setParam( "identifier", identifier );
      uri.setParam( "crs", "epsg:4326" );
      uri.setParam( "version", version );
      uri.setParam( "cache", "AlwaysNetwork" );

      if ( !read( identifier, uri.encodedUri(), QDir::tempPath() + "/" + identifier + ".tif", mReport ) )
      {
        ok = false;
      }
    }
  }
  QVERIFY2( ok, "Reading data failed. See report for details." );
}

bool TestQgsWcsProvider::read( QString theIdentifier, QString theWcsUri, QString theFilePath, QString & theReport )
{
  theReport += QString( "<h2>Identifier (coverage): %1</h2>" ).arg( theIdentifier );

  QgsRasterChecker checker;
  bool ok = checker.runTest( "wcs", theWcsUri, "gdal", theFilePath );

  theReport += checker.report();
  return ok;
}

QTEST_MAIN( TestQgsWcsProvider )
#include "moc_testqgswcsprovider.cxx"
