/***************************************************************************
                                 qgsmeasure.h
                               ------------------
        begin                : March 2005
        copyright            : (C) 2005 by Radim Blazek
        email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmeasuredialog.h"
#include "qgsmeasuretool.h"

#include "qgslogger.h"
#include "qgscontexthelp.h"
#include "qgsdistancearea.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsproject.h"
#include "qgscoordinatereferencesystem.h"

#include <QCloseEvent>
#include <QLocale>
#include <QSettings>
#include <QPushButton>


QgsMeasureDialog::QgsMeasureDialog( QgsMeasureTool* tool, Qt::WindowFlags f )
    : QDialog( tool->canvas()->topLevelWidget(), f ), mTool( tool )
{
  setupUi( this );

  mMeasureArea = tool->measureArea();

  QPushButton *nb = new QPushButton( tr( "&New" ) );
  buttonBox->addButton( nb, QDialogButtonBox::ActionRole );
  connect( nb, SIGNAL( clicked() ), this, SLOT( restart() ) );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( this, SIGNAL( finished( int ) ), this, SLOT( finish() ) );

  mUnitsCombo->addItem( QGis::tr( QGis::Meters ) );
  mUnitsCombo->addItem( QGis::tr( QGis::Feet ) );
  mUnitsCombo->addItem( QGis::tr( QGis::Degrees ) );
  mUnitsCombo->addItem( QGis::tr( QGis::NauticalMiles ) );

  QSettings settings;
  QString units = settings.value( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Meters ) ).toString();
  mUnitsCombo->setCurrentIndex( mUnitsCombo->findText( QGis::tr( QGis::fromLiteral( units ) ), Qt::MatchFixedString ) );
  connect( mUnitsCombo, SIGNAL( currentIndexChanged( const QString & ) ), this, SLOT( unitsChanged( const QString & ) ) );

  groupBox->setCollapsed( true );

  restoreGeometry( settings.value( "/Windows/Measure/geometry" ).toByteArray() );

  updateSettings();
}

void QgsMeasureDialog::updateSettings()
{
  QSettings settings;

  mDecimalPlaces = settings.value( "/qgis/measure/decimalplaces", "3" ).toInt();
  mCanvasUnits = mTool->canvas()->mapUnits();
  // Configure QgsDistanceArea
  mDisplayUnits = QGis::fromTr( mUnitsCombo->currentText() );
  mDa.setSourceCrs( mTool->canvas()->mapSettings().destinationCrs().srsid() );
  mDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
  // Only use ellipsoidal calculation when project wide transformation is enabled.
  mDa.setEllipsoidalMode( mTool->canvas()->mapSettings().hasCrsTransformEnabled() );

  QgsDebugMsg( "****************" );
  QgsDebugMsg( QString( "Ellipsoid ID : %1" ).arg( mDa.ellipsoid() ) );
  QgsDebugMsg( QString( "Ellipsoidal  : %1" ).arg( mDa.ellipsoidalEnabled() ? "true" : "false" ) );
  QgsDebugMsg( QString( "Decimalplaces: %1" ).arg( mDecimalPlaces ) );
  QgsDebugMsg( QString( "Display units: %1" ).arg( QGis::toLiteral( mDisplayUnits ) ) );
  QgsDebugMsg( QString( "Canvas units : %1" ).arg( QGis::toLiteral( mCanvasUnits ) ) );

  updateUi();
}

void QgsMeasureDialog::unitsChanged( const QString &units )
{
  mDisplayUnits = QGis::fromTr( units );
  updateUi();
}

void QgsMeasureDialog::restart()
{
  mTool->restart();
  updateUi();
}

void QgsMeasureDialog::finish()
{
  QSettings().setValue( "/Windows/Measure/geometry", saveGeometry() );
  mTool->restart();
  mTool->deactivate();
}

void QgsMeasureDialog::addPart()
{
  QTreeWidgetItem * item = new QTreeWidgetItem( mTable, QStringList() << formatValue( 0, mMeasureArea ) );
  item->setTextAlignment( 0, Qt::AlignRight );
  mTable->scrollToItem( item );
}

void QgsMeasureDialog::removePoint()
{
  if ( !mMeasureArea )
  {
    QTreeWidgetItem* parent = mTable->topLevelItem( mTable->topLevelItemCount() - 1 );
    delete parent->takeChild( parent->childCount() - 1 );
  }
  updateMeasurements();
}

QString QgsMeasureDialog::formatValue( double value, bool measureArea )
{
  bool baseUnit = QSettings().value( "/qgis/measure/keepbaseunit", false ).toBool();

  QGis::UnitType newDisplayUnits;
  convertMeasurement( value, newDisplayUnits, measureArea );
  return QgsDistanceArea::textUnit( value, mDecimalPlaces, newDisplayUnits, measureArea, baseUnit );
}

double QgsMeasureDialog::measureGeometry( const QList<QgsPoint> &points, bool measureArea ) const
{
  return measureArea ? mDa.measurePolygon( points ) : mDa.measureLine( points );
}

void QgsMeasureDialog::updateUi()
{
  // Set tooltip to indicate how we calculate measurments
  QString toolTip = tr( "The calculations are based on:" );
  if ( ! mTool->canvas()->hasCrsTransformEnabled() )
  {
    toolTip += "<br> * " + tr( "Project CRS transformation is turned off." ) + " ";
    toolTip += tr( "Canvas units setting is taken from project properties setting (%1)." ).arg( QGis::tr( mCanvasUnits ) );
    toolTip += "<br> * " + tr( "Ellipsoidal calculation is not possible, as project CRS is undefined." );
    setWindowTitle( tr( "Measure (OTF off)" ) );
  }
  else
  {
    if ( mDa.ellipsoidalEnabled() )
    {
      toolTip += "<br> * " + tr( "Project CRS transformation is turned on and ellipsoidal calculation is selected." ) + " ";
      toolTip += "<br> * " + tr( "The coordinates are transformed to the chosen ellipsoid (%1), and the result is in meters" ).arg( mDa.ellipsoid() );
    }
    else
    {
      toolTip += "<br> * " + tr( "Project CRS transformation is turned on but ellipsoidal calculation is not selected." );
      toolTip += "<br> * " + tr( "The canvas units setting is taken from the project CRS (%1)." ).arg( QGis::tr( mCanvasUnits ) );
    }
    setWindowTitle( tr( "Measure (OTF on)" ) );
  }

  if (( mCanvasUnits == QGis::Meters && mDisplayUnits == QGis::Feet ) || ( mCanvasUnits == QGis::Feet && mDisplayUnits == QGis::Meters ) )
  {
    toolTip += "<br> * " + tr( "Finally, the value is converted from %1 to %2." ).arg( QGis::tr( mCanvasUnits ) ).arg( QGis::tr( mDisplayUnits ) );
  }

  editTotal->setToolTip( toolTip );
  mTable->setToolTip( toolTip );
  mNotesLabel->setText( toolTip );

  QGis::UnitType newDisplayUnits;
  double dummy = 1.0;
  convertMeasurement( dummy, newDisplayUnits, true );
  mTable->setHeaderLabels( QStringList() << tr( "Parts" ) );

  mTable->clear();
  const QList< QList< QgsPoint > >& points = mTool->getPoints();
  double total = 0.;
  for ( int i = 0, n = points.size(); i < n; ++i )
  {
    double value = measureGeometry( points[i], mMeasureArea );
    QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << formatValue( value, mMeasureArea ) );
    item->setTextAlignment( 0, Qt::AlignRight );
    mTable->addTopLevelItem( item );
    total += value;
  }
  editTotal->setText( formatValue( total, mMeasureArea ) );

  mTool->updateLabels();
}

void QgsMeasureDialog::convertMeasurement( double &measure, QGis::UnitType &u, bool isArea )
{
  // Helper for converting between meters and feet
  // The parameter &u is out only...

  // Get the canvas units
  QGis::UnitType myUnits = mCanvasUnits;

  QgsDebugMsg( QString( "Preferred display units are %1" ).arg( QGis::toLiteral( mDisplayUnits ) ) );

  mDa.convertMeasurement( measure, myUnits, mDisplayUnits, isArea );
  u = myUnits;
}

void QgsMeasureDialog::updateMeasurements()
{
  if ( mTool->getPoints().isEmpty() )
  {
    return;
  }

  QTreeWidgetItem *item = mTable->topLevelItem( mTable->topLevelItemCount() - 1 );
  double value = measureGeometry( mTool->getPoints().last(), mMeasureArea );
  item->setText( 0, formatValue( value, mMeasureArea ) );
  item->setData( 0, Qt::UserRole, value );
  double total = 0;
  for ( int i = 0, n = mTable->topLevelItemCount(); i < n; ++i )
  {
    total += mTable->topLevelItem( i )->data( 0, Qt::UserRole ).toDouble();
  }
  editTotal->setText( formatValue( total, mMeasureArea ) );
}

QString QgsMeasureDialog::getPartMeasurement( int partIdx ) const
{
  QTreeWidgetItem* item = mTable->topLevelItem( partIdx );
  return item ? item->text( 0 ) : "0";
}
