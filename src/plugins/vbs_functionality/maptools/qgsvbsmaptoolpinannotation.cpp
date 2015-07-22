/***************************************************************************
 *  qgsvbsmaptoolpin.cpp                                                   *
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

#include "qgsvbsmaptoolpinannotation.h"
#include "qgssvgannotationitem.h"
#include <QMouseEvent>
#include <QImageReader>

QgsAnnotationItem* QgsVBSMapToolPinAnnotation::createItem( QMouseEvent* e )
{
  QgsSvgAnnotationItem* svgItem = new QgsSvgAnnotationItem( mCanvas );
  svgItem->setMapPosition( toMapCoordinates( e->pos() ) );
  svgItem->setItemFlags( QgsAnnotationItem::ItemIsNotResizeable |
                         QgsAnnotationItem::ItemHasNoFrame |
                         QgsAnnotationItem::ItemHasNoMarker |
                         QgsAnnotationItem::ItemIsNotEditable );
  QSize imageSize = QImageReader( ":/vbsfunctionality/icons/pin.svg" ).size();
  svgItem->setSelected( true );
  svgItem->setFilePath( ":/vbsfunctionality/icons/pin.svg" );
  svgItem->setFrameSize( imageSize );
  svgItem->setOffsetFromReferencePoint( QPointF( 0, -imageSize.height() ) );
  return svgItem;
}
