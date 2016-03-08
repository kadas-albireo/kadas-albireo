/***************************************************************************
    qgsfeaturepicker.h
    ------------------
    begin                : November 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSFEATUREPICKER_H
#define QGSFEATUREPICKER_H

#include "qgis.h"
#include "qgsfeature.h"

class QgsPoint;
class QgsMapLayer;
class QgsMapCanvas;

class GUI_EXPORT QgsFeaturePicker
{
  public:
    struct PickResult
    {
      QgsMapLayer* layer;
      QgsFeature feature;
      QVariant otherResult;
    };

    typedef bool( *filter_t )( const QgsFeature& );
    static PickResult pick( const QgsMapCanvas *canvas, const QgsPoint& mapPos, QGis::GeometryType geomType, filter_t filter = 0 );
};

#endif // QGSFEATUREPICKER_H

