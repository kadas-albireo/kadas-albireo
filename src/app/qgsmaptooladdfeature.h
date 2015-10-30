/***************************************************************************
    qgsmaptooladdfeature.h  -  map tool for adding point/line/polygon features
    ---------------------
    begin                : April 2007
    copyright            : (C) 2007 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptoolcapture.h"
#include "qgsfeature.h"

class QgsCurvePolygonV2;

/**This tool adds new point/line/polygon features to already existing vector layers*/
class APP_EXPORT QgsMapToolAddFeature : public QgsMapToolCapture
{
    Q_OBJECT
  public:
    QgsMapToolAddFeature( QgsMapCanvas* canvas );
    virtual ~QgsMapToolAddFeature();
    void canvasMapReleaseEvent( QgsMapMouseEvent * e ) override;

    bool addFeature( QgsVectorLayer *vlayer, QgsFeature *f, bool showModal = true );
    void activate() override;

  private:
    /**Converts curve to compoundcurve / linestring and adds z/m depending on layer type*/
    QgsCurveV2* convertCurve( const QgsCurveV2* c, bool geomHasCurves, bool providerSupportsCurves ) const;
    QgsCurvePolygonV2* outputPolygon( const QgsCurveV2* c, bool geomHasCurves, bool providerSupportsCurves ) const;
};
