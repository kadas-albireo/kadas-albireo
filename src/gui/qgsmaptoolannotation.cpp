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
#include "qgscrscache.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsredlininglayer.h"
#include "qgspinannotationitem.h"
#include "qgstextannotationitem.h"
#include <QDialog>
#include <QMenu>
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
    : QgsMapTool( canvas )
{
  item->setSelected( true );
  mItems.append( item );
  connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
  connect( canvas, SIGNAL( renderComplete( QPainter* ) ), this, SLOT( updateRect() ) );
  mAnnotationMoveAction = QgsAnnotationItem::NoAction;
}

QgsMapToolEditAnnotation::~QgsMapToolEditAnnotation()
{
for ( const auto& item : mItems )
  {
    if ( item )
    {
      item->setSelected( false );
    }
  }
  delete mRectItem;
}

void QgsMapToolEditAnnotation::canvasDoubleClickEvent( QMouseEvent *e )
{
  if ( mItems.size() == 1 && mItems.front().data() == mCanvas->annotationItemAtPos( e->pos() ) )
  {
    mItems.front()->showItemEditor();
  }
}

void QgsMapToolEditAnnotation::canvasPressEvent( QMouseEvent *e )
{
  int nItems = mItems.size();
  if ( e->button() == Qt::LeftButton && ( e->modifiers() & Qt::ControlModifier ) == 0 )
  {
    mMouseMoveLastXY = e->pos();
    if ( nItems == 1 )
    {
      mAnnotationMoveAction = mItems.front()->moveActionForPosition( e->pos() );
    }
    else if ( nItems > 1 && mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      mDraggingRect = true;
      mCanvas->setCursor( Qt::SizeAllCursor );
    }
  }
  else if ( e->button() == Qt::RightButton )
  {
    // If annotation item is selected, show its context menu
    if ( nItems == 1 && mItems.front()->hitTest( e->pos() ) )
    {
      mItems.front()->showContextMenu( canvas()->mapToGlobal( e->pos() ) );
    }
    else if ( nItems > 1 && mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      QMenu menu;
      // If all items are pins, allow converting to waypoints
      bool allPins = true;
    for ( QgsAnnotationItem* item : mItems )
      {
        if ( !dynamic_cast<QgsPinAnnotationItem*>( item ) )
        {
          allPins = false;
          break;
        }
      }
      if ( allPins )
      {
        menu.addAction( QIcon( ":/images/themes/default/mIconPointLayer.svg" ), tr( "Convert to waypoints" ), this, SLOT( convertToWaypoints() ) );
      }
      menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteAll() ) );
      menu.exec( e->globalPos() );
    }
  }
}

void QgsMapToolEditAnnotation::canvasMoveEvent( QMouseEvent *e )
{
  int nItems = mItems.size();
  if ( e->buttons() & Qt::LeftButton )
  {
    if ( nItems == 1 && mAnnotationMoveAction != QgsAnnotationItem::NoAction )
    {
      mItems.front()->handleMoveAction( mAnnotationMoveAction, e->posF(), mMouseMoveLastXY );
      mMouseMoveLastXY = e->pos();
    }
    else if ( nItems > 1 && mDraggingRect )
    {
    for ( QgsAnnotationItem* item : mItems )
      {
        if (( item->itemFlags() & QgsAnnotationItem::ItemMapPositionLocked ) == 0 )
        {
          item->handleMoveAction( QgsAnnotationItem::MoveMapPosition, e->posF(), mMouseMoveLastXY );
        }
      }
      mMouseMoveLastXY = e->pos();
      updateRect();
    }
  }
  else if ( nItems == 1 )
  {
    int moveAction = mItems.front()->moveActionForPosition( e->pos() );
    if ( moveAction != QgsAnnotationItem::NoAction )
      mCanvas->setCursor( QCursor( mItems.front()->cursorShapeForAction( moveAction ) ) );
    else
      mCanvas->setCursor( mCursor );
  }
}

void QgsMapToolEditAnnotation::canvasReleaseEvent( QMouseEvent *e )
{
  if ( mAnnotationMoveAction != QgsAnnotationItem::NoAction || mDraggingRect )
  {
    mAnnotationMoveAction = QgsAnnotationItem::NoAction;
    if ( mDraggingRect )
    {
      mCanvas->setCursor( mCursor );
    }
    mDraggingRect = false;
  }
  else if (( e->modifiers() & Qt::ControlModifier ) == 0 )
  {
    if (
      ( mItems.size() == 1 && mItems.front() != mCanvas->annotationItemAtPos( e->pos() ) )
      ||
      ( mRectItem && !mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    )
    {
      deleteLater();
    }
  }
  else if ( QgsAnnotationItem* clickedItem = mCanvas->annotationItemAtPos( e->pos() ) )
  {
    int pos = 0, n = mItems.size();
    for ( ; pos < n && mItems[pos].data() != clickedItem; ++pos );
    if ( pos >= n )   // Not already in list
    {
      if ( mItems.size() == 1 )
      {
        disconnect( mItems.front().data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
        mRectItem = new QGraphicsRectItem();
        mRectItem->setPen( QPen( Qt::black, 2, Qt::DashLine ) );
        mCanvas->scene()->addItem( mRectItem );
      }
      mItems.append( QPointer<QgsAnnotationItem>( clickedItem ) );
      clickedItem->setSelected( true );
    }
    else if ( mItems.size() > 1 )
    {
      mItems[pos]->setSelected( false );
      mItems.removeAt( pos );
      if ( mItems.size() == 1 )
      {
        connect( mItems.front().data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
        delete mRectItem;
        mRectItem = nullptr;
      }
    }
    updateRect();
  }
}

void QgsMapToolEditAnnotation::keyReleaseEvent( QKeyEvent *e )
{
  switch ( e->key() )
  {
    case Qt::Key_Escape:
    {
      deleteLater();
      break;
    }
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    {
      deleteAll();
      break;
    }
  }
}

void QgsMapToolEditAnnotation::convertToWaypoints()
{
  QgsRedliningLayer* gpsLayer = QgsMapLayerRegistry::instance()->getOrCreateGpsRoutesLayer();
for ( QgsAnnotationItem* item : mItems )
  {
    QgsPoint wgsPos = QgsCoordinateTransformCache::instance()->transform( item->mapGeoPosCrs().authid(), "EPSG:4326" )->transform( item->mapGeoPos() );
    QString name = item->getName();
    QString flags( "shape=point,symbol=circle,r=0,bold=1" );
    gpsLayer->addShape( new QgsGeometry( new QgsPointV2( wgsPos ) ), Qt::yellow, Qt::yellow, 2, Qt::SolidLine, Qt::SolidPattern, flags, QString(), name );
  }
  deleteAll();
  gpsLayer->triggerRepaint();
}

void QgsMapToolEditAnnotation::deleteAll()
{
for ( const auto& item : mItems )
  {
    delete item.data();
  }
  deleteLater();
}

void QgsMapToolEditAnnotation::updateRect()
{
  if ( mItems.size() < 2 )
  {
    return;
  }
  QRectF rect = mItems.front()->screenBoundingRect();
  for ( int i = 1, n = mItems.size(); i < n; ++i )
  {
    rect = rect.unite( mItems[i]->screenBoundingRect() );
  }
  mRectItem->setPos( 0, 0 );
  mRectItem->setRect( rect );
}
