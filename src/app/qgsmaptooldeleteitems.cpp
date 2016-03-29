/***************************************************************************
 *  qgsmaptooldeleteitems.cpp                                              *
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

#include "qgsmapcanvas.h"
#include "qgsmaptooldeleteitems.h"
#include "qgsannotationitem.h"
#include "qgscrscache.h"
#include <QMessageBox>


QgsMapToolDeleteItems::QgsMapToolDeleteItems( QgsMapCanvas* mapCanvas )
    : QgsMapToolDrawRectangle( mapCanvas )
{
  connect( this, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
}

void QgsMapToolDeleteItems::drawFinished()
{
  QgsPoint p1, p2;
  getPart( 0, p1, p2 );
  QgsRectangle rect( p1, p2 );
  rect.normalize();
  if ( rect.isEmpty() )
  {
    reset();
    return;
  }
  QList<QgsAnnotationItem*> delAnnotationItems;
  QgsCoordinateReferenceSystem rectCrs = canvas()->mapSettings().destinationCrs();
  foreach ( QGraphicsItem* item, canvas()->scene()->items() )
  {
    QgsAnnotationItem* aitem = dynamic_cast<QgsAnnotationItem*>( item );
    if ( aitem && aitem->mapPositionFixed() )
    {
      QgsPoint p = QgsCoordinateTransformCache::instance()->transform( aitem->mapGeoPosCrs().authid(), rectCrs.authid() )->transform( aitem->mapGeoPos() );
      if ( rect.contains( p ) )
      {
        delAnnotationItems.append( aitem );
      }
    }
  }

  int delCount = delAnnotationItems.size();

  if ( delCount > 0 )
  {
    int reply = QMessageBox::question( canvas(), tr( "Delete items?" ), tr( "Do you want do delete the %1 selected items?" ).arg( delCount ), QMessageBox::Yes, QMessageBox::No );
    if ( reply == QMessageBox::Yes )
    {
      qDeleteAll( delAnnotationItems );
    }
  }

  reset();
}
