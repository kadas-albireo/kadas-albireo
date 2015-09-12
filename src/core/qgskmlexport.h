#ifndef QGSKMLEXPORT_H
#define QGSKMLEXPORT_H

#include "qgis.h"
#include <QList>

class QgsKMLPalLabeling;
class QgsMapLayer;
class QgsVectorLayer;
class QgsRectangle;
class QgsRenderContext;
class QIODevice;
class QTextStream;

class QgsKMLExport
{
  public:
    QgsKMLExport();
    ~QgsKMLExport();

    void setLayers( const QList<QgsMapLayer*>& layers ) { mLayers = layers; }
    int writeToDevice( QIODevice *d, const QgsRectangle& bbox, double scale, QGis::UnitType mapUnits );

  private:
    QList<QgsMapLayer*> mLayers;

    void writeSchemas( QTextStream& outStream );
    bool writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc );
};

#endif // QGSKMLEXPORT_H
