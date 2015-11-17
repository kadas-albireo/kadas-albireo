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
    enum Mode { Circle, Rect, Poly };

    QgsMapToolFilter( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand );

    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * /*e*/ ) override;
    void deactivate() override;

  private:
    Mode mMode;
    QgsRubberBand* mRubberBand;
    bool mCapturing;
    QPoint mPressPos;
};

#endif // QGSMAPTOOLFILTER_H
