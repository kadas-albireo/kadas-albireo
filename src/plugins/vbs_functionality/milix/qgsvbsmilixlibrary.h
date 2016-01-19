/***************************************************************************
 *  qgsvbsmilixlibrary.h                                                   *
 *  -------------------                                                    *
 *  begin                : Sep 29, 2015                                    *
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

#ifndef QGSVBSMILIXLIBRARY_H
#define QGSVBSMILIXLIBRARY_H

#include <QDialog>
#include <QPointer>

class QgisInterface;
class QListWidget;
class QListWidgetItem;
class QgsVBSMapToolMilix;
class QgsVBSMilixManager;

class QgsVBSMilixLibrary : public QDialog
{
    Q_OBJECT
  public:
    QgsVBSMilixLibrary( QgisInterface *iface, QgsVBSMilixManager *manager, QWidget* parent = 0 );

  private:
    QgisInterface* mIface;
    QPointer<QgsVBSMapToolMilix> mCurTool;
    QgsVBSMilixManager* mManager;

    void populateSymbols( QListWidget *listViewSymbols );

  private slots:
    void createMapTool( QListWidgetItem*item );
    void unsetMilixTool();
};

#endif // QGSVBSMILIXLIBRARY_H
