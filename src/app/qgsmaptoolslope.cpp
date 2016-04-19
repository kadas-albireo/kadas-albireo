/***************************************************************************
 *  qgsmaptoolslope.cpp                                                    *
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

#include "qgisapp.h"
#include "qgsmaptoolslope.h"
#include "qgscolorrampshader.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgstemporaryfile.h"
#include "qgssinglebandpseudocolorrenderer.h"
#include "qgsslopefilter.h"

#include <QDir>
#include <QProgressDialog>


QgsMapToolSlope::QgsMapToolSlope( QgsMapCanvas* mapCanvas )
    : QgsMapToolDrawRectangle( mapCanvas )
{
  setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  connect( this, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
}

void QgsMapToolSlope::drawFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    QgisApp::instance()->messageBar()->pushMessage( tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." ), QgsMessageBar::INFO, 10 );
    reset();
    return;
  }

  QgsPoint p1, p2;
  getPart( 0, p1, p2 );
  QgsRectangle rect( p1, p2 );
  rect.normalize();
  if ( rect.isEmpty() )
  {
    reset();
    return;
  }
  QgsCoordinateReferenceSystem rectCrs = canvas()->mapSettings().destinationCrs();

  QString outputFileName = QString( "slope_%1-%2_%3-%4.tif" ).arg( rect.xMinimum() ).arg( rect.xMaximum() ).arg( rect.yMinimum() ).arg( rect.yMaximum() );
  QString outputFile = QgsTemporaryFile::createNewFile( outputFileName );

  QgsSlopeFilter slope( layer->source(), outputFile, "GTiff", rect, rectCrs );
  QProgressDialog p( tr( "Calculating slope..." ), tr( "Abort" ), 0, 0 );
  p.setWindowModality( Qt::WindowModal );
  slope.processRaster( &p );
  if ( !p.wasCanceled() )
  {
    QgsRasterLayer* layer = new QgsRasterLayer( outputFile, tr( "Slope [%1]" ).arg( rect.toString( true ) ) );
    QgsColorRampShader* rampShader = new QgsColorRampShader();
    QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
        << QgsColorRampShader::ColorRampItem( 0, QColor( 43, 131, 186 ), "0%" )
        << QgsColorRampShader::ColorRampItem( 5, QColor( 99, 171, 176 ), "5%" )
        << QgsColorRampShader::ColorRampItem( 10, QColor( 156, 211, 166 ), "10%" )
        << QgsColorRampShader::ColorRampItem( 15, QColor( 199, 232, 173 ), "15%" )
        << QgsColorRampShader::ColorRampItem( 20, QColor( 236, 247, 185 ), "20%" )
        << QgsColorRampShader::ColorRampItem( 25, QColor( 254, 237, 170 ), "25%" )
        << QgsColorRampShader::ColorRampItem( 30, QColor( 253, 201, 128 ), "30%" )
        << QgsColorRampShader::ColorRampItem( 35, QColor( 248, 157, 89 ), "35%" )
        << QgsColorRampShader::ColorRampItem( 40, QColor( 231, 91, 58 ), "40%" )
        << QgsColorRampShader::ColorRampItem( 45, QColor( 215, 25, 28 ), "45%" );
    rampShader->setColorRampItemList( colorRampItems );
    QgsRasterShader* shader = new QgsRasterShader();
    shader->setRasterShaderFunction( rampShader );
    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer( 0, 1, shader );
    layer->setRenderer( renderer );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
  }
  reset();
}
