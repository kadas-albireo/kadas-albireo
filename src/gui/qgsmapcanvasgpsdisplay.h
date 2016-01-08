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
