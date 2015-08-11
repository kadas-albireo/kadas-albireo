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

class QgsVBSPinAnnotationItem: public QgsSvgAnnotationItem
{
  public:

    QgsVBSPinAnnotationItem( QgsMapCanvas* canvas );

    void setMapPosition( const QgsPoint& pos ) override;
    double getHeightAtCurrentPos();
};

#endif // QGSVBSPINANNOTATIONITEM_H
