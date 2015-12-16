/***************************************************************************
 *  qgsmaptoolpin.cpp                                                   *
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

#include "qgsmaptoolpinannotation.h"
#include "qgspinannotationitem.h"
#include "qgsmapcanvas.h"
#include <QMouseEvent>

QgsAnnotationItem* QgsMapToolPinAnnotation::createItem( const QPoint &pos )
{
  QgsPinAnnotationItem* pinItem = new QgsPinAnnotationItem( mCanvas, mCoordinateDisplayer );
  pinItem->setMapPosition( toMapCoordinates( pos ) );
  pinItem->setSelected( true );
  return pinItem;
}
