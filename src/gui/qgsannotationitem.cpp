/***************************************************************************
                              qgsannotationitem.cpp
                              ----------------------
  begin                : February 9, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsannotationitem.h"
#include "qgsmapcanvas.h"
#include "qgsrendercontext.h"
#include "qgssymbollayerv2utils.h"
#include "qgssymbolv2.h"
#include <QPainter>
#include <QPen>

QgsAnnotationItem::QgsAnnotationItem( QgsMapCanvas* mapCanvas ): QgsMapCanvasItem( mapCanvas ), mMapPositionFixed( true ), mOffsetFromReferencePoint( QPointF( 50, -50 ) )
{
  setFlag( QGraphicsItem::ItemIsSelectable, true );
  mMarkerSymbol = new QgsMarkerSymbolV2();
  mFrameBorderWidth = 1.0;
  mFrameColor = QColor( 0, 0, 0 );
  mFrameBackgroundColor = QColor( 255, 255, 255 );
  setData( 0, "AnnotationItem" );
}

QgsAnnotationItem::~QgsAnnotationItem()
{
  delete mMarkerSymbol;
}

void QgsAnnotationItem::setMarkerSymbol( QgsMarkerSymbolV2* symbol )
{
  delete mMarkerSymbol;
  mMarkerSymbol = symbol;
  updateBoundingRect();
}

void QgsAnnotationItem::setMapPosition( const QgsPoint& pos )
{
  mMapPosition = pos;
  setPos( toCanvasCoordinates( mMapPosition ) );
}

void QgsAnnotationItem::setOffsetFromReferencePoint( const QPointF& offset )
{
  mOffsetFromReferencePoint = offset;
  updateBoundingRect();
  updateBalloon();
}

void QgsAnnotationItem::setMapPositionFixed( bool fixed )
{
  if ( mMapPositionFixed && !fixed )
  {
    //set map position to the top left corner of the balloon
    setMapPosition( toMapCoordinates( QPointF( pos() + mOffsetFromReferencePoint ).toPoint() ) );
    mOffsetFromReferencePoint = QPointF( 0, 0 );
  }
  else if ( fixed && !mMapPositionFixed )
  {
    setMapPosition( toMapCoordinates( QPointF( pos() + QPointF( -100, -100 ) ).toPoint() ) );
    mOffsetFromReferencePoint = QPointF( 100, 100 );
  }
  mMapPositionFixed = fixed;
  updateBoundingRect();
  updateBalloon();
  update();
}

void QgsAnnotationItem::updatePosition()
{
  if ( mMapPositionFixed )
  {
    setPos( toCanvasCoordinates( mMapPosition ) );
  }
  else
  {
    mMapPosition = toMapCoordinates( pos().toPoint() );
  }
}

QRectF QgsAnnotationItem::boundingRect() const
{
  return mBoundingRect;
}

QSizeF QgsAnnotationItem::minimumFrameSize() const
{
  return QSizeF( 0, 0 );
}

void QgsAnnotationItem::updateBoundingRect()
{
  prepareGeometryChange();
  double halfSymbolSize = 0.0;
  if ( mMarkerSymbol )
  {
    halfSymbolSize = scaledSymbolSize() / 2.0;
  }

  double xMinPos = qMin( -halfSymbolSize, mOffsetFromReferencePoint.x() - mFrameBorderWidth );
  double xMaxPos = qMax( halfSymbolSize, mOffsetFromReferencePoint.x() + mFrameSize.width() + mFrameBorderWidth );
  double yMinPos = qMin( -halfSymbolSize, mOffsetFromReferencePoint.y() - mFrameBorderWidth );
  double yMaxPos = qMax( halfSymbolSize, mOffsetFromReferencePoint.y() + mFrameSize.height() + mFrameBorderWidth );
  mBoundingRect = QRectF( xMinPos, yMinPos, xMaxPos - xMinPos, yMaxPos - yMinPos );
}

void QgsAnnotationItem::updateBalloon()
{
  //first test if the point is in the frame. In that case we don't need a balloon.
  if ( !mMapPositionFixed ||
       ( mOffsetFromReferencePoint.x() < 0 && ( mOffsetFromReferencePoint.x() + mFrameSize.width() ) > 0
         && mOffsetFromReferencePoint.y() < 0 && ( mOffsetFromReferencePoint.y() + mFrameSize.height() ) > 0 ) )
  {
    mBalloonSegment = -1;
    return;
  }

  //edge list
  QList<QLineF> segmentList;
  segmentList << segment( 0 ); segmentList << segment( 1 ); segmentList << segment( 2 ); segmentList << segment( 3 );

  //find  closest edge / closest edge point
  double minEdgeDist = DBL_MAX;
  int minEdgeIndex = -1;
  QLineF minEdge;
  QgsPoint minEdgePoint;
  QgsPoint origin( 0, 0 );

  for ( int i = 0; i < 4; ++i )
  {
    QLineF currentSegment = segmentList.at( i );
    QgsPoint currentMinDistPoint;
    double currentMinDist = origin.sqrDistToSegment( currentSegment.x1(), currentSegment.y1(), currentSegment.x2(), currentSegment.y2(), currentMinDistPoint );
    if ( currentMinDist < minEdgeDist )
    {
      minEdgeIndex = i;
      minEdgePoint = currentMinDistPoint;
      minEdgeDist = currentMinDist;
      minEdge = currentSegment;
    }
  }

  if ( minEdgeIndex < 0 )
  {
    return;
  }

  //make that configurable for the item
  double segmentPointWidth = 10;

  mBalloonSegment = minEdgeIndex;
  QPointF minEdgeEnd = minEdge.p2();
  mBalloonSegmentPoint1 = QPointF( minEdgePoint.x(), minEdgePoint.y() );
  if ( sqrt( minEdgePoint.sqrDist( minEdgeEnd.x(), minEdgeEnd.y() ) ) < segmentPointWidth )
  {
    mBalloonSegmentPoint1 = pointOnLineWithDistance( minEdge.p2(), minEdge.p1(), segmentPointWidth );
  }

  mBalloonSegmentPoint2 = pointOnLineWithDistance( mBalloonSegmentPoint1, minEdge.p2(), 10 );
}

void QgsAnnotationItem::drawFrame( QPainter* p )
{
  QPen framePen( mFrameColor );
  framePen.setWidthF( mFrameBorderWidth );

  p->setPen( framePen );
  QBrush frameBrush( mFrameBackgroundColor );
  p->setBrush( frameBrush );
  p->setRenderHint( QPainter::Antialiasing, true );

  QPolygonF poly;
  for ( int i = 0; i < 4; ++i )
  {
    QLineF currentSegment = segment( i );
    poly << currentSegment.p1();
    if ( i == mBalloonSegment && mMapPositionFixed )
    {
      poly << mBalloonSegmentPoint1;
      poly << QPointF( 0, 0 );
      poly << mBalloonSegmentPoint2;
    }
    poly << currentSegment.p2();
  }
  p->drawPolygon( poly );
}

void QgsAnnotationItem::setFrameSize( const QSizeF& size )
{
  QSizeF frameSize = minimumFrameSize().expandedTo( size ); //don't allow frame sizes below minimum
  mFrameSize = frameSize;
  updateBoundingRect();
  updateBalloon();
}

void QgsAnnotationItem::drawMarkerSymbol( QPainter* p )
{
  if ( !p )
  {
    return;
  }

  QgsRenderContext renderContext;
  if ( !setRenderContextVariables( p, renderContext ) )
  {
    return;
  }

  if ( mMarkerSymbol )
  {
    mMarkerSymbol->startRender( renderContext );
    mMarkerSymbol->renderPoint( QPointF( 0, 0 ), 0, renderContext );
    mMarkerSymbol->stopRender( renderContext );
  }
}

void QgsAnnotationItem::drawSelectionBoxes( QPainter* p )
{
  if ( !p )
  {
    return;
  }

  //no selection boxes for composer mode
  if ( data( 1 ).toString() == "composer" )
  {
    return;
  }

  double handlerSize = 10;
  p->setPen( Qt::NoPen );
  p->setBrush( QColor( 200, 200, 210, 120 ) );
  p->drawRect( QRectF( mBoundingRect.left(), mBoundingRect.top(), handlerSize, handlerSize ) );
  p->drawRect( QRectF( mBoundingRect.right() - handlerSize, mBoundingRect.top(), handlerSize, handlerSize ) );
  p->drawRect( QRectF( mBoundingRect.right() - handlerSize, mBoundingRect.bottom() - handlerSize, handlerSize, handlerSize ) );
  p->drawRect( QRectF( mBoundingRect.left(), mBoundingRect.bottom() - handlerSize, handlerSize, handlerSize ) );
}

QLineF QgsAnnotationItem::segment( int index )
{
  switch ( index )
  {
    case 0:
      return QLineF( mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y(), mOffsetFromReferencePoint.x()
                     + mFrameSize.width(), mOffsetFromReferencePoint.y() );
    case 1:
      return QLineF( mOffsetFromReferencePoint.x() + mFrameSize.width(), mOffsetFromReferencePoint.y(),
                     mOffsetFromReferencePoint.x() + mFrameSize.width(), mOffsetFromReferencePoint.y() + mFrameSize.height() );
    case 2:
      return QLineF( mOffsetFromReferencePoint.x() + mFrameSize.width(), mOffsetFromReferencePoint.y() + mFrameSize.height(),
                     mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y() + mFrameSize.height() );
    case 3:
      return QLineF( mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y() + mFrameSize.height(),
                     mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y() );
    default:
      return QLineF();
  }
}

QPointF QgsAnnotationItem::pointOnLineWithDistance( const QPointF& startPoint, const QPointF& directionPoint, double distance ) const
{
  double dx = directionPoint.x() - startPoint.x();
  double dy = directionPoint.y() - startPoint.y();
  double length = sqrt( dx * dx + dy * dy );
  double scaleFactor = distance / length;
  return QPointF( startPoint.x() + dx * scaleFactor, startPoint.y() + dy * scaleFactor );
}

QgsAnnotationItem::MouseMoveAction QgsAnnotationItem::moveActionForPosition( const QPointF& pos ) const
{
  QPointF itemPos = mapFromScene( pos );

  int cursorSensitivity = 7;

  if ( qAbs( itemPos.x() ) < cursorSensitivity && qAbs( itemPos.y() ) < cursorSensitivity ) //move map point if position is close to the origin
  {
    return MoveMapPosition;
  }

  bool left, right, up, down;
  left = qAbs( itemPos.x() - mOffsetFromReferencePoint.x() ) < cursorSensitivity;
  right = qAbs( itemPos.x() - ( mOffsetFromReferencePoint.x() + mFrameSize.width() ) ) < cursorSensitivity;
  up = qAbs( itemPos.y() - mOffsetFromReferencePoint.y() ) < cursorSensitivity;
  down = qAbs( itemPos.y() - ( mOffsetFromReferencePoint.y() + mFrameSize.height() ) ) < cursorSensitivity;

  if ( left && up )
  {
    return ResizeFrameLeftUp;
  }
  else if ( right && up )
  {
    return ResizeFrameRightUp;
  }
  else if ( left && down )
  {
    return ResizeFrameLeftDown;
  }
  else if ( right && down )
  {
    return ResizeFrameRightDown;
  }
  if ( left )
  {
    return ResizeFrameLeft;
  }
  if ( right )
  {
    return ResizeFrameRight;
  }
  if ( up )
  {
    return ResizeFrameUp;
  }
  if ( down )
  {
    return ResizeFrameDown;
  }

  //finally test if pos is in the frame area
  if ( itemPos.x() >= mOffsetFromReferencePoint.x() && itemPos.x() <= ( mOffsetFromReferencePoint.x() + mFrameSize.width() )
       && itemPos.y() >= mOffsetFromReferencePoint.y() && itemPos.y() <= ( mOffsetFromReferencePoint.y() + mFrameSize.height() ) )
  {
    return MoveFramePosition;
  }
  return NoAction;
}

Qt::CursorShape QgsAnnotationItem::cursorShapeForAction( MouseMoveAction moveAction ) const
{
  switch ( moveAction )
  {
    case NoAction:
      return Qt::ArrowCursor;
    case MoveMapPosition:
    case MoveFramePosition:
      return Qt::SizeAllCursor;
    case ResizeFrameUp:
    case ResizeFrameDown:
      return Qt::SizeVerCursor;
    case ResizeFrameLeft:
    case ResizeFrameRight:
      return Qt::SizeHorCursor;
    case ResizeFrameLeftUp:
    case ResizeFrameRightDown:
      return Qt::SizeFDiagCursor;
    case ResizeFrameRightUp:
    case ResizeFrameLeftDown:
      return Qt::SizeBDiagCursor;
    default:
      return Qt::ArrowCursor;
  }
}

double QgsAnnotationItem::scaledSymbolSize() const
{
  if ( !mMarkerSymbol )
  {
    return 0.0;
  }

  if ( !mMapCanvas )
  {
    return mMarkerSymbol->size();
  }

  double dpmm = mMapCanvas->logicalDpiX() / 25.4;
  return dpmm * mMarkerSymbol->size();
}

void QgsAnnotationItem::_writeXML( QDomDocument& doc, QDomElement& itemElem ) const
{
  if ( itemElem.isNull() )
  {
    return;
  }
  QDomElement annotationElem = doc.createElement( "AnnotationItem" );
  annotationElem.setAttribute( "mapPositionFixed", mMapPositionFixed );
  annotationElem.setAttribute( "mapPosX", qgsDoubleToString( mMapPosition.x() ) );
  annotationElem.setAttribute( "mapPosY", qgsDoubleToString( mMapPosition.y() ) );
  annotationElem.setAttribute( "offsetX", qgsDoubleToString( mOffsetFromReferencePoint.x() ) );
  annotationElem.setAttribute( "offsetY", qgsDoubleToString( mOffsetFromReferencePoint.y() ) );
  annotationElem.setAttribute( "frameWidth", QString::number( mFrameSize.width() ) );
  annotationElem.setAttribute( "frameHeight", QString::number( mFrameSize.height() ) );
  QPointF canvasPos = pos();
  annotationElem.setAttribute( "canvasPosX", qgsDoubleToString( canvasPos.x() ) );
  annotationElem.setAttribute( "canvasPosY", qgsDoubleToString( canvasPos.y() ) );
  annotationElem.setAttribute( "frameBorderWidth", QString::number( mFrameBorderWidth ) );
  annotationElem.setAttribute( "frameColor", mFrameColor.name() );
  annotationElem.setAttribute( "frameColorAlpha", mFrameColor.alpha() );
  annotationElem.setAttribute( "frameBackgroundColor", mFrameBackgroundColor.name() );
  annotationElem.setAttribute( "frameBackgroundColorAlpha", mFrameBackgroundColor.alpha() );
  annotationElem.setAttribute( "visible", isVisible() );
  if ( mMarkerSymbol )
  {
    QDomElement symbolElem = QgsSymbolLayerV2Utils::saveSymbol( "marker symbol", mMarkerSymbol, doc );
    if ( !symbolElem.isNull() )
    {
      annotationElem.appendChild( symbolElem );
    }
  }
  itemElem.appendChild( annotationElem );
}

void QgsAnnotationItem::_readXML( const QDomDocument& doc, const QDomElement& annotationElem )
{
  Q_UNUSED( doc );
  if ( annotationElem.isNull() )
  {
    return;
  }
  QPointF pos;
  pos.setX( annotationElem.attribute( "canvasPosX", "0" ).toDouble() );
  pos.setY( annotationElem.attribute( "canvasPosY", "0" ).toDouble() );
  setPos( pos );
  QgsPoint mapPos;
  mapPos.setX( annotationElem.attribute( "mapPosX", "0" ).toDouble() );
  mapPos.setY( annotationElem.attribute( "mapPosY", "0" ).toDouble() );
  mMapPosition = mapPos;
  mFrameBorderWidth = annotationElem.attribute( "frameBorderWidth", "0.5" ).toDouble();
  mFrameColor.setNamedColor( annotationElem.attribute( "frameColor", "#000000" ) );
  mFrameColor.setAlpha( annotationElem.attribute( "frameColorAlpha", "255" ).toInt() );
  mFrameBackgroundColor.setNamedColor( annotationElem.attribute( "frameBackgroundColor" ) );
  mFrameBackgroundColor.setAlpha( annotationElem.attribute( "frameBackgroundColorAlpha", "255" ).toInt() );
  mFrameSize.setWidth( annotationElem.attribute( "frameWidth", "50" ).toDouble() );
  mFrameSize.setHeight( annotationElem.attribute( "frameHeight", "50" ).toDouble() );
  mOffsetFromReferencePoint.setX( annotationElem.attribute( "offsetX", "0" ).toDouble() );
  mOffsetFromReferencePoint.setY( annotationElem.attribute( "offsetY", "0" ).toDouble() );
  mMapPositionFixed = annotationElem.attribute( "mapPositionFixed", "1" ).toInt();
  setVisible( annotationElem.attribute( "visible", "1" ).toInt() );

  //marker symbol
  QDomElement symbolElem = annotationElem.firstChildElement( "symbol" );
  if ( !symbolElem.isNull() )
  {
    QgsMarkerSymbolV2* symbol = QgsSymbolLayerV2Utils::loadSymbol<QgsMarkerSymbolV2>( symbolElem );
    if ( symbol )
    {
      delete mMarkerSymbol;
      mMarkerSymbol = symbol;
    }
  }

  updateBoundingRect();
  updateBalloon();
}
