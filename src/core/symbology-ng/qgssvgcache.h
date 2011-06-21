/***************************************************************************
                              qgssvgcache.h
                            ------------------------------
  begin                :  2011
  copyright            : (C) 2011 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSVGCACHE_H
#define QGSSVGCACHE_H

#include <QColor>
#include <QDateTime>
#include <QMap>
#include <QMultiHash>
#include <QString>

class QImage;
class QPicture;

struct QgsSvgCacheEntry
{
  QString file;
  double size;
  double outlineWidth;
  QColor fill;
  QColor outline;
  QImage* image;
  QPicture* picture;
  QDateTime lastUsed;
};

/**A cache for images / pictures derived from svg files. This class supports parameter replacement in svg files
according to the svg params specification (http://www.w3.org/TR/2009/WD-SVGParamPrimer-20090616/). Supported are
the parameters 'fill-color', 'pen-color', 'outline-width', 'stroke-width'. E.g. <circle fill="param(fill-color red)" stroke="param(pen-color black)" stroke-width="param(outline-width 1)"*/
class QgsSvgCache
{
  public:

    static QgsSvgCache* instance();
    ~QgsSvgCache();

    const QImage& svgAsImage( const QString& file, double size, const QColor& fill, const QColor& outline, double outlineWidth ) const;
    const QPicture& svgAsPicture( const QString& file, double size, const QColor& fill, const QColor& outline, double outlineWidth ) const;

  protected:
    QgsSvgCache();

    void insertSVG( const QString& file, double size, const QColor& fill, const QColor& outline, double outlineWidth );
    void cacheImage( QgsSvgCacheEntry );
    void cachePicture( QgsSvgCacheEntry );

  private:
    static QgsSvgCache* mInstance;

    /**Entries sorted by last used time*/
    QMap< QDateTime, QgsSvgCacheEntry > mEntries;
    /**Entry pointers accessible by file name*/
    QMultiHash< QString, QgsSvgCacheEntry* > mEntryLookup;
    /**Estimated total size of all images and pictures*/
    double mTotalSize;
};

#endif // QGSSVGCACHE_H
