/***************************************************************************
 *  qgsgeometrysliverpolygoncheck.h                                        *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

#ifndef QGS_GEOMETRY_SLIVERPOLYGON_CHECK_H
#define QGS_GEOMETRY_SLIVERPOLYGON_CHECK_H

#include "qgsgeometryareacheck.h"
#include <qmath.h>

class QgsGeometrySliverPolygonCheck : public QgsGeometryAreaCheck
{
    Q_OBJECT

  public:
    QgsGeometrySliverPolygonCheck( QgsFeaturePool* featurePool, double threshold, double maxArea = 0. )
        : QgsGeometryAreaCheck( featurePool, threshold ), mMaxArea( maxArea ) {}
    QString errorDescription() const { return tr( "Sliver polygon" ); }
    QString errorName() const { return "QgsGeometrySliverPolygonCheck"; }

  private:
    double mMaxArea;

    bool checkThreshold( const QgsAbstractGeometryV2 *geom, double &value ) const override
    {
      QgsRectangle bb = geom->boundingBox();
      double maxDim = qMax( bb.width(), bb.height() );
      double area = geom->area();
      value = ( maxDim * maxDim ) / area;
      if ( mMaxArea > 0. && area > mMaxArea )
      {
        return false;
      }
      return value > mThreshold;
    }
};

#endif // QGS_GEOMETRY_SLIVERPOLYGON_CHECK_H
