/***************************************************************************
    qgsredliningrendererv2.cpp
    ---------------------
    begin                : Sep 2015
    copyright            : (C) 2015 Sandro Mani
    email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsredliningrendererv2.h"
#include "qgssymbolv2.h"
#include "qgssymbollayerv2.h"
#include "qgslogger.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"


QgsRedliningRendererV2::QgsRedliningRendererV2()
    : QgsFeatureRendererV2( "redliningSymbol" )
    , mMarkerSymbol( new QgsMarkerSymbolV2() )
    , mLineSymbol( new QgsLineSymbolV2() )
    , mFillSymbol( new QgsFillSymbolV2() )
{
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "color", "\"fill\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "color_border", "\"outline\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "size", "2 * \"size\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "outline_width", "\"size\" / 10.0" );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "color", "\"fill\"" );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "color_border", "\"outline\"" );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "width", "\"size\"" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "color", "\"fill\"" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "color_border", "\"outline\"" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "width_border", "\"size\"" );
}

QgsSymbolV2* QgsRedliningRendererV2::originalSymbolForFeature( QgsFeature& feature )
{
  switch ( QGis::flatType( QGis::singleType( feature.geometry()->wkbType() ) ) )
  {
    case QGis::WKBPoint:
      return mMarkerSymbol.data();
    case QGis::WKBLineString:
      return mLineSymbol.data();
    case QGis::WKBPolygon:
      return mFillSymbol.data();
    default:
      return 0;
  }
  return 0;
}

void QgsRedliningRendererV2::startRender( QgsRenderContext& context, const QgsFields& fields )
{
  mMarkerSymbol->startRender( context, &fields );
  mLineSymbol->startRender( context, &fields );
  mFillSymbol->startRender( context, &fields );
}

bool QgsRedliningRendererV2::renderFeature( QgsFeature& feature, QgsRenderContext& context, int layer, bool selected, bool drawVertexMarker )
{
  if ( !feature.constGeometry() || !feature.geometry()->geometry() )
  {
    return false;
  }
  // Don't draw features whose text attribute is set - they are drawn as labels only
  if ( !feature.attribute( "text" ).toString().isEmpty() )
  {
    return true;
  }
  QgsAbstractGeometryV2* geom = feature.geometry()->geometry();
  QgsAbstractGeometryV2* segmentized = 0;

  //convert curve types to normal point/line/polygon ones
  switch ( QgsWKBTypes::flatType( geom->wkbType() ) )
  {
    case QgsWKBTypes::CurvePolygon:
    case QgsWKBTypes::CircularString:
    case QgsWKBTypes::CompoundCurve:
    case QgsWKBTypes::MultiSurface:
    case QgsWKBTypes::MultiCurve:
    {
      segmentized = geom->segmentize();
      geom = segmentized;
    }
    default:
      break;
  }
  int wkbSize;

  switch ( QgsWKBTypes::flatType( geom->wkbType() ) )
  {
    case QgsWKBTypes::Point:
    {
      QPointF pt;
      _getPoint( pt, context, geom->asWkb( wkbSize ) );
      mMarkerSymbol->renderPoint( pt, &feature, context, layer, selected );
      break;
    }
    case QgsWKBTypes::LineString:
    {
      QPolygonF pts;
      _getLineString( pts, context, geom->asWkb( wkbSize ) );
      mLineSymbol->renderPolyline( pts, &feature, context, layer, selected );

      if ( drawVertexMarker )
        renderVertexMarkerPolyline( pts, context );
      break;
    }
    case QgsWKBTypes::Polygon:
    {
      QPolygonF pts;
      QList<QPolygonF> holes;
      _getPolygon( pts, holes, context, geom->asWkb( wkbSize ) );
      mFillSymbol->renderPolygon( pts, ( holes.count() ? &holes : NULL ), &feature, context, layer, selected );

      if ( drawVertexMarker )
        renderVertexMarkerPolygon( pts, ( holes.count() ? &holes : NULL ), context );
      break;
    }
    default:
      break;
  }
  delete segmentized;
  return true;
}

void QgsRedliningRendererV2::stopRender( QgsRenderContext& context )
{
  mMarkerSymbol->stopRender( context );
  mLineSymbol->stopRender( context );
  mFillSymbol->stopRender( context );
}
