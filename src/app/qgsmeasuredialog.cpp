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
#include "qgscoordinatereferencesystem.h"

#include <QCloseEvent>
#include <QLocale>
#include <QSettings>
#include <QPushButton>


QgsMeasureDialog::QgsMeasureDialog( QgsMeasureTool* tool, Qt::WFlags f )
    : QDialog( tool->canvas()->topLevelWidget(), f ), mTool( tool )
{
  setupUi( this );

  QPushButton *nb = new QPushButton( tr( "&New" ) );
  buttonBox->addButton( nb, QDialogButtonBox::ActionRole );
  connect( nb, SIGNAL( clicked() ), this, SLOT( restart() ) );

  mMeasureArea = tool->measureArea();
  mTotal = 0.;

  // Set one cell row where to update current distance
  // If measuring area, the table doesn't get shown
  QTreeWidgetItem* item = new QTreeWidgetItem( QStringList( QString::number( 0, 'f', 1 ) ) );
  item->setTextAlignment( 0, Qt::AlignRight );
  mTable->addTopLevelItem( item );

  // Update when the ellipsoidal button has changed state.
  connect( mcbProjectionEnabled, SIGNAL( stateChanged( int ) ),
           this, SLOT( ellipsoidalButton() ) );
  // Update whenever the canvas has refreshed. Maybe more often than needed,
  // but at least every time any canvas related settings changes
  connect( mTool->canvas(), SIGNAL( mapCanvasRefreshed() ),
           this, SLOT( updateSettings() ) );
//   // Update when project wide transformation has changed
//   connect( mTool->canvas()->mapRenderer(), SIGNAL( hasCrsTransformEnabled( bool ) ),
//            this, SLOT( changeProjectionEnabledState() ) );
//   // Update when project CRS has changed
//   connect( mTool->canvas()->mapRenderer(), SIGNAL( destinationSrsChanged() ),
//            this, SLOT( changeProjectionEnabledState() ) );

  updateSettings();
}

void QgsMeasureDialog::ellipsoidalButton()
{
  QSettings settings;

  if ( mcbProjectionEnabled->isChecked() )
  {
    settings.setValue( "/qgis/measure/projectionEnabled", 2 );
  }
  else
  {
    settings.setValue( "/qgis/measure/projectionEnabled", 0 );
  }
  updateSettings();
}

void QgsMeasureDialog::updateSettings()
{
  QSettings settings;

  int s = settings.value( "/qgis/measure/projectionEnabled", "2" ).toInt();
  if ( s == 2 )
  {
    mEllipsoidal = true;
  }
  else
  {
    mEllipsoidal = false;
  }

  mDecimalPlaces = settings.value( "/qgis/measure/decimalplaces", "3" ).toInt();  
  mCanvasUnits = mTool->canvas()->mapUnits();
  mDisplayUnits = QGis::fromLiteral( settings.value( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Meters ) ).toString() );

  QgsDebugMsg( "****************" );
  QgsDebugMsg( QString( "Ellipsoidal: %1" ).arg( mEllipsoidal ? "true" : "false" ) );
  QgsDebugMsg( QString( "Decimalpla.: %1" ).arg( mDecimalPlaces ) );
  QgsDebugMsg( QString( "Display u. : %1" ).arg( QGis::toLiteral( mDisplayUnits ) ) );
  QgsDebugMsg( QString( "Canvas u.  : %1" ).arg( QGis::toLiteral( mCanvasUnits ) ) );
  
  configureDistanceArea();

  // clear interface
  mTable->clear();
  QTreeWidgetItem* item = new QTreeWidgetItem( QStringList( QString::number( 0, 'f', 1 ) ) );
  item->setTextAlignment( 0, Qt::AlignRight );
  mTable->addTopLevelItem( item );
  mTotal = 0;
  updateUi();

}

void QgsMeasureDialog::restart()
{
  mTool->restart();

  // Set one cell row where to update current distance
  // If measuring area, the table doesn't get shown
  mTable->clear();
  QTreeWidgetItem* item = new QTreeWidgetItem( QStringList( QString::number( 0, 'f', 1 ) ) );
  item->setTextAlignment( 0, Qt::AlignRight );
  mTable->addTopLevelItem( item );
  mTotal = 0.;
  updateUi();
}


void QgsMeasureDialog::mousePress( QgsPoint &point )
{
  if ( mTool->points().size() == 0 )
  {
    addPoint( point );
    show();
  }
  raise();

  mouseMove( point );
}

void QgsMeasureDialog::mouseMove( QgsPoint &point )
{
  // show current distance/area while moving the point
  // by creating a temporary copy of point array
  // and adding moving point at the end
  if ( mMeasureArea && mTool->points().size() > 1 )
  {
    QList<QgsPoint> tmpPoints = mTool->points();
    tmpPoints.append( point );
    double area = mDa.measurePolygon( tmpPoints );
    editTotal->setText( formatArea( area ) );
  }
  else if ( !mMeasureArea && mTool->points().size() > 0 )
  {
    QgsPoint p1( mTool->points().last() ), p2( point );

    double d = mDa.measureLine( p1, p2 );
    editTotal->setText( formatDistance( mTotal + d ) );
    QGis::UnitType myDisplayUnits;
    // Ignore units
    convertMeasurement( d, myDisplayUnits, false );
    QTreeWidgetItem *item = mTable->topLevelItem( mTable->topLevelItemCount() - 1 );
    item->setText( 0, QLocale::system().toString( d, 'f', mDecimalPlaces ) );
    QgsDebugMsg( QString( "Final result is %1" ).arg( item->text( 0 ) ) );
  }
}

void QgsMeasureDialog::addPoint( QgsPoint &p )
{
  Q_UNUSED( p );

  int numPoints = mTool->points().size();
  if ( mMeasureArea && numPoints > 2 )
  {
    double area = mDa.measurePolygon( mTool->points() );
    editTotal->setText( formatArea( area ) );
  }
  else if ( !mMeasureArea && numPoints > 1 )
  {
    int last = numPoints - 2;

    QgsPoint p1 = mTool->points()[last], p2 = mTool->points()[last+1];

    double d = mDa.measureLine( p1, p2 );

    mTotal += d;
    editTotal->setText( formatDistance( mTotal ) );

    QGis::UnitType myDisplayUnits;
    // Ignore units
    convertMeasurement( d, myDisplayUnits, false );

    QTreeWidgetItem *item = mTable->topLevelItem( mTable->topLevelItemCount() - 1 );
    item->setText( 0, QLocale::system().toString( d, 'f' ) );

    item = new QTreeWidgetItem( QStringList( QLocale::system().toString( 0.0, 'f' ) ) );
    item->setTextAlignment( 0, Qt::AlignRight );
    mTable->addTopLevelItem( item );
    mTable->scrollToItem( item );
  }
}

void QgsMeasureDialog::on_buttonBox_rejected( void )
{
  restart();
  QDialog::close();
}

void QgsMeasureDialog::closeEvent( QCloseEvent *e )
{
  saveWindowLocation();
  e->accept();
}

void QgsMeasureDialog::restorePosition()
{
  QSettings settings;
  restoreGeometry( settings.value( "/Windows/Measure/geometry" ).toByteArray() );
  int wh;
  if ( mMeasureArea )
    wh = settings.value( "/Windows/Measure/hNoTable", 70 ).toInt();
  else
    wh = settings.value( "/Windows/Measure/h", 200 ).toInt();
  resize( width(), wh );
  updateUi();
}

void QgsMeasureDialog::saveWindowLocation()
{
  QSettings settings;
  settings.setValue( "/Windows/Measure/geometry", saveGeometry() );
  const QString &key = mMeasureArea ? "/Windows/Measure/hNoTable" : "/Windows/Measure/h";
  settings.setValue( key, height() );
}

QString QgsMeasureDialog::formatDistance( double distance )
{
  QSettings settings;
  bool baseUnit = settings.value( "/qgis/measure/keepbaseunit", false ).toBool();

  QGis::UnitType newDisplayUnits;
  convertMeasurement( distance, newDisplayUnits, false );
  return QgsDistanceArea::textUnit( distance, mDecimalPlaces, newDisplayUnits, false, baseUnit );
}

QString QgsMeasureDialog::formatArea( double area )
{
  QSettings settings;
  bool baseUnit = settings.value( "/qgis/measure/keepbaseunit", false ).toBool();

  QGis::UnitType newDisplayUnits;
  convertMeasurement( area, newDisplayUnits, true );
  return QgsDistanceArea::textUnit( area, mDecimalPlaces, newDisplayUnits, true, baseUnit );
}

void QgsMeasureDialog::updateUi()
{
  // If project wide transformation is off, disbale checkbox and unmark it.
  // When on, enable checbox and mark with saved value.
  mcbProjectionEnabled->setEnabled( mTool->canvas()->hasCrsTransformEnabled() );

  // Set tooltip to indicate how we calculate measurments
  QString toolTip = tr( "The calculations are based on:" );
  if ( ! mTool->canvas()->hasCrsTransformEnabled() )
  {
    toolTip += "<br> * " + tr( "Project CRS transformation is turned off." ) + " ";
    toolTip += tr( "Canvas units setting is taken from project properties setting (%1)." ).arg( QGis::tr( mCanvasUnits ) );
    toolTip += "<br> * " + tr( "Ellipsoidal calculation is not possible, as project CRS is undefined." );
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
  }

  if (( mCanvasUnits == QGis::Meters && mDisplayUnits == QGis::Feet ) || ( mCanvasUnits == QGis::Feet && mDisplayUnits == QGis::Meters ) )
  {
    toolTip += "<br> * " + tr( "Finally, the value is converted from %2 to %3." ).arg( QGis::tr( mCanvasUnits ) ).arg( QGis::tr( mDisplayUnits ) );
  }

  editTotal->setToolTip( toolTip );
  mTable->setToolTip( toolTip );

  mTable->setHeaderLabels( QStringList( tr( "Segments [%1]" ).arg( QGis::tr( mDisplayUnits ) ) ) );

  if ( mMeasureArea )
  {
    double area = 0.0;
    if ( mTool->points().size() > 1 )
    {
      area = mDa.measurePolygon( mTool->points() );
    }
    editTotal->setText( formatArea( area ) );
  }
  else
  {
    QList<QgsPoint>::const_iterator it;
    bool b = true; // first point

    QgsPoint p1, p2;

    for ( it = mTool->points().constBegin(); it != mTool->points().constEnd(); ++it )
    {
      p2 = *it;
      if ( !b )
      {
        double d  = mDa.measureLine( p1, p2 );
        mTotal += d;
        editTotal->setText( formatDistance( mTotal ) );
        QGis::UnitType myDisplayUnits;

        convertMeasurement( d, myDisplayUnits, false );

        QTreeWidgetItem *item = mTable->topLevelItem( mTable->topLevelItemCount() - 1 );
        item->setText( 0, QLocale::system().toString( d, 'f' ) );
        item = new QTreeWidgetItem( QStringList( QLocale::system().toString( 0.0, 'f', mDecimalPlaces ) ) );
        item->setTextAlignment( 0, Qt::AlignRight );
        mTable->addTopLevelItem( item );
        mTable->scrollToItem( item );
      }
      p1 = p2;
      b = false;
    }
  }
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

void QgsMeasureDialog::configureDistanceArea()
{
  QSettings settings;
  QString ellipsoidId = settings.value( "/qgis/measure/ellipsoid", "WGS84" ).toString();
  mDa.setSourceCrs( mTool->canvas()->mapRenderer()->destinationCrs().srsid() );
  mDa.setEllipsoid( ellipsoidId );
  // Only use ellipsoidal calculation when project wide transformation is enabled.
  mDa.setEllipsoidalMode( mEllipsoidal && mTool->canvas()->hasCrsTransformEnabled() );
}
