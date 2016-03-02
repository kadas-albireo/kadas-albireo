#ifndef QGSKMLEXPORT_H
#define QGSKMLEXPORT_H

#include "qgis.h"
#include "qgsmapsettings.h"
#include <QList>

class QgsAnnotationItem;
class QgsFeature;
class QgsFeatureRendererV2;
class QgsKMLPalLabeling;
class QgsMapLayer;
class QgsVectorLayer;
class QgsRectangle;
class QgsRenderContext;
class QIODevice;
class QTextStream;

class GUI_EXPORT QgsKMLExport
{
  public:
    QgsKMLExport();
    ~QgsKMLExport();

    void setLayers( const QList<QgsMapLayer*>& layers ) { mLayers = layers; }
    void setAnnotationItems( const QList<QgsAnnotationItem*>& items ) { mAnnotationItems = items; }
    /**Writes KML file to device
        @param d output device
        @param settings mapSettings
        @param visibleExtentOnly exports all data from mLayers if false
        @param usedLocalFiles out: files referenced in the KML document (e.g. photos). Needs to be added to the kmz*/
    int writeToDevice( QIODevice *d, const QgsMapSettings& settings, bool visibleExtentOnly, QStringList& usedLocalFiles, QList<QgsMapLayer*>& superOverlayLayers );

    bool addSuperOverlayLayer( QgsMapLayer* mapLayer, QuaZip* quaZip, const QString& filePath, int drawingOrder );

    static QString convertColor( const QColor& c );

  private:
    QList<QgsMapLayer*> mLayers;
    QList<QgsAnnotationItem*> mAnnotationItems;

    void writeSchemas( QTextStream& outStream );
    bool writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, const QgsRectangle &filterExtent, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc );
    bool writeAnnotationItem( QgsAnnotationItem* item, QTextStream& outStream, QStringList& usedLocalFiles );
    void addStyle( QTextStream& outStream, QgsFeature& f, QgsFeatureRendererV2& r, QgsRenderContext& rc );
    /**Return WGS84 bbox from layer set*/
    QgsRectangle bboxFromLayers();
    /**Returns wgs84 bbox for layer*/
    static QgsRectangle wgs84LayerExtent( QgsMapLayer* ml );
    static QgsRectangle superOverlayStartExtent( const QgsRectangle& wgs84Extent );
    static QString convertToHexValue( int value );
    static void writeLatLongBox( QTextStream& outStream, const QgsRectangle& rect );
    static void writeNetworkLink( QTextStream& outStream, const QgsRectangle& rect, const QString& link );
    static void writeWMSOverlay( QTextStream& outStream, const QgsRectangle& latLongBox, const QString& baseUrl, const QString& version, const QString& format, const QString& layers, const QString& styles );
    static void writeGroundOverlay( QTextStream& outStream, const QString& href, const QgsRectangle& latLongBox, int drawingOrder = -1 );
    static int levelsToGo( double resolution, double minResolution );
    static int offset( int nLevelsToGo );


    void addOverlay( const QgsRectangle& extent, QgsMapLayer* mapLayer, QuaZip* quaZip, const QString& filePath, int& currentTileNumber, int drawingOrder );
    QIODevice* openDeviceForNewFile( const QString& fileName, QuaZip* quaZip, const QString& filePath );
};

#endif // QGSKMLEXPORT_H
