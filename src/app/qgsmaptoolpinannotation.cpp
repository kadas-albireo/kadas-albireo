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

#include "qgisapp.h"
#include "qgscoordinateutils.h"
#include "qgsmaptoolpinannotation.h"
#include "qgspinannotationitem.h"

QgsAnnotationItem* QgsMapToolPinAnnotation::createItem( const QPoint &pos )
{
  QgsCoordinateUtils::TargetFormat format;
  QString epsg;
  QgisApp::instance()->getCoordinateDisplayFormat(format, epsg);
  QgsPinAnnotationItem* pinItem = new QgsPinAnnotationItem( mCanvas, format, epsg );
  connect(QgisApp::instance(), SIGNAL(coordinateDisplayFormatChanged(QgsCoordinateUtils::TargetFormat&,QString&)), pinItem, SLOT(changeCoordinateFormatter(QgsCoordinateUtils::TargetFormat,QString)));
  pinItem->setMapPosition( toMapCoordinates( pos ) );
  pinItem->setSelected( true );
  return pinItem;
}
