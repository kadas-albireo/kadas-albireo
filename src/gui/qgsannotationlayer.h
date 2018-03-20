/***************************************************************************
                          qgsannotationlayer.h
                             -------------------
  begin                : August 2017
  copyright            : (C) 2017 by Sandro Mani
  email                : manisandro@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSANNOTATIONLAYER_H
#define QGSANNOTATIONLAYER_H

#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"

class QgsAnnotationItem;
class QgsMapCanvas;

class GUI_EXPORT QgsAnnotationLayer : public QgsPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "annotationlayer"; }
    static QgsAnnotationLayer* getLayer( QgsMapCanvas *canvas, const QString& itemType, const QString &layerTitle );

    QgsAnnotationLayer( QgsMapCanvas* canvas, const QString& itemType = QString() );
    ~QgsAnnotationLayer();
    void addItem( QgsAnnotationItem* item );
    void addChildCanvas( QgsMapCanvas* canvas );
    const QString& itemType() const { return mItemType; }

    bool writeSymbology( QDomNode &/*node*/, QDomDocument& /*doc*/, QString& /*errorMessage*/ ) const override { return true; }
    bool readSymbology( const QDomNode &/*node*/, QString &/*errorMessage*/ ) override { return true; }
    QgsRectangle extent() override;
    int margin() const override;

    void setLayerTransparency( int value ) override;
    void setSymbolScale( double scale );
    double symbolScale() const { return mSymbolScale; }

  protected:
    bool readXml( const QDomNode& layer_node ) override;
    bool writeXml( QDomNode & layer_node, QDomDocument & document ) override;

  private:
    QgsMapCanvas* mCanvas;
    QSet<QString> mItemIds;
    QString mItemType;
    double mSymbolScale = 1.;

  private slots:
    void updateItemVisibility();
};

class GUI_EXPORT QgsAnnotationLayerType : public QgsPluginLayerType
{
  public:
    QgsAnnotationLayerType( QgsMapCanvas* canvas ) : QgsPluginLayerType( QgsAnnotationLayer::layerTypeKey() ), mCanvas( canvas ) {}
    QgsPluginLayer* createLayer() override { return new QgsAnnotationLayer( mCanvas ); }
    int hasLayerProperties() const override { return 0; }

  private:
    QgsMapCanvas* mCanvas;
};

#endif //  QGSANNOTATIONLAYER_H
