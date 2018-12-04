/***************************************************************************
                              qgsmaptoolannotation.h
                              -------------------------
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

#ifndef QGSMAPTOOLANNOTATION_H
#define QGSMAPTOOLANNOTATION_H

#include "qgsannotationitem.h"
#include "qgsmaptoolpan.h"

#include <QPointer>

class GUI_EXPORT QgsMapToolAnnotation : public QgsMapTool
{
  public:
    QgsMapToolAnnotation( QgsMapCanvas* canvas );

    void canvasReleaseEvent( QMouseEvent* e ) override;
    void keyReleaseEvent( QKeyEvent* e ) override;
    void deactivate() override;

  protected:
    virtual QgsAnnotationItem* createItem( const QPoint &/*pos*/ ) { return 0; }

  private:
    QPointer<QgsAnnotationItem> mItem;
};


class GUI_EXPORT QgsMapToolEditAnnotation : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsMapToolEditAnnotation( QgsMapCanvas* canvas , QgsAnnotationItem *item );
    ~QgsMapToolEditAnnotation();

    void canvasDoubleClickEvent( QMouseEvent *e ) override;
    void canvasPressEvent( QMouseEvent *e ) override;
    void canvasMoveEvent( QMouseEvent *e ) override;
    void canvasReleaseEvent( QMouseEvent* e ) override;
    void keyReleaseEvent( QKeyEvent* e ) override;

  private:
    QList<QPointer<QgsAnnotationItem>> mItems;
    QPointF mMouseMoveLastXY;
    int mAnnotationMoveAction = QgsAnnotationItem::NoAction;
    bool mDraggingRect = false;

    QGraphicsRectItem* mRectItem = nullptr;

  private slots:
    void convertToWaypoints();
    void deleteAll();
    void updateRect();
};

#endif // QGSMAPTOOLANNOTATION_H
