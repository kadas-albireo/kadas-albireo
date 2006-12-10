/***************************************************************************
    qgsmaptoolselect.cpp  -  map tool for selecting features
    ----------------------
    begin                : January 2006
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
/* $Id$ */

#include "qgsmaptoolselect.h"
#include "qgsmapcanvas.h"
#include "qgsmaptopixel.h"
#include "qgsmaplayer.h"
#include "qgscursors.h"
#include <QMessageBox>
#include <QMouseEvent>
#include <QRubberBand>
#include <QRect>


QgsMapToolSelect::QgsMapToolSelect(QgsMapCanvas* canvas)
  : QgsMapTool(canvas), mDragging(false)
{
  QPixmap mySelectQPixmap = QPixmap((const char **) select_cursor);
  mCursor = QCursor(mySelectQPixmap, 1, 1);
}


void QgsMapToolSelect::canvasPressEvent(QMouseEvent * e)
{
  mSelectRect.setRect(0, 0, 0, 0);
}


void QgsMapToolSelect::canvasMoveEvent(QMouseEvent * e)
{
  if ( e->buttons() != Qt::LeftButton )
    return;
  
  if (!mDragging)
  {
    mDragging = TRUE;
    mRubberBand = new QRubberBand(QRubberBand::Rectangle, mCanvas);
    mSelectRect.setTopLeft(e->pos());
  }
  
  mSelectRect.setBottomRight(e->pos());
  mRubberBand->setGeometry(mSelectRect.normalized());
  mRubberBand->show();
}


void QgsMapToolSelect::canvasReleaseEvent(QMouseEvent * e)
{
  if (!mDragging)
    return;
  
  mDragging = FALSE;
  
  delete mRubberBand;
  mRubberBand = 0;

  if (!mCanvas->currentLayer())
  {
    QMessageBox::warning(mCanvas, QObject::tr("No active layer"),
       QObject::tr("To select features, you must choose an layer active by clicking on its name in the legend"));
    return;
  }
    
  // store the rectangle
  mSelectRect.setRight(e->pos().x());
  mSelectRect.setBottom(e->pos().y());

  QgsMapToPixel* transform = mCanvas->getCoordinateTransform();
  QgsPoint ll = transform->toMapCoordinates(mSelectRect.left(), mSelectRect.bottom());
  QgsPoint ur = transform->toMapCoordinates(mSelectRect.right(), mSelectRect.top());

  QgsRect search(ll.x(), ll.y(), ur.x(), ur.y());

  // if Ctrl key is pressed, selected features will be added to selection
  // instead of removing old selection
  bool lock = (e->modifiers() & Qt::ControlModifier);
  mCanvas->currentLayer()->select(&search, lock);

}
