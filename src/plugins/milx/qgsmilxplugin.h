/***************************************************************************
 *  qgsmilxplugin.h                                                        *
 *  ---------------                                                        *
 *  begin                : Jul 09, 2015                                    *
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

#ifndef QGS_MILX_PLUGIN_H
#define QGS_MILX_PLUGIN_H

#include "qgisplugin.h"
#include <QObject>
#include <QPointer>

class QAction;
class QComboBox;
class QSlider;

class QgsMapLayer;
class QgsMessageBarItem;
class QgsMilXLibrary;
class QgsMilXEditTool;

class QgsMilXPlugin: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsMilXPlugin( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;
    QAction* mActionMilx;
    QAction* mActionSaveMilx;
    QAction* mActionLoadMilx;
    QAction* mActionApprovedLayer;
    QgsMilXLibrary* mMilXLibrary;
    QSlider* mSymbolSizeSlider;
    QSlider* mLineWidthSlider;
    QComboBox* mWorkModeCombo;
    QPointer<QgsMilXEditTool> mActiveEditTool;

  private slots:
    void toggleMilXLibrary();
    void saveMilx();
    void loadMilx();
    void setMilXSymbolSize( int value );
    void setMilXLineWidth( int value );
    void setMilXWorkMode( int idx );
    void stopEditing();
    void connectPickHandlers();
    void manageSymbolPick( int );
    void setApprovedLayer( bool approved );
};

#endif // QGS_MILX_PLUGIN_H
