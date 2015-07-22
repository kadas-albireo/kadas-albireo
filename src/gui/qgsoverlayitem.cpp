/***************************************************************************
                              qgsoverlayitem.cpp
                              ------------------
  begin                : July 21, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsoverlayitem.h"
#include "qgsmapcanvas.h"
#include "qgsmapsettings.h"
#include "qgscoordinatetransform.h"

QgsOverlayItem::QgsOverlayItem( QgsMapCanvas *mapCanvas )
    : QgsMapCanvasItem( mapCanvas )
{
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( updateCanvasPosition() ) );
  connect( mMapCanvas, SIGNAL( hasCrsTransformEnabledChanged( bool ) ), this, SLOT( updateCanvasPosition() ) );
}

void QgsOverlayItem::writeXML( QDomDocument& doc, QDomElement& parentItem ) const
{
  QDomElement overlayElem = doc.createElement( "OverlayItem" );
  overlayElem.setAttribute( "posX", qgsDoubleToString( mPos.x() ) );
  overlayElem.setAttribute( "posY", qgsDoubleToString( mPos.y() ) );
  overlayElem.setAttribute( "authid", mPosCrs.authid() );
  parentItem.appendChild( overlayElem );
}

void QgsOverlayItem::readXML( const QDomElement& parentItem )
{
  QDomElement overlayElem = parentItem.firstChildElement( "Overlayitem" );
  if ( overlayElem.isNull() )
  {
    return;
  }

  mPos.setX( overlayElem.attribute( "posX", "0" ).toDouble() );
  mPos.setY( overlayElem.attribute( "posY", "0" ).toDouble() );
  mPosCrs = QgsCoordinateReferenceSystem( overlayElem.attribute( "authid", "EPSG:4326" ) );

  updateBoundingRect();
}

void QgsOverlayItem::updateCanvasPosition()
{
  const QgsCoordinateReferenceSystem& destCrs = mMapCanvas->mapSettings().destinationCrs();
  QgsPoint destPos = QgsCoordinateTransform( mPosCrs, destCrs ).transform( mPos );
  mCanvasPos = mMapCanvas->mapSettings().mapToPixel().transform( destPos ).toQPointF();
}
