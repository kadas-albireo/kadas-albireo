/***************************************************************************
    qgsbottombar.cpp
    ------------------
    begin                : May 2017
    copyright            : (C) 2017 Sandro Mani
    email                : smani@sourcepole.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsbottombar.h"
#include "qgsmapcanvas.h"

QgsBottomBar::QgsBottomBar( QgsMapCanvas* canvas, const QString& color )
    : QFrame( canvas )
    , mCanvas( canvas )
{
  mCanvas->installEventFilter( this );

  setObjectName( "QgsBottomBar" );
  setStyleSheet( QString( "QFrame#QgsBottomBar { background-color: %1; }" ).arg( color ) );
  setCursor( Qt::ArrowCursor );
}

bool QgsBottomBar::eventFilter( QObject *obj, QEvent *event )
{
  if ( obj == mCanvas && event->type() == QEvent::Resize )
  {
    updatePosition();
  }
  return QObject::eventFilter( obj, event );
}

void QgsBottomBar::updatePosition()
{
  int w = width();
  int h = height();
  QRect canvasGeometry = mCanvas->geometry();
  move( canvasGeometry.x() + 0.5 * ( canvasGeometry.width() - w ),
        canvasGeometry.y() + canvasGeometry.height() - h );
}
