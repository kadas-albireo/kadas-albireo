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
#include "qgsfeature.h"

class QgsGeometryRubberBand;
class QgsVectorLayer;

class QgsMapCanvasContextMenu : public QMenu
{
    Q_OBJECT
  public:
    QgsMapCanvasContextMenu( const QgsPoint& mapPos );
    ~QgsMapCanvasContextMenu();

  private slots:
    void cutFeature();
    void copyFeature();
    void pasteFeature();
    void drawPin();
    void drawPointMarker();
    void drawSquareMarker();
    void drawTriangleMarker();
    void drawLine();
    void drawRectangle();
    void drawPolygon();
    void drawCircle();
    void drawText();
    void measureLine();
    void measurePolygon();
    void measureCircle();
    void measureAngle();
    void measureHeightProfile();
    void terrainSlope();
    void terrainHillshade();
    void terrainViewshed();
    void terrainViewshedSector();
    void terrainLineOfSight();
    void copyCoordinates();
    void copyMap();
    void print();

  private:
    QgsPoint mMapPos;
    QgsVectorLayer* mSelectedLayer;
    QgsFeatureId mSelectedFeature;
    QgsGeometryRubberBand* mRubberBand;
};

#endif // QGSMAPCANVASCONTEXTMENU_H
