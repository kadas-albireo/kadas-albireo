/***************************************************************************
 *  qgsvbsmilixmanager.h                                                   *
 *  -------------------                                                    *
 *  begin                : January, 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSVBSMILIXMANAGER_H
#define QGSVBSMILIXMANAGER_H

#include <QObject>

class QgsMapCanvas;
class QgsVBSMilixAnnotationItem;

class QgsVBSMilixManager : public QObject
{
    Q_OBJECT
  public:
    QgsVBSMilixManager( QgsMapCanvas* mapCanvas, QObject* parent = 0 );
    void addItem( QgsVBSMilixAnnotationItem* item );
    const QList<QgsVBSMilixAnnotationItem*>& getItems() const { return mItems; }

  private slots:
    void updateItems();
    void removeItem( QObject* item );
  private:
    QgsMapCanvas* mMapCanvas;
    QList<QgsVBSMilixAnnotationItem*> mItems;
};

#endif // QGSVBSMILIXMANAGER_H
