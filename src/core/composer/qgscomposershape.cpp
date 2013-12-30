/***************************************************************************
                         qgscomposershape.cpp
                         ----------------------
    begin                : November 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco@hugis.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposershape.h"
#include <QPainter>

QgsComposerShape::QgsComposerShape( QgsComposition* composition ): QgsComposerItem( composition ), mShape( Ellipse ), mCornerRadius( 0 )
{
  setFrameEnabled( true );
}

QgsComposerShape::QgsComposerShape( qreal x, qreal y, qreal width, qreal height, QgsComposition* composition ): QgsComposerItem( x, y, width, height, composition ),
    mShape( Ellipse ),
    mCornerRadius( 0 )
{
  setSceneRect( QRectF( x, y, width, height ) );
  setFrameEnabled( true );
}

QgsComposerShape::~QgsComposerShape()
{

}

void QgsComposerShape::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  Q_UNUSED( itemStyle );
  Q_UNUSED( pWidget );
  if ( !painter )
  {
    return;
  }
  drawBackground( painter );
  drawFrame( painter );

  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}


void QgsComposerShape::drawShape( QPainter* p )
{

  p->save();
  p->setRenderHint( QPainter::Antialiasing );

  switch ( mShape )
  {
    case Ellipse:
      p->drawEllipse( QRectF( 0, 0 , rect().width(), rect().height() ) );
      break;
    case Rectangle:
      //if corner radius set, then draw a rounded rectangle
      if ( mCornerRadius > 0 )
      {
        p->drawRoundedRect( QRectF( 0, 0 , rect().width(), rect().height() ), mCornerRadius, mCornerRadius );
      }
      else
      {
        p->drawRect( QRectF( 0, 0 , rect().width(), rect().height() ) );
      }
      break;
    case Triangle:
      QPolygonF triangle;
      triangle << QPointF( 0, rect().height() );
      triangle << QPointF( rect().width() , rect().height() );
      triangle << QPointF( rect().width() / 2.0, 0 );
      p->drawPolygon( triangle );
      break;
  }
  p->restore();

}


void QgsComposerShape::drawFrame( QPainter* p )
{
  if ( mFrame && p )
  {
    p->setPen( pen() );
    p->setBrush( Qt::NoBrush );
    p->setRenderHint( QPainter::Antialiasing, true );
    drawShape( p );
  }
}

void QgsComposerShape::drawBackground( QPainter* p )
{
  if ( mBackground && p )
  {
    p->setBrush( brush() );//this causes a problem in atlas generation
    p->setPen( Qt::NoPen );
    p->setRenderHint( QPainter::Antialiasing, true );
    drawShape( p );
  }
}


bool QgsComposerShape::writeXML( QDomElement& elem, QDomDocument & doc ) const
{
  QDomElement composerShapeElem = doc.createElement( "ComposerShape" );
  composerShapeElem.setAttribute( "shapeType", mShape );
  composerShapeElem.setAttribute( "cornerRadius", mCornerRadius );
  elem.appendChild( composerShapeElem );
  return _writeXML( composerShapeElem, doc );
}

bool QgsComposerShape::readXML( const QDomElement& itemElem, const QDomDocument& doc )
{
  mShape = QgsComposerShape::Shape( itemElem.attribute( "shapeType", "0" ).toInt() );
  mCornerRadius = itemElem.attribute( "cornerRadius", "0" ).toDouble();

  //restore general composer item properties
  QDomNodeList composerItemList = itemElem.elementsByTagName( "ComposerItem" );
  if ( composerItemList.size() > 0 )
  {
    QDomElement composerItemElem = composerItemList.at( 0 ).toElement();

    //rotation
    if ( composerItemElem.attribute( "rotation", "0" ).toDouble() != 0 )
    {
      //check for old (pre 2.1) rotation attribute
      setItemRotation( composerItemElem.attribute( "rotation", "0" ).toDouble() );
    }

    _readXML( composerItemElem, doc );
  }
  emit itemChanged();
  return true;
}

void QgsComposerShape::setCornerRadius( double radius )
{
  mCornerRadius = radius;
}
