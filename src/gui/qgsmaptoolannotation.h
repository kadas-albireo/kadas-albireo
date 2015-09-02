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

#include "qgsmaptool.h"
#include "qgsannotationitem.h"

class GUI_EXPORT QgsMapToolAnnotation: public QgsMapTool
{
  public:
    QgsMapToolAnnotation( QgsMapCanvas* canvas );

    void canvasPressEvent( QMouseEvent * e ) override;

  protected:
    virtual QgsAnnotationItem* createItem( QMouseEvent* ) { return 0; }
};

#endif // QGSMAPTOOLANNOTATION_H
