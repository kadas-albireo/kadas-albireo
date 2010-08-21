/***************************************************************************
                              qgsmslayercache.h
                              -------------------
  begin                : September 21, 2007
  copyright            : (C) 2007 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMSLAYERCACHE_H
#define QGSMSLAYERCACHE_H

#include <time.h>
#include <QMap>
#include <QPair>
#include <QString>

class QgsMapLayer;

struct QgsMSLayerCacheEntry
{
  time_t creationTime; //time this layer was created
  time_t lastUsedTime; //last time this layer was in use
  QString url; //datasource url
  QgsMapLayer* layerPointer;
  QList<QString> temporaryFiles; //path to the temporary files written for the layer
};

/**A singleton class that caches layer objects for the
QGIS mapserver*/
class QgsMSLayerCache
{
 public:
  static QgsMSLayerCache* instance();
  ~QgsMSLayerCache();

  /**Inserts a new layer into the cash
  @param url the layer datasource
  @param layerName the layer name (to distinguish between different layers in a request using the same datasource
  @param tempFiles some layers have temporary files. The cash makes sure they are removed when removing the layer from the cash*/
  void insertLayer(const QString& url, const QString& layerName, QgsMapLayer* layer, const QList<QString>& tempFiles = QList<QString>());
  /**Searches for the layer with the given url. 
   @return a pointer to the layer or 0 if no such layer*/
  QgsMapLayer* searchLayer(const QString& url, const QString& layerName);
 
 protected:
  /**Protected singleton constructor*/
  QgsMSLayerCache();
  /**Goes through the list and removes entries and layers 
   depending on their time stamps and the number of other 
  layers*/
  void updateEntries();
  /**Removes the cash entry with the lowest 'lastUsedTime'*/
  void removeLeastUsedEntry();
  /**Frees memory and removes temporary files of an entry*/
  void freeEntryRessources(QgsMSLayerCacheEntry& entry);
 
 private:
  static QgsMSLayerCache* mInstance;

  /**Cash entries with pair url/layer name as a key. The layer name is necessary for cases where the same
    url is used several time in a request. It ensures that different layer instances are created for different
    layer names*/
  QMap<QPair<QString, QString>, QgsMSLayerCacheEntry> mEntries;
};

#endif
