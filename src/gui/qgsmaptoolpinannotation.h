/***************************************************************************
 *  qgsmaptoolpin.h                                                     *
 *  -------------------                                                    *
 *  begin                : Jul 22, 2015                                    *
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

#ifndef QGSMAPTOOLPIN_H
#define QGSMAPTOOLPIN_H

#include "qgsmaptoolannotation.h"

class QgsCoordinateDisplayer;

class GUI_EXPORT QgsMapToolPinAnnotation: public QgsMapToolAnnotation
{
  public:
    QgsMapToolPinAnnotation( QgsMapCanvas* canvas, QgsCoordinateDisplayer* coordinateDisplayer )
        : QgsMapToolAnnotation( canvas ), mCoordinateDisplayer( coordinateDisplayer ) {}

  protected:
    QgsAnnotationItem* createItem( const QPoint &pos ) override;

  private:
    QgsCoordinateDisplayer* mCoordinateDisplayer;
};

#endif // QGSMAPTOOLPIN_H

