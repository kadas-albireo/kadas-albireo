#ifndef QGSKMLEXPORT_H
#define QGSKMLEXPORT_H

#include <QList>

class QgsMapLayer;
class QgsVectorLayer;
class QIODevice;
class QTextStream;

class QgsKMLExport
{
  public:
    QgsKMLExport();
    ~QgsKMLExport();

    void setLayers( const QList<QgsMapLayer*>& layers ) { mLayers = layers; }
    int writeToDevice( QIODevice *d );

  private:
    QList<QgsMapLayer*> mLayers;

    void writeSchemas( QTextStream& outStream );
    bool writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream );
};

#endif // QGSKMLEXPORT_H
