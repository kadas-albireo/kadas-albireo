
#ifndef QGSMARKERSYMBOLLAYERV2_H
#define QGSMARKERSYMBOLLAYERV2_H

#include "qgssymbollayerv2.h"

#define DEFAULT_SIMPLEMARKER_NAME         "circle"
#define DEFAULT_SIMPLEMARKER_COLOR        QColor(255,0,0)
#define DEFAULT_SIMPLEMARKER_BORDERCOLOR  QColor(0,0,0)
#define DEFAULT_SIMPLEMARKER_SIZE         9
#define DEFAULT_SIMPLEMARKER_ANGLE        0

#include <QPen>
#include <QBrush>
#include <QPicture>
#include <QPolygonF>

class QgsSimpleMarkerSymbolLayerV2 : public QgsMarkerSymbolLayerV2
{
public:
  QgsSimpleMarkerSymbolLayerV2(QString name = DEFAULT_SIMPLEMARKER_NAME,
                               QColor color = DEFAULT_SIMPLEMARKER_COLOR,
                               QColor borderColor = DEFAULT_SIMPLEMARKER_BORDERCOLOR,
                               double size = DEFAULT_SIMPLEMARKER_SIZE,
                               double angle = DEFAULT_SIMPLEMARKER_ANGLE);
	
	// static stuff
	
	static QgsSymbolLayerV2* create(const QgsStringMap& properties = QgsStringMap());
	
	// implemented from base classes
	
	QString layerType() const;
	
	void startRender(QgsRenderContext& context);
	
	void stopRender(QgsRenderContext& context);
	
	void renderPoint(const QPointF& point, QgsRenderContext& context);
	
	QgsStringMap properties() const;
	
  QgsSymbolLayerV2* clone() const;
  
  QString name() const { return mName; }
  void setName(QString name) { mName = name; }
  
  QColor borderColor() const { return mBorderColor; }
  void setBorderColor(QColor color) { mBorderColor = color; }
	
protected:
  
  void drawMarker(QPainter* p);
  
  QColor mBorderColor;
  QPen mPen;
  QBrush mBrush;
  QPolygonF mPolygon;
  QString mName;
  QImage mCache;
};

//////////

#define DEFAULT_SVGMARKER_NAME         "symbol/Star1.svg"
#define DEFAULT_SVGMARKER_SIZE         9
#define DEFAULT_SVGMARKER_ANGLE        0

class QgsSvgMarkerSymbolLayerV2 : public QgsMarkerSymbolLayerV2
{
public:
  QgsSvgMarkerSymbolLayerV2(QString name = DEFAULT_SVGMARKER_NAME,
                            double size = DEFAULT_SVGMARKER_SIZE,
                            double angle = DEFAULT_SVGMARKER_ANGLE);
	
	// static stuff
	
	static QgsSymbolLayerV2* create(const QgsStringMap& properties = QgsStringMap());
	
	// implemented from base classes
	
	QString layerType() const;
	
	void startRender(QgsRenderContext& context);
	
	void stopRender(QgsRenderContext& context);
	
	void renderPoint(const QPointF& point, QgsRenderContext& context);
	
	QgsStringMap properties() const;
	
  QgsSymbolLayerV2* clone() const;
  
  QString name() const { return mName; }
  void setName(QString name) { mName = name; }
  
protected:
  
  void loadSvg();
  
  QString mName;
  QPicture mPicture;
};

#endif
