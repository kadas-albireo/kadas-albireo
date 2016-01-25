/***************************************************************************
 *  qgsvbsfunctionality.h                                                  *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
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

#ifndef QGSVBSFUNCTIONALITY_H
#define QGSVBSFUNCTIONALITY_H

#include "qgisplugin.h"

#include <QPointer>
class QAction;

class QgsMapLayer;
class QgsMessageBarItem;
class QgsVBSMilixLibrary;
class QgsVBSMilixManager;

class QgsVBSFunctionality: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsVBSFunctionality( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;
    QAction* mActionOvlImport;
    QAction* mActionMilx;
    QAction* mActionSaveMilx;
    QAction* mActionLoadMilx;
    QgsVBSMilixLibrary* mMilXLibrary;
    QgsVBSMilixManager* mMilXManager;

  private slots:
    void importOVL();
    void toggleMilXLibrary();
    void saveMilx();
    void loadMilx();
};

#endif // QGSVBSFUNCTIONALITY_H
