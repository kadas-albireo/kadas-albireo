/***************************************************************************
                         qgsmapcanvasgpsdisplay.cpp
                         --------------------------
    begin                : Jan 2016
    copyright            : (C) 2016 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmapcanvasgpsdisplay.h"
#include "qgscoordinatetransform.h"
#include "qgsmapcanvas.h"
#include "qgsgpsconnection.h"
#include "qgsgpsdetector.h"
#include "qgsgpsmarker.h"
#include <QSettings>

QgsMapCanvasGPSDisplay::QgsMapCanvasGPSDisplay( QgsMapCanvas* canvas ): QObject( 0 ), mCanvas( canvas ), mCenterMap( false ), mShowMarker( true ),
    mMarker( 0 ), mConnection( 0 )
{
  init();
}

QgsMapCanvasGPSDisplay::QgsMapCanvasGPSDisplay(): QObject( 0 ), mCanvas( 0 ), mCenterMap( false ), mShowMarker( true ), mMarker( 0 ), mConnection( 0 )
{
  init();
}

QgsMapCanvasGPSDisplay::~QgsMapCanvasGPSDisplay()
{
  closeGPSConnection();
}

void QgsMapCanvasGPSDisplay::connectGPS()
{
  closeGPSConnection();
  QgsGPSDetector* gpsDetector = new QgsGPSDetector( mPort );
  connect( gpsDetector, SIGNAL( detected( QgsGPSConnection* ) ), this, SLOT( gpsDetected( QgsGPSConnection* ) ) );
  connect( gpsDetector, SIGNAL( detectionFailed() ), this, SLOT( gpsDetectionFailed() ) );
  gpsDetector->advance();
}

void QgsMapCanvasGPSDisplay::disconnectGPS()
{
  closeGPSConnection();
}

void QgsMapCanvasGPSDisplay::init()
{
  mMarkerSize = defaultMarkerSize();
  mSpinMapExtentMultiplier = defaultSpinMapExtentMultiplier();
  mWgs84CRS.createFromOgcWmsCrs( "EPSG:4326" );
}

void QgsMapCanvasGPSDisplay::closeGPSConnection()
{
  if ( mConnection )
  {
    mConnection->close();
    delete mConnection;
    mConnection = 0;
    emit gpsDisconnected();
  }
  removeMarker();
}

void QgsMapCanvasGPSDisplay::gpsDetected( QgsGPSConnection* conn )
{
  mConnection = conn;
  connect( conn, SIGNAL( stateChanged( const QgsGPSInformation& ) ), this, SLOT( updateGPSInformation( const QgsGPSInformation& ) ) );
  emit gpsConnected();
}

void QgsMapCanvasGPSDisplay::gpsDetectionFailed()
{
  emit gpsConnectionFailed();
}

void QgsMapCanvasGPSDisplay::updateGPSInformation( const QgsGPSInformation& info )
{
  //todo: send signal for service who want to do further actions (e.g. satellite position display, digitising, ...)
  if ( !mCanvas || !QgsGPSConnection::gpsInfoValid( info ) )
  {
    return;
  }

  QgsPoint position( info.longitude, info.latitude );
  if ( mCenterMap && mLastGPSPosition != position )
  {
    //recenter map
    QgsCoordinateReferenceSystem destCRS = mCanvas->mapSettings().destinationCrs();
    QgsCoordinateTransform myTransform( mWgs84CRS, destCRS ); // use existing WGS84 CRS

    QgsPoint centerPoint = myTransform.transform( position );
    QgsRectangle myRect( centerPoint, centerPoint );

    // testing if position is outside some proportion of the map extent
    // this is a user setting - useful range: 5% to 100% (0.05 to 1.0)
    QgsRectangle extentLimit( mCanvas->extent() );
    extentLimit.scale( mSpinMapExtentMultiplier * 0.01 );

    if ( !extentLimit.contains( centerPoint ) )
    {
      mCanvas->setExtent( myRect );
      mCanvas->refresh();
    }
    mLastGPSPosition = position;
  }

  if ( mShowMarker )
  {
    if ( ! mMarker )
    {
      mMarker = new QgsGpsMarker( mCanvas );
    }
    mMarker->setSize( mMarkerSize );
    mMarker->setCenter( position );
  }
}

int QgsMapCanvasGPSDisplay::defaultMarkerSize()
{
  QSettings s;
  return s.value( "/gps/markerSize", "24" ).toInt();
}

int QgsMapCanvasGPSDisplay::defaultSpinMapExtentMultiplier()
{
  QSettings s;
  return s.value( "/gps/mapExtentMultiplier", "50" ).toInt();
}

void QgsMapCanvasGPSDisplay::removeMarker()
{
  delete mMarker;
  mMarker = 0;
}
