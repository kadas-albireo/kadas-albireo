/***************************************************************************
    qgsmeasurecircletool.h  -  map tool for measuring circular areas
    ---------------------
    begin                : October 2015
    copyright            : (C) 2015 Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSMEASURECIRCLETOOL_H
#define QGSMEASURECIRCLETOOL_H

#include "qgsmeasuretool.h"

class QgsMapCanvas;
class QgsMeasureDialog;
class QgsRubberBand;
class QGraphicsTextItem;


class APP_EXPORT QgsMeasureCircleTool : public QgsMeasureTool
{
    Q_OBJECT

  public:
    QgsMeasureCircleTool( QgsMapCanvas* canvas ) : QgsMeasureTool( canvas, true ) { }

    // Inherited from QgsMeasureTool

    //! Mouse move event for overriding
    virtual void canvasMoveEvent( QMouseEvent * e ) override;

    //! Mouse release event for overriding
    virtual void canvasReleaseEvent( QMouseEvent * e ) override;

    virtual void keyPressEvent( QKeyEvent* /*e*/ ) override {}

  private:
    QgsPoint mCenterPos;

    virtual void updateLabel( int idx ) override;

    void updateRubberbandGeometry( const QgsPoint& point );

    void addPart( const QgsPoint& pos );
};

#endif // QGSMEASURECIRCLETOOL_H
