/***************************************************************************
    qgsfeaturepicker.cpp
    --------------------
    begin                : November 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsfeaturepicker.h"
#include "qgsgeometry.h"
#include "qgslegendinterface.h"
#include "qgsmapcanvas.h"
#include "qgsrendererv2.h"
#include "qgsvectorlayer.h"

QPair<QgsFeature, QgsVectorLayer*> QgsFeaturePicker::pick( const QgsMapCanvas* canvas, const QgsPoint &mapPos, QGis::GeometryType geomType, filter_t filter )
{
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas->mapSettings() );
  double radiusmm = QSettings().value( "/Map/searchRadiusMM", QGis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : QGis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  QgsRectangle filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );


  QgsFeatureList features;
  foreach ( QgsMapLayer* layer, canvas->layers() )
  {
    if (( layer->type() != QgsMapLayer::VectorLayer && layer->type() != QgsMapLayer::RedliningLayer ) )
    {
      continue;
    }
    QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( layer );
    if ( geomType != QGis::AnyGeometry && geomType != QGis::AnyGeometry && vlayer->geometryType() != QGis::AnyGeometry && vlayer->geometryType() != geomType )
    {
      continue;
    }
    if ( vlayer->hasScaleBasedVisibility() &&
         ( vlayer->minimumScale() > canvas->mapSettings().scale() ||
           vlayer->maximumScale() <= canvas->mapSettings().scale() ) )
    {
      continue;
    }

    QgsFeatureRendererV2* renderer = vlayer->rendererV2();
    bool filteredRendering = false;
    if ( renderer && renderer->capabilities() & QgsFeatureRendererV2::ScaleDependent )
    {
      // setup scale for scale dependent visibility (rule based)
      renderer->startRender( renderContext, vlayer->pendingFields() );
      filteredRendering = renderer->capabilities() & QgsFeatureRendererV2::Filter;
    }

    QgsRectangle layerFilterRect = canvas->mapSettings().mapToLayerCoordinates( vlayer, filterRect );
    QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
    QgsFeature feature;
    while ( fit.nextFeature( feature ) )
    {
      if ( filteredRendering && !renderer->willRenderFeature( feature ) )
      {
        continue;
      }
      if ( filter && !filter( feature ) )
      {
        continue;
      }
      if ( geomType != QGis::AnyGeometry && feature.geometry()->type() != geomType )
      {
        continue;
      }
      features.append( feature );
    }
    if ( renderer && renderer->capabilities() & QgsFeatureRendererV2::ScaleDependent )
    {
      renderer->stopRender( renderContext );
    }
    if ( !features.empty() )
    {
      return qMakePair( features.front(), vlayer );
    }
  }
  return qMakePair( QgsFeature(), ( QgsVectorLayer* )( 0 ) );
}
