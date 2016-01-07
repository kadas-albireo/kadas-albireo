/***************************************************************************
 *  qgsmultimapmanager.h                                                *
 *  -------------------                                                    *
 *  begin                : Sep 16, 2015                                    *
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

#ifndef QGSMULTIMAPMANAGER_H
#define QGSMULTIMAPMANAGER_H

#include <QObject>
#include <QPointer>

class QAction;
class QDomDocument;
class QMainWindow;
class QgsMapCanvas;
class QgsMapWidget;


class GUI_EXPORT QgsMultiMapManager : public QObject
{
    Q_OBJECT
  public:
    QgsMultiMapManager(QgsMapCanvas* masterCanvas, QMainWindow *parent = 0 );
    ~QgsMultiMapManager();

  public slots:
    void addMapWidget();
    void clearMapWidgets();

  private:
    QMainWindow* mMainWindow;
    QgsMapCanvas* mMasterCanvas;
    QList<QPointer<QgsMapWidget> > mMapWidgets;
    QAction* mActionAddMapWidget;

  private slots:
    void mapWidgetDestroyed( QObject* mapWidget );
    void writeProjectSettings( QDomDocument& doc );
    void readProjectSettings( const QDomDocument& doc );
};

#endif // QGSMULTIMAPMANAGER_H
