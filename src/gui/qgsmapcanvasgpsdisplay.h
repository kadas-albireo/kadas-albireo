/***************************************************************************
                         qgsmapcanvasgpsdisplay.h
                         ------------------------
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

#ifndef QGSMAPCANVASGPSDISPLAY_H
#define QGSMAPCANVASGPSDISPLAY_H

#include "qgspoint.h"
#include "qgscoordinatereferencesystem.h"
#include <QObject>

class QgsGPSConnection;
struct QgsGPSInformation;
class QgsGpsMarker;
class QgsMapCanvas;

/**Manages display of canvas gps marker and moving of map extent with changing GPS position*/
class GUI_EXPORT QgsMapCanvasGPSDisplay: public QObject
{
    Q_OBJECT
  public:

    enum RecenterMode
    {
      Never,
      Always,
      WhenNeeded
    };

    enum FixStatus
    {
      NoData,
      NoFix,
      Fix2D,
      Fix3D
    };

    QgsMapCanvasGPSDisplay( QgsMapCanvas* canvas );
    QgsMapCanvasGPSDisplay();

    ~QgsMapCanvasGPSDisplay();

    void connectGPS();
    void disconnectGPS();

    void setMapCanvas( QgsMapCanvas* canvas ) { mCanvas = canvas; }

    RecenterMode recenterMap() const { return mRecenterMap; }
    void setRecenterMap( RecenterMode m ) { mRecenterMap = m; }

    bool showMarker() const { return mShowMarker; }
    void setShowMarker( bool showMarker ) { mShowMarker = showMarker; removeMarker(); }

    int markerSize() const { return mMarkerSize; }
    void setMarkerSize( int size ) { mMarkerSize = size; }

    double spinMapExtentMultiplier() const { return mSpinMapExtentMultiplier; }
    void setSpinMapExtentMultiplier( double value ) { mSpinMapExtentMultiplier = value; }

    QString port() const { return mPort; }
    /**Sets the port for the GPS connection. Empty string (default) means autodetect. For gpsd connections, use '<host>:<port>:<device>'.
    To use an integrated gps (e.g. on tablet or mobile), set 'internalGPS'*/
    void setPort( const QString& port ) { mPort = port; }

    QgsGPSInformation currentGPSInformation() const;
    FixStatus currentFixStatus() const { return mCurFixStatus; }

  private slots:
    void gpsDetected( QgsGPSConnection* conn );
    void gpsDetectionFailed();
    void updateGPSInformation( const QgsGPSInformation& info );

  signals:
    void gpsConnected();
    void gpsDisconnected();
    void gpsConnectionFailed();
    void gpsFixStatusChanged( FixStatus status );

    void gpsInformationReceived( const QgsGPSInformation& info );
    void nmeaSentenceReceived( const QString& substring );

  private:
    QgsMapCanvas* mCanvas;
    bool mShowMarker;
    QgsGpsMarker* mMarker;
    int mMarkerSize;
    int mSpinMapExtentMultiplier;
    QgsPoint mLastGPSPosition;
    QgsCoordinateReferenceSystem mWgs84CRS;
    FixStatus mCurFixStatus;

    /**Port for the GPS connectio*/
    QString mPort;
    QgsGPSConnection* mConnection;
    RecenterMode mRecenterMap;

    void init();
    void closeGPSConnection();

    void removeMarker();

    //read default settings
    static int defaultMarkerSize();
    static int defaultSpinMapExtentMultiplier();
    static QgsMapCanvasGPSDisplay::RecenterMode defaultRecenterMode();
};

#endif // QGSMAPCANVASGPSDISPLAY_H
