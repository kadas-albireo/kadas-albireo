/***************************************************************************
 *  qgsvbsspinner.cpp                                                      *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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

#include "qgsvbsspinner.h"
#include <QPainter>

QgsVBSSpinner::QgsVBSSpinner( QWidget *parent )
    : QWidget( parent )
{
  mTicks = 12;
  mCounter = 0;
  mTimer = 0;
  setMinimumSize( 16, 16 );
}

void QgsVBSSpinner::setVisible( bool visible )
{
  QWidget::setVisible( visible );
  if ( visible )
  {
    mTimer = startTimer( 1000 / mTicks );
    mCounter = 0;
  }
  else
  {
    if ( mTimer != 0 )
    {
      killTimer( mTimer );
      mTimer = 0;
    }
    mCounter = 0;
  }
}

void QgsVBSSpinner::timerEvent( QTimerEvent * )
{
  mCounter = ( mCounter + 1 ) % mTicks;
  update();
}

void QgsVBSSpinner::paintEvent( QPaintEvent * )
{
  QSize sz = size();
  double l = 0.5 * std::min( sz.width(), sz.height() );

  QPainter painter;
  painter.setPen( Qt::NoPen );
  painter.setRenderHint( QPainter::Antialiasing, true );
  for ( int i = 0; i < mTicks; ++i )
  {
    painter.save();
    painter.translate( 0.5 * sz.width(), 0.5 * sz.height() );
    painter.rotate(( 360. * i ) / mTicks );
    painter.translate( 0.4 * l, 0 );
    float k = float(( mTicks + ( i - mCounter ) ) % mTicks ) / mTicks;
    painter.setBrush( QColor( 0, 0, 0, 255 * ( 0.9 * k + 0.1 ) ) );
    painter.drawRoundedRect(
      QRect( 0, -0.1 * l, 0.6 * l, 0.2 * l ), 0.15 * l, 0.1 * l );
    painter.restore();
  }
}
