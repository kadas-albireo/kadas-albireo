#include "qgsmapcanvasgpsdisplay.h"
#include "qgsgpsconnection.h"
#include "qgsgpsmarker.h"
#include <QSettings>

QgsMapCanvasGPSDisplay::QgsMapCanvasGPSDisplay( QgsMapCanvas* canvas ): QObject( 0 ), mCanvas( canvas ), mCenterMap( false ), mShowMarker( true ),
    mMarker( 0 )
{
    mMarkerSize = defaultMarkerSize();
}

QgsMapCanvasGPSDisplay::QgsMapCanvasGPSDisplay(): QObject( 0 ), mCanvas( 0 ), mCenterMap( false ), mShowMarker( true ), mMarker( 0 )
{
    mMarkerSize = defaultMarkerSize();
}

QgsMapCanvasGPSDisplay::~QgsMapCanvasGPSDisplay()
{
    delete mMarker;
}

void QgsMapCanvasGPSDisplay::updateGPSInformation( const QgsGPSInformation& info )
{
    if( !mCanvas )
    {
        return;
    }

    QgsPoint position( info.longitude, info.latitude );
    if( mShowMarker )
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
    return s.value( "/gps/markerSize", "12" ).toInt();
}
