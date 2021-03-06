/***************************************************************************
    qgsmeasuretoolv2.cpp
    --------------------
    begin                : January 2016
    copyright            : (C) 2016 Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsfeaturepicker.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "qgsmeasuretoolv2.h"
#include "qgsvectorlayer.h"

#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMouseEvent>

QgsMeasureWidget::QgsMeasureWidget( QgsMapCanvas *canvas, QgsMeasureToolV2::MeasureMode measureMode )
    : QgsBottomBar( canvas ), mNorthComboBox( 0 ), mMeasureMode( measureMode )
{
  bool measureAngle = measureMode == QgsMeasureToolV2::MeasureAngle || measureMode == QgsMeasureToolV2::MeasureAzimuth;
  setLayout( new QHBoxLayout() );

  mMeasurementLabel = new QLabel();
  mMeasurementLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
  layout()->addWidget( mMeasurementLabel );

  static_cast<QHBoxLayout*>( layout() )->addStretch( 1 );

  mUnitComboBox = new QComboBox();
  if ( !measureAngle )
  {
    mUnitComboBox->addItem( tr( "Metric" ), static_cast<int>( QGis::Meters ) );
    mUnitComboBox->addItem( tr( "Imperial" ), static_cast<int>( QGis::Feet ) );
//    mUnitComboBox->addItem( QGis::tr( QGis::Degrees ), static_cast<int>( QGis::Degrees ) );
    mUnitComboBox->addItem( tr( "Nautical" ), static_cast<int>( QGis::NauticalMiles ) );
    int defUnit = QSettings().value( "/Qgis/measure/last_measure_unit", QGis::Meters ).toInt();
    mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
  }
  else
  {
    mUnitComboBox->addItem( tr( "Degrees" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_DEGREES ) );
    mUnitComboBox->addItem( tr( "Radians" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_RADIANS ) );
    mUnitComboBox->addItem( tr( "Gradians" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_GRADIANS ) );
    mUnitComboBox->addItem( tr( "Angular Mil" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_MIL ) );
    if ( measureMode == QgsMeasureToolV2::MeasureAngle )
    {
      int defUnit = QSettings().value( "/Qgis/measure/last_angle_unit", static_cast<int>( QgsGeometryRubberBand::ANGLE_DEGREES ) ).toInt();
      mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
    }
    else
    {
      int defUnit = QSettings().value( "/Qgis/measure/last_azimuth_unit", static_cast<int>( QgsGeometryRubberBand::ANGLE_MIL ) ).toInt();
      mUnitComboBox->setCurrentIndex( defUnit );
    }
  }

  connect( mUnitComboBox, SIGNAL( currentIndexChanged( const QString & ) ), this, SIGNAL( unitsChanged( ) ) );
  connect( mUnitComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveDefaultUnits( int ) ) );
  layout()->addWidget( mUnitComboBox );

  if ( measureMode == QgsMeasureToolV2::MeasureAzimuth )
  {
    layout()->addWidget( new QLabel( tr( "North:" ) ) );
    mNorthComboBox = new QComboBox();
    mNorthComboBox->addItem( tr( "Geographic" ), static_cast<int>( QgsGeometryRubberBand::AZIMUTH_NORTH_GEOGRAPHIC ) );
    mNorthComboBox->addItem( tr( "Map" ), static_cast<int>( QgsGeometryRubberBand::AZIMUTH_NORTH_MAP ) );
    int defNorth = QSettings().value( "/Qgis/measure/last_azimuth_north", static_cast<int>( QgsGeometryRubberBand::AZIMUTH_NORTH_GEOGRAPHIC ) ).toInt();
    mNorthComboBox->setCurrentIndex( defNorth );
    connect( mNorthComboBox, SIGNAL( currentIndexChanged( const QString & ) ), this, SIGNAL( unitsChanged( ) ) );
    connect( mNorthComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveAzimuthNorth( int ) ) );
    layout()->addWidget( mNorthComboBox );
  }

  if ( measureMode != QgsMeasureToolV2::MeasureAngle )
  {
    QToolButton* pickButton = new QToolButton();
    pickButton->setIcon( QIcon( ":/images/themes/default/mActionSelect.svg" ) );
    pickButton->setToolTip( tr( "Pick existing geometry" ) );
    connect( pickButton, SIGNAL( clicked( bool ) ), this, SIGNAL( pickRequested() ) );
    layout()->addWidget( pickButton );
  }

  QToolButton* clearButton = new QToolButton();
  clearButton->setIcon( QIcon( ":/images/themes/default/mIconClear.png" ) );
  clearButton->setToolTip( tr( "Clear" ) );
  connect( clearButton, SIGNAL( clicked( bool ) ), this, SIGNAL( clearRequested() ) );
  layout()->addWidget( clearButton );

  QToolButton* closeButton = new QToolButton();
  closeButton->setIcon( QIcon( ":/images/themes/default/mIconClose.png" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, SIGNAL( clicked( bool ) ), this, SIGNAL( closeRequested() ) );
  layout()->addWidget( closeButton );

  show();
  if ( !measureAngle )
  {
    setFixedWidth( 400 );
  }
  else
  {
    setFixedWidth( width() );
  }

  updatePosition();
}

void QgsMeasureWidget::saveDefaultUnits( int index )
{
  if ( mMeasureMode == QgsMeasureToolV2::MeasureAzimuth )
  {
    QSettings().setValue( "/Qgis/measure/last_azimuth_unit", mUnitComboBox->itemData( index ).toInt() );
  }
  else if ( mMeasureMode == QgsMeasureToolV2::MeasureAngle )
  {
    QSettings().setValue( "/Qgis/measure/last_angle_unit", mUnitComboBox->itemData( index ).toInt() );
  }
  else
  {
    QSettings().setValue( "/Qgis/measure/last_measure_unit", mUnitComboBox->itemData( index ).toInt() );
  }
}

void QgsMeasureWidget::saveAzimuthNorth( int index )
{
  QSettings().setValue( "/Qgis/measure/last_azimuth_north", mNorthComboBox->itemData( index ).toInt() );
}

void QgsMeasureWidget::updateMeasurement( const QString& measurement )
{
  mMeasurementLabel->setText( QString( "<b>%1</b>" ).arg( measurement ) );
}

QGis::UnitType QgsMeasureWidget::currentUnit() const
{
  return static_cast<QGis::UnitType>( mUnitComboBox->itemData( mUnitComboBox->currentIndex() ).toInt() );
}

QgsGeometryRubberBand::AngleUnit QgsMeasureWidget::currentAngleUnit() const
{
  return static_cast<QgsGeometryRubberBand::AngleUnit>( mUnitComboBox->itemData( mUnitComboBox->currentIndex() ).toInt() );
}

QgsGeometryRubberBand::AzimuthNorth QgsMeasureWidget::currentAzimuthNorth() const
{
  return static_cast<QgsGeometryRubberBand::AzimuthNorth>( mNorthComboBox->itemData( mNorthComboBox->currentIndex() ).toInt() );
}

///////////////////////////////////////////////////////////////////////////////


QgsMeasureToolV2::QgsMeasureToolV2( QgsMapCanvas *canvas, MeasureMode measureMode )
    : QgsMapTool( canvas ), mPickFeature( false ), mMeasureMode( measureMode )
{
  if ( mMeasureMode == MeasureAngle )
  {
    mDrawTool = new QgsMapToolDrawCircularSector( canvas );
  }
  else if ( mMeasureMode == MeasureCircle )
  {
    mDrawTool = new QgsMapToolDrawCircle( canvas, true );
  }
  else
  {
    mDrawTool = new QgsMapToolDrawPolyLine( canvas, mMeasureMode == MeasurePolygon, true );
  }
  mDrawTool->setParent( this );
  mDrawTool->setAllowMultipart( mMeasureMode != MeasureAngle && mMeasureMode != MeasureAzimuth );
  mDrawTool->getRubberBand()->setIconType( mMeasureMode != MeasureCircle ? QgsGeometryRubberBand::ICON_CIRCLE : QgsGeometryRubberBand::ICON_NONE );
  mDrawTool->setSnapPoints( true );
  mDrawTool->setParentTool( this );
  mMeasureWidget = 0;
  connect( mDrawTool, SIGNAL( geometryChanged() ), this, SLOT( updateTotal() ) );
}

QgsMeasureToolV2::~QgsMeasureToolV2()
{
  delete mDrawTool;
}

void QgsMeasureToolV2::addGeometry( const QgsGeometry* geometry, const QgsVectorLayer* layer )
{
  mDrawTool->addGeometry( geometry->geometry(), layer->crs() );
}

void QgsMeasureToolV2::activate()
{
  mPickFeature = false;
  mMeasureWidget = new QgsMeasureWidget( mCanvas, mMeasureMode );
  setUnits();
  connect( mMeasureWidget, SIGNAL( unitsChanged() ), this, SLOT( setUnits() ) );
  connect( mMeasureWidget, SIGNAL( clearRequested() ), mDrawTool, SLOT( reset() ) );
  connect( mMeasureWidget, SIGNAL( closeRequested() ), this, SLOT( close() ) );
  connect( mMeasureWidget, SIGNAL( pickRequested() ), this, SLOT( requestPick() ) );
  setCursor( Qt::ArrowCursor );
  mDrawTool->getRubberBand()->setVisible( true );
  mDrawTool->setShowInputWidget( QSettings().value( "/Qgis/showNumericInput", false ).toBool() );
  mDrawTool->activate();
  QgsMapTool::activate();
}

void QgsMeasureToolV2::deactivate()
{
  delete mMeasureWidget;
  mMeasureWidget = 0;
  mDrawTool->getRubberBand()->setVisible( false );
  mDrawTool->deactivate();
  QgsMapTool::deactivate();
}

void QgsMeasureToolV2::close()
{
  canvas()->unsetMapTool( this );
}

void QgsMeasureToolV2::setUnits()
{
  switch ( mMeasureMode )
  {
    case MeasureLine:
      mDrawTool->setMeasurementMode( QgsGeometryRubberBand::MEASURE_LINE_AND_SEGMENTS, mMeasureWidget->currentUnit() ); break;
    case MeasurePolygon:
      mDrawTool->setMeasurementMode( QgsGeometryRubberBand::MEASURE_POLYGON, mMeasureWidget->currentUnit() ); break;
    case MeasureCircle:
      mDrawTool->setMeasurementMode( QgsGeometryRubberBand::MEASURE_CIRCLE, mMeasureWidget->currentUnit() ); break;
    case MeasureAngle:
      mDrawTool->setMeasurementMode( QgsGeometryRubberBand::MEASURE_ANGLE, QGis::Meters, mMeasureWidget->currentAngleUnit() ); break;
    case MeasureAzimuth:
      mDrawTool->setMeasurementMode( QgsGeometryRubberBand::MEASURE_AZIMUTH, QGis::Meters, mMeasureWidget->currentAngleUnit(), mMeasureWidget->currentAzimuthNorth() ); break;
  }
}

void QgsMeasureToolV2::updateTotal()
{
  if ( mMeasureWidget )
  {
    mMeasureWidget->updateMeasurement( mDrawTool->getRubberBand()->getTotalMeasurement() );
  }
}

void QgsMeasureToolV2::requestPick()
{
  mPickFeature = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void QgsMeasureToolV2::canvasPressEvent( QMouseEvent *e )
{
  if ( !mPickFeature )
  {
    mDrawTool->canvasPressEvent( e );
  }
}

void QgsMeasureToolV2::canvasMoveEvent( QMouseEvent *e )
{
  if ( !mPickFeature )
  {
    mDrawTool->canvasMoveEvent( e );
  }
}

void QgsMeasureToolV2::canvasReleaseEvent( QMouseEvent *e )
{
  if ( !mPickFeature )
  {
    mDrawTool->canvasReleaseEvent( e );
  }
  else
  {
    QgsFeaturePicker::PickResult pickResult = QgsFeaturePicker::pick( mCanvas, e->pos(), toMapCoordinates( e->pos() ), ( mMeasureMode == MeasureLine || mMeasureMode == MeasureAzimuth ) ? QGis::Line : QGis::Polygon );
    if ( pickResult.feature.isValid() )
    {
      mDrawTool->addGeometry( pickResult.feature.geometry()->geometry(), pickResult.layer->crs() );
    }
    mPickFeature = false;
    setCursor( Qt::ArrowCursor );
  }
}

void QgsMeasureToolV2::keyReleaseEvent( QKeyEvent *e )
{
  if ( mPickFeature && e->key() == Qt::Key_Escape )
  {
    mPickFeature = false;
    setCursor( Qt::ArrowCursor );
  }
  else if ( e->key() == Qt::Key_Escape && mDrawTool->getStatus() == QgsMapToolDrawShape::StatusReady )
  {
    canvas()->unsetMapTool( this );
  }
  else
  {
    mDrawTool->keyReleaseEvent( e );
  }
}
