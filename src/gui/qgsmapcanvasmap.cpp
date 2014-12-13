/***************************************************************************
    qgsmapcanvasmap.cpp  -  draws the map in map canvas
    ----------------------
    begin                : February 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
#include "qgsmaprenderer.h"
#include "qgsmapsettings.h"
#include "qgsmaplayer.h"

#include <QPainter>

QgsMapCanvasMap::QgsMapCanvasMap( QgsMapCanvas* canvas )
    : QgsMapCanvasItem( canvas )
{
  setZValue( -10 );
}

QgsMapCanvasMap::~QgsMapCanvasMap()
{
}

void QgsMapCanvasMap::setContent( const QImage& image, const QgsRectangle& rect )
{
  mImage = image;

  // For true retro fans: this is approximately how the graphics looked like in 1990
  if ( mMapCanvas->property( "retro" ).toBool() )
    mImage = mImage.scaled( mImage.width() / 3, mImage.height() / 3 )
             .convertToFormat( QImage::Format_Indexed8, Qt::OrderedDither | Qt::OrderedAlphaDither );

  setRect( rect );
}

void QgsMapCanvasMap::paint( QPainter* painter )
{
  int w = qRound( boundingRect().width() ) - 2, h = qRound( boundingRect().height() ) - 2; // setRect() makes the size +2 :-(
  if ( mImage.size() != QSize( w, h ) )
  {
    QgsDebugMsg( QString( "map paint DIFFERENT SIZE: img %1,%2  item %3,%4" ).arg( mImage.width() ).arg( mImage.height() ).arg( w ).arg( h ) );
    // This happens on zoom events when ::paint is called before
    // the renderer has completed
  }

  painter->drawImage( QRect( 0, 0, w, h ), mImage );

  // For debugging:
#if 0
  QRectF br = boundingRect();
  QPointF c = br.center();
  double rad = std::max( br.width(), br.height() ) / 10;
  painter->drawRoundedRect( br, rad, rad );
  painter->drawLine( QLineF( 0, 0, br.width(), br.height() ) );
  painter->drawLine( QLineF( br.width(), 0, 0, br.height() ) );

  double nw = br.width() * 0.5; double nh = br.height() * 0.5;
  br = QRectF( c - QPointF( nw / 2, nh / 2 ), QSize( nw, nh ) );
  painter->drawRoundedRect( br, rad, rad );

  nw = br.width() * 0.5; nh = br.height() * 0.5;
  br = QRectF( c - QPointF( nw / 2, nh / 2 ), QSize( nw, nh ) );
  painter->drawRoundedRect( br, rad, rad );
#endif
}

QPaintDevice& QgsMapCanvasMap::paintDevice()
{
  return mImage;
}
