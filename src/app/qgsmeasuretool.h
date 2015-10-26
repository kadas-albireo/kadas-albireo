/***************************************************************************
    qgsmeasuretool.h  -  map tool for measuring distances and areas
    ---------------------
    begin                : April 2007
    copyright            : (C) 2007 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSMEASURETOOL_H
#define QGSMEASURETOOL_H

#include "qgsmaptool.h"

class QgsMapCanvas;
class QgsMeasureDialog;
class QgsRubberBand;
class QGraphicsTextItem;


class APP_EXPORT QgsMeasureTool : public QgsMapTool
{
    Q_OBJECT

  public:

    QgsMeasureTool( QgsMapCanvas* canvas, bool measureArea );

    ~QgsMeasureTool();

    //! Reset and start new
    void restart();

    //! returns whether measuring distance or area
    bool measureArea() { return mMeasureArea; }

    //! Get the current points
    const QList< QList<QgsPoint> >& getPoints() const;

    // Inherited from QgsMapTool

    //! Mouse move event for overriding
    virtual void canvasMoveEvent( QMouseEvent * e ) override;

    //! Mouse release event for overriding
    virtual void canvasReleaseEvent( QMouseEvent * e ) override;

    //! called when set as currently active map tool
    virtual void activate() override;

    //! called when map tool is being deactivated
    virtual void deactivate() override;

    virtual void keyPressEvent( QKeyEvent* e ) override;

    //! update measurement labels
    void updateLabels();

  public slots:
    //! updates the projections we're using
    void updateSettings();


  protected:
    QgsMeasureDialog* mDialog;

    //! Rubberband widget tracking the lines being drawn
    QgsRubberBand *mRubberBand;

    //! Rubberband widget tracking the added nodes to line
    QgsRubberBand *mRubberBandPoints;

    //! The current geometry part of the rubber band
    int mCurrentPart;

    //! indicates whether we're measuring distances or areas
    bool mMeasureArea;

    //! indicates whether we've recently warned the user about having the wrong
    // project projection
    bool mWrongProjectProjection;

    //! text labels with measurements
    QList< QGraphicsTextItem* > mTextLabels;

    //! Add new point
    void addPoint( QgsPoint &point );

    //! Returns the snapped (map) coordinate
    //@param p (pixel) coordinate
    QgsPoint snapPoint( const QPoint& p );

    /**Removes the last vertex from mRubberBand*/
    void undo();

    /**Update label text*/
    virtual void updateLabel( int idx );

  private slots:
    void updateLabelPositions();
};

#endif
