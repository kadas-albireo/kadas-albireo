/***************************************************************************
  qgsvectorlayerrenderer.h
  --------------------------------------
  Date                 : December 2013
  Copyright            : (C) 2013 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVECTORLAYERRENDERER_H
#define QGSVECTORLAYERRENDERER_H

class QgsFeatureRendererV2;
class QgsRenderContext;
class QgsVectorLayer;
class QgsVectorLayerFeatureSource;

class QgsDiagramRendererV2;
class QgsDiagramLayerSettings;

class QgsGeometryCache;
class QgsFeatureIterator;
class QgsSingleSymbolRendererV2;

#include <QList>
#include <QPainter>

typedef QList<int> QgsAttributeList;

#include "qgis.h"
#include "qgsfield.h"  // QgsFields
#include "qgsfeature.h"  // QgsFeatureIds
#include "qgsfeatureiterator.h"
#include "qgsvectorsimplifymethod.h"

#include "qgsmaplayerrenderer.h"


/**
 * Implementation of threaded rendering for vector layers.
 *
 * @note added in 2.4
 */
class QgsVectorLayerRenderer : public QgsMapLayerRenderer
{
  public:
    QgsVectorLayerRenderer( QgsVectorLayer* layer, QgsRenderContext& context );
    ~QgsVectorLayerRenderer();

    virtual bool render() override;

    //! where to save the cached geometries
    //! @note The way how geometries are cached is really suboptimal - this method may be removed in future releases
    void setGeometryCachePointer( QgsGeometryCache* cache );

  private:

    /**Registers label and diagram layer
      @param layer diagram layer
      @param attributeNames attributes needed for labeling and diagrams will be added to the list
     */
    void prepareLabeling( QgsVectorLayer* layer, QStringList& attributeNames );
    void prepareDiagrams( QgsVectorLayer* layer, QStringList& attributeNames );

    /** Draw layer with renderer V2. QgsFeatureRenderer::startRender() needs to be called before using this method
     */
    void drawRendererV2( QgsFeatureIterator& fit );

    /** Draw layer with renderer V2 using symbol levels. QgsFeatureRenderer::startRender() needs to be called before using this method
     */
    void drawRendererV2Levels( QgsFeatureIterator& fit );

    /** Stop version 2 renderer and selected renderer (if required) */
    void stopRendererV2( QgsSingleSymbolRendererV2* selRenderer );


  protected:

    QgsRenderContext& mContext;

    QgsFields mFields; // TODO: use fields from mSource

    QgsFeatureIds mSelectedFeatureIds;

    QgsVectorLayerFeatureSource* mSource;

    QgsFeatureRendererV2 *mRendererV2;

    bool mCacheFeatures;
    QgsGeometryCache* mCache;

    bool mDrawVertexMarkers;
    bool mVertexMarkerOnlyForSelection;
    int mVertexMarkerStyle, mVertexMarkerSize;

    QGis::GeometryType mGeometryType;

    QStringList mAttrNames;

    bool mLabeling;
    bool mDiagrams;

    int mLayerTransparency;
    QPainter::CompositionMode mFeatureBlendMode;

    QgsVectorSimplifyMethod mSimplifyMethod;
    bool mSimplifyGeometry;
};


#endif // QGSVECTORLAYERRENDERER_H
