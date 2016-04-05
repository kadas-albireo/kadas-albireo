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
#include "qgsellipsesymbollayerv2.h"
#include "qgslogger.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgspointv2.h"


QgsRedliningRendererV2::QgsRedliningRendererV2()
    : QgsFeatureRendererV2( "redliningSymbol" )
    , mMarkerSymbol( new QgsMarkerSymbolV2( QgsSymbolLayerV2List() << new QgsEllipseSymbolLayerV2() ) )
    , mLineSymbol( new QgsLineSymbolV2() )
    , mFillSymbol( new QgsFillSymbolV2() )
{
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "fill_color", "\"fill\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "fill_style", "\"fill_style\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "outline_color", "\"outline\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "outline_style", "\"outline_style\"" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "outline_width", "\"size\" / 4" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "width", "eval(regexp_substr(\"flags\",'w=([^,]+)'))" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "height", "eval(regexp_substr(\"flags\",'h=([^,]+)'))" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "rotation", "eval(regexp_substr(\"flags\",'r=([^,]+)'))" );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( "symbol_name", "regexp_substr(\"flags\",'symbol=(\\\\w+)')" );

  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "color", "\"outline\"" );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "color_border", "\"outline\"" );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "width", "\"size\" / 4" );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( "line_style", "\"outline_style\"" );

  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "color", "\"fill\"" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "color_border", "\"outline\"" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "width_border", "\"size\" / 4" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "border_style", "\"outline_style\"" );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( "fill_style", "\"fill_style\"" );
}

QgsSymbolV2* QgsRedliningRendererV2::originalSymbolForFeature( QgsFeature& feature )
{
  switch ( QgsWKBTypes::flatType( QgsWKBTypes::singleType( feature.geometry()->geometry()->wkbType() ) ) )
  {
    case QgsWKBTypes::Point:
      return mMarkerSymbol.data();
    case QgsWKBTypes::LineString:
    case QgsWKBTypes::CircularString:
    case QgsWKBTypes::CompoundCurve:
      return mLineSymbol.data();
    case QgsWKBTypes::Polygon:
    case QgsWKBTypes::CurvePolygon:
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
  if ( !feature.attribute( "text" ).toString().isEmpty() && !feature.attribute( "flags" ).toString().contains( "symbol=" ) )
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
        drawVertexMarkers( feature.geometry()->geometry(), context );
      break;
    }
    case QgsWKBTypes::Polygon:
    {
      QPolygonF pts;
      QList<QPolygonF> holes;
      _getPolygon( pts, holes, context, geom->asWkb( wkbSize ) );
      mFillSymbol->renderPolygon( pts, ( holes.count() ? &holes : NULL ), &feature, context, layer, selected );

      if ( drawVertexMarker )
        drawVertexMarkers( feature.geometry()->geometry(), context );
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

void QgsRedliningRendererV2::drawVertexMarkers( QgsAbstractGeometryV2 *geom, QgsRenderContext& context )
{
  const QgsCoordinateTransform* ct = context.coordinateTransform();
  const QgsMapToPixel& mtp = context.mapToPixel();

  QgsPointV2 vertexPoint;
  QgsVertexId vertexId;
  double x, y, z;
  while ( geom->nextVertex( vertexId, vertexPoint ) )
  {
    //transform
    x = vertexPoint.x(); y = vertexPoint.y(); z = vertexPoint.z();
    if ( ct )
    {
      ct->transformInPlace( x, y, z );
    }
    mtp.transformInPlace( x, y );
    renderVertexMarker( QPointF( x, y ), context );
  }
}
