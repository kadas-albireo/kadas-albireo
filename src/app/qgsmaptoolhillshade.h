/***************************************************************************
 *  qgsmaptoolhillshade.h                                                  *
 *  -------------------                                                    *
 *  begin                : Nov 15, 2015                                    *
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

#ifndef QGSMAPTOOLHILLSHADE_H
#define QGSMAPTOOLHILLSHADE_H

#include "qgsmaptooldrawshape.h"

class APP_EXPORT QgsMapToolHillshade : public QgsMapToolDrawRectangle
{
    Q_OBJECT
  public:
    QgsMapToolHillshade( QgsMapCanvas* mapCanvas );
    void activate();

  private slots:
    void drawFinished();
};

#endif // QGSMAPTOOLHILLSHADE_H
