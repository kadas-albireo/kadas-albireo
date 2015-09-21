#ifndef QGSKMLEXPORT_H
#define QGSKMLEXPORT_H

#include "qgis.h"
#include "qgsmapsettings.h"
#include <QList>

class QgsFeature;
class QgsFeatureRendererV2;
class QgsKMLPalLabeling;
class QgsMapLayer;
class QgsVectorLayer;
class QgsRectangle;
class QgsRenderContext;
class QIODevice;
class QTextStream;

class CORE_EXPORT QgsKMLExport
{
  public:
    QgsKMLExport();
    ~QgsKMLExport();

    void setLayers( const QList<QgsMapLayer*>& layers ) { mLayers = layers; }
    int writeToDevice( QIODevice *d, const QgsMapSettings& settings );

    static QString convertColor( const QColor& c );

  private:
    QList<QgsMapLayer*> mLayers;

    void writeSchemas( QTextStream& outStream );
    bool writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc );
    void addStyle( QTextStream& outStream, QgsFeature& f, QgsFeatureRendererV2& r, QgsRenderContext& rc );
    /**Return WGS84 bbox from layer set*/
    QgsRectangle bboxFromLayers();
    static QString convertToHexValue( int value );
};

#endif // QGSKMLEXPORT_H
