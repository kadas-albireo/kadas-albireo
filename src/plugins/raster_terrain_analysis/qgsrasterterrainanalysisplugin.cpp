/***************************************************************************
                          qgsrasterterrainanalysisplugin.cpp  -  description
                             -------------------
    begin                : August 6th, 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrasterterrainanalysisplugin.h"
#include "qgis.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsaspectfilter.h"
#include "qgshillshadefilter.h"
#include "qgsslopefilter.h"
#include "qgsruggednessfilter.h"
#include "qgstotalcurvaturefilter.h"
#include "qgsrelief.h"
#include "qgsrasterterrainanalysisdialog.h"
#include <QAction>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressDialog>

static const QString name_ = QObject::tr( "Raster Terrain Analysis plugin" );
static const QString description_ = QObject::tr( "A plugin for raster based terrain analysis" );
static const QString version_ = QObject::tr( "Version 0.1" );
static const QString icon_ = ":/raster/dem.png";
static const QString category_ = QObject::tr( "Raster" );

QgsRasterTerrainAnalysisPlugin::QgsRasterTerrainAnalysisPlugin( QgisInterface* iface ): mIface( iface ), mTerrainAnalysisMenu( 0 )
{

}

QgsRasterTerrainAnalysisPlugin::~QgsRasterTerrainAnalysisPlugin()
{

}

void QgsRasterTerrainAnalysisPlugin::initGui()
{
  //create Action
  if ( mIface )
  {
    QMenu* rasterMenu = mIface->rasterMenu();
    if ( !rasterMenu )
    {
      return;
    }

    mTerrainAnalysisMenu = new QMenu( tr( "Terrain Analysis" ), rasterMenu );
    mTerrainAnalysisMenu->setObjectName( "mTerrainAnalysisMenu" );
    mTerrainAnalysisMenu->setIcon( QIcon( ":/raster/dem.png" ) );
    QAction *slopeAction = mTerrainAnalysisMenu->addAction( tr( "Slope" ), this, SLOT( slope() ) );
    slopeAction->setObjectName( "slopeAction" );

    QAction *aspectAction = mTerrainAnalysisMenu->addAction( tr( "Aspect..." ), this, SLOT( aspect() ) );
    aspectAction->setObjectName( "aspectAction" );
    QAction *hilshadeAction = mTerrainAnalysisMenu->addAction( tr( "Hillshade..." ), this, SLOT( hillshade() ) );
    hilshadeAction->setObjectName( "hilshadeAction" );
    QAction *reliefAction = mTerrainAnalysisMenu->addAction( tr( "Relief..." ), this, SLOT( relief() ) );
    reliefAction->setObjectName( "reliefAction" );
    QAction *ruggednesIndex = mTerrainAnalysisMenu->addAction( tr( "Ruggedness Index..." ), this, SLOT( ruggedness() ) );
    ruggednesIndex->setObjectName( "ruggednesIndex" );

    rasterMenu->addMenu( mTerrainAnalysisMenu );


  }
}

void QgsRasterTerrainAnalysisPlugin::unload()
{
  if ( mIface )
  {
    delete mTerrainAnalysisMenu;
  }
}

void QgsRasterTerrainAnalysisPlugin::hillshade()
{
  QgsRasterTerrainAnalysisDialog d( QgsRasterTerrainAnalysisDialog::HillshadeInput, mIface->mainWindow() );
  d.setWindowTitle( tr( "Hillshade" ) );
  if ( d.exec() == QDialog::Accepted )
  {
    QString outputFile = d.outputFile();
    QgsCoordinateReferenceSystem limitRegionCrs;
    QgsRectangle limitRegion = getLimitRegion( d.limitToVisibleArea(), limitRegionCrs );
    QgsHillshadeFilter hillshade( d.inputFile(), outputFile, d.outputFormat(), d.lightAzimuth(), d.lightAngle(), limitRegion, limitRegionCrs );
    hillshade.setZFactor( d.zFactor() );
    QProgressDialog p( tr( "Calculating hillshade..." ), tr( "Abort" ), 0, 0 );
    p.setWindowModality( Qt::WindowModal );
    hillshade.processRaster( &p );
    if ( d.addResultToProject() )
    {
      mIface->addRasterLayer( outputFile, QFileInfo( outputFile ).baseName() );
    }
  }
}

void QgsRasterTerrainAnalysisPlugin::relief()
{
  QgsRasterTerrainAnalysisDialog d( QgsRasterTerrainAnalysisDialog::ReliefInput, mIface->mainWindow() );
  d.setWindowTitle( tr( "Relief" ) );
  if ( d.exec() == QDialog::Accepted )
  {
    QString outputFile = d.outputFile();
    QgsCoordinateReferenceSystem limitRegionCrs;
    QgsRectangle limitRegion = getLimitRegion( d.limitToVisibleArea(), limitRegionCrs );
    QgsRelief relief( d.inputFile(), outputFile, d.outputFormat(), limitRegion, limitRegionCrs );
    relief.setReliefColors( d.reliefColors() );
    relief.setZFactor( d.zFactor() );
    QProgressDialog p( tr( "Calculating relief..." ), tr( "Abort" ), 0, 0 );
    p.setWindowModality( Qt::WindowModal );
    relief.processRaster( &p );
    if ( d.addResultToProject() )
    {
      mIface->addRasterLayer( outputFile, QFileInfo( outputFile ).baseName() );
    }
  }
}

void QgsRasterTerrainAnalysisPlugin::slope()
{
  QgsRasterTerrainAnalysisDialog d( QgsRasterTerrainAnalysisDialog::NoParameter, mIface->mainWindow() );
  d.setWindowTitle( tr( "Slope" ) );
  if ( d.exec() == QDialog::Accepted )
  {
    QString outputFile = d.outputFile();
    QgsCoordinateReferenceSystem limitRegionCrs;
    QgsRectangle limitRegion = getLimitRegion( d.limitToVisibleArea(), limitRegionCrs );
    QgsSlopeFilter slope( d.inputFile(), outputFile, d.outputFormat(), limitRegion, limitRegionCrs );
    slope.setZFactor( d.zFactor() );
    QProgressDialog p( tr( "Calculating slope..." ), tr( "Abort" ), 0, 0 );
    p.setWindowModality( Qt::WindowModal );
    slope.processRaster( &p );
    if ( d.addResultToProject() )
    {
      mIface->addRasterLayer( outputFile, QFileInfo( outputFile ).baseName() );
    }
  }
}

void QgsRasterTerrainAnalysisPlugin::aspect()
{
  QgsRasterTerrainAnalysisDialog d( QgsRasterTerrainAnalysisDialog::NoParameter, mIface->mainWindow() );
  d.setWindowTitle( tr( "Aspect" ) );
  if ( d.exec() == QDialog::Accepted )
  {
    QString outputFile = d.outputFile();
    QgsCoordinateReferenceSystem limitRegionCrs;
    QgsRectangle limitRegion = getLimitRegion( d.limitToVisibleArea(), limitRegionCrs );
    QgsAspectFilter aspect( d.inputFile(), outputFile, d.outputFormat(), limitRegion, limitRegionCrs );
    aspect.setZFactor( d.zFactor() );
    QProgressDialog p( tr( "Calculating aspect..." ), tr( "Abort" ), 0, 0 );
    p.setWindowModality( Qt::WindowModal );
    aspect.processRaster( &p );
    if ( d.addResultToProject() )
    {
      mIface->addRasterLayer( outputFile, QFileInfo( outputFile ).baseName() );
    }
  }
}

void QgsRasterTerrainAnalysisPlugin::ruggedness()
{
  QgsRasterTerrainAnalysisDialog d( QgsRasterTerrainAnalysisDialog::NoParameter, mIface->mainWindow() );
  d.setWindowTitle( tr( "Ruggedness" ) );
  if ( d.exec() == QDialog::Accepted )
  {
    QString outputFile = d.outputFile();
    QgsCoordinateReferenceSystem limitRegionCrs;
    QgsRectangle limitRegion = getLimitRegion( d.limitToVisibleArea(), limitRegionCrs );
    QgsRuggednessFilter ruggedness( d.inputFile(), outputFile, d.outputFormat(), limitRegion, limitRegionCrs );
    ruggedness.setZFactor( d.zFactor() );
    QProgressDialog p( tr( "Calculating ruggedness..." ), tr( "Abort" ), 0, 0 );
    p.setWindowModality( Qt::WindowModal );
    ruggedness.processRaster( &p );
    if ( d.addResultToProject() )
    {
      mIface->addRasterLayer( outputFile, QFileInfo( outputFile ).baseName() );
    }
  }
}

QgsRectangle QgsRasterTerrainAnalysisPlugin::getLimitRegion( bool limitEnabled, QgsCoordinateReferenceSystem& regionCrs ) const
{
  if ( !limitEnabled )
  {
    return QgsRectangle();
  }
  regionCrs = mIface->mapCanvas()->mapSettings().destinationCrs();
  return mIface->mapCanvas()->mapSettings().visibleExtent();
}

//global methods for the plugin manager
QGISEXTERN QgisPlugin* classFactory( QgisInterface * ifacePointer )
{
  return new QgsRasterTerrainAnalysisPlugin( ifacePointer );
}

QGISEXTERN QString name()
{
  return name_;
}

QGISEXTERN QString description()
{
  return description_;
}

QGISEXTERN QString version()
{
  return version_;
}

QGISEXTERN QString icon()
{
  return icon_;
}

QGISEXTERN int type()
{
  return QgisPlugin::UI;
}

QGISEXTERN void unload( QgisPlugin* pluginPointer )
{
  delete pluginPointer;
}

QGISEXTERN QString category()
{
  return category_;
}


