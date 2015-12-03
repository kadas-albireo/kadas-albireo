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
#include <QPointer>

class QAction;
class QToolBar;
class QgsVBSSearchBox;
class QgsMapTool;
class QgsMapLayer;
class QgsMessageBarItem;
class QgsVBSCrashHandler;
class QgsVBSMultiMapManager;
class QgsVBSSlopeTool;
class QgsVBSViewshedTool;
class QgsVBSHillshadeTool;

class QgsVBSFunctionality: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsVBSFunctionality( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;
    QAction* mActionPinAnnotation;
    QToolBar* mSearchToolbar;
    QgsVBSSearchBox* mSearchBox;
    QPointer<QgsMessageBarItem> mReprojMsgItem;
    QgsVBSCrashHandler* mCrashHandler;
    QgsVBSMultiMapManager* mMultiMapManager;
    QAction* mActionOvlImport;
    QAction* mActionSlope;
    QAction* mActionViewshed;
    QAction* mActionHillshade;
    QgsVBSSlopeTool* mSlopeTool;
    QgsVBSViewshedTool* mViewshedTool;
    QgsVBSHillshadeTool* mHillshadeTool;

  private slots:
    void checkOnTheFlyProjection( const QList<QgsMapLayer*>& newLayers = QList<QgsMapLayer*>() );
    void importOVL();
    void computeSlope( bool checked );
    void computeViewshed( bool checked );
    void computeHillshade( bool checked );
};

#endif // QGSVBSFUNCTIONALITY_H
