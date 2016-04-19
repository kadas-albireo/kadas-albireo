/***************************************************************************
 *  qgsmaptoolviewshed.cpp                                                 *
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

#include "qgisapp.h"
#include "qgsmaptoolviewshed.h"
#include "qgisinterface.h"
#include "qgscircularstringv2.h"
#include "qgscoordinateformat.h"
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
#include "qgssinglebandpseudocolorrenderer.h"
#include "qgsviewshed.h"
#include "qgspinannotationitem.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QProgressDialog>

QgsViewshedDialog::QgsViewshedDialog( double radius, QWidget *parent )
    : QDialog( parent )
{
  setWindowTitle( tr( "Viewshed setup" ) );

  QGis::UnitType vertDisplayUnit = QgsCoordinateFormat::instance()->getHeightDisplayUnit();

  QGridLayout* heightDialogLayout = new QGridLayout();

  heightDialogLayout->addWidget( new QLabel( tr( "Observer height:" ) ), 0, 0, 1, 1 );
  mSpinBoxObserverHeight = new QDoubleSpinBox();
  mSpinBoxObserverHeight->setRange( 0, 8000 );
  mSpinBoxObserverHeight->setDecimals( 1 );
  mSpinBoxObserverHeight->setValue( 2. );
  mSpinBoxObserverHeight->setSuffix( vertDisplayUnit == QGis::Feet ? " ft" : " m" );
  heightDialogLayout->addWidget( mSpinBoxObserverHeight, 0, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Target height:" ) ), 1, 0, 1, 1 );
  mSpinBoxTargetHeight = new QDoubleSpinBox();
  mSpinBoxTargetHeight->setRange( 0, 8000 );
  mSpinBoxTargetHeight->setDecimals( 1 );
  mSpinBoxTargetHeight->setValue( 2. );
  mSpinBoxTargetHeight->setSuffix( vertDisplayUnit == QGis::Feet ? " ft" : " m" );
  heightDialogLayout->addWidget( mSpinBoxTargetHeight, 1, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Heights relative to:" ) ), 2, 0, 1, 1 );
  mComboHeightMode = new QComboBox();
  mComboHeightMode->addItem( tr( "Ground" ), static_cast<int>( HeightRelToGround ) );
  mComboHeightMode->addItem( tr( "Sea level" ), static_cast<int>( HeightRelToSeaLevel ) );
  heightDialogLayout->addWidget( mComboHeightMode, 2, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Radius:" ) ), 3, 0, 1, 1 );
  QDoubleSpinBox* spinRadius = new QDoubleSpinBox();
  spinRadius->setRange( 1, 100000 );
  spinRadius->setDecimals( 0 );
  spinRadius->setValue( radius );
  spinRadius->setSuffix( " m" );
  spinRadius->setKeyboardTracking( false );
  connect( spinRadius, SIGNAL( valueChanged( double ) ), this, SIGNAL( radiusChanged( double ) ) );
  heightDialogLayout->addWidget( spinRadius, 3, 1, 1, 1 );

  heightDialogLayout->addWidget( new QLabel( tr( "Display:" ) ), 4, 0, 1, 1 );
  mDisplayModeCombo = new QComboBox();
  mDisplayModeCombo->addItem( tr( "Visible area" ) );
  mDisplayModeCombo->addItem( tr( "Invisible area" ) );
  heightDialogLayout->addWidget( mDisplayModeCombo, 4, 1, 1, 1 );

  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  heightDialogLayout->addWidget( bbox, 5, 0, 1, 2 );

  setLayout( heightDialogLayout );
  setFixedSize( sizeHint() );
}

double QgsViewshedDialog::getObserverHeight() const
{
  return mSpinBoxObserverHeight->value();
}

double QgsViewshedDialog::getTargetHeight() const
{
  return mSpinBoxTargetHeight->value();
}

bool QgsViewshedDialog::getHeightRelativeToGround() const
{
  return static_cast<HeightMode>( mComboHeightMode->itemData( mComboHeightMode->currentIndex() ).toInt() ) == HeightRelToGround;
}

QgsViewshedDialog::DisplayMode QgsViewshedDialog::getDisplayMode() const
{
  return static_cast<DisplayMode>( mDisplayModeCombo->currentIndex() );
}

///////////////////////////////////////////////////////////////////////////////

QgsMapToolViewshed::QgsMapToolViewshed( QgsMapCanvas* mapCanvas )
    : QgsMapToolDrawCircularSector( mapCanvas )
{
  setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  connect( this, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
}

void QgsMapToolViewshed::drawFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    QgisApp::instance()->messageBar()->pushMessage( tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." ), QgsMessageBar::INFO, 10 );
    reset();
    return;
  }

  QgsCoordinateReferenceSystem canvasCrs = canvas()->mapSettings().destinationCrs();
  double curRadius;
  QgsPoint center;
  double trash;
  getPart( 0, center, curRadius, trash, trash );

  QGis::UnitType measureUnit = canvasCrs.mapUnits();
  QgsDistanceArea().convertMeasurement( curRadius, measureUnit, QGis::Meters, false );

  QgsViewshedDialog viewshedDialog( curRadius );
  connect( &viewshedDialog, SIGNAL( radiusChanged( double ) ), this, SLOT( adjustRadius( double ) ) );
  if ( viewshedDialog.exec() == QDialog::Rejected )
  {
    reset();
    return;
  }

  QString outputFileName = QString( "viewshed_%1,%2.tif" ).arg( center.x() ).arg( center.y() );
  QString outputFile = QgsTemporaryFile::createNewFile( outputFileName );

  QVector<QgsPoint> filterRegion;
  QgsPolygon poly = QgsGeometry( getRubberBand()->geometry()->clone() ).asPolygon();
  if ( !poly.isEmpty() )
  {
    filterRegion = poly.front();
  }
  getPart( 0, center, curRadius, trash, trash );

  if ( mCanvas->mapSettings().mapUnits() == QGis::Degrees )
  {
    // Need to compute radius in meters
    QgsDistanceArea da;
    da.setSourceCrs( mCanvas->mapSettings().destinationCrs() );
    da.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
    da.setEllipsoidalMode( mCanvas->mapSettings().hasCrsTransformEnabled() );
    curRadius = da.measureLine( center, QgsPoint( center.x() + curRadius, center.y() ) );
    QGis::UnitType measureUnits = mCanvas->mapSettings().mapUnits();
    da.convertMeasurement( curRadius, measureUnits, QGis::Meters, false );
  }

  double heightConv = QGis::fromUnitToUnitFactor( QgsCoordinateFormat::instance()->getHeightDisplayUnit(), QGis::Meters );

  QProgressDialog p( tr( "Calculating viewshed..." ), tr( "Abort" ), 0, 0 );
  p.setWindowModality( Qt::WindowModal );
  bool displayVisible = viewshedDialog.getDisplayMode() == QgsViewshedDialog::DisplayVisibleArea;
  bool success = QgsViewshed::computeViewshed( layer->source(), outputFile, "GTiff", center, canvasCrs, viewshedDialog.getObserverHeight() * heightConv, viewshedDialog.getTargetHeight() * heightConv, viewshedDialog.getHeightRelativeToGround(), curRadius, QGis::Meters, filterRegion, displayVisible, &p );
  if ( success )
  {
    QgsRasterLayer* layer = new QgsRasterLayer( outputFile, tr( "Viewshed [%1]" ).arg( center.toString() ) );
    QgsColorRampShader* rampShader = new QgsColorRampShader();
    if ( displayVisible )
    {
      QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
          << QgsColorRampShader::ColorRampItem( 255, QColor( 0, 255, 0 ), tr( "Visible" ) );
      rampShader->setColorRampItemList( colorRampItems );
    }
    else
    {
      QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
          << QgsColorRampShader::ColorRampItem( 0, QColor( 255, 0, 0 ), tr( "Invisible" ) );
      rampShader->setColorRampItemList( colorRampItems );
    }
    QgsRasterShader* shader = new QgsRasterShader();
    shader->setRasterShaderFunction( rampShader );
    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer( 0, 1, shader );
    layer->setRenderer( renderer );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    QgsPinAnnotationItem* pin = new QgsPinAnnotationItem( canvas() );
    pin->setMapPosition( center, canvasCrs );
  }
  reset();
}

void QgsMapToolViewshed::adjustRadius( double newRadius )
{
  QGis::UnitType measureUnit = QGis::Meters;
  QGis::UnitType targetUnit = canvas()->mapSettings().destinationCrs().mapUnits();
  QgsDistanceArea().convertMeasurement( newRadius, measureUnit, targetUnit, false );
  double radius, startAngle, stopAngle;
  QgsPoint center;
  getPart( 0, center, radius, startAngle, stopAngle );
  setPart( 0, center, newRadius, startAngle, stopAngle );
  update();
}
