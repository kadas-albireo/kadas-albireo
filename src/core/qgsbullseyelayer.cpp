/***************************************************************************
 *  qgsbullseyelayer.cpp                                                   *
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

#include "qgscrscache.h"
#include "qgsbullseyelayer.h"
#include "qgsmaplayerrenderer.h"
#include "qgsdistancearea.h"
#include "qgssymbollayerv2utils.h"

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>


class QgsBullsEyeLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( QgsBullsEyeLayer* layer, QgsRenderContext& rendererContext )
        : QgsMapLayerRenderer( layer->id() )
        , mLayer( layer )
        , mRendererContext( rendererContext )
        , mGeod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() )
    {
      mDa.setEllipsoid( "WGS84" );
      mDa.setEllipsoidalMode( true );
      mDa.setSourceCrs( QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
    }

    bool render() override
    {
      if ( mLayer->mRings <= 0 || mLayer->mInterval <= 0 )
      {
        return true;
      }

      const QgsMapToPixel& mapToPixel = mRendererContext.mapToPixel();
      bool labelAxes = mLayer->mLabellingMode == LABEL_AXES || mLayer->mLabellingMode == LABEL_AXES_RINGS;
      bool labelRings = mLayer->mLabellingMode == LABEL_RINGS || mLayer->mLabellingMode == LABEL_AXES_RINGS;

      mRendererContext.painter()->save();
      mRendererContext.painter()->setPen( QPen( mLayer->mColor, mLayer->mLineWidth ) );
      QFont font = mRendererContext.painter()->font();
      font.setPixelSize( mLayer->mFontSize );
      mRendererContext.painter()->setFont( font );
      QFontMetrics metrics( mRendererContext.painter()->font() );

      const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mLayer->crs().authid(), "EPSG:4326" );
      const QgsCoordinateTransform* rct = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", mRendererContext.coordinateTransform() ? mRendererContext.coordinateTransform()->destCRS().authid() : mLayer->crs().authid() );

      // Draw rings
      QgsPoint wgsCenter = ct->transform( mLayer->mCenter );
      for ( int iRing = 0; iRing < mLayer->mRings; ++iRing )
      {
        double radMeters = mLayer->mInterval * ( 1 + iRing ) * QGis::fromUnitToUnitFactor( QGis::NauticalMiles, QGis::Meters );

        QPolygonF poly;
        for ( int a = 0; a <= 360; ++a )
        {
          QgsPoint wgsPoint = mDa.computeDestination( wgsCenter, radMeters, a );
          QgsPoint mapPoint = rct->transform( wgsPoint );
          poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        }
        QPainterPath path;
        path.addPolygon( poly );
        mRendererContext.painter()->drawPath( path );
        if ( labelRings )
        {
          QString label = QString( "%1 nm" ).arg(( iRing + 1 ) * mLayer->mInterval, 0, 'f', 2 );
          double x = poly.last().x() - 0.5 * metrics.width( label );
          mRendererContext.painter()->drawText( x, poly.last().y() - 0.25 * metrics.height(), label );
        }
      }

      // Draw axes
      double axisRadiusMeters = mLayer->mInterval * ( mLayer->mRings + 1 ) * QGis::fromUnitToUnitFactor( QGis::NauticalMiles, QGis::Meters );
      for ( int bearing = 0; bearing < 360; bearing += mLayer->mAxesInterval )
      {
        QgsPoint wgsPoint = mDa.computeDestination( wgsCenter, axisRadiusMeters, bearing );
        GeographicLib::GeodesicLine line = mGeod.InverseLine( wgsCenter.y(), wgsCenter.x(), wgsPoint.y(), wgsPoint.x() );
        double dist = line.Distance();
        double sdist = 500000; // ~500km segments
        int nSegments = qMax( 1, int( std::ceil( dist / sdist ) ) );
        QPolygonF poly;
        for ( int iSeg = 0; iSeg < nSegments; ++iSeg )
        {
          double lat, lon;
          line.Position( iSeg * sdist, lat, lon );
          QgsPoint mapPoint = rct->transform( QgsPoint( lon, lat ) );
          poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        }
        double lat, lon;
        line.Position( dist, lat, lon );
        QgsPoint mapPoint = rct->transform( QgsPoint( lon, lat ) );
        poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        QPainterPath path;
        path.addPolygon( poly );
        mRendererContext.painter()->drawPath( path );
        if ( labelAxes )
        {
          QString label = QString( "%1°" ).arg( bearing );
          int n = poly.size();
          double dx = n > 1 ? poly[n - 1].x() - poly[n - 2].x() : 0;
          double dy = n > 1 ? poly[n - 1].y() - poly[n - 2].y() : 0;
          double l = std::sqrt( dx * dx + dy * dy );
          double d = mLayer->mFontSize;
          double w = metrics.width( label );
          double x = n < 2 ? poly.last().x() : poly.last().x() + d * dx / l;
          double y = n < 2 ? poly.last().y() : poly.last().y() + d * dy / l;
          mRendererContext.painter()->drawText( x - 0.5 * w, y - d, w, 2 * d, Qt::AlignCenter | Qt::AlignHCenter, label );
        }
      }

      mRendererContext.painter()->restore();
      return true;
    }

  private:
    QgsBullsEyeLayer* mLayer;
    QgsRenderContext& mRendererContext;
    QgsDistanceArea mDa;
    GeographicLib::Geodesic mGeod;

    QPair<QPointF, QPointF> screenLine( const QgsPoint& p1, const QgsPoint& p2 ) const
    {
      const QgsCoordinateTransform* crst = mRendererContext.coordinateTransform();
      const QgsMapToPixel& mapToPixel = mRendererContext.mapToPixel();
      QPointF sp1 = mapToPixel.transform( crst ? crst->transform( p1 ) : p1 ).toQPointF();
      QPointF sp2 = mapToPixel.transform( crst ? crst->transform( p2 ) : p2 ).toQPointF();
      return qMakePair( sp1, sp2 );
    }
};

QgsBullsEyeLayer::QgsBullsEyeLayer( const QString& name )
    : QgsPluginLayer( layerTypeKey(), name )
{
  mValid = true;
  mPriority = 100;
}

void QgsBullsEyeLayer::setup( const QgsPoint &center, const QgsCoordinateReferenceSystem &crs, int rings, double interval, double axesInterval )
{
  mCenter = center;
  mRings = rings;
  mInterval = interval;
  mAxesInterval = axesInterval;
  setCrs( crs, false );
}

QgsMapLayerRenderer* QgsBullsEyeLayer::createMapRenderer( QgsRenderContext& rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle QgsBullsEyeLayer::extent()
{
  QgsDistanceArea da;
  double radius = mRings * mInterval * QGis::fromUnitToUnitFactor( QGis::NauticalMiles, QGis::Meters );
  radius *= QGis::fromUnitToUnitFactor( QGis::Meters, crs().mapUnits() );
  return QgsRectangle( mCenter.x() - radius, mCenter.y() - radius, mCenter.x() + radius, mCenter.y() + radius );
}

bool QgsBullsEyeLayer::readXml( const QDomNode& layer_node )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  mTransparency = layerEl.attribute( "transparency" ).toInt();
  mCenter.setX( layerEl.attribute( "x" ).toDouble() );
  mCenter.setY( layerEl.attribute( "y" ).toDouble() );
  mRings = layerEl.attribute( "rings" ).toInt();
  mAxesInterval = layerEl.attribute( "axes" ).toDouble();
  mInterval = layerEl.attribute( "interval" ).toDouble();
  mFontSize = layerEl.attribute( "fontSize" ).toInt();
  mLineWidth = layerEl.attribute( "lineWidth" ).toInt();
  mColor = QgsSymbolLayerV2Utils::decodeColor( layerEl.attribute( "color" ) );
  mLabellingMode = static_cast<LabellingMode>( layerEl.attribute( "labellingMode" ).toInt() );

  setCrs( QgsCRSCache::instance()->crsByAuthId( layerEl.attribute( "crs" ) ) );
  return true;
}

bool QgsBullsEyeLayer::writeXml( QDomNode & layer_node, QDomDocument & /*document*/ )
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  layerEl.setAttribute( "transparency", mTransparency );
  layerEl.setAttribute( "x", mCenter.x() );
  layerEl.setAttribute( "y", mCenter.y() );
  layerEl.setAttribute( "rings", mRings );
  layerEl.setAttribute( "axes", mAxesInterval );
  layerEl.setAttribute( "interval", mInterval );
  layerEl.setAttribute( "crs", crs().authid() );
  layerEl.setAttribute( "fontSize", mFontSize );
  layerEl.setAttribute( "lineWidth", mLineWidth );
  layerEl.setAttribute( "color", QgsSymbolLayerV2Utils::encodeColor( mColor ) );
  layerEl.setAttribute( "labellingMode", static_cast<int>( mLabellingMode ) );
  return true;
}
