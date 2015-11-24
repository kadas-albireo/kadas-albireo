/***************************************************************************
    qgsmaptoolfilter.h  -  generic map tool for choosing a filter area
    ----------------------
    begin                : November 2015
    copyright            : (C) 2006 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLFILTER_H
#define QGSMAPTOOLFILTER_H

#include "qgsmaptool.h"

class QgsRubberBand;

class GUI_EXPORT QgsMapToolFilter : public QgsMapTool
{
  public:
    enum Mode { Rect, Poly, Circle, CircularSector };

    struct FilterData
    {
      virtual ~FilterData() {}
      virtual void updateRubberband( QgsRubberBand* rubberband ) = 0;
    };

    struct PolyData : FilterData
    {
      QList<QgsPoint> points;
      void updateRubberband( QgsRubberBand* rubberband ) override;
    };

    struct RectData : FilterData
    {
      QgsPoint p1, p2;
      void updateRubberband( QgsRubberBand* rubberband ) override;
    };

    struct CircleData : FilterData
    {
      CircleData() : radius( 0 ) {}
      double radius;
      QgsPoint center;
      void updateRubberband( QgsRubberBand* rubberband ) override;
    };

    struct CircularSectorData : CircleData
    {
      CircularSectorData() : stage( Empty ), startAngle( 0 ), stopAngle( 0 ) {}
      enum Stage { Empty, HaveCenter, HaveArc } stage;
      bool incomplete = true;
      double startAngle;
      double stopAngle;
      void updateRubberband( QgsRubberBand* rubberband ) override;
    };

    QgsMapToolFilter( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand, FilterData* filterData = 0 );
    ~QgsMapToolFilter();

    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * /*e*/ ) override;
    void deactivate() override;

  private:
    Mode mMode;
    QgsRubberBand* mRubberBand;
    bool mCapturing;
    bool mOwnFilterData;
    FilterData* mFilterData;
};

#endif // QGSMAPTOOLFILTER_H
