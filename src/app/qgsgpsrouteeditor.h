/***************************************************************************
    qgsgpsrouteeditor.h - GPS route editor
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGPSROUTEEDITOR_H
#define QGSGPSROUTEEDITOR_H

#include <QObject>
#include <QPointer>
#include "qgsredliningmanager.h"
#include "qgsfeature.h"
#include "qgsmaptoolmovelabel.h"

class QAction;
class QgisApp;
class QgsMapToolDrawShape;
class QgsRedliningLayer;

class QgsGPSRouteEditor : public QgsRedliningManager
{
    Q_OBJECT
  public:
    QgsGPSRouteEditor( QgisApp *app, QAction* actionCreateWaypoints, QAction* actionCreateRoutes );
    QgsRedliningLayer *getOrCreateLayer();
    QgsRedliningLayer *getLayer() const;
    void editFeature( const QgsFeature &feature ) override;
    void editLabel( const QgsLabelPosition &labelPos ) override;
    void editFeatures( const QList<QgsFeature>& features ) override;

  public slots:
    void importGpx();
    void exportGpx();

  signals:
    void featureStyleChanged();

  private:
    class WaypointEditor;
    class RouteEditor;

    static int sFeatureSize;
    QgisApp* mApp;
    QAction* mActionCreateWaypoints;
    QAction* mActionCreateRoutes;
    QAction* mActionEdit;

    QPointer<QgsMapToolDrawShape> mRedliningTool;

    void setLayer( QgsRedliningLayer* layer );
    bool setTool( QgsMapToolDrawShape *tool, QAction *action, bool active = true );

  private slots:
    void setWaypointsTool( bool active = true, const QgsFeature* editFeature = 0 );
    void setRoutesTool( bool active = true, const QgsFeature* editFeature = 0 );
    void checkLayerRemoved( const QString &layerId );
    void deactivateTool();
    void readProject( const QDomDocument&doc );
    void writeProject( QDomDocument&doc );
};

#endif // QGSREDLINING_H
