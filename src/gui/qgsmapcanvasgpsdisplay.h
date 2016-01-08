#ifndef QGSMAPCANVASGPSDISPLAY_H
#define QGSMAPCANVASGPSDISPLAY_H

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

        void setMapCanvas( QgsMapCanvas* canvas ){ mCanvas = canvas; }

        bool centerMap() const { return mCenterMap; }
        void setCenterMap( bool center ){ mCenterMap = center; }

        bool showMarker() const { return mShowMarker; }
        void setShowMarker( bool showMarker ){ mShowMarker = showMarker; }

        int markerSize() const { return mMarkerSize; }
        void setMarkerSize( int size ){ mMarkerSize = size; }

    public slots:
        void updateGPSInformation( const QgsGPSInformation& info );

    private:
        QgsMapCanvas* mCanvas;
        bool mCenterMap;
        bool mShowMarker;
        QgsGpsMarker* mMarker;
        int mMarkerSize;

        static int defaultMarkerSize();
};

#endif // QGSMAPCANVASGPSDISPLAY_H
