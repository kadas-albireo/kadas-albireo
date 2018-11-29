/***************************************************************************
 *  qgsbullseyelayer.h                                                     *
 *  --------------                                                         *
 *  begin                : November 2018                                   *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSBULLEYELAYER_H
#define QGSBULLEYELAYER_H

#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"

class CORE_EXPORT QgsBullsEyeLayer : public QgsPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "bullseye"; }
    enum LabellingMode { NO_LABELS, LABEL_AXES, LABEL_RINGS, LABEL_AXES_RINGS };

    QgsBullsEyeLayer( const QString &name );
    void setup( const QgsPoint& center, const QgsCoordinateReferenceSystem& crs, int rings, double interval, double axesInterval );
    bool writeSymbology( QDomNode &/*node*/, QDomDocument& /*doc*/, QString& /*errorMessage*/ ) const override { return true; }
    bool readSymbology( const QDomNode &/*node*/, QString &/*errorMessage*/ ) override { return true; }
    QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext ) override;
    QgsRectangle extent() override;
    QgsPoint center() const { return mCenter; }
    int rings() const { return mRings; }
    double ringInterval() const { return mInterval; }
    double axesInterval() const { return mAxesInterval; }

    const QColor& color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    LabellingMode labellingMode() const { return mLabellingMode; }

  public slots:
    void setColor( const QColor& color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabellingMode( LabellingMode labellingMode ) { mLabellingMode = labellingMode; }
    void setLineWidth( int lineWidth ) { mLineWidth = lineWidth; }

  protected:
    bool readXml( const QDomNode& layer_node ) override;
    bool writeXml( QDomNode & layer_node, QDomDocument & document ) override;

  private:
    class Renderer;

    QgsPoint mCenter;
    int mRings;
    double mInterval;
    double mAxesInterval;
    int mFontSize = 10;
    int mLineWidth = 1;
    QColor mColor = Qt::black;
    LabellingMode mLabellingMode = NO_LABELS;
};

class CORE_EXPORT QgsBullsEyeLayerType : public QgsPluginLayerType
{
  public:
    QgsBullsEyeLayerType( )
        : QgsPluginLayerType( QgsBullsEyeLayer::layerTypeKey() ) {}
    QgsPluginLayer* createLayer() override { return new QgsBullsEyeLayer( "" ); }
    int hasLayerProperties() const override { return 0; }
};

#endif // QGSBULLEYELAYER_H
