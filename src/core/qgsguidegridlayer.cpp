/***************************************************************************
 *  qgsguidegridlayer.cpp                                                  *
 *  --------------                                                         *
 *  begin                : March 2018                                      *
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
#include "qgsguidegridlayer.h"
#include "qgsmaplayerrenderer.h"
#include "qgspolygonv2.h"
#include "qgslinestringv2.h"
#include "qgssymbollayerv2utils.h"


static QString alphaLabel( int i )
{
  QString label;
  do
  {
    i -= 1;
    int res = i % 26;
    label.prepend( QChar( 'A' + res ) );
    i /= 26;
  }
  while ( i > 0 );
  return label;
}

class QgsGuideGridLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( QgsGuideGridLayer* layer, QgsRenderContext& rendererContext )
        : QgsMapLayerRenderer( layer->id() )
        , mLayer( layer )
        , mRendererContext( rendererContext )
    {}

    bool render() override
    {
      if ( mLayer->mRows == 0 || mLayer->mCols == 0 )
      {
        return true;
      }

      static int labelBoxSize = mLayer->mFontSize + 5;
      mRendererContext.painter()->save();
      mRendererContext.painter()->setOpacity(( 100. - mLayer->mTransparency ) / 100. );
      mRendererContext.painter()->setPen( QPen( mLayer->mColor, 1. ) );

      QFont font;
      font.setPixelSize( mLayer->mFontSize );
      mRendererContext.painter()->setFont( font );
      const QgsCoordinateTransform* crst = mRendererContext.coordinateTransform();
      const QgsMapToPixel& mapToPixel = mRendererContext.mapToPixel();
      const QgsRectangle& gridRect = mLayer->mGridRect;
      QgsPoint pTL = QgsPoint( mRendererContext.extent().xMinimum(), mRendererContext.extent().yMaximum() );
      QgsPoint pBR = QgsPoint( mRendererContext.extent().xMaximum(), mRendererContext.extent().yMinimum() );
      QPointF screenTL = mapToPixel.transform( crst ? crst->transform( pTL ) : pTL ).toQPointF();
      QPointF screenBR = mapToPixel.transform( crst ? crst->transform( pBR ) : pBR ).toQPointF();
      QRectF screenRect( screenTL, screenBR );

      // Draw vertical lines
      double ix = gridRect.width() / mLayer->mCols;
      QPair<QPointF, QPointF> vLine1 = screenLine( QgsPoint( gridRect.xMinimum(), gridRect.yMaximum() ), QgsPoint( gridRect.xMinimum(), gridRect.yMinimum() ) );
      mRendererContext.painter()->drawLine( vLine1.first, vLine1.second );
      double sy1 = qMax( vLine1.first.y(), screenRect.top() );
      double sy2 = qMin( vLine1.second.y(), screenRect.bottom() );
      for ( int col = 1; col <= mLayer->mCols; ++col )
      {
        double x2 = gridRect.xMinimum() + col * ix;
        QPair<QPointF, QPointF> vLine2 = screenLine( QgsPoint( x2, gridRect.yMaximum() ), QgsPoint( x2, gridRect.yMinimum() ) );
        mRendererContext.painter()->drawLine( vLine2.first, vLine2.second );

        double sx1 = vLine1.first.x();
        double sx2 = vLine2.first.x();
        QString label = mLayer->mLabellingMode == QgsGuideGridLayer::LABEL_A_1 ? alphaLabel( col ) : QString( "%1" ).arg( col );
        if ( sy1 < vLine1.second.y() - 2 * labelBoxSize )
        {
          mRendererContext.painter()->drawText( QRectF( sx1, sy1, sx2 - sx1, labelBoxSize ), Qt::AlignHCenter | Qt::AlignVCenter, label );
        }
        if ( sy2 > vLine1.first.y() + 2 * labelBoxSize )
        {
          mRendererContext.painter()->drawText( QRectF( sx1, sy2 - labelBoxSize, sx2 - sx1, labelBoxSize ), Qt::AlignHCenter | Qt::AlignVCenter, label );
        }

        vLine1 = vLine2;
      }

      // Draw horizontal lines
      double iy = gridRect.height() / mLayer->mRows;
      QPair<QPointF, QPointF> hLine1 = screenLine( QgsPoint( gridRect.xMinimum(), gridRect.yMaximum() ), QgsPoint( gridRect.xMaximum(), gridRect.yMaximum() ) );
      mRendererContext.painter()->drawLine( hLine1.first, hLine1.second );
      double sx1 = qMax( hLine1.first.x(), screenRect.left() );
      double sx2 = qMin( vLine1.second.x(), screenRect.right() );;
      for ( int row = 1; row <= mLayer->mRows; ++row )
      {
        double y = gridRect.yMaximum() - row * iy;
        QPair<QPointF, QPointF> hLine2 = screenLine( QgsPoint( gridRect.xMinimum(), y ), QgsPoint( gridRect.xMaximum(), y ) );
        mRendererContext.painter()->drawLine( hLine2.first, hLine2.second );

        double sy1 = hLine1.first.y();
        double sy2 = hLine2.first.y();
        QString label = mLayer->mLabellingMode == QgsGuideGridLayer::LABEL_1_A ? alphaLabel( row ) : QString( "%1" ).arg( row );
        if ( sx1 < vLine1.second.x() - 2 * labelBoxSize )
        {
          mRendererContext.painter()->drawText( QRectF( sx1, sy1, labelBoxSize, sy2 - sy1 ), Qt::AlignHCenter | Qt::AlignVCenter, label );
        }
        if ( sx2 > hLine1.first.x() + 2 * labelBoxSize )
        {
          mRendererContext.painter()->drawText( QRectF( sx2 - labelBoxSize, sy1, labelBoxSize, sy2 - sy1 ), Qt::AlignHCenter | Qt::AlignVCenter, label );
        }

        hLine1 = hLine2;
      }
      mRendererContext.painter()->restore();
      return true;
    }

  private:
    QgsGuideGridLayer* mLayer;
    QgsRenderContext& mRendererContext;

    QPair<QPointF, QPointF> screenLine( const QgsPoint& p1, const QgsPoint& p2 ) const
    {
      const QgsCoordinateTransform* crst = mRendererContext.coordinateTransform();
      const QgsMapToPixel& mapToPixel = mRendererContext.mapToPixel();
      QPointF sp1 = mapToPixel.transform( crst ? crst->transform( p1 ) : p1 ).toQPointF();
      QPointF sp2 = mapToPixel.transform( crst ? crst->transform( p2 ) : p2 ).toQPointF();
      return qMakePair( sp1, sp2 );
    }
};

QgsGuideGridLayer::QgsGuideGridLayer( const QString& name )
    : QgsPluginLayer( layerTypeKey(), name )
{
  mValid = true;
  mPriority = 100;
}

void QgsGuideGridLayer::setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs , bool colSizeLocked, bool rowSizeLocked )
{
  mGridRect = gridRect;
  mCols = cols;
  mRows = rows;
  mColSizeLocked = colSizeLocked;
  mRowSizeLocked = rowSizeLocked;
  setCrs( crs, false );
}

QgsMapLayerRenderer* QgsGuideGridLayer::createMapRenderer( QgsRenderContext& rendererContext )
{
  return new Renderer( this, rendererContext );
}

bool QgsGuideGridLayer::readXml( const QDomNode& layer_node )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  mTransparency = layerEl.attribute( "transparency" ).toInt();
  mGridRect.setXMinimum( layerEl.attribute( "xmin" ).toDouble() );
  mGridRect.setYMinimum( layerEl.attribute( "ymin" ).toDouble() );
  mGridRect.setXMaximum( layerEl.attribute( "xmax" ).toDouble() );
  mGridRect.setYMaximum( layerEl.attribute( "ymax" ).toDouble() );
  mCols = layerEl.attribute( "cols" ).toInt();
  mRows = layerEl.attribute( "rows" ).toInt();
  mColSizeLocked = layerEl.attribute( "colSizeLocked", "0" ).toInt();
  mRowSizeLocked = layerEl.attribute( "rowSizeLocked", "0" ).toInt();
  mFontSize = layerEl.attribute( "fontSize" ).toInt();
  mColor = QgsSymbolLayerV2Utils::decodeColor( layerEl.attribute( "color" ) );
  mLabellingMode = static_cast<LabellingMode>( layerEl.attribute( "labellingMode" ).toInt() );

  setCrs( QgsCRSCache::instance()->crsByAuthId( layerEl.attribute( "crs" ) ) );
  return true;
}

QList<QgsGuideGridLayer::IdentifyResult> QgsGuideGridLayer::identify( const QgsPoint& mapPos, const QgsMapSettings& mapSettings )
{
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mapSettings.destinationCrs().authid(), crs().authid() );
  QgsPoint pos = crst->transform( mapPos );

  double colWidth = ( mGridRect.xMaximum() - mGridRect.xMinimum() ) / mCols;
  double rowHeight = ( mGridRect.yMaximum() - mGridRect.yMinimum() ) / mRows;

  int i = std::floor(( pos.x() - mGridRect.xMinimum() ) / colWidth );
  int j = std::floor(( mGridRect.yMaximum() - pos.y() ) / rowHeight );

  QgsPolygonV2* bbox = new QgsPolygonV2();
  QgsLineStringV2* ring = new QgsLineStringV2();
  ring->setPoints(
    QList<QgsPointV2>()
    << QgsPointV2( mGridRect.xMinimum() + i * colWidth,     mGridRect.yMaximum() - j * rowHeight )
    << QgsPointV2( mGridRect.xMinimum() + ( i + 1 ) * colWidth, mGridRect.yMaximum() - j * rowHeight )
    << QgsPointV2( mGridRect.xMinimum() + ( i + 1 ) * colWidth, mGridRect.yMaximum() - ( j + 1 ) * rowHeight )
    << QgsPointV2( mGridRect.xMinimum() + i * colWidth,     mGridRect.yMaximum() - ( j + 1 ) * rowHeight )
    << QgsPointV2( mGridRect.xMinimum() + i * colWidth,     mGridRect.yMaximum() - j * rowHeight )
  );
  bbox->setExteriorRing( ring );
  QMap<QString, QVariant> attrs;

  if ( mLabellingMode == QgsGuideGridLayer::LABEL_1_A )
  {
    return QList<IdentifyResult>() << IdentifyResult( tr( "Cell %1, %2" ).arg( alphaLabel( 1 + j ) ).arg( 1 + i ), attrs, QgsGeometry( bbox ) );
  }
  else
  {
    return QList<IdentifyResult>() << IdentifyResult( tr( "Cell %1, %2" ).arg( 1 + j ).arg( alphaLabel( 1 + i ) ), attrs, QgsGeometry( bbox ) );
  }
}

bool QgsGuideGridLayer::writeXml( QDomNode & layer_node, QDomDocument & /*document*/ )
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  layerEl.setAttribute( "transparency", mTransparency );
  layerEl.setAttribute( "xmin", mGridRect.xMinimum() );
  layerEl.setAttribute( "ymin", mGridRect.yMinimum() );
  layerEl.setAttribute( "xmax", mGridRect.xMaximum() );
  layerEl.setAttribute( "ymax", mGridRect.yMaximum() );
  layerEl.setAttribute( "cols", mCols );
  layerEl.setAttribute( "rows", mRows );
  layerEl.setAttribute( "colSizeLocked", mColSizeLocked ? 1 : 0 );
  layerEl.setAttribute( "rowSizeLocked", mRowSizeLocked ? 1 : 0 );
  layerEl.setAttribute( "crs", crs().authid() );
  layerEl.setAttribute( "fontSize", mFontSize );
  layerEl.setAttribute( "color", QgsSymbolLayerV2Utils::encodeColor( mColor ) );
  layerEl.setAttribute( "labellingMode", static_cast<int>( mLabellingMode ) );
  return true;
}
