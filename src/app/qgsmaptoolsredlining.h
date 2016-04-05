/***************************************************************************
    qgsredlining.h - Redlining support
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

#ifndef QGSMAPTOOLSREDLINING_H
#define QGSMAPTOOLSREDLINING_H

#include <QObject>
#include <QPointer>
#include "qgsfeature.h"
#include "qgsmaptooldrawshape.h"
#include "qgsmaprenderer.h"

class QgsRubberBand;
class QgsSelectedFeature;
class QgsVectorLayer;

class QgsRedliningPointMapTool : public QgsMapToolDrawPoint
{
    Q_OBJECT
  public:
    QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& shape );
  private:
    QgsVectorLayer* mLayer;
    QString mShape;
  private slots:
    void onFinished();
};

class QgsRedliningRectangleMapTool : public QgsMapToolDrawRectangle
{
    Q_OBJECT
  public:
    QgsRedliningRectangleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer );
  private:
    QgsVectorLayer* mLayer;
  private slots:
    void onFinished();
};

class QgsRedliningPolylineMapTool : public QgsMapToolDrawPolyLine
{
    Q_OBJECT
  public:
    QgsRedliningPolylineMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, bool closed );
  private:
    QgsVectorLayer* mLayer;
  private slots:
    void onFinished();
};

class QgsRedliningCircleMapTool : public QgsMapToolDrawCircle
{
    Q_OBJECT
  public:
    QgsRedliningCircleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer );
  private:
    QgsVectorLayer* mLayer;
  private slots:
    void onFinished();
};

class QgsRedliningTextTool : public QgsMapToolDrawPoint
{
    Q_OBJECT
  public:
    QgsRedliningTextTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& marker = QString(), bool showEditor = true );
  private:
    QgsVectorLayer* mLayer;
    QString mMarker;
    bool mShowEditor;
  private slots:
    void onFinished();
};

class QgsRedliningEditTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsRedliningEditTool( QgsMapCanvas* canvas, QgsVectorLayer* layer );
    void setUnsetOnMiss( bool unsetOnMiss ) { mUnsetOnMiss = unsetOnMiss; }
    void selectFeature( const QgsFeature &feature );
    void selectLabel( const QgsLabelPosition& labelPos );
    ~QgsRedliningEditTool();
    void canvasPressEvent( QMouseEvent *e ) override;
    void canvasReleaseEvent( QMouseEvent *e ) override;
    void canvasMoveEvent( QMouseEvent* e );
    void canvasDoubleClickEvent( QMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    bool isEditTool() { return true; }
    void deactivate() override;

  public slots:
    void onStyleChanged();

  signals:
    void featureSelected( const QgsFeature& feature );
    void updateFeatureStyle( const QgsFeatureId& fid );

  private:
    QgsVectorLayer* mLayer;
    enum Mode { NoSelection, TextSelected, FeatureSelected } mMode;
    QgsLabelPosition mCurrentLabel;
    QgsGeometryRubberBand* mRubberBand;
    QPointer<QgsSelectedFeature> mCurrentFeature;
    int mCurrentVertex;
    bool mIsRectangle;
    QString mRectangleCRS;
    QgsPoint mPressPos;
    QgsPoint mPrevPos;
    bool mUnsetOnMiss;

    void clearCurrent( bool refresh = true );
    void checkVertexSelection();

  private slots:
    void updateLabelBoundingBox();
};

#endif // QGSMAPTOOLSREDLINING_H
