class QgsRedliningLayer : public QgsVectorLayer
{
%TypeHeaderCode
#include "qgsredlininglayer.h"
%End
  public:
    QgsRedliningLayer( const QString& name, const QString& crs = "EPSG:3857" );
    bool addShape( QgsGeometry* geometry /Transfer/, const QColor& outline, const QColor& fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle , const QString &flags = QString() , const QString &tooltip = QString() );
    bool addText( const QString &text, const QgsPointV2 &pos, const QColor& color, const QFont& font , const QString &tooltip = QString() , double rotation = 0, int markerSize = 2, const QString &extraFlags = QString() );
    void read( const QDomElement& redliningElem );
    void write( QDomElement& redliningElem );
    void pasteFeatures( const QList<QgsFeature> &features );
};
