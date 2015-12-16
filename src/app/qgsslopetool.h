/***************************************************************************
 *  qgsslopetool.h                                                      *
 *  -------------------                                                    *
 *  begin                : Nov 11, 2015                                    *
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

#ifndef QGSSLOPETOOL_H
#define QGSSLOPETOOL_H

#include <QObject>

class QgsMapCanvas;
class QgsMapToolDrawRectangle;

class GUI_EXPORT QgsSlopeTool : public QObject
{
    Q_OBJECT
  public:
    QgsSlopeTool( QgsMapCanvas* mapCanvas, QObject* parent = 0 );
    ~QgsSlopeTool();

  signals:
    void finished();

  private:
    QgsMapCanvas* mMapCanvas;
    QgsMapToolDrawRectangle* mRectangleTool;

  private slots:
    void drawFinished();
};

#endif // QGSSLOPETOOL_H
