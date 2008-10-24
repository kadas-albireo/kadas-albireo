/***************************************************************************
    qgsmeasuretool.cpp  -  map tool for measuring distances and areas
    ---------------------
    begin                : April 2007
    copyright            : (C) 2007 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsdistancearea.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"

#include "qgsmeasuredialog.h"
#include "qgsmeasuretool.h"
#include "qgscursors.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>

QgsMeasureTool::QgsMeasureTool( QgsMapCanvas* canvas, bool measureArea )
    : QgsMapTool( canvas )
{
  mMeasureArea = measureArea;

  mRubberBand = new QgsRubberBand( canvas, mMeasureArea );

  QPixmap myCrossHairQPixmap = QPixmap(( const char ** ) cross_hair_cursor );
  mCursor = QCursor( myCrossHairQPixmap, 8, 8 );

  mRightMouseClicked = false;

  mDialog = new QgsMeasureDialog( this );
}

QgsMeasureTool::~QgsMeasureTool()
{
  delete mRubberBand;
}


const QList<QgsPoint>& QgsMeasureTool::points()
{
  return mPoints;
}


void QgsMeasureTool::activate()
{
  mDialog->restorePosition();
  QgsMapTool::activate();
  mRightMouseClicked = false;

  // ensure that we have correct settings
  updateProjection();

  // If we suspect that they have data that is projected, yet the
  // map CRS is set to a geographic one, warn them.
  if ( mCanvas->mapRenderer()->distanceArea()->geographic() &&
       ( mCanvas->extent().height() > 360 ||
         mCanvas->extent().width() > 720 ) )
  {
    QMessageBox::warning( NULL, tr( "Incorrect measure results" ),
                          tr( "<p>This map is defined with a geographic coordinate system "
                              "(latitude/longitude) "
                              "but the map extents suggests that it is actually a projected "
                              "coordinate system (e.g., Mercator). "
                              "If so, the results from line or area measurements will be "
                              "incorrect.</p>"
                              "<p>To fix this, explicitly set an appropriate map coordinate "
                              "system using the <tt>Settings:Project Properties</tt> menu." ) );
    mWrongProjectProjection = true;
  }
}

void QgsMeasureTool::deactivate()
{
  mDialog->close();
  QgsMapTool::deactivate();
}


void QgsMeasureTool::restart()
{
  updateProjection();
  mPoints.clear();

  mRubberBand->reset( mMeasureArea );

  // re-read color settings
  QSettings settings;
  int myRed = settings.value( "/qgis/default_measure_color_red", 180 ).toInt();
  int myGreen = settings.value( "/qgis/default_measure_color_green", 180 ).toInt();
  int myBlue = settings.value( "/qgis/default_measure_color_blue", 180 ).toInt();
  mRubberBand->setColor( QColor( myRed, myGreen, myBlue ) );

  mRightMouseClicked = false;
  mWrongProjectProjection = false;

}





void QgsMeasureTool::updateProjection()
{
  // set ellipsoid
  QSettings settings;
  // QString ellipsoid = settings.readEntry("/qgis/measure/ellipsoid", "WGS84");
  // mCalc->setEllipsoid(ellipsoid);

  // set source CRS and projections enabled flag
  // QgsMapRenderer* mapRender = mCanvas->mapRenderer();
  // mCalc->setProjectionsEnabled(mapRender->hasCrsTransformEnabled());
  // int srsid = mapRender->destinationSrs().srsid();
  // mCalc->setSourceCrs(srsid);

  int myRed = settings.value( "/qgis/default_measure_color_red", 180 ).toInt();
  int myGreen = settings.value( "/qgis/default_measure_color_green", 180 ).toInt();
  int myBlue = settings.value( "/qgis/default_measure_color_blue", 180 ).toInt();
  mRubberBand->setColor( QColor( myRed, myGreen, myBlue ) );

}

//////////////////////////

void QgsMeasureTool::canvasPressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( mRightMouseClicked )
      mDialog->restart();

    QgsPoint  idPoint = mCanvas->getCoordinateTransform()->toMapCoordinates( e->x(), e->y() );
    mDialog->mousePress( idPoint );
  }
}


void QgsMeasureTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( !mRightMouseClicked )
  {
    QgsPoint point = mCanvas->getCoordinateTransform()->toMapCoordinates( e->pos().x(), e->pos().y() );
    mRubberBand->movePoint( point );
    mDialog->mouseMove( point );
  }
}


void QgsMeasureTool::canvasReleaseEvent( QMouseEvent * e )
{
  QgsPoint point = mCanvas->getCoordinateTransform()->toMapCoordinates( e->x(), e->y() );

  if ( e->button() == Qt::RightButton && ( e->buttons() & Qt::LeftButton ) == 0 ) // restart
  {
    if ( mRightMouseClicked )
      mDialog->restart();
    else
      mRightMouseClicked = true;
  }
  else if ( e->button() == Qt::LeftButton )
  {
    addPoint( point );
    mDialog->show();
  }

}


void QgsMeasureTool::addPoint( QgsPoint &point )
{
  QgsDebugMsg( "point=" + point.toString() );

  if ( mWrongProjectProjection )
  {
    updateProjection();
    mWrongProjectProjection = false;
  }

  // don't add points with the same coordinates
  if ( mPoints.size() > 0 && point == mPoints[0] )
    return;

  QgsPoint pnt( point );
  mPoints.append( pnt );


  mRubberBand->addPoint( point );
  mDialog->addPoint( point );
}
