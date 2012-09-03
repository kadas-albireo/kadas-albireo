/***************************************************************************
     testqgsvectorfilewriter.cpp
     --------------------------------------
    Date                 : Frida  Nov 23  2007
    Copyright            : (C) 2007 by Tim Sutton
    Email                : tim@linfiniti.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QObject>
#include <iostream>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QTime>
#include <QDesktopServices>


//qgis includes...
#include <qgsrasterlayer.h>
#include <qgsrasterpyramid.h>
#include <qgsrasterbandstats.h>
#include <qgsrasterpyramid.h>
#include <qgsmaplayerregistry.h>
#include <qgsapplication.h>
#include <qgsmaprenderer.h>
#include <qgsmaplayerregistry.h>
#include "qgssinglebandpseudocolorrenderer.h"
#include "qgsvectorcolorrampv2.h"
#include "qgscptcityarchive.h"

//qgis unit test includes
#include <qgsrenderchecker.h>


/** \ingroup UnitTests
 * This is a unit test for the QgsRasterLayer class.
 */
class TestQgsRasterLayer: public QObject
{
    Q_OBJECT;
  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init() {};// will be called before each testfunction is executed.
    void cleanup() {};// will be called after every testfunction.

    void isValid();
    void pseudoColor();
    void colorRamp1();
    void colorRamp2();
    void colorRamp3();
    void colorRamp4();
    void landsatBasic();
    void landsatBasic875Qml();
    void checkDimensions();
    void checkStats();
    void buildExternalOverviews();
    void registry();
  private:
    bool render( QString theFileName );
    bool setQml( QString theType );
    void populateColorRampShader( QgsColorRampShader* colorRampShader,
                                  QgsVectorColorRampV2* colorRamp,
                                  int numberOfEntries );
    bool testColorRamp( QString name, QgsVectorColorRampV2* colorRamp,
                        QgsColorRampShader::ColorRamp_TYPE type, int numberOfEntries );
    QString mTestDataDir;
    QgsRasterLayer * mpRasterLayer;
    QgsRasterLayer * mpLandsatRasterLayer;
    QgsMapRenderer * mpMapRenderer;
    QString mReport;
};

//runs before all tests
void TestQgsRasterLayer::initTestCase()
{
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  // disable any PAM stuff to make sure stats are consistent
  CPLSetConfigOption( "GDAL_PAM_ENABLED", "NO" );
  QString mySettings = QgsApplication::showSettings();
  mySettings = mySettings.replace( "\n", "<br />" );
  //create some objects that will be used in all tests...
  //create a raster layer that will be used in all tests...
  mTestDataDir = QString( TEST_DATA_DIR ) + QDir::separator(); //defined in CmakeLists.txt
  QString myFileName = mTestDataDir + "tenbytenraster.asc";
  QString myLandsatFileName = mTestDataDir + "landsat.tif";
  QFileInfo myRasterFileInfo( myFileName );
  mpRasterLayer = new QgsRasterLayer( myRasterFileInfo.filePath(),
                                      myRasterFileInfo.completeBaseName() );
  qDebug() << "tenbyteraster metadata: " << mpRasterLayer->dataProvider()->metadata();
  QFileInfo myLandsatRasterFileInfo( myLandsatFileName );
  mpLandsatRasterLayer = new QgsRasterLayer( myLandsatRasterFileInfo.filePath(),
      myLandsatRasterFileInfo.completeBaseName() );
  qDebug() << "landsat metadata: " << mpLandsatRasterLayer->dataProvider()->metadata();
  // Register the layer with the registry
  QgsMapLayerRegistry::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mpRasterLayer );
  QgsMapLayerRegistry::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mpLandsatRasterLayer );
  // add the test layer to the maprender
  mpMapRenderer = new QgsMapRenderer();
  QStringList myLayers;
  myLayers << mpRasterLayer->id();
  mpMapRenderer->setLayerSet( myLayers );
  mReport += "<h1>Raster Layer Tests</h1>\n";
  mReport += "<p>" + mySettings + "</p>";
}
//runs after all tests
void TestQgsRasterLayer::cleanupTestCase()
{
  QString myReportFile = QDir::tempPath() + QDir::separator() + "qgistest.html";
  QFile myFile( myReportFile );
  if ( myFile.open( QIODevice::WriteOnly | QIODevice::Append ) )
  {
    QTextStream myQTextStream( &myFile );
    myQTextStream << mReport;
    myFile.close();
    //QDesktopServices::openUrl( "file:///" + myReportFile );
  }

}

void TestQgsRasterLayer::isValid()
{
  QVERIFY( mpRasterLayer->isValid() );
  mpRasterLayer->setContrastEnhancementAlgorithm( QgsContrastEnhancement::StretchToMinimumMaximum, false );
  mpMapRenderer->setExtent( mpRasterLayer->extent() );
  QVERIFY( render( "raster" ) );
}

void TestQgsRasterLayer::pseudoColor()
{
  QgsRasterShader* rasterShader = new QgsRasterShader();
  QgsColorRampShader* colorRampShader = new QgsColorRampShader();
  colorRampShader->setColorRampType( QgsColorRampShader::INTERPOLATED );

  //items to imitate old pseudo color renderer
  QList<QgsColorRampShader::ColorRampItem> colorRampItems;
  QgsColorRampShader::ColorRampItem firstItem;
  firstItem.value = 0.0;
  firstItem.color = QColor( "#0000ff" );
  colorRampItems.append( firstItem );
  QgsColorRampShader::ColorRampItem secondItem;
  secondItem.value = 3.0;
  secondItem.color = QColor( "#00ffff" );
  colorRampItems.append( secondItem );
  QgsColorRampShader::ColorRampItem thirdItem;
  thirdItem.value = 6.0;
  thirdItem.color = QColor( "#ffff00" );
  colorRampItems.append( thirdItem );
  QgsColorRampShader::ColorRampItem fourthItem;
  fourthItem.value = 9.0;
  fourthItem.color = QColor( "#ff0000" );
  colorRampItems.append( fourthItem );

  colorRampShader->setColorRampItemList( colorRampItems );
  rasterShader->setRasterShaderFunction( colorRampShader );
  QgsSingleBandPseudoColorRenderer* r = new QgsSingleBandPseudoColorRenderer( mpRasterLayer->dataProvider(), 1, rasterShader );
  mpRasterLayer->setRenderer( r );
  mpMapRenderer->setExtent( mpRasterLayer->extent() );
  QVERIFY( render( "raster_pseudo" ) );
}

void TestQgsRasterLayer::populateColorRampShader( QgsColorRampShader* colorRampShader,
    QgsVectorColorRampV2* colorRamp,
    int numberOfEntries )

{
  // adapted from QgsSingleBandPseudoColorRendererWidget::on_mClassifyButton_clicked()
  // and TestQgsRasterLayer::pseudoColor()
  // would be better to add a more generic function to api that does this
  int bandNr = 1;
  QgsRasterBandStats myRasterBandStats = mpRasterLayer->dataProvider()->bandStatistics( bandNr );

  QList<double> entryValues;
  QList<QColor> entryColors;
  double currentValue = myRasterBandStats.minimumValue;
  double intervalDiff;
  if ( numberOfEntries > 1 )
  {
    //because the highest value is also an entry, there are (numberOfEntries - 1)
    //intervals
    intervalDiff = ( myRasterBandStats.maximumValue - myRasterBandStats.minimumValue ) /
                   ( numberOfEntries - 1 );
  }
  else
  {
    intervalDiff = myRasterBandStats.maximumValue - myRasterBandStats.minimumValue;
  }

  for ( int i = 0; i < numberOfEntries; ++i )
  {
    entryValues.push_back( currentValue );
    currentValue += intervalDiff;
    entryColors.push_back( colorRamp->color((( double ) i ) / numberOfEntries ) );
  }

  //items to imitate old pseudo color renderer
  QList<QgsColorRampShader::ColorRampItem> colorRampItems;
  QList<double>::const_iterator value_it = entryValues.begin();
  QList<QColor>::const_iterator color_it = entryColors.begin();
  for ( ; value_it != entryValues.end(); ++value_it, ++color_it )
  {
    colorRampItems.append( QgsColorRampShader::ColorRampItem( *value_it, *color_it ) );
  }
  colorRampShader->setColorRampItemList( colorRampItems );
}

bool TestQgsRasterLayer::testColorRamp( QString name, QgsVectorColorRampV2* colorRamp,
                                        QgsColorRampShader::ColorRamp_TYPE type, int numberOfEntries )
{
  QgsRasterShader* rasterShader = new QgsRasterShader();
  QgsColorRampShader* colorRampShader = new QgsColorRampShader();
  colorRampShader->setColorRampType( type );

  populateColorRampShader( colorRampShader, colorRamp, numberOfEntries );

  rasterShader->setRasterShaderFunction( colorRampShader );
  QgsSingleBandPseudoColorRenderer* r = new QgsSingleBandPseudoColorRenderer( mpRasterLayer->dataProvider(), 1, rasterShader );
  mpRasterLayer->setRenderer( r );
  mpMapRenderer->setExtent( mpRasterLayer->extent() );
  return render( name );
}

void TestQgsRasterLayer::colorRamp1()
{
  // gradient ramp
  QgsVectorGradientColorRampV2* colorRamp = new QgsVectorGradientColorRampV2( QColor( Qt::red ), QColor( Qt::black ) );
  QgsVectorGradientColorRampV2::StopsMap stops;
  stops[ 0.5 ] = QColor( Qt::white );
  colorRamp->setStops( stops );

  // QVERIFY( testColorRamp( "raster_colorRamp1", colorRamp, QgsColorRampShader::INTERPOLATED, 5 ) );
  QVERIFY( testColorRamp( "raster_colorRamp1", colorRamp, QgsColorRampShader::DISCRETE, 10 ) );
}

void TestQgsRasterLayer::colorRamp2()
{
  // ColorBrewer ramp
  QVERIFY( testColorRamp( "raster_colorRamp2",
                          new QgsVectorColorBrewerColorRampV2( "BrBG", 10 ),
                          QgsColorRampShader::DISCRETE, 10 ) );
}

void TestQgsRasterLayer::colorRamp3()
{
  // cpt-city ramp, discrete
  QgsCptCityArchive::initArchives();
  QVERIFY( testColorRamp( "raster_colorRamp3",
                          new QgsCptCityColorRampV2( "cb/div/BrBG", "_10" ),
                          QgsColorRampShader::DISCRETE, 10 ) );
}

void TestQgsRasterLayer::colorRamp4()
{
  // cpt-city ramp, continuous
  QVERIFY( testColorRamp( "raster_colorRamp4",
                          new QgsCptCityColorRampV2( "grass/elevation", "" ),
                          QgsColorRampShader::DISCRETE, 10 ) );
}

void TestQgsRasterLayer::landsatBasic()
{
  mpLandsatRasterLayer->setContrastEnhancementAlgorithm( QgsContrastEnhancement::StretchToMinimumMaximum, false );
  QStringList myLayers;
  myLayers << mpLandsatRasterLayer->id();
  mpMapRenderer->setLayerSet( myLayers );
  mpMapRenderer->setExtent( mpLandsatRasterLayer->extent() );
  QVERIFY( render( "landsat_basic" ) );
}
void TestQgsRasterLayer::landsatBasic875Qml()
{
  //a qml that orders the rgb bands as 8,7,5
  QStringList myLayers;
  myLayers << mpLandsatRasterLayer->id();
  mpMapRenderer->setLayerSet( myLayers );
  mpMapRenderer->setExtent( mpLandsatRasterLayer->extent() );
  QVERIFY( setQml( "875" ) );
  QVERIFY( render( "landsat_875" ) );
}
void TestQgsRasterLayer::checkDimensions()
{
  mReport += "<h2>Check Dimensions</h2>\n";
  QVERIFY( mpRasterLayer->width() == 10 );
  QVERIFY( mpRasterLayer->height() == 10 );
  // regression check for ticket #832
  // note bandStatistics call is base 1
  // TODO: elementCount is not collected by GDAL, use other stats.
  //QVERIFY( mpRasterLayer->dataProvider()->bandStatistics( 1 ).elementCount == 100 );
  mReport += "<p>Passed</p>";
}
void TestQgsRasterLayer::checkStats()
{
  mReport += "<h2>Check Stats</h2>\n";
  QgsRasterBandStats myStatistics = mpRasterLayer->dataProvider()->bandStatistics( 1,
                                    QgsRasterBandStats::Min | QgsRasterBandStats::Max |
                                    QgsRasterBandStats::Mean | QgsRasterBandStats::StdDev );
  QVERIFY( mpRasterLayer->width() == 10 );
  QVERIFY( mpRasterLayer->height() == 10 );
  //QVERIFY( myStatistics.elementCount == 100 );
  QVERIFY( myStatistics.minimumValue == 0 );
  QVERIFY( myStatistics.maximumValue == 9 );
  QVERIFY( myStatistics.mean == 4.5 );
  double stdDev = 2.87228132326901431;
  // TODO: verify why GDAL stdDev is so different from generic (2.88675)
  mReport += QString( "stdDev = %1 expected = %2<br>\n" ).arg( myStatistics.stdDev ).arg( stdDev );
  QVERIFY( fabs( myStatistics.stdDev - stdDev )
           < 0.0000000000000001 );
  mReport += "<p>Passed</p>";
}

void TestQgsRasterLayer::buildExternalOverviews()
{
  //before we begin delete any old ovr file (if it exists)
  //and make a copy of the landsat raster into the temp dir
  QString myTempPath = QDir::tempPath() + QDir::separator();
  QFile::remove( myTempPath + "landsat.tif.ovr" );
  QFile::remove( myTempPath + "landsat.tif" );
  QVERIFY( QFile::copy( mTestDataDir + "landsat.tif", myTempPath + "landsat.tif" ) );
  QFileInfo myRasterFileInfo( myTempPath + "landsat.tif" );
  QgsRasterLayer * mypLayer = new QgsRasterLayer( myRasterFileInfo.filePath(),
      myRasterFileInfo.completeBaseName() );

  QVERIFY( mypLayer->isValid() );

  //
  // Ok now we can go on to test
  //

  QgsRasterDataProvider::RasterPyramidsFormat myFormatFlag = QgsRasterDataProvider::PyramidsGTiff;
  QList< QgsRasterPyramid > myPyramidList = mypLayer->dataProvider()->buildPyramidList();
  for ( int myCounterInt = 0; myCounterInt < myPyramidList.count(); myCounterInt++ )
  {
    //mark to be pyramided
    myPyramidList[myCounterInt].build = true;
  }
  //now actually make the pyramids
  QString myResult =
    mypLayer->dataProvider()->buildPyramids( myPyramidList, "NEAREST", myFormatFlag );
  qDebug( "%s", myResult.toLocal8Bit().constData() );
  //
  // Lets verify we have pyramids now...
  //
  myPyramidList = mypLayer->dataProvider()->buildPyramidList();
  for ( int myCounterInt = 0; myCounterInt < myPyramidList.count(); myCounterInt++ )
  {
    //mark to be pyramided
    QVERIFY( myPyramidList.at( myCounterInt ).exists );
  }

  //
  // And that they were indeed in an external file...
  //
  QVERIFY( QFile::exists( myTempPath + "landsat.tif.ovr" ) );
  //cleanup
  delete mypLayer;
  mReport += "<h2>Check Overviews</h2>\n";
  mReport += "<p>Passed</p>";
}


void TestQgsRasterLayer::registry()
{
  QString myTempPath = QDir::tempPath() + QDir::separator();
  QFile::remove( myTempPath + "landsat.tif.ovr" );
  QFile::remove( myTempPath + "landsat.tif" );
  QVERIFY( QFile::copy( mTestDataDir + "landsat.tif", myTempPath + "landsat.tif" ) );
  QFileInfo myRasterFileInfo( myTempPath + "landsat.tif" );
  QgsRasterLayer * mypLayer = new QgsRasterLayer( myRasterFileInfo.filePath(),
      myRasterFileInfo.completeBaseName() );
  QVERIFY( mypLayer->isValid() );

  QgsMapLayerRegistry::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mypLayer, false );
  QgsMapLayerRegistry::instance()->removeMapLayers(
    QStringList() << mypLayer->id() );
}

//
// Helper methods
//


bool TestQgsRasterLayer::render( QString theTestType )
{
  mReport += "<h2>" + theTestType + "</h2>\n";
  QgsRenderChecker myChecker;
  myChecker.setControlName( "expected_" + theTestType );
  myChecker.setMapRenderer( mpMapRenderer );
  bool myResultFlag = myChecker.runTest( theTestType );
  mReport += "\n\n\n" + myChecker.report();
  return myResultFlag;
}

bool TestQgsRasterLayer::setQml( QString theType )
{
  //load a qml style and apply to our layer
  // huh? this is failing but shouldnt!
  //if (! mpLandsatRasterLayer->isValid() )
  //{
  //  qDebug(" **** setQml -> mpLandsatRasterLayer is invalid");
  //  return false;
  //}
  QString myFileName = mTestDataDir + "landsat_" + theType + ".qml";
  bool myStyleFlag = false;
  mpLandsatRasterLayer->loadNamedStyle( myFileName, myStyleFlag );
  if ( !myStyleFlag )
  {
    qDebug( " **** setQml -> mpLandsatRasterLayer is invalid" );
    qDebug( "Qml File :%s", myFileName.toLocal8Bit().constData() );
  }
  return myStyleFlag;
}

QTEST_MAIN( TestQgsRasterLayer )
#include "moc_testqgsrasterlayer.cxx"
