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
#include <QMenu>
#include <QPainter>
#include <QPen>

QgsAnnotationItem::QgsAnnotationItem( QgsMapCanvas* mapCanvas )
    : QObject( 0 ), QgsMapCanvasItem( mapCanvas )
    , mFlags( ItemAllFeatures )
    , mMapPositionFixed( true )
    , mOffsetFromReferencePoint( QPointF( 50, -50 ) )
    , mBalloonSegment( -1 )
{
  setFlag( QGraphicsItem::ItemIsSelectable, true );
  mMarkerSymbol = new QgsMarkerSymbolV2();
  mFrameBorderWidth = 1.0;
  mFrameColor = QColor( 0, 0, 0 );
  mFrameBackgroundColor = QColor( 255, 255, 255 );
  setData( 0, "AnnotationItem" );

  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( syncGeoPos() ) );
  connect( mMapCanvas, SIGNAL( hasCrsTransformEnabledChanged( bool ) ), this, SLOT( syncGeoPos() ) );
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

void QgsAnnotationItem::setMapPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem& crs )
{
  mMapPosition = mGeoPos = pos;
  if ( crs.isValid() && crs != mMapCanvas->mapSettings().destinationCrs() )
  {
    mGeoPosCrs = crs;
  }
  else
  {
    mGeoPosCrs = mMapCanvas->mapSettings().destinationCrs();
  }
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
  if ( mFlags & ItemHasNoFrame )
  {
    return;
  }
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
  double minEdgeDist = std::numeric_limits<double>::max();
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
  if ( mFlags & ItemHasNoFrame )
  {
    return;
  }
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
  if ( !p || !mMarkerSymbol || ( mFlags & ItemHasNoMarker ) )
  {
    return;
  }

  QgsRenderContext renderContext;
  if ( !setRenderContextVariables( p, renderContext ) )
  {
    return;
  }

  mMarkerSymbol->startRender( renderContext );
  mMarkerSymbol->renderPoint( QPointF( 0, 0 ), 0, renderContext );
  mMarkerSymbol->stopRender( renderContext );
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

  QRectF frameRect( mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y(), mFrameSize.width(), mFrameSize.height() );
  double handlerSize = 10;
  p->setPen( Qt::NoPen );
  p->setBrush( QColor( 200, 200, 210, 120 ) );
  p->drawRect( QRectF( frameRect.left(), frameRect.top(), handlerSize, handlerSize ) );
  p->drawRect( QRectF( frameRect.right() - handlerSize, frameRect.top(), handlerSize, handlerSize ) );
  p->drawRect( QRectF( frameRect.right() - handlerSize, frameRect.bottom() - handlerSize, handlerSize, handlerSize ) );
  p->drawRect( QRectF( frameRect.left(), frameRect.bottom() - handlerSize, handlerSize, handlerSize ) );
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

  if (( mFlags & ItemIsNotResizeable ) == 0 &&
      QRectF( mOffsetFromReferencePoint.x() - cursorSensitivity,
              mOffsetFromReferencePoint.y() - cursorSensitivity,
              mFrameSize.width() + cursorSensitivity,
              mFrameSize.height() + cursorSensitivity ).contains( itemPos )
     )
  {

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
  }

  if (( mFlags & ItemHasNoFrame ) == 0 )
  {
    //finally test if pos is in the frame area
    if ( itemPos.x() >= mOffsetFromReferencePoint.x() && itemPos.x() <= ( mOffsetFromReferencePoint.x() + mFrameSize.width() ) &&
         itemPos.y() >= mOffsetFromReferencePoint.y() && itemPos.y() <= ( mOffsetFromReferencePoint.y() + mFrameSize.height() ) )
    {
      return MoveFramePosition;
    }
  }
  else
  {
    if ( itemPos.x() >= mOffsetFromReferencePoint.x() && itemPos.x() <= ( mOffsetFromReferencePoint.x() + mFrameSize.width() ) &&
         itemPos.y() >= mOffsetFromReferencePoint.y() && itemPos.y() <= ( mOffsetFromReferencePoint.y() + mFrameSize.height() ) )
    {
      return MoveMapPosition;
    }
  }
  return NoAction;
}

void QgsAnnotationItem::handleMoveAction( MouseMoveAction moveAction, const QPointF &newPos, const QPointF &oldPos )
{
  if ( moveAction == QgsAnnotationItem::MoveMapPosition )
  {
    QgsPoint newMapPos = mMapCanvas->getCoordinateTransform()->toMapCoordinates( newPos.toPoint() );
    QgsPoint oldMapPos = mMapCanvas->getCoordinateTransform()->toMapCoordinates( oldPos.toPoint() );
    setMapPosition( mMapPosition + ( newMapPos - oldMapPos ) );
    update();
  }
  else if ( moveAction == QgsAnnotationItem::MoveFramePosition )
  {
    if ( mMapPositionFixed )
    {
      setOffsetFromReferencePoint( mOffsetFromReferencePoint + ( newPos - oldPos ) );
    }
    else
    {
      QPointF newCanvasPos = pos() + ( newPos - oldPos );
      setMapPosition( mMapCanvas->getCoordinateTransform()->toMapCoordinates( newCanvasPos.toPoint() ) );
    }
    update();
  }
  else if ( moveAction != QgsAnnotationItem::NoAction )
  {
    //handle the frame resize actions
    double xmin = mOffsetFromReferencePoint.x();
    double ymin = mOffsetFromReferencePoint.y();
    double xmax = xmin + mFrameSize.width();
    double ymax = ymin + mFrameSize.height();

    if ( moveAction == QgsAnnotationItem::ResizeFrameRight ||
         moveAction == QgsAnnotationItem::ResizeFrameRightDown ||
         moveAction == QgsAnnotationItem::ResizeFrameRightUp )
    {
      xmax += newPos.x() - oldPos.x();
    }
    if ( moveAction == QgsAnnotationItem::ResizeFrameLeft ||
         moveAction == QgsAnnotationItem::ResizeFrameLeftDown ||
         moveAction == QgsAnnotationItem::ResizeFrameLeftUp )
    {
      xmin += newPos.x() - oldPos.x();
    }
    if ( moveAction == QgsAnnotationItem::ResizeFrameUp ||
         moveAction == QgsAnnotationItem::ResizeFrameLeftUp ||
         moveAction == QgsAnnotationItem::ResizeFrameRightUp )
    {
      ymin += newPos.y() - oldPos.y();
    }
    if ( moveAction == QgsAnnotationItem::ResizeFrameDown ||
         moveAction == QgsAnnotationItem::ResizeFrameLeftDown ||
         moveAction == QgsAnnotationItem::ResizeFrameRightDown )
    {
      ymax += newPos.y() - oldPos.y();
    }

    //switch min / max if necessary
    double tmp;
    if ( xmax < xmin )
    {
      tmp = xmax; xmax = xmin; xmin = tmp;
    }
    if ( ymax < ymin )
    {
      tmp = ymax; ymax = ymin; ymin = tmp;
    }

    setOffsetFromReferencePoint( QPointF( xmin, ymin ) );
    setFrameSize( QSizeF( xmax - xmin, ymax - ymin ) );
    update();
  }
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
  annotationElem.setAttribute( "geoPosX", qgsDoubleToString( mGeoPos.x() ) );
  annotationElem.setAttribute( "geoPosY", qgsDoubleToString( mGeoPos.y() ) );
  annotationElem.setAttribute( "mapGeoPosAuthID", mGeoPosCrs.authid() );
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
  annotationElem.setAttribute( "flags", mFlags );
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
  mMapPosition.setX( annotationElem.attribute( "mapPosX", "0" ).toDouble() );
  mMapPosition.setY( annotationElem.attribute( "mapPosY", "0" ).toDouble() );
  mGeoPos.setX( annotationElem.attribute( "geoPosX", qgsDoubleToString( mMapPosition.x() ) ).toDouble() );
  mGeoPos.setY( annotationElem.attribute( "geoPosY", qgsDoubleToString( mMapPosition.y() ) ).toDouble() );
  mGeoPosCrs = QgsCoordinateReferenceSystem( annotationElem.attribute( "mapGeoPosAuthID", mMapCanvas->mapSettings().destinationCrs().authid() ) );
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
  mFlags = annotationElem.attribute( "flags", QString::number( ItemAllFeatures ) ).toInt();
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

  syncGeoPos();
  updateBoundingRect();
  updateBalloon();
}

void QgsAnnotationItem::showItemEditor()
{
  if (( mFlags & ItemIsNotEditable ) == 0 )
  {
    _showItemEditor();
  }
}

void QgsAnnotationItem::showContextMenu( const QPoint &screenPos )
{
  QMenu menu;
  menu.addAction( tr( "Remove" ), this, SLOT( deleteLater() ) );
  menu.exec( screenPos );
}

void QgsAnnotationItem::syncGeoPos()
{
  if ( mMapPositionFixed )
  {
    mMapPosition = QgsCoordinateTransform( mGeoPosCrs, mMapCanvas->mapSettings().destinationCrs() ).transform( mGeoPos );
    updatePosition();
  }
  else
  {
    updatePosition();
    mGeoPos = QgsCoordinateTransform( mMapCanvas->mapSettings().destinationCrs(), mGeoPosCrs ).transform( mMapPosition );
  }
}
