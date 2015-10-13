class QgsRedliningLayer : public QgsVectorLayer
{
%TypeHeaderCode
#include "qgsredlininglayer.h"
%End
  public:
    QgsRedliningLayer();
    void addFeature( QgsGeometry* geometry, const QColor& outline, const QColor& fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle );
    void addText(const QString &text, const QgsPoint& pos, const QColor& color, const QFont& font);
    void read( const QDomElement& redliningElem );
    void write( QDomElement& redliningElem );
};
