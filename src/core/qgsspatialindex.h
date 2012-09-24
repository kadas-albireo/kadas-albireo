/***************************************************************************
    qgsspatialindex.h  - wrapper class for spatial index library
    ----------------------
    begin                : December 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSPATIALINDEX_H
#define QGSSPATIALINDEX_H

// forward declaration
namespace SpatialIndex
{
  class IStorageManager;
  class ISpatialIndex;
  class Region;
  class Point;

  namespace StorageManager
  {
    class IBuffer;
  }
}

class QgsFeature;
class QgsRectangle;
class QgsPoint;

#include <QList>

#include "qgsfeature.h"

class CORE_EXPORT QgsSpatialIndex
{

  public:

    /* creation of spatial index */

    /** constructor - creates R-tree */
    QgsSpatialIndex();

    /** destructor finalizes work with spatial index */
    ~QgsSpatialIndex();


    /* operations */

    /** add feature to index */
    bool insertFeature( QgsFeature& f );

    /** remove feature from index */
    bool deleteFeature( QgsFeature& f );


    /* queries */

    /** returns features that intersect the specified rectangle */
    QList<QgsFeatureId> intersects( QgsRectangle rect );

    /** returns nearest neighbors (their count is specified by second parameter) */
    QList<QgsFeatureId> nearestNeighbor( QgsPoint point, int neighbors );


  protected:
    // @note not available in python bindings
    SpatialIndex::Region rectToRegion( QgsRectangle rect );
    // @note not available in python bindings
    bool featureInfo( QgsFeature& f, SpatialIndex::Region& r, QgsFeatureId &id );

  private:

    /** storage manager */
    SpatialIndex::IStorageManager* mStorageManager;

    /** buffer for index data */
    SpatialIndex::StorageManager::IBuffer* mStorage;

    /** R-tree containing spatial index */
    SpatialIndex::ISpatialIndex* mRTree;

};

#endif

