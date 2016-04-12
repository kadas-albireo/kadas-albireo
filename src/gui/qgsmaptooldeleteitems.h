/***************************************************************************
 *  qgsmaptooldeleteitems.h                                                *
 *  -------------------                                                    *
 *  begin                : March, 2016                                     *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLDELETEITEMS_H
#define QGSMAPTOOLDELETEITEMS_H

#include "qgsmaptooldrawshape.h"

class GUI_EXPORT QgsMapToolDeleteItems : public QgsMapToolDrawRectangle
{
    Q_OBJECT
  public:
    QgsMapToolDeleteItems( QgsMapCanvas* mapCanvas );
    void deleteItems(const QgsRectangle& filterRect, const QgsCoordinateReferenceSystem& filterRectCrs);

  private slots:
    void drawFinished();
};

#endif // QGSMAPTOOLDELETEITEMS_H
