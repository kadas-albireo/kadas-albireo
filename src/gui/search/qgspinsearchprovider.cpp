/***************************************************************************
 *  qgspinsearchprovider.cpp                                               *
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

#include "qgspinsearchprovider.h"
#include "qgscoordinatetransform.h"
#include "qgsmapcanvas.h"
#include "qgspinannotationitem.h"
#include <QGraphicsItem>

const QString QgsPinSearchProvider::sCategoryName = QgsPinSearchProvider::tr( "Pins" );

void QgsPinSearchProvider::startSearch( const QString &searchtext, const SearchRegion &/*searchRegion*/ )
{
  foreach ( QGraphicsItem* item, mMapCanvas->scene()->items() )
  {
    if ( dynamic_cast<QgsPinAnnotationItem*>( item ) )
    {
      QgsPinAnnotationItem* pin = static_cast<QgsPinAnnotationItem*>( item );
      if ( pin->getName().contains( searchtext, Qt::CaseInsensitive ) ||
           pin->getRemarks().contains( searchtext, Qt::CaseInsensitive ) )
      {
        SearchResult searchResult;
        searchResult.zoomScale = 1000;
        searchResult.category = sCategoryName;
        searchResult.categoryPrecedence = 2;
        searchResult.text = tr( "Pin %1" ).arg( pin->getName() );
        searchResult.pos = pin->mapGeoPos();
        searchResult.crs = pin->mapGeoPosCrs().authid();
        searchResult.showPin = false;
        emit searchResultFound( searchResult );
      }
    }
  }
  emit searchFinished();
}
