/***************************************************************************
 *  qgsvbsslopetool.h                                                      *
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

#ifndef QGSVBSSLOPETOOL_H
#define QGSVBSSLOPETOOL_H

#include <QObject>

class QgisInterface;
class QgsMapToolDrawRectangle;

class QgsVBSSlopeTool : public QObject
{
    Q_OBJECT
  public:
    QgsVBSSlopeTool( QgisInterface* iface, QObject* parent = 0 );
    ~QgsVBSSlopeTool();

  signals:
    void finished();

  private:
    QgisInterface* mIface;
    QgsMapToolDrawRectangle* mRectangleTool;

  private slots:
    void drawFinished();
};

#endif // QGSVBSSLOPETOOL_H
