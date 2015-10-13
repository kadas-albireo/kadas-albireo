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
class QComboBox;
class QSpinBox;
class QToolButton;
class QgisApp;
class QgsCurvePolygonV2;
class QgsColorButtonV2;
class QgsRedliningLayer;
class QgsRubberBand;
class QgsSelectedFeature;
class QgsVectorLayer;

class QgsRedlining : public QObject
{
    Q_OBJECT
  public:
    QgsRedlining( QgisApp* app );
    QgsRedliningLayer *getOrCreateLayer();

  signals:
    void featureStyleChanged();

  private:
    QgisApp* mApp;
    QToolButton* mBtnNewObject;
    QAction* mActionEditObject;
    QgsColorButtonV2* mBtnOutlineColor;
    QgsColorButtonV2* mBtnFillColor;
    QSpinBox* mSpinBorderSize;
    QComboBox* mOutlineStyleCombo;
    QComboBox* mFillStyleCombo;

    QgsRedliningLayer* mLayer;
    int mLayerRefCount;

    void activateTool( QgsMapTool* tool, QAction *action );
    static QIcon createOutlineStyleIcon( Qt::PenStyle style );
    static QIcon createFillStyleIcon( Qt::BrushStyle style );

  private slots:
    void clearLayer();
    void deactivateTool();
    void editObject();
    void newPoint();
    void newLine();
    void newRectangle();
    void newPolygon();
    void newCircle();
    void newText();
    void saveColor();
    void saveOutlineWidth();
    void saveStyle();
    void syncStyleWidgets( const QgsFeatureId& fid );
    void updateFeatureStyle( const QgsFeatureId& fid );
    void readProject( const QDomDocument&doc );
    void writeProject( QDomDocument&doc );
    void checkLayerRemoved(const QString& layerId);
};

class QgsRedliningNewShapeMapTool : public QgsMapTool
{
  public:
    QgsRedliningNewShapeMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer );
    ~QgsRedliningNewShapeMapTool();
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    bool isEditTool() { return true; }

  protected:
    QgsVectorLayer* mLayer;
    QPoint mPressPos;
    QgsCurvePolygonV2* mGeometry;
    QgsRubberBand* mRubberBand;
};

class QgsRedliningRectangleMapTool : public QgsRedliningNewShapeMapTool
{
  public:
    QgsRedliningRectangleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
        : QgsRedliningNewShapeMapTool( canvas, layer ) {}
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
};

class QgsRedliningCircleMapTool : public QgsRedliningNewShapeMapTool
{
  public:
    QgsRedliningCircleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
        : QgsRedliningNewShapeMapTool( canvas, layer ) {}
    void canvasMoveEvent( QMouseEvent * e ) override;
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
    void deactivate() override;

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
    QPointer<QgsSelectedFeature> mCurrentFeature;
    int mCurrentVertex;
    bool mIsRectangle;
    QString mRectangleCRS;
    QgsPoint mPressPos;
    QgsPoint mPrevPos;

    void clearCurrent( bool refresh = true );

  private slots:
    void updateLabelBoundingBox();
};

#endif // QGSREDLINING_H
