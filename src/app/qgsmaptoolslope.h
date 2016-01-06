/***************************************************************************
 *  qgsmaptoolslope.h                                                      *
 *  -------------------                                                    *
 *  begin                : Nov 11, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSMAPTOOLSLOPE_H
#define QGSMAPTOOLSLOPE_H

#include "qgsmaptooldrawshape.h"

class APP_EXPORT QgsMapToolSlope : public QgsMapToolDrawRectangle
{
    Q_OBJECT
  public:
    QgsMapToolSlope(QgsMapCanvas* mapCanvas);

  private slots:
    void drawFinished();
};

#endif // QGSMAPTOOLSLOPE_H
