/***************************************************************************
                          qgszonalstatisticsplugin.cpp  -  description
                          ----------------------------
    begin                : August 29th, 2009
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

#include "qgszonalstatisticsplugin.h"
#include "qgisinterface.h"
#include "qgszonalstatistics.h"
#include "qgszonalstatisticsdialog.h"
#include "qgsvectorlayer.h"
#include <QAction>
#include <QProgressDialog>

static const QString name_ = QObject::tr( "Zonal statistics plugin" );
static const QString description_ = QObject::tr( "A plugin to calculate count, sum, mean of rasters for each polygon of a vector layer" );
static const QString category_ = QObject::tr( "Raster" );
static const QString version_ = QObject::tr( "Version 0.1" );
static const QString pluginIcon_ = ":/zonal_statistics/raster-stats.png";

QgsZonalStatisticsPlugin::QgsZonalStatisticsPlugin( QgisInterface* iface )
    : mIface( iface )
    , mAction( 0 )
{
}

QgsZonalStatisticsPlugin::~QgsZonalStatisticsPlugin()
{
}

void QgsZonalStatisticsPlugin::initGui()
{
  delete mAction;

  mAction = new QAction( QIcon( ":/zonal_statistics/raster-stats.png" ), tr( "&Zonal statistics" ), 0 );
  mAction->setObjectName( "ZonalStatistics" );
  QObject::connect( mAction, SIGNAL( triggered() ), this, SLOT( run() ) );
  mIface->addRasterToolBarIcon( mAction );
  mIface->addPluginToRasterMenu( tr( "&Zonal statistics" ), mAction );
}

void QgsZonalStatisticsPlugin::unload()
{
  mIface->removeRasterToolBarIcon( mAction );
  mIface->removePluginRasterMenu( tr( "&Zonal statistics" ), mAction );
  delete mAction;
}

void QgsZonalStatisticsPlugin::run()
{
  QgsZonalStatisticsDialog d( mIface );
  if ( d.exec() == QDialog::Rejected )
  {
    return;
  }

  QString rasterFile = d.rasterFilePath();
  QgsVectorLayer* vl = d.polygonLayer();
  if ( !vl )
  {
    return;
  }

  QgsZonalStatistics zs( vl, rasterFile, d.attributePrefix(), 1 ); //atm hardcode first band
  QProgressDialog p( tr( "Calculating zonal statistics..." ), tr( "Abort..." ), 0, 0 );
  p.setWindowModality( Qt::WindowModal );
  zs.calculateStatistics( &p );
}

//global methods for the plugin manager
QGISEXTERN QgisPlugin* classFactory( QgisInterface * ifacePointer )
{
  return new QgsZonalStatisticsPlugin( ifacePointer );
}

QGISEXTERN QString name()
{
  return name_;
}

QGISEXTERN QString description()
{
  return description_;
}

QGISEXTERN QString category()
{
  return category_;
}

QGISEXTERN QString version()
{
  return version_;
}

QGISEXTERN QString icon()
{
  return pluginIcon_;
}

QGISEXTERN int type()
{
  return QgisPlugin::UI;
}

QGISEXTERN void unload( QgisPlugin* pluginPointer )
{
  delete pluginPointer;
}



