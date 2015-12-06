/***************************************************************************
 *  qgsvbsviewshedtool.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Nov 12, 2015                                    *
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

#include "qgsvbsviewshedtool.h"
#include "qgisinterface.h"
#include "qgscircularstringv2.h"
#include "qgscurvepolygonv2.h"
#include "qgscolorrampshader.h"
#include "qgsdistancearea.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooldrawshape.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgstemporaryfile.h"
#include "raster/qgssinglebandpseudocolorrenderer.h"
#include "raster/qgsviewshed.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QProgressDialog>


QgsVBSViewshedTool::QgsVBSViewshedTool( QgsMapCanvas* mapCanvas, bool sectorOnly , QObject *parent )
    : QObject( parent ), mMapCanvas( mapCanvas )
{
  if ( sectorOnly )
  {
    mDrawTool = new QgsMapToolDrawCircularSector( mMapCanvas );
  }
  else
  {
    mDrawTool = new QgsMapToolDrawCircle( mMapCanvas );
  }
  connect( mDrawTool, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
  mMapCanvas->setMapTool( mDrawTool );
}

QgsVBSViewshedTool::~QgsVBSViewshedTool()
{
  delete mDrawTool;
}

void QgsVBSViewshedTool::drawFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    mIface->messageBar()->pushMessage( tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." ), QgsMessageBar::INFO, 10 );
    emit finished();
    return;
  }

  QgsCoordinateReferenceSystem canvasCrs = mMapCanvas->mapSettings().destinationCrs();
  double curRadius;
  QgsPoint center;
  double trash;
  if ( dynamic_cast<QgsMapToolDrawCircularSector*>( mDrawTool ) )
  {
    static_cast<QgsMapToolDrawCircularSector*>( mDrawTool )->getPart( 0, center, curRadius, trash, trash );
  }
  else
  {
    static_cast<QgsMapToolDrawCircle*>( mDrawTool )->getPart( 0, center, curRadius );
  }
  QGis::UnitType measureUnit = canvasCrs.mapUnits();
  QgsDistanceArea().convertMeasurement( curRadius, measureUnit, QGis::Meters, false );

  QDialog heightDialog;
  heightDialog.setWindowTitle( tr( "Viewshed setup" ) );
  QGridLayout* heightDialogLayout = new QGridLayout();
  heightDialogLayout->addWidget( new QLabel( tr( "Observer height:" ) ), 0, 0, 1, 1 );
  QDoubleSpinBox* spinObserverHeight = new QDoubleSpinBox();
  spinObserverHeight->setRange( 0, 8000 );
  spinObserverHeight->setDecimals( 1 );
  spinObserverHeight->setValue( 2. );
  spinObserverHeight->setSuffix( " m" );
  heightDialogLayout->addWidget( spinObserverHeight, 0, 1, 1, 1 );
  heightDialogLayout->addWidget( new QLabel( tr( "Target height:" ) ), 1, 0, 1, 1 );
  QDoubleSpinBox* spinTargetHeight = new QDoubleSpinBox();
  spinTargetHeight->setRange( 0, 8000 );
  spinTargetHeight->setDecimals( 1 );
  spinTargetHeight->setValue( 2. );
  spinTargetHeight->setSuffix( " m" );
  heightDialogLayout->addWidget( spinTargetHeight, 1, 1, 1, 1 );
  heightDialogLayout->addWidget( new QLabel( tr( "Radius:" ) ), 2, 0, 1, 1 );
  QDoubleSpinBox* spinRadius = new QDoubleSpinBox();
  spinRadius->setRange( 0, 100000 );
  spinRadius->setDecimals( 0 );
  spinRadius->setValue( curRadius );
  spinRadius->setSuffix( " m" );
  spinRadius->setKeyboardTracking( false );
  connect( spinRadius, SIGNAL( valueChanged( double ) ), this, SLOT( adjustRadius( double ) ) );
  heightDialogLayout->addWidget( spinRadius, 2, 1, 1, 1 );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, SIGNAL( accepted() ), &heightDialog, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), &heightDialog, SLOT( reject() ) );
  heightDialogLayout->addWidget( bbox, 3, 0, 1, 2 );
  heightDialog.setLayout( heightDialogLayout );
  heightDialog.setFixedSize( heightDialog.sizeHint() );
  if ( heightDialog.exec() == QDialog::Rejected )
  {
    emit finished();
    return;
  }

  QString outputFileName = QString( "viewshed_%1,%2.tif" ).arg( center.x() ).arg( center.y() );
  QString outputFile = QgsTemporaryFile::createNewFile( outputFileName );

  QVector<QgsPoint> filterRegion;
  if ( dynamic_cast<QgsMapToolDrawCircularSector*>( mDrawTool ) )
  {
    QgsPolygon poly = QgsGeometry( mDrawTool->getRubberBand()->geometry()->clone() ).asPolygon();
    if ( !poly.isEmpty() )
    {
      filterRegion = poly.front();
    }
    static_cast<QgsMapToolDrawCircularSector*>( mDrawTool )->getPart( 0, center, curRadius, trash, trash );
  }
  else
  {
    static_cast<QgsMapToolDrawCircle*>( mDrawTool )->getPart( 0, center, curRadius );
  }

  QProgressDialog p( tr( "Calculating viewshed..." ), tr( "Abort" ), 0, 0 );
  p.setWindowModality( Qt::WindowModal );
  bool success = QgsViewshed::computeViewshed( layer->source(), outputFile, "GTiff", center, canvasCrs, spinObserverHeight->value(), spinTargetHeight->value(), curRadius, QGis::Meters, filterRegion, &p );
  if ( success )
  {
    QgsRasterLayer* layer = new QgsRasterLayer( outputFile, tr( "Viewshed [%1]" ).arg( center.toString() ) );
    QgsColorRampShader* rampShader = new QgsColorRampShader();
    QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
        << QgsColorRampShader::ColorRampItem( 255, QColor( 0, 255, 0 ), tr( "Visible" ) );
    rampShader->setColorRampItemList( colorRampItems );
    QgsRasterShader* shader = new QgsRasterShader();
    shader->setRasterShaderFunction( rampShader );
    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer( 0, 1, shader );
    layer->setRenderer( renderer );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
  }
  emit finished();
}

void QgsVBSViewshedTool::adjustRadius( double newRadius )
{
  QGis::UnitType measureUnit = QGis::Meters;
  QGis::UnitType targetUnit = mMapCanvas->mapSettings().destinationCrs().mapUnits();
  QgsDistanceArea().convertMeasurement( newRadius, measureUnit, targetUnit, false );
  if ( dynamic_cast<QgsMapToolDrawCircularSector*>( mDrawTool ) )
  {
    double radius, startAngle, stopAngle;
    QgsPoint center;
    static_cast<QgsMapToolDrawCircularSector*>( mDrawTool )->getPart( 0, center, radius, startAngle, stopAngle );
    static_cast<QgsMapToolDrawCircularSector*>( mDrawTool )->setPart( 0, center, newRadius, startAngle, stopAngle );
  }
  else
  {
    double radius;
    QgsPoint center;
    static_cast<QgsMapToolDrawCircle*>( mDrawTool )->getPart( 0, center, radius );
    static_cast<QgsMapToolDrawCircle*>( mDrawTool )->setPart( 0, center, newRadius );
  }
  mDrawTool->update();
}
