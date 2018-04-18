/***************************************************************************
    qgsmapidentifydialog.h
    ----------------------
    begin                : Aprile 2018
    copyright            : (C) 2018 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPIDENTIFYDIALOG_H
#define QGSMAPIDENTIFYDIALOG_H

#include <QDialog>
#include <QMap>
#include <QPointer>

#include "qgspluginlayer.h"

class QNetworkReply;
class QTreeWidget;
class QTreeWidgetItem;
class QgsAbstractGeometryV2;
class QgsFeature;
class QgsGeometryRubberBand;
class QgsMapCanvas;
class QgsPinAnnotationItem;
class QgsPoint;
class QgsRasterLayer;
class QgsVectorLayer;


class GUI_EXPORT QgsMapIdentifyDialog : public QDialog
{
    Q_OBJECT
  public:
    QgsMapIdentifyDialog( QgsMapCanvas *canvas, const QgsPoint& mapPos );
    ~QgsMapIdentifyDialog();

  private:
    static const int sGeometryRole;
    static const int sGeometryCrsRole;

    QgsMapCanvas* mCanvas;
    QTreeWidget* mTreeWidget;
    QPointer<QgsGeometryRubberBand> mRubberBand;
    QPointer<QgsPinAnnotationItem> mPosPin;
    QPointer<QgsPinAnnotationItem> mPin;
    QList<QgsAbstractGeometryV2*> mGeometries;
    QTimer* mTimeoutTimer = nullptr;
    QNetworkReply* mRasterIdentifyReply = nullptr;
    QMap<QString, QTreeWidgetItem*> mLayerTreeItemMap;

    void collectInfo( const QgsPoint &mapPos );
    void addPluginLayerResults( QgsPluginLayer* pLayer, const QList<QgsPluginLayer::IdentifyResult>& results );
    void addVectorLayerResult( QgsVectorLayer* vLayer, const QgsFeature& feature );

  private slots:
    void onItemClicked( QTreeWidgetItem *item, int /*col*/ );
    void rasterIdentifyFinished();

};


#endif // QGSVBSVECTORIDENTIFY_H
