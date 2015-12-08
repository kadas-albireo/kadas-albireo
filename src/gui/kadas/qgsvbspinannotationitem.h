/***************************************************************************
                              qgsvbspinannotationitem.h
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

#ifndef QGSVBSPINANNOTATIONITEM_H
#define QGSVBSPINANNOTATIONITEM_H

#include "qgssvgannotationitem.h"

class QgsVBSCoordinateDisplayer;

class GUI_EXPORT QgsVBSPinAnnotationItem: public QgsSvgAnnotationItem
{
    Q_OBJECT
  public:

    QgsVBSPinAnnotationItem( QgsMapCanvas* canvas, QgsVBSCoordinateDisplayer* coordinateDisplayer );

    void setMapPosition( const QgsPoint& pos ) override;
    void showContextMenu( const QPoint& screenPos );

  private:
    QgsVBSCoordinateDisplayer* mCoordinateDisplayer;


  private slots:
    void copyPosition();
    void updateToolTip();
};

#endif // QGSVBSPINANNOTATIONITEM_H
