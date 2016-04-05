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
#include "qgsfeature.h"
#include "qgsmaptoolmovelabel.h"

class QAction;
class QComboBox;
class QSpinBox;
class QToolButton;
class QgisApp;
class QgsColorButtonV2;
class QgsRedliningLayer;

class QgsGPSRouteEditor : public QObject
{
    Q_OBJECT
  public:
    QgsGPSRouteEditor( QgisApp *app, QAction* actionCreateWaypoints, QAction* actionCreateRoutes );
    QgsRedliningLayer *getOrCreateLayer();
    QgsRedliningLayer *getLayer() const;
    void editFeature( const QgsFeature &feature );
    void editLabel( const QgsLabelPosition &labelPos );

  public slots:
    void importGpx();
    void exportGpx();

  signals:
    void featureStyleChanged();

  private:
    static int sFeatureSize;
    QgisApp* mApp;
    QAction* mActionCreateWaypoints;
    QAction* mActionCreateRoutes;
    QAction* mActionEdit;

    QPointer<QgsRedliningLayer> mLayer;
    QPointer<QgsMapTool> mRedliningTool;
    int mLayerRefCount;

    void setTool( QgsMapTool* tool, QAction *action, bool active = true );

  private slots:
    void createWaypoints( bool active );
    void createRoutes( bool active );
    void clearLayer();
    void deactivateTool();
    void editObject();
    void updateFeatureStyle( const QgsFeatureId& fid );
    void readProject( const QDomDocument&doc );
    void writeProject( QDomDocument&doc );
};

#endif // QGSREDLINING_H
