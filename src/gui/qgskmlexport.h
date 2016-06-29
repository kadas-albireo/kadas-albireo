#ifndef QGSKMLEXPORT_H
#define QGSKMLEXPORT_H

#include "qgis.h"
#include "qgsmapsettings.h"
#include <QList>

class QgsAnnotationItem;
class QgsBillBoardItem;
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
    void setExportAnnotatons( bool exportAnnotations ) { mExportAnnotations = exportAnnotations; }
    int writeToDevice( QIODevice *d, const QgsMapSettings& settings, QStringList& usedTemporaryFiles, QList<QgsMapLayer*>& superOverlayLayers );

    bool addSuperOverlayLayer( QgsMapLayer* mapLayer, QuaZip* quaZip, const QString& filePath, int drawingOrder,
                               const QgsCoordinateReferenceSystem& mapCRS, double mapUnitsPerPixel );

    static QString convertColor( const QColor& c );

  private:
    QList<QgsMapLayer*> mLayers;
    bool mExportAnnotations;
    QList<QgsAnnotationItem*> mAnnotationItems;

    void writeSchemas( QTextStream& outStream );
    bool writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc );
    void writeBillboards( const QString &layerId, QTextStream& outStream, QStringList& usedTemporaryFiles );
    void addStyle( QTextStream& outStream, QgsFeature& f, QgsFeatureRendererV2& r, QgsRenderContext& rc );
    /**Return WGS84 bbox from layer set*/
    QgsRectangle bboxFromLayers();
    /**Returns wgs84 bbox for layer*/
    static QgsRectangle wgs84LayerExtent( QgsMapLayer* ml );
    static QgsRectangle superOverlayStartExtent( const QgsRectangle& wgs84Extent );
    static QString convertToHexValue( int value );
    static void writeRectangle( QTextStream& outStream, const QgsRectangle& rect );
    static void writeNetworkLink( QTextStream& outStream, const QgsRectangle& rect, const QString& link );
    static void writeWMSOverlay( QTextStream& outStream, const QgsRectangle& latLongBox, const QString& baseUrl, const QString& version, const QString& format, const QString& layers, const QString& styles );
    static void writeGroundOverlay( QTextStream& outStream, const QString& href, const QgsRectangle& latLongBox, int drawingOrder = -1 );
    static int levelsToGo( double resolution, double minResolution );
    static int offset( int nLevelsToGo );


    void addOverlay( const QgsRectangle& extent, QgsMapLayer* mapLayer, QuaZip* quaZip, const QString& filePath, int& currentTileNumber, int drawingOrder );
    QIODevice* openDeviceForNewFile( const QString& fileName, QuaZip* quaZip, const QString& filePath );
};

#endif // QGSKMLEXPORT_H
