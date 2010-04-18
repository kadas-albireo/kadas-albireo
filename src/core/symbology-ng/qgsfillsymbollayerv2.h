
#ifndef QGSFILLSYMBOLLAYERV2_H
#define QGSFILLSYMBOLLAYERV2_H

#include "qgssymbollayerv2.h"

#define DEFAULT_SIMPLEFILL_COLOR        QColor(0,0,255)
#define DEFAULT_SIMPLEFILL_STYLE        Qt::SolidPattern
#define DEFAULT_SIMPLEFILL_BORDERCOLOR  QColor(0,0,0)
#define DEFAULT_SIMPLEFILL_BORDERSTYLE  Qt::SolidLine
#define DEFAULT_SIMPLEFILL_BORDERWIDTH  DEFAULT_LINE_WIDTH

#include <QPen>
#include <QBrush>

class CORE_EXPORT QgsSimpleFillSymbolLayerV2 : public QgsFillSymbolLayerV2
{
  public:
    QgsSimpleFillSymbolLayerV2( QColor color = DEFAULT_SIMPLEFILL_COLOR,
                                Qt::BrushStyle style = DEFAULT_SIMPLEFILL_STYLE,
                                QColor borderColor = DEFAULT_SIMPLEFILL_BORDERCOLOR,
                                Qt::PenStyle borderStyle = DEFAULT_SIMPLEFILL_BORDERSTYLE,
                                double borderWidth = DEFAULT_SIMPLEFILL_BORDERWIDTH );

    // static stuff

    static QgsSymbolLayerV2* create( const QgsStringMap& properties = QgsStringMap() );

    // implemented from base classes

    QString layerType() const;

    void startRender( QgsSymbolV2RenderContext& context );

    void stopRender( QgsSymbolV2RenderContext& context );

    void renderPolygon( const QPolygonF& points, QList<QPolygonF>* rings, QgsSymbolV2RenderContext& context );

    QgsStringMap properties() const;

    QgsSymbolLayerV2* clone() const;

    Qt::BrushStyle brushStyle() const { return mBrushStyle; }
    void setBrushStyle( Qt::BrushStyle style ) { mBrushStyle = style; }

    QColor borderColor() const { return mBorderColor; }
    void setBorderColor( QColor borderColor ) { mBorderColor = borderColor; }

    Qt::PenStyle borderStyle() const { return mBorderStyle; }
    void setBorderStyle( Qt::PenStyle borderStyle ) { mBorderStyle = borderStyle; }

    double borderWidth() const { return mBorderWidth; }
    void setBorderWidth( double borderWidth ) { mBorderWidth = borderWidth; }

  protected:
    QBrush mBrush;
    QBrush mSelBrush;
    Qt::BrushStyle mBrushStyle;
    QColor mBorderColor;
    Qt::PenStyle mBorderStyle;
    double mBorderWidth;
    QPen mPen;
};

/**A class for svg fill patterns. The class automatically scales the pattern to
   the appropriate pixel dimensions of the output device*/
class CORE_EXPORT QgsSVGFillSymbolLayer: public QgsFillSymbolLayerV2
{
  public:
    QgsSVGFillSymbolLayer( const QString& svgFilePath = "", double width = 20 );
    QgsSVGFillSymbolLayer( const QByteArray& svgData, double width = 20 );
    ~QgsSVGFillSymbolLayer();

    static QgsSymbolLayerV2* create( const QgsStringMap& properties = QgsStringMap() );

    // implemented from base classes

    QString layerType() const;

    void startRender( QgsSymbolV2RenderContext& context );
    void stopRender( QgsSymbolV2RenderContext& context );

    void renderPolygon( const QPolygonF& points, QList<QPolygonF>* rings, QgsSymbolV2RenderContext& context );

    QgsStringMap properties() const;

    QgsSymbolLayerV2* clone() const;

    //gettersn and setters
    void setSvgFilePath( const QString& svgPath );
    QString svgFilePath() const { return mSvgFilePath; }
    void setPatternWidth( double width ) { mPatternWidth = width;}
    double patternWidth() const { return mPatternWidth; }

    QgsSymbolV2* subSymbol() { return mOutline; }
    bool setSubSymbol( QgsSymbolV2* symbol );

  protected:
    /**Width of the pattern (in QgsSymbolV2 output units)*/
    double mPatternWidth;
    /**SVG data*/
    QByteArray mSvgData;
    /**Path to the svg file (or empty if constructed directly from data)*/
    QString mSvgFilePath;
    /**SVG view box (to keep the aspect ratio */
    QRectF mSvgViewBox;
    /**Brush that receives rendered pixel image in startRender() method*/
    QBrush mBrush;
    /**Outline width*/
    double mOutlineWidth;
    /**Custom outline*/
    QgsLineSymbolV2* mOutline;

  private:
    /**Helper function that gets the view box from the byte array*/
    void storeViewBox();
};

#endif
