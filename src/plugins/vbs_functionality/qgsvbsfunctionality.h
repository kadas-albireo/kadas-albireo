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

#include <QObject>

class QAction;
class QToolBar;
class QgsVBSCoordinateDisplayer;
class QgsVBSCrsSelection;
class QgsVBSMapToolPinAnnotation;
class QgsVBSSearchBox;
class QgsMapTool;

class QgsVBSFunctionality: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsVBSFunctionality( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;
    QgsVBSCoordinateDisplayer* mCoordinateDisplayer;
    QgsVBSCrsSelection* mCrsSelection;
    QAction* mActionPinAnnotation;
    QgsVBSMapToolPinAnnotation* mMapToolPinAnnotation;
    QToolBar* mSearchToolbar;
    QgsVBSSearchBox* mSearchBox;

  private slots:
    void activateMapToolPinAnnotation();
    void onMapToolSet( QgsMapTool*tool );
};

#endif // QGSVBSFUNCTIONALITY_H