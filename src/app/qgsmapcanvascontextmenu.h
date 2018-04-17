/***************************************************************************
                          qgsmapcanvasmenu.h
                          ------------------
    begin                : Wed Dec 02 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : smani@sourcepole.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPCANVASCONTEXTMENU_H
#define QGSMAPCANVASCONTEXTMENU_H

#include <QMenu>
#include "qgspoint.h"
#include "qgsfeaturepicker.h"

class QGraphicsRectItem;
class QgsGeometryRubberBand;
class QgsVectorLayer;

class QgsMapCanvasContextMenu : public QMenu
{
    Q_OBJECT
  public:
    QgsMapCanvasContextMenu( QgsMapCanvas *canvas, const QPoint &canvasPos, const QgsPoint& mapPos );
    ~QgsMapCanvasContextMenu();

  private slots:
    void deleteAnnotation();
    void deleteFeature();
    void deleteLabel();
    void deleteOtherResult();
    void editAnnotation();
    void editFeature();
    void editOtherResult();
    void editLabel();
    void cutFeature();
    void cutOtherResult();
    void copyFeature();
    void copyOtherResult();
    void paste();
    void drawPin();
    void drawPointMarker();
    void drawSquareMarker();
    void drawTriangleMarker();
    void drawLine();
    void drawRectangle();
    void drawPolygon();
    void drawCircle();
    void drawText();
    void identify();
    void measureLine();
    void measurePolygon();
    void measureCircle();
    void measureAngle();
    void measureHeightProfile();
    void terrainSlope();
    void terrainHillshade();
    void terrainViewshed();
    void copyCoordinates();
    void copyMap();
    void print();
    void deleteItems();

  private:
    QgsPoint mMapPos;
    QgsMapCanvas* mCanvas;
    QgsFeaturePicker::PickResult mPickResult;
    QgsGeometryRubberBand* mRubberBand;
    QGraphicsRectItem* mRectItem;
};

#endif // QGSMAPCANVASCONTEXTMENU_H
