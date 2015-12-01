/***************************************************************************
    qgsmaptooldrawshape.h  -  Generic map tool for drawing shapes
    -------------------------------------------------------------
  begin                : Nov 26, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLDRAWSHAPE_H
#define QGSMAPTOOLDRAWSHAPE_H

#include "qgsmaptool.h"
#include "qgsgeometryrubberband.h"

class QgsDistanceArea;
class QGraphicsTextItem;

class QgsMapToolDrawShape : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsMapToolDrawShape( QgsMapCanvas* canvas, bool isArea );
    ~QgsMapToolDrawShape();
    void setAllowMultipart( bool multipart ) { mMultipart = multipart; }
    void setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits );

    void canvasPressEvent( QMouseEvent* e ) override;
    void canvasMoveEvent( QMouseEvent* e ) override;
    void canvasReleaseEvent( QMouseEvent* e ) override;
    virtual QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const = 0;
    void reset();

  signals:
    void finished();

  protected:
    enum State { StateReady, StateDrawing, StateFinished } mState;
    bool mIsArea;
    bool mMultipart;
    QgsGeometryRubberBand* mRubberBand;

    virtual State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint& pos ) = 0;
    virtual void clear() = 0;
    virtual void onFinished() {}
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawPoint : public QgsMapToolDrawShape
{
  public:
    QgsMapToolDrawPoint( QgsMapCanvas* canvas );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    QList< QList<QgsPoint> > mPoints;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawPolyLine : public QgsMapToolDrawShape
{
  public:
    QgsMapToolDrawPolyLine( QgsMapCanvas* canvas, bool closed );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    QList< QList<QgsPoint> > mPoints;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawRectangle : public QgsMapToolDrawShape
{
  public:
    QgsMapToolDrawRectangle( QgsMapCanvas* canvas );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    QList<QgsPoint> mP1, mP2;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawCircle : public QgsMapToolDrawShape
{
  public:
    QgsMapToolDrawCircle( QgsMapCanvas* canvas );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    QList<QgsPoint> mCenters;
    QList<double> mRadii;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawCircularSector : public QgsMapToolDrawShape
{
  public:
    QgsMapToolDrawCircularSector( QgsMapCanvas* canvas );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    enum Stage { HaveNothing, HaveCenter, HaveArc } mSectorStage;
    QList<QgsPoint> mCenters;
    QList<double> mRadii;
    QList<double> mStartAngles;
    QList<double> mStopAngles;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
};

#endif // QGSMAPTOOLDRAWSHAPE_H
