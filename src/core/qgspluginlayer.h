/***************************************************************************
    qgspluginlayer.h
    ---------------------
    begin                : January 2010
    copyright            : (C) 2010 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSPLUGINLAYER_H
#define QGSPLUGINLAYER_H

#include "geometry/qgsgeometry.h"
#include "qgsmaplayer.h"

typedef QList< QPair<QString, QPixmap> > QgsLegendSymbologyList;

/** \ingroup core
  Base class for plugin layers. These can be implemented by plugins
  and registered in QgsPluginLayerRegistry.

  In order to be readable from project files, they should set these attributes in layer DOM node:
   "type" = "plugin"
   "name" = "your_layer_type"
 */
class CORE_EXPORT QgsPluginLayer : public QgsMapLayer
{
    Q_OBJECT

  public:
    QgsPluginLayer( QString layerType, QString layerName = QString() );

    /** return plugin layer type (the same as used in QgsPluginLayerRegistry) */
    QString pluginLayerType();

    void setExtent( const QgsRectangle &extent ) override;

    //! return a list of symbology items for the legend
    //! (defult implementation returns nothing)
    //! @note Added in v2.1
    virtual QgsLegendSymbologyList legendSymbologyItems( const QSize& iconSize );

    /** Return new instance of QgsMapLayerRenderer that will be used for rendering of given context
     *
     * The default implementation returns map layer renderer which just calls draw().
     * This may work, but it is unsafe for multi-threaded rendering because of the run
     * conditions that may happen (e.g. something is changed in the layer while it is
     * being rendered).
     *
     * @note added in 2.4
     */
    virtual QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext ) override;

    /** Test for mouse pick. */
    virtual bool testPick( const QgsPoint& /*mapPos*/, const QgsMapSettings& /*mapSettings*/, QVariant& /*pickResult*/, QRect &/*pickResultsExtent*/ ) { return false; }
    /** Handle a pick result. */
    virtual void handlePick( const QVariant& /*pick*/ ) {}
    /** Get items in extent */
    virtual QVariantList getItems( const QgsRectangle& /*extent*/ ) const { return QVariantList(); }
    /** Copy or cut the specified items */
    virtual void copyItems( const QVariantList& /*items*/, bool /*cut*/ ) {}
    /** Delete the specified items */
    virtual void deleteItems( const QVariantList& /*items*/ ) {}

    /** return  the current layer transparency */
    virtual int layerTransparency() const { return mTransparency; }
    /** set the layer transparency */
    virtual void setLayerTransparency( int value ) { mTransparency = value; }

    /** Get attributes at the specified map location */
    class IdentifyResult
    {
      public:
        IdentifyResult( const QString& featureName, const QMap<QString, QVariant>& attributes, const QgsGeometry& geom )
            : mFeatureName( featureName ), mAttributes( attributes ), mGeom( geom ) {}
        const QString& featureName() const { return mFeatureName; }
        const QMap<QString, QVariant>& attributes() const { return mAttributes; }
        const QgsGeometry& geometry() const { return mGeom; }

      private:
        QString mFeatureName;
        QMap<QString, QVariant> mAttributes;
        QgsGeometry mGeom;
    };
    virtual QList<IdentifyResult> identify( const QgsPoint& /*mapPos*/, const QgsMapSettings& /*mapSettings*/ ) { return QList<IdentifyResult>(); }

  protected:
    QString mPluginLayerType;
    int mTransparency;
};

#endif // QGSPLUGINLAYER_H
