/***************************************************************************
 *  qgsvbssearchprovider.h                                                 *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
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

#ifndef QGSVBSSEARCHPROVIDER_H
#define QGSVBSSEARCHPROVIDER_H

#include <QObject>
#include "qgsgeometry.h"
#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"

class QgisInterface;
class QgsMapCanvas;

class QgsVBSSearchProvider : public QObject
{
    Q_OBJECT
  public:
    struct SearchResult
    {
      QString category;
      QString text;
      QgsPoint pos;
      QgsRectangle bbox;
      QgsCoordinateReferenceSystem crs;
      double zoomScale;
    };
    struct SearchRegion
    {
      QgsPolyline polygon;
      QgsCoordinateReferenceSystem crs;
    };

    QgsVBSSearchProvider( QgsMapCanvas* mapCanvas ) : mMapCanvas( mapCanvas ) { }
    virtual ~QgsVBSSearchProvider() {}
    virtual void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) = 0;
    virtual void cancelSearch() {}

  signals:
    void searchResultFound( QgsVBSSearchProvider::SearchResult result );
    void searchFinished();

  protected:
    QgsMapCanvas* mMapCanvas;
};

Q_DECLARE_METATYPE( QgsVBSSearchProvider::SearchResult )

#endif // QGSVBSSEARCHPROVIDER_H
