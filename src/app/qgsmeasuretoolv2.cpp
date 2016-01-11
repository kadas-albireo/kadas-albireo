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

#include "qgscursors.h"
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

QgsMeasureWidget::QgsMeasureWidget( QgsMapCanvas *canvas, bool measureAngle )
    : QFrame( canvas ), mMeasureAngle( measureAngle )
{
  mCanvas = canvas;
  setLayout( new QHBoxLayout() );
  if ( !measureAngle )
  {
    layout()->addWidget( new QLabel( QString( "<b>%1</b>" ).arg( tr( "Total:" ) ) ) );
  }

  mMeasurementLabel = new QLabel();
  mMeasurementLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
  layout()->addWidget( mMeasurementLabel );

  static_cast<QHBoxLayout*>( layout() )->addStretch( 1 );

  mUnitComboBox = new QComboBox();
  if ( !mMeasureAngle )
  {
    mUnitComboBox->addItem( QGis::tr( QGis::Meters ), static_cast<int>( QGis::Meters ) );
    mUnitComboBox->addItem( QGis::tr( QGis::Feet ), static_cast<int>( QGis::Feet ) );
    mUnitComboBox->addItem( QGis::tr( QGis::Degrees ), static_cast<int>( QGis::Degrees ) );
    mUnitComboBox->addItem( QGis::tr( QGis::NauticalMiles ), static_cast<int>( QGis::NauticalMiles ) );
    QString units = QSettings().value( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Meters ) ).toString();
    mUnitComboBox->setCurrentIndex( mUnitComboBox->findText( QGis::tr( QGis::fromLiteral( units ) ), Qt::MatchFixedString ) );
  }
  else
  {
    mUnitComboBox->addItem( tr( "Degrees" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_DEGREES ) );
    mUnitComboBox->addItem( tr( "Radians" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_RADIANS ) );
    mUnitComboBox->addItem( tr( "Gradians" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_GRADIANS ) );
    mUnitComboBox->addItem( tr( "Angular Mil" ), static_cast<int>( QgsGeometryRubberBand::ANGLE_MIL ) );
    mUnitComboBox->setCurrentIndex( 0 );
  }

  connect( mUnitComboBox, SIGNAL( currentIndexChanged( const QString & ) ), this, SIGNAL( unitsChanged( ) ) );
  layout()->addWidget( mUnitComboBox );

  QToolButton* pickButton = new QToolButton();
  pickButton->setIcon( QIcon( ":/images/themes/default/mActionSelect.svg" ) );
  pickButton->setToolTip( tr( "Pick existing geometry" ) );
  connect( pickButton, SIGNAL( clicked( bool ) ), this, SIGNAL( pickRequested() ) );
  layout()->addWidget( pickButton );

  QToolButton* clearButton = new QToolButton();
  clearButton->setIcon( QIcon( ":/images/themes/default/mIconClear.png" ) );
  clearButton->setToolTip( tr( "Clear" ) );
  connect( clearButton, SIGNAL( clicked( bool ) ), this, SIGNAL( clearRequested() ) );
  layout()->addWidget( clearButton );

  mCanvas->installEventFilter( this );

  setStyleSheet( QString( "QFrame { background-color: orange; }" ) );
  setCursor( Qt::ArrowCursor );

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

bool QgsMeasureWidget::eventFilter( QObject* obj, QEvent* event )
{
  if ( obj == mCanvas && event->type() == QEvent::Resize )
  {
    updatePosition();
  }
  return false;
}

void QgsMeasureWidget::updatePosition()
{
  int w = width();
  int h = height();
  QRect canvasGeometry = mCanvas->geometry();
  move( canvasGeometry.x() + 0.5 * ( canvasGeometry.width() - w ),
        canvasGeometry.y() + canvasGeometry.height() - h );
}

///////////////////////////////////////////////////////////////////////////////


QgsMeasureToolV2::QgsMeasureToolV2( QgsMapCanvas *canvas, MeasureMode measureMode )
    : QgsMapTool( canvas ), mPickFeature( false ), mMeasureMode( measureMode )
{
  if ( mMeasureMode == MeasureCircle )
  {
    mDrawTool = new QgsMapToolDrawCircle( canvas );
  }
  else
  {
    mDrawTool = new QgsMapToolDrawPolyLine( canvas, mMeasureMode == MeasurePolygon );
  }
  mDrawTool->setParent( this );
  mDrawTool->setAllowMultipart( mMeasureMode != MeasureAngle );
  mDrawTool->setShowNodes( true );
  mDrawTool->setSnapPoints( true );
  mMeasureWidget = 0;
  connect( mDrawTool, SIGNAL( geometryChanged() ), this, SLOT( updateTotal() ) );
}

void QgsMeasureToolV2::addGeometry( const QgsGeometry* geometry, const QgsVectorLayer* layer )
{
  mDrawTool->addGeometry( geometry->geometry(), layer->crs() );
}

void QgsMeasureToolV2::activate()
{
  mPickFeature = false;
  mMeasureWidget = new QgsMeasureWidget( mCanvas, mMeasureMode == MeasureAngle );
  setUnits();
  connect( mMeasureWidget, SIGNAL( unitsChanged() ), this, SLOT( setUnits() ) );
  connect( mMeasureWidget, SIGNAL( clearRequested() ), mDrawTool, SLOT( reset() ) );
  connect( mMeasureWidget, SIGNAL( pickRequested() ), this, SLOT( requestPick() ) );
  setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );
  QgsMapTool::activate();
}

void QgsMeasureToolV2::deactivate()
{
  delete mMeasureWidget;
  mMeasureWidget = 0;
  mDrawTool->reset();
  QgsMapTool::deactivate();
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
      mDrawTool->setMeasurementMode( QgsGeometryRubberBand::MEASURE_ANGLES, QGis::Meters, mMeasureWidget->currentAngleUnit() ); break;
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
  setCursor( QCursor( Qt::ArrowCursor ) );
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
    QPair<QgsFeature, QgsVectorLayer*> pickResult = QgsFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), ( mMeasureMode == MeasureLine || mMeasureMode == MeasureAngle ) ? QGis::Line : QGis::Polygon );
    if ( pickResult.first.isValid() )
    {
      mDrawTool->addGeometry( pickResult.first.geometry()->geometry(), pickResult.second->crs() );
    }
    mPickFeature = false;
    setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );
  }
}
