/***************************************************************************
 *  qgsvbsslopetool.cpp                                                    *
 *  -------------------                                                    *
 *  begin                : Nov 11, 2015                                    *
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

#include "qgsvbsslopetool.h"
#include "qgscolorrampshader.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgsrubberband.h"
#include "qgstemporaryfile.h"
#include "raster/qgssinglebandpseudocolorrenderer.h"
#include "raster/qgsslopefilter.h"

#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>


QgsVBSSlopeTool::QgsVBSSlopeTool( QgisInterface *iface, QObject *parent )
    : QObject( parent ), mIface( iface )
{
  mRubberBand = new QgsRubberBand( mIface->mapCanvas(), QGis::Polygon );
  mRubberBand->setFillColor( QColor( 254, 178, 76, 63 ) );
  mRubberBand->setBorderColor( QColor( 254, 58, 29, 100 ) );

  mRectData = new QgsMapToolFilter::RectData();

  QgsMapToolFilter* filterTool = new QgsMapToolFilter( mIface->mapCanvas(), QgsMapToolFilter::Rect, mRubberBand, mRectData );
  connect( filterTool, SIGNAL( deactivated() ), this, SLOT( filterFinished() ) );
  connect( filterTool, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
  mIface->mapCanvas()->setMapTool( filterTool );
}

QgsVBSSlopeTool::~QgsVBSSlopeTool()
{
  delete mRubberBand;
  delete mRectData;
}

void QgsVBSSlopeTool::filterFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    QMessageBox::warning( 0, tr( "Error" ), tr( "No heightmap is defined in the project. Right-click a raster layer in the layer tree and select it to be used as heightmap." ) );
    emit finished();
    return;
  }

  QgsRectangle rect( mRectData->p1, mRectData->p2 );
  rect.normalize();
  QgsCoordinateReferenceSystem rectCrs = mIface->mapCanvas()->mapSettings().destinationCrs();

  QString outputFileName = QString( "slope_%1-%2_%3-%4.tif" ).arg( rect.xMinimum() ).arg( rect.xMaximum() ).arg( rect.yMinimum() ).arg( rect.yMaximum() );
  QString outputFile = QgsTemporaryFile::createNewFile( outputFileName );

  QgsSlopeFilter slope( layer->source(), outputFile, "GTiff", rect, rectCrs );
  slope.setZFactor( 1 );
  QProgressDialog p( tr( "Calculating slope..." ), tr( "Abort" ), 0, 0 );
  p.setWindowModality( Qt::WindowModal );
  slope.processRaster( &p );
  if ( !p.wasCanceled() )
  {
    QgsRasterLayer* layer = mIface->addRasterLayer( outputFile, tr( "Slope [%1]" ).arg( rect.toString( true ) ) );
    QgsColorRampShader* rampShader = new QgsColorRampShader();
    QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
        << QgsColorRampShader::ColorRampItem( 0, QColor( 0, 0, 255 ), "0%" )
        << QgsColorRampShader::ColorRampItem( 7.13, QColor( 0, 127, 255 ), "12.5%" )
        << QgsColorRampShader::ColorRampItem( 14.04, QColor( 0, 255, 255 ), "25%" )
        << QgsColorRampShader::ColorRampItem( 20.56, QColor( 0, 255, 200 ), "37.5%" )
        << QgsColorRampShader::ColorRampItem( 26.57, QColor( 0, 255, 0 ), "50%" )
        << QgsColorRampShader::ColorRampItem( 32.01, QColor( 200, 255, 0 ), "62.5%" )
        << QgsColorRampShader::ColorRampItem( 36.87, QColor( 255, 255, 0 ), "75%" )
        << QgsColorRampShader::ColorRampItem( 41.19, QColor( 255, 127, 0 ), "87.5%" )
        << QgsColorRampShader::ColorRampItem( 45, QColor( 255, 0, 0 ), "100%" );
    rampShader->setColorRampItemList( colorRampItems );
    QgsRasterShader* shader = new QgsRasterShader();
    shader->setRasterShaderFunction( rampShader );
    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer( 0, 1, shader );
    layer->setRenderer( renderer );
  }
  emit finished();
}
