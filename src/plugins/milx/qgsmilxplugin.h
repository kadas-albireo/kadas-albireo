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
#include "qgsclipboard.h"
#include <QObject>
#include <QPointer>

class QAction;
class QComboBox;
class QSlider;

class QgsMapLayer;
class QgsMessageBarItem;
class QgsMilXLibrary;
class QgsMilXEditTool;
class QgsMilXPasteHandler;

class QgsMilXPlugin: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsMilXPlugin( QgisInterface * theInterface );
    ~QgsMilXPlugin();

    void initGui();
    void unload();

    void paste( const QString &mimeType, const QByteArray &data, const QgsPoint* mapPos );

  private:
    QgisInterface* mQGisIface;
    QAction* mActionCreateMilx;
    QAction* mActionSaveMilx;
    QAction* mActionLoadMilx;
    QAction* mActionApprovedLayer;
    QgsMilXLibrary* mMilXLibrary;
    QSlider* mSymbolSizeSlider;
    QSlider* mLineWidthSlider;
    QComboBox* mWorkModeCombo;
    QPointer<QgsMilXEditTool> mActiveEditTool;
    QgsMilXPasteHandler* mPasteHandler;

  private slots:
    void createMilx();
    void saveMilx();
    void loadMilx();
    void setMilXSymbolSize( int value );
    void setMilXLineWidth( int value );
    void setMilXWorkMode( int idx );
    void stopEditing();
    void connectPickHandlers();
    void manageSymbolPick( int );
    void manageSymbolCopy( QVector<int> );
    void setApprovedLayer( bool approved );
    void setApprovedActionState( QgsMapLayer*layer );
};

class QgsMilXPasteHandler : public QgsPasteHandler
{
  public:
    QgsMilXPasteHandler( QgsMilXPlugin* milxPlugin ) : mMilxPlugin( milxPlugin ) {}
    void paste( const QString &mimeData, const QByteArray &data, const QgsPoint* mapPos ) override
    {
      mMilxPlugin->paste( mimeData, data, mapPos );
    }
  private:
    QgsMilXPlugin* mMilxPlugin;
};

#endif // QGS_MILX_PLUGIN_H
