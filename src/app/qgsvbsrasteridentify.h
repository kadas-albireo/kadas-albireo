/***************************************************************************
    qgsvbsrasteridentify.h
    ----------------------
    begin                : July 2016
    copyright            : (C) 2016 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVBSRASTERIDENTIFY_H
#define QGSVBSRASTERIDENTIFY_H

#include <QDialog>
#include <QPointer>
#include <QVariantMap>

class QNetworkReply;
class QTimer;
class QTreeWidgetItem;
class QgsGeometryRubberBand;
class QgsMapCanvas;
class QgsPinAnnotationItem;
class QgsPoint;
class QgsVBSRasterIdentifyResultDialog;

class APP_EXPORT QgsVBSRasterIdentify : public QObject
{
    Q_OBJECT
  public:
    static void identify( const QgsMapCanvas* canvas, const QgsPoint& canvasPos );

  private:
    QgsVBSRasterIdentify() : mIdentifyReply( 0 ), mTimeoutTimer( 0 ) {}

    static QgsVBSRasterIdentify* instance();
    QNetworkReply* mIdentifyReply;
    QTimer* mTimeoutTimer;
    QPointer<QgsVBSRasterIdentifyResultDialog> mDialog;

  private slots:
    void replyFinished();
};

class APP_EXPORT QgsVBSRasterIdentifyResultDialog : public QDialog
{
    Q_OBJECT
  public:
    QgsVBSRasterIdentifyResultDialog( const QVariantList &results, const QMap<QString, QVariant>& layerMap );
    ~QgsVBSRasterIdentifyResultDialog();

  private slots:
    void onItemClicked( QTreeWidgetItem* item, int col );

  private:
    static const int sGeometryRole;
    static const int sGeometryTypeRole;

    QPointer<QgsGeometryRubberBand> mRubberBand;
    QPointer<QgsPinAnnotationItem> mPin;
};


#endif // QGSVBSRASTERIDENTIFY_H
