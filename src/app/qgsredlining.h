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
    QAction* mActionEditLabel;
    QgsColorButtonV2* mBtnOutlineColor;
    QgsColorButtonV2* mBtnFillColor;
    QSpinBox* mSpinBorderSize;

    QgsVectorLayer* mLayer;
    int mLayerRefCount;

    void activateTool( QgsMapTool* tool, QAction *action );

  private slots:
    void createLayer();
    void deactivateTool();
    void editLabel();
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
    QgsRedliningCircleMapTool( QgsMapCanvas* canvas );
    ~QgsRedliningCircleMapTool();
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    bool isEditTool() { return true; }

  private:
    QPoint mPressPos;
    QgsCurvePolygonV2* mGeometry;
    QgsRubberBand* mRubberBand;
};

class QgsRedliningTextTool : public QgsMapTool
{
  public:
    QgsRedliningTextTool( QgsMapCanvas* canvas ) : QgsMapTool( canvas ) {}
    void canvasReleaseEvent( QMouseEvent * e ) override;
    bool isEditTool() { return true; }
};

class QgsRedliningEditTextTool : public QgsMapToolMoveLabel
{
    Q_OBJECT
  public:
    QgsRedliningEditTextTool( QgsMapCanvas* canvas ) : QgsMapToolMoveLabel( canvas ) {}
    void canvasPressEvent( QMouseEvent *e ) override;
    void canvasDoubleClickEvent( QMouseEvent *e ) override;

  public slots:
    void onStyleChanged();

  signals:
    void featureSelected( const QgsFeatureId& fid );
    void updateFeatureStyle( const QgsFeatureId& fid );
};

#endif // QGSREDLINING_H
