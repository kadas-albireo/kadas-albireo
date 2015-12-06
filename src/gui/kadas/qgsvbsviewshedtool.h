/***************************************************************************
 *  qgsvbsviewshedtool.h                                                   *
 *  -------------------                                                    *
 *  begin                : Nov 12, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVBSVIEWSHEDTOOL_H
#define QGSVBSVIEWSHEDTOOL_H

#include <QObject>

class QgsMapCanvas;
class QgsMapToolDrawShape;

class GUI_EXPORT QgsVBSViewshedTool : public QObject
{
    Q_OBJECT
  public:
    QgsVBSViewshedTool( QgsMapCanvas* mapCanvas, bool sectorOnly, QObject* parent = 0 );
    ~QgsVBSViewshedTool();

  signals:
    void finished();

  private:
    QgsMapCanvas* mMapCanvas;
    QgsMapToolDrawShape* mDrawTool;

  private slots:
    void drawFinished();
    void adjustRadius( double newRadius );
};

#endif // QGSVBSVIEWSHEDTOOL_H
