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

#ifndef QGSREDLINING_H
#define QGSREDLINING_H

#include <QObject>
#include "qgsfeature.h"
#include "qgsmaptoolmovelabel.h"

class QAction;
class QSpinBox;
class QToolButton;
class QgisApp;
class QgsCurvePolygonV2;
class QgsColorButtonV2;
class QgsRubberBand;
class QgsSelectedFeature;
class QgsVectorLayer;

class QgsRedlining : public QObject
{
    Q_OBJECT
  public:
    QgsRedlining( QgisApp* app );

  signals:
    void styleChanged();

  private:
    QgisApp* mApp;
    QToolButton* mBtnNewObject;
    QAction* mActionEditObject;
    QgsColorButtonV2* mBtnOutlineColor;
    QgsColorButtonV2* mBtnFillColor;
    QSpinBox* mSpinBorderSize;

    QgsVectorLayer* mLayer;
    int mLayerRefCount;

    void activateTool( QgsMapTool* tool, QAction *action );

  private slots:
    void createLayer();
    void deactivateTool();
    void editObject();
    void newPoint();
    void newLine();
    void newPolygon();
    void newCircle();
    void newText();
    void saveColor();
    void saveOutlineWidth();
    void syncStyleWidgets( const QgsFeatureId& fid );
    void updateFeatureStyle( const QgsFeatureId& fid );
};

class QgsRedliningCircleMapTool : public QgsMapTool
{
  public:
    QgsRedliningCircleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer );
    ~QgsRedliningCircleMapTool();
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    bool isEditTool() { return true; }

  private:
    QgsVectorLayer* mLayer;
    QPoint mPressPos;
    QgsCurvePolygonV2* mGeometry;
    QgsRubberBand* mRubberBand;
};

class QgsRedliningTextTool : public QgsMapTool
{
  public:
    QgsRedliningTextTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
        : QgsMapTool( canvas ), mLayer( layer ) {}
    void canvasReleaseEvent( QMouseEvent * e ) override;
    bool isEditTool() { return true; }
  private:
    QgsVectorLayer* mLayer;
};

class QgsRedliningEditTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsRedliningEditTool( QgsMapCanvas* canvas, QgsVectorLayer* layer );
    ~QgsRedliningEditTool();
    void canvasPressEvent( QMouseEvent *e ) override;
    void canvasReleaseEvent( QMouseEvent *e ) override;
    void canvasMoveEvent( QMouseEvent* e );
    void canvasDoubleClickEvent( QMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    bool isEditTool() { return true; }

  public slots:
    void onStyleChanged();

  signals:
    void featureSelected( const QgsFeatureId& fid );
    void updateFeatureStyle( const QgsFeatureId& fid );

  private:
    QgsVectorLayer* mLayer;
    enum Mode { NoSelection, TextSelected, FeatureSelected } mMode;
    QgsLabelPosition mCurrentLabel;
    QgsRubberBand* mRubberBand;
    QgsSelectedFeature* mCurrentFeature;
    int mCurrentVertex;
    QgsPoint mPressPos;
    QgsPoint mPrevPos;

    void clearCurrent( bool refresh = true );
};

#endif // QGSREDLINING_H
