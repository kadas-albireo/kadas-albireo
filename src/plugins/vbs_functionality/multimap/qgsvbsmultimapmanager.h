/***************************************************************************
 *  qgsvbsmultimapmanager.h                                                *
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

#ifndef QGSVBSMULTIMAPMANAGER_H
#define QGSVBSMULTIMAPMANAGER_H

#include <QObject>

class QAction;
class QDomDocument;
class QgisInterface;
class QgsVBSMapWidget;


class QgsVBSMultiMapManager : public QObject
{
    Q_OBJECT
  public:
    QgsVBSMultiMapManager( QgisInterface* iface, QObject* parent = 0 );
    ~QgsVBSMultiMapManager();

  private:
    QgisInterface* mIface;
    QList<QgsVBSMapWidget*> mMapWidgets;
    QAction* mActionAddMapWidget;

  private slots:
    void addMapWidget();
    void clearMapWidgets();
    void mapWidgetDestroyed( QObject* mapWidget );
    void writeProjectSettings( QDomDocument& doc );
    void readProjectSettings( const QDomDocument& doc );
};

#endif // QGSVBSMULTIMAPMANAGER_H
