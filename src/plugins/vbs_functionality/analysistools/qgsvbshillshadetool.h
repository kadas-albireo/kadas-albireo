/***************************************************************************
 *  qgsvbshillshadetool.h                                                  *
 *  -------------------                                                    *
 *  begin                : Nov 15, 2015                                    *
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

#ifndef QGSVBSHILLSHADETOOL_H
#define QGSVBSHILLSHADETOOL_H

#include <QObject>
#include "qgsmaptoolfilter.h"

class QgisInterface;
class QgsRubberBand;

class QgsVBSHillshadeTool : public QObject
{
    Q_OBJECT
  public:
    QgsVBSHillshadeTool( QgisInterface* iface, QObject* parent = 0 );
    ~QgsVBSHillshadeTool();

  signals:
    void finished();

  private:
    QgisInterface* mIface;
    QgsRubberBand* mRubberBand;
    QgsMapToolFilter::RectData* mRectData;

  private slots:
    void filterFinished();
};

#endif // QGSVBSHILLSHADETOOL_H
