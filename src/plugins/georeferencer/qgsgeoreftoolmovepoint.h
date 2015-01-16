/***************************************************************************
     qgsgeoreftoolmovepoint.h
     --------------------------------------
    Date                 : 14-Feb-2010
    Copyright            : (C) 2010 by Jack R, Maxim Dubinin (GIS-Lab)
    Email                : sim@gis-lab.info
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGEOREFTOOLMOVEPOINT_H
#define QGSGEOREFTOOLMOVEPOINT_H

#include <QMouseEvent>
#include <QRubberBand>

#include "qgsmaptool.h"
#include "qgsrubberband.h"

class QgsGeorefToolMovePoint : public QgsMapTool
{
    Q_OBJECT

  public:
    QgsGeorefToolMovePoint( QgsMapCanvas *canvas );

    void canvasPressEvent( QMouseEvent *e ) override;
    void canvasMoveEvent( QMouseEvent *e ) override;
    void canvasReleaseEvent( QMouseEvent *e ) override;

    bool isCanvas( QgsMapCanvas * );

  signals:
    void pointPressed( const QPoint &p );
    void pointMoved( const QPoint &p );
    void pointReleased( const QPoint &p );

  private:
    /**Start point of the move in map coordinates*/
    QPoint mStartPointMapCoords;

    /**Rubberband that shows the feature being moved*/
    QRubberBand *mRubberBand;
};

#endif // QGSGEOREFTOOLMOVEPOINT_H
