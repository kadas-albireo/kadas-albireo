/***************************************************************************
                         qgspaperitem.cpp
                       -------------------
    begin                : September 2008
    copyright            : (C) 2008 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgspaperitem.h"
#include "qgscomposition.h"
#include "qgslogger.h"
#include <QGraphicsRectItem>
#include <QPainter>

//QgsPaperGrid

QgsPaperGrid::QgsPaperGrid( double x, double y, double width, double height, QgsComposition* composition ): QGraphicsRectItem( 0, 0, width, height ), mComposition( composition )
{
  setFlag( QGraphicsItem::ItemIsSelectable, false );
  setFlag( QGraphicsItem::ItemIsMovable, false );
  setZValue( 1000 );
  setPos( x, y );
}

QgsPaperGrid::~QgsPaperGrid()
{
}

void QgsPaperGrid::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  Q_UNUSED( itemStyle );
  Q_UNUSED( pWidget );

  //draw grid
  if ( mComposition )
  {
    if ( mComposition->gridVisible() && mComposition->plotStyle() ==  QgsComposition::Preview
         && mComposition->snapGridResolution() > 0 )
    {
      int gridMultiplyX = ( int )( mComposition->snapGridOffsetX() / mComposition->snapGridResolution() );
      int gridMultiplyY = ( int )( mComposition->snapGridOffsetY() / mComposition->snapGridResolution() );
      double currentXCoord = mComposition->snapGridOffsetX() - gridMultiplyX * mComposition->snapGridResolution();
      double currentYCoord;
      double minYCoord = mComposition->snapGridOffsetY() - gridMultiplyY * mComposition->snapGridResolution();

      painter->save();
      //turn of antialiasing so grid is nice and sharp
      painter->setRenderHint( QPainter::Antialiasing, false );

      if ( mComposition->gridStyle() == QgsComposition::Solid )
      {
        painter->setPen( mComposition->gridPen() );

        //draw vertical lines
        for ( ; currentXCoord <= rect().width(); currentXCoord += mComposition->snapGridResolution() )
        {
          painter->drawLine( QPointF( currentXCoord, 0 ), QPointF( currentXCoord, rect().height() ) );
        }

        //draw horizontal lines
        currentYCoord = minYCoord;
        for ( ; currentYCoord <= rect().height(); currentYCoord += mComposition->snapGridResolution() )
        {
          painter->drawLine( QPointF( 0, currentYCoord ), QPointF( rect().width(), currentYCoord ) );
        }
      }
      else //'Dots' or 'Crosses'
      {
        QPen gridPen = mComposition->gridPen();
        painter->setPen( gridPen );
        painter->setBrush( QBrush( gridPen.color() ) );
        double halfCrossLength = 1;
        if ( mComposition->gridStyle() == QgsComposition::Dots )
        {
          //dots are actually drawn as tiny crosses a few pixels across
          //check QGraphicsView to get current transform
          if ( scene() )
          {
            QList<QGraphicsView*> viewList = scene()->views();
            if ( viewList.size() > 0 )
            {
              QGraphicsView* currentView = viewList.at( 0 );
              if ( currentView->isVisible() )
              {
                //set halfCrossLength to equivalent of 1 pixel
                halfCrossLength = 1 / currentView->transform().m11();
              }
            }
          }
        }
        else if ( mComposition->gridStyle() == QgsComposition::Crosses )
        {
          halfCrossLength = mComposition->snapGridResolution() / 6;
        }

        for ( ; currentXCoord <= rect().width(); currentXCoord += mComposition->snapGridResolution() )
        {
          currentYCoord = minYCoord;
          for ( ; currentYCoord <= rect().height(); currentYCoord += mComposition->snapGridResolution() )
          {
            painter->drawLine( QPointF( currentXCoord - halfCrossLength, currentYCoord ), QPointF( currentXCoord + halfCrossLength, currentYCoord ) );
            painter->drawLine( QPointF( currentXCoord, currentYCoord - halfCrossLength ), QPointF( currentXCoord, currentYCoord + halfCrossLength ) );
          }
        }
      }
      painter->restore();
    }
  }
}


//QgsPaperItem

QgsPaperItem::QgsPaperItem( QgsComposition* c ): QgsComposerItem( c, false ),
    mPageGrid( 0 )
{
  initialize();
}

QgsPaperItem::QgsPaperItem( qreal x, qreal y, qreal width, qreal height, QgsComposition* composition ): QgsComposerItem( x, y, width, height, composition, false ),
    mPageGrid( 0 )
{
  initialize();
}

QgsPaperItem::QgsPaperItem(): QgsComposerItem( 0, false ),
    mPageGrid( 0 )
{
  initialize();
}

QgsPaperItem::~QgsPaperItem()
{
  delete mPageGrid;
}

void QgsPaperItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  Q_UNUSED( itemStyle );
  Q_UNUSED( pWidget );
  if ( !painter )
  {
    return;
  }

  drawBackground( painter );
}

bool QgsPaperItem::writeXML( QDomElement& elem, QDomDocument & doc ) const
{
  Q_UNUSED( elem );
  Q_UNUSED( doc );
  return true;
}

bool QgsPaperItem::readXML( const QDomElement& itemElem, const QDomDocument& doc )
{
  Q_UNUSED( itemElem );
  Q_UNUSED( doc );
  return true;
}

void QgsPaperItem::setSceneRect( const QRectF& rectangle )
{
  QgsComposerItem::setSceneRect( rectangle );
  //update size and position of attached QgsPaperGrid to reflect new page size and position
  mPageGrid->setRect( 0, 0, rect().width(), rect().height() );
  mPageGrid->setPos( transform().dx(), transform().dy() );
}

void QgsPaperItem::initialize()
{
  setFlag( QGraphicsItem::ItemIsSelectable, false );
  setFlag( QGraphicsItem::ItemIsMovable, false );
  setZValue( 0 );

  //create a new QgsPaperGrid for this page, and add it to the composition
  mPageGrid = new QgsPaperGrid( transform().dx(), transform().dy(), rect().width(), rect().height(), mComposition );
  mComposition->addItem( mPageGrid );
}
