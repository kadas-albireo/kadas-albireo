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

class QgsRedliningAttributeEditor : public QObject
{
  public:
    virtual QString getName() const = 0;
    virtual bool exec( QgsFeature& f, QStringList& changedAttributes ) = 0;
};

class QgsRedliningPointMapTool : public QgsMapToolDrawPoint
{
    Q_OBJECT
  public:
    QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& shape, QgsRedliningAttributeEditor* editor = 0 );
    ~QgsRedliningPointMapTool();
  private:
    QgsVectorLayer* mLayer;
    QString mShape;
    QgsRedliningAttributeEditor* mEditor;
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
    QgsRedliningPolylineMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, bool closed, QgsRedliningAttributeEditor* editor = 0 );
    ~QgsRedliningPolylineMapTool();
  private:
    QgsVectorLayer* mLayer;
    QgsRedliningAttributeEditor* mEditor;
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

class QgsRedliningEditTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsRedliningEditTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, QgsRedliningAttributeEditor* editor = 0 );
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
    QgsRedliningAttributeEditor* mEditor;
    enum Mode { NoSelection, TextSelected, FeatureSelected } mMode;
    QgsLabelPosition mCurrentLabel;
    QgsGeometryRubberBand* mRubberBand;
    QPointer<QgsSelectedFeature> mCurrentFeature;
    int mCurrentVertex;
    bool mIsRectangle;
    bool mLabelIsForPoint;
    QString mRectangleCRS;
    QgsPoint mPressPos;
    QgsPoint mPrevPos;
    bool mUnsetOnMiss;

    void clearCurrent( bool refresh = true );
    void checkVertexSelection();
    void showContextMenu( QMouseEvent *e );
    void addVertex( const QPoint& pos );
    void deleteCurrentVertex();
    void runEditor( const QgsFeatureId& featureId );

  private slots:
    void updateLabelBoundingBox();
};

#endif // QGSMAPTOOLSREDLINING_H
