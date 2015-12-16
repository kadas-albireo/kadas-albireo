/***************************************************************************
                              qgspinannotationitem.h
                              ------------------------
  begin                : August, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPINANNOTATIONITEM_H
#define QGSPINANNOTATIONITEM_H

#include "qgssvgannotationitem.h"

class QgsCoordinateDisplayer;

class GUI_EXPORT QgsPinAnnotationItem: public QgsSvgAnnotationItem
{
    Q_OBJECT
  public:

    QgsPinAnnotationItem( QgsMapCanvas* canvas, QgsCoordinateDisplayer* coordinateDisplayer );

    void setMapPosition( const QgsPoint& pos ) override;
    void showContextMenu( const QPoint& screenPos );

  private:
    QgsCoordinateDisplayer* mCoordinateDisplayer;


  private slots:
    void copyPosition();
    void updateToolTip();
};

#endif // QGSPINANNOTATIONITEM_H
