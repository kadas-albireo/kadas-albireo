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
#include "qgsgpsconnectionregistry.h"
#include "qgsgpsdetector.h"
#include "qgsgpsmarker.h"
#include "gps/info.h"
#include <QSettings>

QgsMapCanvasGPSDisplay::QgsMapCanvasGPSDisplay( QgsMapCanvas* canvas ): QObject( 0 ), mCanvas( canvas ), mShowMarker( true ),
    mMarker( 0 ), mCurFixStatus( NoFix ), mConnection( 0 ), mRecenterMap( Never )
{
  init();
}

QgsMapCanvasGPSDisplay::QgsMapCanvasGPSDisplay(): QObject( 0 ), mCanvas( 0 ), mShowMarker( true ), mMarker( 0 ), mCurFixStatus( NoFix ), mConnection( 0 ), mRecenterMap( Never )
{
  init();
}

QgsMapCanvasGPSDisplay::~QgsMapCanvasGPSDisplay()
{
  closeGPSConnection();

  QSettings s;
  s.setValue( "/gps/markerSize", mMarkerSize );
  s.setValue( "/gps/mapExtentMultiplier", mSpinMapExtentMultiplier );
  QString panModeString = "none";
  if ( mRecenterMap == QgsMapCanvasGPSDisplay::Always )
  {
    panModeString = "recenterAlways";
  }
  else if ( mRecenterMap == QgsMapCanvasGPSDisplay::WhenNeeded )
  {
    panModeString = "recenterWhenNeeded";
  }
  s.setValue( "/gps/panMode", panModeString );
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
  mCurFixStatus = NoFix;
}

void QgsMapCanvasGPSDisplay::init()
{
  mMarkerSize = defaultMarkerSize();
  mSpinMapExtentMultiplier = defaultSpinMapExtentMultiplier();
  mRecenterMap = defaultRecenterMode();
  mWgs84CRS.createFromOgcWmsCrs( "EPSG:4326" );
}

void QgsMapCanvasGPSDisplay::closeGPSConnection()
{
  if ( mConnection )
  {
    QgsGPSConnectionRegistry::instance()->unregisterConnection( mConnection );
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
  QgsGPSConnectionRegistry::instance()->registerConnection( mConnection );
  connect( conn, SIGNAL( stateChanged( const QgsGPSInformation& ) ), this, SLOT( updateGPSInformation( const QgsGPSInformation& ) ) );
  connect( conn, SIGNAL( nmeaSentenceReceived( const QString& ) ), this, SIGNAL( nmeaSentenceReceived( QString ) ) );
  emit gpsConnected();
}

void QgsMapCanvasGPSDisplay::gpsDetectionFailed()
{
  emit gpsConnectionFailed();
}

void QgsMapCanvasGPSDisplay::updateGPSInformation( const QgsGPSInformation& info )
{
  FixStatus fixStatus = NoData;

  // no fix if any of the three report bad; default values are invalid values and won't be changed if the corresponding NMEA msg is not received
  if ( info.status == 'V' || info.fixType == NMEA_FIX_BAD || info.quality == 0 ) // some sources say that 'V' indicates position fix, but is below acceptable quality
  {
    fixStatus = NoFix;
  }
  else if ( info.fixType == NMEA_FIX_2D ) // 2D indication (from GGA)
  {
    fixStatus = Fix2D;
  }
  else if ( info.status == 'A' || info.fixType == NMEA_FIX_3D || info.quality > 0 ) // good
  {
    fixStatus = Fix3D;
  }
  if ( fixStatus != mCurFixStatus )
  {
    emit gpsFixStatusChanged( fixStatus );
  }
  mCurFixStatus = fixStatus;

  emit gpsInformationReceived( info ); //send signal for service who want to do further actions (e.g. satellite position display, digitising, ...)

  if ( !mCanvas || !QgsGPSConnection::gpsInfoValid( info ) )
  {
    return;
  }

  QgsPoint position( info.longitude, info.latitude );
  if ( mRecenterMap != Never && mLastGPSPosition != position )
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

    if ( mRecenterMap == Always || !extentLimit.contains( centerPoint ) )
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

QgsGPSInformation QgsMapCanvasGPSDisplay::currentGPSInformation() const
{
  if ( mConnection )
  {
    return mConnection->currentGPSInformation();
  }

  return QgsGPSConnection::emptyGPSInformation();
}

QgsMapCanvasGPSDisplay::RecenterMode QgsMapCanvasGPSDisplay::defaultRecenterMode()
{
  QSettings s;
  QString panMode = s.value( "/gps/panMode", "none" ).toString();
  if ( panMode == "recenterAlways" )
  {
    return QgsMapCanvasGPSDisplay::Always;
  }
  else if ( panMode == "none" )
  {
    return QgsMapCanvasGPSDisplay::Never;
  }
  else //recenterWhenNeeded
  {
    return QgsMapCanvasGPSDisplay::WhenNeeded;
  }
}
