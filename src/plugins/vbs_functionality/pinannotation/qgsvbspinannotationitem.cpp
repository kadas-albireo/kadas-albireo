/***************************************************************************
                              qgsvbspinannotationitem.cpp
                              ------------------------
  begin                : August, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbspinannotationitem.h"
#include "qgscoordinatetransform.h"
#include "../qgsvbscoordinatedisplayer.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include <gdal.h>
#include <QApplication>
#include <QClipboard>
#include <QGraphicsSceneContextMenuEvent>
#include <QImageReader>
#include <qmath.h>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

QgsVBSPinAnnotationItem::QgsVBSPinAnnotationItem( QgsMapCanvas* canvas , QgsVBSCoordinateDisplayer *coordinateDisplayer )
    : QgsSvgAnnotationItem( canvas ), mCoordinateDisplayer( coordinateDisplayer )
{
  setItemFlags( QgsAnnotationItem::ItemIsNotResizeable |
                QgsAnnotationItem::ItemHasNoFrame |
                QgsAnnotationItem::ItemHasNoMarker |
                QgsAnnotationItem::ItemIsNotEditable );
  QSize imageSize = QImageReader( ":/vbsfunctionality/icons/pin_red.svg" ).size();
  setFilePath( ":/vbsfunctionality/icons/pin_red.svg" );
  setFrameSize( imageSize );
  setOffsetFromReferencePoint( QPointF( -imageSize.width() / 2., -imageSize.height() ) );
  connect( mCoordinateDisplayer, SIGNAL( displayFormatChanged() ), this, SLOT( updateToolTip() ) );
}

void QgsVBSPinAnnotationItem::updateToolTip()
{
  QString posStr = mCoordinateDisplayer->getDisplayString( mGeoPos, mGeoPosCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mGeoPos.toString() ).arg( mGeoPosCrs.authid() );
  }
  QString toolTipText = tr( "Position: %1\nHeight: %3" )
                        .arg( posStr )
                        .arg( getHeightAtCurrentPos() );
  setToolTip( toolTipText );
}

void QgsVBSPinAnnotationItem::setMapPosition( const QgsPoint& pos )
{
  QgsSvgAnnotationItem::setMapPosition( pos );
  updateToolTip();
}

double QgsVBSPinAnnotationItem::getHeightAtCurrentPos()
{
  QString layerid = QgsProject::instance()->property( "heightmap" ).toString();
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    QMessageBox::warning( 0, tr( "Error" ), tr( "No heightmap is defined in the project. Right-click a raster layer in the layer tree and select it to be used as heightmap." ) );
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
  QgsPoint pRaster = QgsCoordinateTransform( mGeoPosCrs, rasterCrs ).transform( mGeoPos );
  QgsDebugMsg( QString( "Transform %1 from %2 to %3 gives %4" ).arg( mGeoPos.toString() )
               .arg( mGeoPosCrs.authid() ).arg( rasterCrs.authid() ).arg( pRaster.toString() ) );

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

  return ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
         + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
}

void QgsVBSPinAnnotationItem::showContextMenu( const QPoint& screenPos )
{
  QMenu menu;
  menu.addAction( tr( "Copy position" ), this, SLOT( copyPosition() ) );
  menu.addAction( tr( "Remove" ), this, SLOT( deleteLater() ) );
  menu.exec( screenPos );
}

void QgsVBSPinAnnotationItem::copyPosition()
{
  QApplication::clipboard()->setText( toolTip() );
}
