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
#include "qgsmaptool.h"
#include "qgsmaprenderer.h"

class QgsRubberBand;
class QgsSelectedFeature;
class QgsVectorLayer;

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
    QgsAbstractGeometryV2* mGeometry;
    QgsRubberBand* mRubberBand;
};

class QgsRedliningPointMapTool : public QgsRedliningNewShapeMapTool
{
  public:
    QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& shape )
        : QgsRedliningNewShapeMapTool( canvas, layer ), mShape( shape ) {}
    void canvasPressEvent( QMouseEvent * /*e*/ ) override {}
    void canvasReleaseEvent( QMouseEvent * e ) override;
  private:
    QString mShape;
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
    void checkVertexSelection();

  private slots:
    void updateLabelBoundingBox();
};

#endif // QGSMAPTOOLSREDLINING_H
