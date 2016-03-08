/***************************************************************************
 *  qgsvbsmilixlayer.h                                                     *
 *  -------------------                                                    *
 *  begin                : February 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGS_VBS_MILIX_LAYER_H
#define QGS_VBS_MILIX_LAYER_H

#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"
#include "Client/VBSMilixClient.hpp"

class QgsVBSMilixItem
{
  public:
    QgsVBSMilixItem() {}
    void initialize( const QString& mssString, const QString& militaryName, const QList<QgsPoint> &points, const QList<int>& controlPoints = QList<int>(), const QPoint& userOffset = QPoint(), bool haveEnoughPoints = false );
    const QString& mssString() const { return mMssString; }
    const QString& militaryName() const { return mMilitaryName; }
    const QList<QgsPoint>& points() const { return mPoints; }
    QList<QPoint> screenPoints( const QgsMapToPixel& mapToPixel, const QgsCoordinateTransform *crst ) const;
    const QList<int>& controlPoints() const { return mControlPoints; }
    const QPoint& userOffset() const { return mUserOffset; }
    bool hasEnoughPoints() const { return mHaveEnoughPoints; }
    bool isMultiPoint() const { return mPoints.size() > 1; }

    void writeMilx( QDomDocument& doc, QDomElement& graphicListEl, const QString &versionTag, QString &messages ) const;
    void readMilx( const QDomElement& graphicEl, const QString &symbolXml, const QgsCoordinateTransform *crst, int symbolSize );

  private:
    QString mMssString;
    QString mMilitaryName;
    QList<QgsPoint> mPoints; // Points as WGS84 coordinates
    QList<int> mControlPoints;
    QPoint mUserOffset; // In pixels
    bool mHaveEnoughPoints;
};

class QgsVBSMilixLayer : public QgsPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "MilX_Layer"; }

    QgsVBSMilixLayer( const QString& name = QString( "MilX" ) );
    ~QgsVBSMilixLayer();
    void addItem( QgsVBSMilixItem* item ) { mItems.append( item ); }
    void removeItem( int idx ) { mItems.removeAt( idx ); }
    const QList<QgsVBSMilixItem*>& items() const { return mItems; }
    QgsLegendSymbologyList legendSymbologyItems( const QSize& iconSize ) override;
    void exportToMilxly( QDomElement &milxDocumentEl, const QString &versionTag, QStringList& exportMessages );
    bool importMilxly( QDomElement &milxLayerEl, const QString &fileMssVer, QString &errorMsg, QStringList& importMessages );
    bool writeSymbology( QDomNode &/*node*/, QDomDocument& /*doc*/, QString& /*errorMessage*/ ) const override { return true; }
    bool readSymbology( const QDomNode &/*node*/, QString &/*errorMessage*/ ) override { return true; }
    QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext ) override;
    QgsRectangle extent() override;

    bool testPick( const QgsPoint& mapPos, const QgsMapSettings& mapSettings, QVariant& pickResult ) override;
    void handlePick( const QVariant& pick ) override;

  signals:
    void symbolPicked( int symbolIdx );

  protected:
    bool readXml( const QDomNode& layer_node ) override;
    bool writeXml( QDomNode & layer_node, QDomDocument & document ) override;

  private:
    class Renderer;

    QList<QgsVBSMilixItem*> mItems;
};

class QgsVBSMilixLayerType : public QgsPluginLayerType
{
  public:
    QgsVBSMilixLayerType() : QgsPluginLayerType( QgsVBSMilixLayer::layerTypeKey() ) {}
    QgsPluginLayer* createLayer() override { return new QgsVBSMilixLayer(); }
    bool showLayerProperties( QgsPluginLayer* layer ) override { return false; }
};

#endif // QGS_VBS_MILIX_LAYER_H
