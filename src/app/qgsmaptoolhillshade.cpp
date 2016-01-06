/***************************************************************************
 *  qgsmaptoolhillshade.cpp                                                *
 *  -------------------                                                    *
 *  begin                : Nov 15, 2015                                    *
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
#include "qgsmaptoolhillshade.h"
#include "qgscolorrampshader.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgstemporaryfile.h"
#include "qgssinglebandpseudocolorrenderer.h"
#include "qgshillshadefilter.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QProgressDialog>


QgsMapToolHillshade::QgsMapToolHillshade( QgsMapCanvas* mapCanvas )
    : QgsMapToolDrawRectangle( mapCanvas )
{
  connect( this, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
}

void QgsMapToolHillshade::drawFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    QgisApp::instance()->messageBar()->pushMessage( tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." ), QgsMessageBar::INFO, 10 );
    reset();
    return;
  }

  QDialog anglesDialog;
  anglesDialog.setWindowTitle( tr( "Hillshade setup" ) );
  QGridLayout* anglesDialogLayout = new QGridLayout();
  anglesDialogLayout->addWidget( new QLabel( tr( "Azimuth (horizontal angle):" ) ), 0, 0, 1, 1 );
  anglesDialogLayout->addWidget( new QLabel( tr( "Vertical angle:" ) ), 1, 0, 1, 1 );
  QDoubleSpinBox* spinHorAngle = new QDoubleSpinBox();
  spinHorAngle ->setRange( 0, 359.9 );
  spinHorAngle ->setDecimals( 1 );
  spinHorAngle ->setValue( 180 );
  spinHorAngle->setWrapping( true );
  spinHorAngle ->setSuffix( QChar( 0x00B0 ) );
  anglesDialogLayout->addWidget( spinHorAngle , 0, 1, 1, 1 );
  QDoubleSpinBox* spinVerAngle = new QDoubleSpinBox();
  spinVerAngle->setRange( 0, 90. );
  spinVerAngle->setDecimals( 1 );
  spinVerAngle->setValue( 60. );
  spinVerAngle->setSuffix( QChar( 0x00B0 ) );
  anglesDialogLayout->addWidget( spinVerAngle, 1, 1, 1, 1 );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, SIGNAL( accepted() ), &anglesDialog, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), &anglesDialog, SLOT( reject() ) );
  anglesDialogLayout->addWidget( bbox, 2, 0, 1, 2 );
  anglesDialog.setLayout( anglesDialogLayout );
  anglesDialog.setFixedSize( anglesDialog.sizeHint() );
  if ( anglesDialog.exec() == QDialog::Rejected )
  {
    reset();
    return;
  }

  QgsPoint p1, p2;
  getPart( 0, p1, p2 );
  QgsRectangle rect( p1, p2 );
  rect.normalize();
  QgsCoordinateReferenceSystem rectCrs = canvas()->mapSettings().destinationCrs();

  QString outputFileName = QString( "hillshade_%1-%2_%3-%4.tif" ).arg( rect.xMinimum() ).arg( rect.xMaximum() ).arg( rect.yMinimum() ).arg( rect.yMaximum() );
  QString outputFile = QgsTemporaryFile::createNewFile( outputFileName );

  QgsHillshadeFilter hillshade( layer->source(), outputFile, "GTiff", spinHorAngle->value(), spinVerAngle->value(), rect, rectCrs );
  hillshade.setZFactor( 1 );
  QProgressDialog p( tr( "Calculating hillshade..." ), tr( "Abort" ), 0, 0 );
  p.setWindowModality( Qt::WindowModal );
  hillshade.processRaster( &p );
  if ( !p.wasCanceled() )
  {
    QgsRasterLayer* layer = new QgsRasterLayer( outputFile, tr( "Hillshade [%1]" ).arg( rect.toString( true ) ) );
    layer->renderer()->setOpacity( 0.6 );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
  }
  reset();
}
