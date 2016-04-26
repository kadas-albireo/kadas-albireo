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

#include "qgsmaptoolpan.h"


class QgsAnnotationItem;

class GUI_EXPORT QgsMapToolAnnotation : public QgsMapToolPan
{
  public:
    QgsMapToolAnnotation( QgsMapCanvas* canvas );

    void canvasReleaseEvent( QMouseEvent* e ) override;
    void keyReleaseEvent( QKeyEvent* e );

  protected:
    virtual QgsAnnotationItem* createItem( const QPoint &/*pos*/ ) { return 0; }
};

#endif // QGSMAPTOOLANNOTATION_H
