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
class QgsGPSInformation;
class QgsGpsMarker;
class QgsMapCanvas;

/**Manages display of canvas gps marker and moving of map extent with changing GPS position*/
class GUI_EXPORT QgsMapCanvasGPSDisplay: public QObject
{
    Q_OBJECT
  public:
    QgsMapCanvasGPSDisplay( QgsMapCanvas* canvas );
    QgsMapCanvasGPSDisplay();

    ~QgsMapCanvasGPSDisplay();

    void connectGPS();
    void disconnectGPS();

    void setMapCanvas( QgsMapCanvas* canvas ) { mCanvas = canvas; }

    bool centerMap() const { return mCenterMap; }
    void setCenterMap( bool center ) { mCenterMap = center; }

    bool showMarker() const { return mShowMarker; }
    void setShowMarker( bool showMarker ) { mShowMarker = showMarker; }

    int markerSize() const { return mMarkerSize; }
    void setMarkerSize( int size ) { mMarkerSize = size; }

    double spinMapExtentMultiplier() const { return mSpinMapExtentMultiplier; }
    void setSpinMapExtentMultiplier( double value ) { mSpinMapExtentMultiplier = value; }

    QString port() const { return mPort; }
    /**Sets the port for the GPS connection. Empty string (default) means autodetect. For gpsd connections, use '<host>:<port>:<device>'.
    To use an integrated gps (e.g. on tablet or mobile), set 'internalGPS'*/
    void setPort( const QString& port ) { mPort = port; }

  private slots:
    void gpsDetected( QgsGPSConnection* conn );
    void gpsDetectionFailed();
    void updateGPSInformation( const QgsGPSInformation& info );

  signals:
    void gpsConnected();
    void gpsDisconnected();
    void gpsConnectionFailed();

  private:
    QgsMapCanvas* mCanvas;
    bool mCenterMap;
    bool mShowMarker;
    QgsGpsMarker* mMarker;
    int mMarkerSize;
    int mSpinMapExtentMultiplier;
    QgsPoint mLastGPSPosition;
    QgsCoordinateReferenceSystem mWgs84CRS;

    /**Port for the GPS connectio*/
    QString mPort;
    QgsGPSConnection* mConnection;

    void init();
    void closeGPSConnection();

    void removeMarker();

    static int defaultMarkerSize();
    static int defaultSpinMapExtentMultiplier();
};

#endif // QGSMAPCANVASGPSDISPLAY_H
