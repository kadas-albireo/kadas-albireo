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
    : QgsMapTool( canvas )
{
  setCursor( QCursor( Qt::CrossCursor ) );
}

void QgsMapToolAnnotation::canvasReleaseEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( mItem )
      mItem->setSelected( false );
    mItem = createItem( e->pos() );
  }
  else if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void QgsMapToolAnnotation::keyReleaseEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
  }
}

void QgsMapToolAnnotation::deactivate()
{
  QgsMapTool::deactivate();
  if ( mItem )
    mItem->setSelected( false );
}


QgsMapToolEditAnnotation::QgsMapToolEditAnnotation( QgsMapCanvas *canvas, QgsAnnotationItem* item )
    : QgsMapTool( canvas ), mItem( item )
{
  mItem->setSelected( true );
  connect( mItem, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
  mAnnotationMoveAction = QgsAnnotationItem::NoAction;
}

void QgsMapToolEditAnnotation::deactivate()
{
  QgsMapTool::deactivate();
  if ( mItem )
  {
    mItem->setSelected( false );
  }
}

void QgsMapToolEditAnnotation::canvasDoubleClickEvent( QMouseEvent *e )
{
  if ( mItem == mCanvas->annotationItemAtPos( e->pos() ) )
  {
    mAnnotationMoveAction = QgsAnnotationItem::NoAction;
    mItem->showItemEditor();
  }
}

void QgsMapToolEditAnnotation::canvasPressEvent( QMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    mMouseMoveLastXY = e->pos();
    if ( mItem == mCanvas->annotationItemAtPos( e->pos() ) )
    {
      mAnnotationMoveAction = mItem->moveActionForPosition( e->posF() );
    }
  }
  else if ( e->button() == Qt::RightButton )
  {
    // If annotation item is selected, show its context menu
    if ( mItem->hitTest( e->pos() ) )
    {
      mItem->showContextMenu( canvas()->mapToGlobal( e->pos() ) );
    }
  }
}

void QgsMapToolEditAnnotation::canvasMoveEvent( QMouseEvent *e )
{
  if (( e->buttons() & Qt::LeftButton ) )
  {
    if ( mAnnotationMoveAction != QgsAnnotationItem::NoAction )
    {
      mItem->handleMoveAction( mAnnotationMoveAction, e->posF(), mMouseMoveLastXY );
      mMouseMoveLastXY = e->pos();
    }
  }
  else
  {
    int moveAction = mItem->moveActionForPosition( e -> pos() );
    if ( moveAction != QgsAnnotationItem::NoAction )
      mCanvas->setCursor( QCursor( mItem->cursorShapeForAction( moveAction ) ) );
    else
      mCanvas->setCursor( mCursor );
  }
}

void QgsMapToolEditAnnotation::canvasReleaseEvent( QMouseEvent *e )
{
  mAnnotationMoveAction = QgsAnnotationItem::NoAction;
  if ( mItem != mCanvas->annotationItemAtPos( e->pos() ) )
  {
    mItem->setSelected( false );
    deleteLater();
  }
}

void QgsMapToolEditAnnotation::keyReleaseEvent( QKeyEvent *e )
{
  switch ( e->key() )
  {
    case Qt::Key_Escape:
    {
      mItem->setSelected( false );
      deleteLater();
      break;
    }
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    {
      delete mItem.data();
      break;
    }
  }
}
