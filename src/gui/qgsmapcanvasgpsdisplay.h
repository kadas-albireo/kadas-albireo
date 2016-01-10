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

    void setMapCanvas( QgsMapCanvas* canvas ) { mCanvas = canvas; }

    bool centerMap() const { return mCenterMap; }
    void setCenterMap( bool center ) { mCenterMap = center; }

    bool showMarker() const { return mShowMarker; }
    void setShowMarker( bool showMarker ) { mShowMarker = showMarker; }

    int markerSize() const { return mMarkerSize; }
    void setMarkerSize( int size ) { mMarkerSize = size; }

    double spinMapExtentMultiplier() const { return mSpinMapExtentMultiplier; }
    void setSpinMapExtentMultiplier( double value ) { mSpinMapExtentMultiplier = value; }

    void removeMarker();

  public slots:
    void updateGPSInformation( const QgsGPSInformation& info );

  private:
    QgsMapCanvas* mCanvas;
    bool mCenterMap;
    bool mShowMarker;
    QgsGpsMarker* mMarker;
    int mMarkerSize;
    int mSpinMapExtentMultiplier;
    QgsPoint mLastGPSPosition;
    QgsCoordinateReferenceSystem mWgs84CRS;

    void init();

    static int defaultMarkerSize();
    static int defaultSpinMapExtentMultiplier();
};

#endif // QGSMAPCANVASGPSDISPLAY_H
