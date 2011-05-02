/***************************************************************************
    qgsgeometryanalyzer.h - QGIS Tools for vector geometry analysis
                             -------------------
    begin                : 19 March 2009
    copyright            : (C) Carson Farmer
    email                : carson.farmer@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id: qgis.h 9774 2008-12-12 05:41:24Z timlinux $ */

#ifndef QGSGEOMETRYANALYZERH
#define QGSGEOMETRYANALYZERH

#include "qgsvectorlayer.h"
#include "qgsfield.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgsfield.h"
#include "qgsdistancearea.h"

class QgsVectorFileWriter;
class QProgressDialog;


/** \ingroup analysis
 * The QGis class provides vector geometry analysis functions
 */

class ANALYSIS_EXPORT QgsGeometryAnalyzer
{
  public:

    /**Simplify vector layer using (a modified) Douglas-Peucker algorithm
     and write it to a new shape file
      @param layer input vector layer
      @param shapefileName path to the output shp
      @param tolerance (level of simplification)
      @param onlySelectedFeatures if true, only selected features are considered, else all the features
      @param p progress dialog (or 0 if no progress dialog is to be shown)
      @note: added in version 1.4*/
    bool simplify( QgsVectorLayer* layer, const QString& shapefileName, double tolerance,
                   bool onlySelectedFeatures = false, QProgressDialog* p = 0 );

    /**Calculate the true centroids, or 'center of mass' for a vector layer and
       write it to a new shape file
      @param layer input vector layer
      @param shapefileName path to the output shp
      @param onlySelectedFeatures if true, only selected features are considered, else all the features
      @param p progress dialog (or 0 if no progress dialog is to be shown)
      @note: added in version 1.4*/
    bool centroids( QgsVectorLayer* layer, const QString& shapefileName,
                    bool onlySelectedFeatures = false, QProgressDialog* p = 0 );

    /**Create a polygon based on the extent of all (selected) features and write it to a new shape file
      @param layer input vector layer
      @param shapefileName path to the output shp
      @param onlySelectedFeatures if true, only selected features are considered, else all the features
      @param p progress dialog (or 0 if no progress dialog is to be shown)
      @note: added in version 1.4*/
    bool extent( QgsVectorLayer* layer, const QString& shapefileName, bool onlySelectedFeatures = false, QProgressDialog* p = 0 );

    /**Create buffers for a vector layer and write it to a new shape file
      @param layer input vector layer
      @param shapefileName path to the output shp
      @param bufferDistance distance for buffering (if no buffer field is specified)
      @param onlySelectedFeatures if true, only selected features are considered, else all the features
      @param dissolve if true, merge all the buffers to a big multipolygon
      @param bufferDistanceField index of the attribute field that contains the buffer distance (or -1 if all features have the same buffer distance)
      @param p progress dialog (or 0 if no progress dialog is to be shown)
      @note: added in version 1.3*/
    bool buffer( QgsVectorLayer* layer, const QString& shapefileName, double bufferDistance,
                 bool onlySelectedFeatures = false, bool dissolve = false, int bufferDistanceField = -1, QProgressDialog* p = 0 );

    /**Create convex hull(s) of a vector layer and write it to a new shape file
      @param layer input vector layer
      @param shapefileName path to the output shp
      @param onlySelectedFeatures if true, only selected features are considered, else all the features
      @param uniqueIdField index of the attribute field that contains the unique convex hull id (or -1 if
      all features have the same buffer distance)
      @param p progress dialog (or 0 if no progress dialog is to be shown)
      @note: added in version 1.4*/
    bool convexHull( QgsVectorLayer* layer, const QString& shapefileName, bool onlySelectedFeatures = false,
                     int uniqueIdField = -1, QProgressDialog* p = 0 );

    /**Dissolve a vector layer and write it to a new shape file
      @param layer input vector layer
      @param shapefileName path to the output shp
      @param onlySelectedFeatures if true, only selected features are considered, else all the features
      @param uniqueIdField index of the attribute field that contains the unique id to dissolve on (or -1 if
      all features should be dissolved together)
      @param p progress dialog (or 0 if no progress dialog is to be shown)
      @note: added in version 1.4*/
    bool dissolve( QgsVectorLayer* layer, const QString& shapefileName, bool onlySelectedFeatures = false,
                   int uniqueIdField = -1, QProgressDialog* p = 0 );

  private:

    QList<double> simpleMeasure( QgsGeometry* geometry );
    double perimeterMeasure( QgsGeometry* geometry, QgsDistanceArea& measure );
    /**Helper function to simplify an individual feature*/
    void simplifyFeature( QgsFeature& f, QgsVectorFileWriter* vfw, double tolerance );
    /**Helper function to get the cetroid of an individual feature*/
    void centroidFeature( QgsFeature& f, QgsVectorFileWriter* vfw );
    /**Helper function to buffer an individual feature*/
    void bufferFeature( QgsFeature& f, int nProcessedFeatures, QgsVectorFileWriter* vfw, bool dissolve, QgsGeometry** dissolveGeometry,
                        double bufferDistance, int bufferDistanceField );
    /**Helper function to get the convex hull of feature(s)*/
    void convexFeature( QgsFeature& f, int nProcessedFeatures, QgsGeometry** dissolveGeometry );
    /**Helper function to dissolve feature(s)*/
    void dissolveFeature( QgsFeature& f, int nProcessedFeatures, QgsGeometry** dissolveGeometry );

};
#endif //QGSVECTORANALYZER
