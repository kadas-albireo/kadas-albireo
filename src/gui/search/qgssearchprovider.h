/***************************************************************************
 *  qgssearchprovider.h                                                 *
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

#ifndef QGSSEARCHPROVIDER_H
#define QGSSEARCHPROVIDER_H

#include <QObject>
#include "qgsgeometry.h"
#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"

class QgisInterface;
class QgsMapCanvas;

class GUI_EXPORT QgsSearchProvider : public QObject
{
    Q_OBJECT
  public:
    struct SearchResult
    {
      QString category;
      /**
       * Lower number means higher precedence.
       * 1: coordinate
       * 2: pins
       * 10: local features
       * 11: remote features
       * 20: addresses
       * 21: places
       * 22: plz codes
       * 23: municipalities
       * 24: districts
       * 25: cantons
       * 30: world locations
       * 100: unknown
       */
      int categoryPrecedence;
      QString text;
      QgsPoint pos;
      QgsRectangle bbox;
      QString crs;
      double zoomScale;
      bool showPin;
    };
    struct SearchRegion
    {
      QgsPolyline polygon;
      QString crs;
    };

    QgsSearchProvider( QgsMapCanvas* mapCanvas ) : mMapCanvas( mapCanvas ) { }
    virtual ~QgsSearchProvider() {}
    virtual void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) = 0;
    virtual void cancelSearch() {}

  signals:
    void searchResultFound( QgsSearchProvider::SearchResult result );
    void searchFinished();

  protected:
    QgsMapCanvas* mMapCanvas;
};

Q_DECLARE_METATYPE( QgsSearchProvider::SearchResult )

#endif // QGSSEARCHPROVIDER_H
