/***************************************************************************
    qgsmaptoolcapture.h  -  map tool for capturing points, lines, polygons
    ---------------------
    begin                : January 2006
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

#ifndef QGSMAPTOOLCAPTURE_H
#define QGSMAPTOOLCAPTURE_H


#include "qgsmaptooledit.h"
#include "qgspoint.h"
#include "qgsgeometry.h"

#include <QPoint>
#include <QList>

class QgsRubberBand;
class QgsVertexMarker;
class QgsMapLayer;
class QgsGeometryValidator;

class APP_EXPORT QgsMapToolCapture : public QgsMapToolEdit
{
    Q_OBJECT

  public:
    //! constructor
    QgsMapToolCapture( QgsMapCanvas* canvas, CaptureMode mode = CaptureNone );

    //! destructor
    virtual ~QgsMapToolCapture();

    //! Overridden mouse move event
    virtual void canvasMapMoveEvent( QgsMapMouseEvent* e ) override;

    //! Overridden mouse press event
    virtual void canvasMapPressEvent( QgsMapMouseEvent * e ) override;

    //! Overridden key press event
    virtual void canvasKeyPressEvent( QKeyEvent* e ) override;

    //! deactive the tool
    virtual void deactivate() override;

  public slots:
    void currentLayerChanged( QgsMapLayer *layer );
    void addError( QgsGeometry::Error );
    void validationFinished();

  protected:
    int nextPoint( const QgsPoint& mapPoint, QgsPoint& layerPoint );

    /** Adds a point to the rubber band (in map coordinates) and to the capture list (in layer coordinates)
     @return 0 in case of success, 1 if current layer is not a vector layer, 2 if coordinate transformation failed*/
    int addVertex( const QgsPoint& point );

    /**Removes the last vertex from mRubberBand and mCaptureList*/
    void undo();

    void startCapturing();
    bool isCapturing() const;
    void stopCapturing();
    void deleteTempRubberBand();

    int size() { return mCaptureList.size(); }
    QList<QgsPoint>::iterator begin() { return mCaptureList.begin(); }
    QList<QgsPoint>::iterator end() { return mCaptureList.end(); }
    const QList<QgsPoint> &points() { return mCaptureList; }
    void setPoints( const QList<QgsPoint>& pointList ) { mCaptureList = pointList; }
    void closePolygon();

  private:
    /** Flag to indicate a map canvas capture operation is taking place */
    bool mCapturing;

    /** rubber band for polylines and polygons */
    QgsRubberBand* mRubberBand;

    /** temporary rubber band for polylines and polygons. this connects the last added point to the mouse cursor position */
    QgsRubberBand* mTempRubberBand;

    /** List to store the points of digitised lines and polygons (in layer coordinates)*/
    QList<QgsPoint> mCaptureList;

    void validateGeometry();
    QString mTip;
    QgsGeometryValidator *mValidator;
    QList< QgsGeometry::Error > mGeomErrors;
    QList< QgsVertexMarker * > mGeomErrorMarkers;

    bool mCaptureModeFromLayer;

    QgsVertexMarker* mSnappingMarker;
};

#endif
