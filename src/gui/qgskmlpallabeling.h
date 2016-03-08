#ifndef QGSKMLPALLABELING_H
#define QGSKMLPALLABELING_H

#include "qgspallabeling.h"
#include "qgsrendercontext.h"

class QTextStream;

class GUI_EXPORT QgsKMLPalLabeling: public QgsPalLabeling
{
  public:
    QgsKMLPalLabeling( QTextStream* outStream, const QgsRectangle& bbox, double scale, QGis::UnitType mapUnits );
    ~QgsKMLPalLabeling();

    QgsRenderContext& renderContext() { return mRenderContext; }
    void drawLabel( pal::LabelPosition* label, QgsRenderContext& context, QgsPalLayerSettings& tmpLyr, DrawLabelType drawType, double dpiRatio = 1.0 ) override;

  private:
    QTextStream* mOutStream;
    QgsRenderContext mRenderContext;
    QImage* mImage;
    QPainter* mPainter;
    QgsMapSettings* mSettings;

    QgsKMLPalLabeling(); //private
};

#endif // QGSKMLPALLABELING_H
