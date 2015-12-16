/***************************************************************************
 *  qgsviewshedtool.h                                                   *
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

#ifndef QGSVIEWSHEDTOOL_H
#define QGSVIEWSHEDTOOL_H

#include <QObject>

class QgsMapCanvas;
class QgsMapToolDrawShape;

class GUI_EXPORT QgsViewshedTool : public QObject
{
    Q_OBJECT
  public:
    QgsViewshedTool( QgsMapCanvas* mapCanvas, bool sectorOnly, QObject* parent = 0 );
    ~QgsViewshedTool();

  signals:
    void finished();

  private:
    QgsMapCanvas* mMapCanvas;
    QgsMapToolDrawShape* mDrawTool;

  private slots:
    void drawFinished();
    void adjustRadius( double newRadius );
};

#endif // QGSVIEWSHEDTOOL_H
