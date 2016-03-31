/***************************************************************************
 *  qgsmilxlayer.h                                                         *
 *  --------------                                                         *
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

#ifndef QGSMILXLAYER_H
#define QGSMILXLAYER_H

#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"
#include "MilXClient.hpp"

class QgsLayerTreeViewMenuProvider;

class QGS_MILX_EXPORT QgsMilXItem
{
  public:
    static bool validateMssString( const QString& mssString, QString &adjustedMssString, QString& messages );

    ~QgsMilXItem();
    void initialize( const QString& mssString, const QString& militaryName, const QList<QgsPoint> &points, const QList<int>& controlPoints = QList<int>(), const QPoint& userOffset = QPoint(), bool queryControlPoints = false );
    const QString& mssString() const { return mMssString; }
    const QString& militaryName() const { return mMilitaryName; }
    const QList<QgsPoint>& points() const { return mPoints; }
    QList<QPoint> screenPoints( const QgsMapToPixel& mapToPixel, const QgsCoordinateTransform *crst ) const;
    const QList<int>& controlPoints() const { return mControlPoints; }
    const QPoint& userOffset() const { return mUserOffset; }
    bool isMultiPoint() const { return mPoints.size() > 1; }

    void writeMilx( QDomDocument& doc, QDomElement& graphicListEl, const QString &versionTag, QString &messages ) const;
    void readMilx( const QDomElement& graphicEl, const QString &symbolXml, const QgsCoordinateTransform *crst, int symbolSize );

  private:
    QString mMssString;
    QString mMilitaryName;
    QList<QgsPoint> mPoints; // Points as WGS84 coordinates
    QList<int> mControlPoints;
    QPoint mUserOffset; // In pixels
};


class QGS_MILX_EXPORT QgsMilXLayer : public QgsPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "MilX_Layer"; }

    QgsMilXLayer( QgsLayerTreeViewMenuProvider *menuProvider = 0, const QString& name = "MilX" );
    ~QgsMilXLayer();
    void addItem( QgsMilXItem* item ) { mItems.append( item ); }
    QgsMilXItem* takeItem( int idx ) { return mItems.takeAt( idx ); }
    const QList<QgsMilXItem*>& items() const { return mItems; }
    void setApproved( bool approved ) { mIsApproved = approved; }
    bool isApproved() const { return mIsApproved; }
    QgsLegendSymbologyList legendSymbologyItems( const QSize& iconSize ) override;
    void exportToMilxly( QDomElement &milxDocumentEl, const QString &versionTag, QStringList& exportMessages );
    bool importMilxly( QDomElement &milxLayerEl, const QString &fileMssVer, QString &errorMsg, QStringList& importMessages );
    bool writeSymbology( QDomNode &/*node*/, QDomDocument& /*doc*/, QString& /*errorMessage*/ ) const override { return true; }
    bool readSymbology( const QDomNode &/*node*/, QString &/*errorMessage*/ ) override { return true; }
    QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext ) override;
    QgsRectangle extent() override;
    int margin() const override;

    bool testPick( const QgsPoint& mapPos, const QgsMapSettings& mapSettings, QVariant& pickResult ) override;
    void handlePick( const QVariant& pick ) override;
    QVariantList getItems( const QgsRectangle& extent ) const override;
    void deleteItems( const QVariantList& items ) override;

  signals:
    void symbolPicked( int symbolIdx );

  protected:
    bool readXml( const QDomNode& layer_node ) override;
    bool writeXml( QDomNode & layer_node, QDomDocument & document ) override;

  private:
    class Renderer;

    QgsLayerTreeViewMenuProvider* mMenuProvider;
    QList<QgsMilXItem*> mItems;
    int mMargin;
    bool mIsApproved;
};

class QGS_MILX_EXPORT QgsMilXLayerType : public QgsPluginLayerType
{
  public:
    QgsMilXLayerType( QgsLayerTreeViewMenuProvider* menuProvider )
        : QgsPluginLayerType( QgsMilXLayer::layerTypeKey() ), mMenuProvider( menuProvider ) {}
    QgsPluginLayer* createLayer() override { return new QgsMilXLayer( mMenuProvider ); }
    bool showLayerProperties( QgsPluginLayer* /*layer*/ ) override { return false; }

  private:
    QgsLayerTreeViewMenuProvider* mMenuProvider;
};

#endif // QGSMILXLAYER_H
