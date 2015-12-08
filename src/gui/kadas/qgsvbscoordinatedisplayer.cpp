/***************************************************************************
 *  qgsvbscoordinatedisplayer.cpp                                          *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbscoordinatedisplayer.h"
#include "qgsvbscoordinateconverter.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"
#include "qgsproject.h"
#include "qgslogger.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessagebar.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QStatusBar>
#include <QMessageBox>
#include <gdal.h>


template<class T>
static inline QVariant ptr2variant( T* ptr )
{
  return QVariant::fromValue<void*>( reinterpret_cast<void*>( ptr ) );
}

template<class T>
static inline T* variant2ptr( const QVariant& v )
{
  return reinterpret_cast<T*>( v.value<void*>() );
}


QgsVBSCoordinateDisplayer::QgsVBSCoordinateDisplayer( QComboBox* crsComboBox, QLineEdit* coordLineEdit, QgsMapCanvas* mapCanvas,
    QWidget *parent ) : QWidget( parent ), mMapCanvas( mapCanvas ),
    mCRSSelectionCombo( crsComboBox ), mCoordinateLineEdit( coordLineEdit )
{
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mIconLabel = new QLabel( this );
  mIconLabel->setPixmap( QPixmap( ":/vbsfunctionality/icons/mousecoordinates.svg" ) );

  mCRSSelectionCombo->addItem( "LV03", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:21781", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "LV95", ptr2variant( new QgsEPSGCoordinateConverter( "EPSG:2056", mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DMS", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMinSec, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DM", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DegMin, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "DD", ptr2variant( new QgsWGS84CoordinateConverter( QgsWGS84CoordinateConverter::DecDeg, mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "UTM", ptr2variant( new QgsUTMCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->addItem( "MGRS", ptr2variant( new QgsMGRSCoordinateConverter( mCRSSelectionCombo ) ) );
  mCRSSelectionCombo->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  mCRSSelectionCombo->setCurrentIndex( 0 );

  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );
  mCoordinateLineEdit->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  connect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), mCoordinateLineEdit, SLOT( clear() ) );
  connect( mCRSSelectionCombo, SIGNAL( currentIndexChanged( int ) ), this, SIGNAL( displayFormatChanged() ) );

  syncProjectCrs();
}

QgsVBSCoordinateDisplayer::~QgsVBSCoordinateDisplayer()
{
  disconnect( mMapCanvas, SIGNAL( xyCoordinates( QgsPoint ) ), this, SLOT( displayCoordinates( QgsPoint ) ) );
  disconnect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncProjectCrs() ) );
}

QString QgsVBSCoordinateDisplayer::getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs )
{
  if ( mCRSSelectionCombo )
  {
    QVariant v = mCRSSelectionCombo->itemData( mCRSSelectionCombo->currentIndex() );
    QgsVBSCoordinateConverter* conv = variant2ptr<QgsVBSCoordinateConverter>( v );
    if ( conv )
    {
      return conv->convert( p, crs );
    }
  }
  return QString();
}

double QgsVBSCoordinateDisplayer::getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QGis::UnitType unit )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
#pragma message( "warning: TODO" )
    // mQGisIface->messageBar()->pushMessage(tr( "No heightmap is defined in the project."), tr("Right-click a raster layer in the layer tree and select it to be used as heightmap."), QgsMessageBar::INFO, 10);
    return 0;
  }
  QString rasterFile = layer->source();
  GDALDatasetH raster = GDALOpen( rasterFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( !raster )
  {
    QMessageBox::warning( 0, tr( "Error" ), tr( "Failed to open raster file: %1" ).arg( rasterFile ) );
    return 0;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    QgsDebugMsg( "Failed to get raster geotransform" );
    GDALClose( raster );
    return 0;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs( proj );
  if ( !rasterCrs.isValid() )
  {
    QgsDebugMsg( "Failed to get raster CRS" );
    GDALClose( raster );
    return 0;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !raster )
  {
    QgsDebugMsg( "Failed to open raster band 0" );
    GDALClose( raster );
    return 0;
  }

  // Transform geo position to raster CRS
  QgsPoint pRaster = QgsCoordinateTransform( crs, rasterCrs ).transform( p );
  QgsDebugMsg( QString( "Transform %1 from %2 to %3 gives %4" ).arg( p.toString() )
               .arg( crs.authid() ).arg( rasterCrs.authid() ).arg( pRaster.toString() ) );

  // Transform raster geo position to pixel coordinates
  double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
  double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );

  double pixValues[4] = {};
  if ( CE_None != GDALRasterIO( band, GF_Read,
                                qFloor( col ), qFloor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
  {
    QgsDebugMsg( "Failed to read pixel values" );
    GDALClose( raster );
    return 0;
  }

  GDALClose( raster );

  // Interpolate values
  double lambdaR = row - qFloor( row );
  double lambdaC = col - qFloor( col );

  double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                 + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
  if ( rasterCrs.mapUnits() != unit )
  {
    value *= QGis::fromUnitToUnitFactor( rasterCrs.mapUnits(), unit );
  }
  return value;
}


void QgsVBSCoordinateDisplayer::displayCoordinates( const QgsPoint &p )
{
  if ( mCoordinateLineEdit )
  {
    mCoordinateLineEdit->setText( getDisplayString( p, mMapCanvas->mapSettings().destinationCrs() ) );
  }
}

void QgsVBSCoordinateDisplayer::syncProjectCrs()
{
  const QgsCoordinateReferenceSystem& crs = mMapCanvas->mapSettings().destinationCrs();
  if ( crs.srsid() == 4326 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "DMS" ) );
  }
  else if ( crs.srsid() == 21781 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "LV03" ) );
  }
  else if ( crs.srsid() == 2056 )
  {
    mCRSSelectionCombo->setCurrentIndex( mCRSSelectionCombo->findText( "LV95" ) );
  }
}
