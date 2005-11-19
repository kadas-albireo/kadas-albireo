/***************************************************************************
              qgssvgcache.cpp  -  SVG rendering and pixmap caching
                             -------------------
    begin                : Sat Jul 17 2004
    copyright            : (C) 2004 by Lars Luthman
    email                : larsl at users dot sourceforge dot org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id$ */

#include <iostream>

#include <qimage.h>
#include <qpainter.h>
#include <q3picture.h>
#include <qsettings.h>
#include <qmessagebox.h>

#include "qgssvgcache.h"
//Added by qt3to4:
#include <QPixmap>


QgsSVGCache::QgsSVGCache() {
  QSettings settings;
  oversampling = settings.readNumEntry("/qgis/svgoversampling", 4);
  pixelLimit = settings.readNumEntry("/qgis/svgcachesize", 200000);
  totalPixels = 0;
}

Q3Picture QgsSVGCache::getPicture(QString filename) 
{
    PictureMap::const_iterator iter;
    PictureMap::key_type key(filename);

    iter = pictureMap.find(key);

    if (iter != pictureMap.end()) {
#if QGISDEBUG > 2
        std::cerr<<"SVGCACHE: " << filename << " is already loaded"<<std::endl;
#endif
	return iter->second;
    }

#if QGISDEBUG > 2
        std::cerr<<"SVGCACHE: loading " << filename << std::endl;
#endif

    Q3Picture pic;
    pic.load(filename,"svg");
    
    pictureMap[key] = pic;

    return pic;
}  

QPixmap QgsSVGCache::getPixmap(QString filename, double scaleFactor) {
  
  // make the symbols smaller
  scaleFactor *= 0.30;
  
  PixmapMap::const_iterator iter;
  PixmapMap::key_type key(filename, scaleFactor);
  iter = pixmapMap.find(key);
  
  // if we already have the pixmap, return it
  if (iter != pixmapMap.end()) {
#if QGISDEBUG > 2
    std::cerr<<"SVGCACHE: "<<filename<<"["<<scaleFactor
	     <<"] is already loaded"<<std::endl;
#endif
    return iter->second;
  }
  
  // if not, try to load it
#if QGISDEBUG > 2
  std::cerr<<"SVGCACHE: loading "<<filename<<"["<<scaleFactor<<"]"<<std::endl;
#endif
  Q3Picture pic;
  pic.load(filename,"svg");
  int width=pic.boundingRect().width();
  width=static_cast<int>(static_cast<double>(width)*scaleFactor);
  int height=pic.boundingRect().height();
  height=static_cast<int>(static_cast<double>(height)*scaleFactor);
  
  //prevent 0 width or height, which would cause a crash
  if (width == 0) {
    width = 1;
  }
  if (height == 0) {
    height = 1;
  }
  
  // render and rescale it (with smoothing)
  QPixmap osPixmap(oversampling*width,oversampling*height);
  osPixmap.fill(QColor(qRgb(255, 255, 0)));
  QPainter p(&osPixmap);
  p.scale(scaleFactor*oversampling,scaleFactor*oversampling);
  p.drawPicture(0,0,pic);
  QImage osImage = osPixmap.convertToImage();
  // set a mask - this is probably terribly inefficient
  osImage.setAlphaBuffer(true);
  for (int i = 0; i < osImage.width(); ++i) {
    for (int j = 0; j < osImage.height(); ++j) {
#ifdef Q_OS_MACX
      // set opaque since pixels are transparent by default
      QRgb pixel = osImage.pixel(i, j);
      if (pixel != qRgba(255, 255, 0, 0)) {
        osImage.setPixel(i, j, qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), 255));
      }
#else
      // set transparent since pixels are opaque by default
      if (osImage.pixel(i, j) == qRgb(255, 255, 0)) {
        osImage.setPixel(i, j, qRgba(255, 255, 0, 0));
      }
#endif
    }
  }
  if (oversampling != 1)
    osImage = osImage.smoothScale(width, height);
  QPixmap pixmap = QPixmap(osImage);
 
  // cache it if possible, and remove other pixmaps from the cache
  // if it grows too large
  if (width * height < pixelLimit) {
#if QGISDEBUG > 2
    std::cerr<<"SVGCACHE: Caching "<<filename<<"["<<scaleFactor<<"]"
	     <<std::endl;
#endif
    pixmapMap[key] = pixmap;
    fifo.push(key);
    totalPixels += width * height;
    while (totalPixels > pixelLimit) {
#if QGISDEBUG > 2
      std::cerr<<"SVGCACHE: Deleting "<<fifo.front().first<<"["
	       <<fifo.front().second<<"] from cache"<<std::endl;
#endif
      QPixmap& oldPM(pixmapMap[fifo.front()]);
      fifo.pop();
      totalPixels -= oldPM.width() * oldPM.height();
    }
  }
  
  return pixmap;
}
  

void QgsSVGCache::clear() {
  pixmapMap.clear();
  fifo = std::queue<PixmapMap::key_type>();
  totalPixels = 0;
}


void QgsSVGCache::setOversampling(int oversamplingFactor) {
  oversampling = oversamplingFactor;
}
  

int QgsSVGCache::getOversampling() const {
  return oversampling;
}
