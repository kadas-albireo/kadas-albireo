/***************************************************************************
                              qgsmaptoolannotation.cpp
                              ----------------------------
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

#include "qgsmaptoolannotation.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgstextannotationitem.h"
#include <QDialog>
#include <QMouseEvent>

QgsMapToolAnnotation::QgsMapToolAnnotation( QgsMapCanvas* canvas )
    : QgsMapToolPan( canvas )
{
  setCursor( QCursor( Qt::CrossCursor ) );
}

void QgsMapToolAnnotation::canvasReleaseEvent( QMouseEvent * e )
{
  QgsAnnotationItem* itemAtPos = mCanvas->annotationItemAtPos( e->pos() );
  if ( !mDragging && e->button() == Qt::LeftButton && !itemAtPos )
  {
    QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
    if ( selectedItem )
      selectedItem->setSelected( false );
    createItem( e->pos() );
  }
  else
  {
    QgsMapToolPan::canvasReleaseEvent( e );
  }
}

void QgsMapToolAnnotation::keyReleaseEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this ); // unset
  }
  else
  {
    QgsMapToolPan::keyReleaseEvent( e );
  }
}
