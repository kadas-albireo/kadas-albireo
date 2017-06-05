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
#include "qgsbottombar.h"
#include "qgsmaptooldrawshape.h"
#include "qgsmaptoolpan.h"
#include "qgsmaprenderer.h"

class QgsRubberBand;
class QgsSelectedFeature;
class QgsVectorLayer;

class QgsRedliningAttribEditor : public QWidget
{
    Q_OBJECT
  public:
    QgsRedliningAttribEditor( const QString& title ) { setWindowTitle( title ); }
    virtual void set( const QgsAttributes& attribs, const QgsFields& fields ) = 0;
    virtual void get( QgsAttributes& attrib, const QgsFields& fields ) const = 0;
    virtual bool isValid() const { return true; }
    virtual void setFocus() {}
  signals:
    void changed();
};

class QgsRedliningBottomBar: public QgsBottomBar
{
    Q_OBJECT
  public:
    QgsRedliningBottomBar( QgsMapTool* tool, QgsRedliningAttribEditor* editor = 0 );
    QgsRedliningAttribEditor* editor() const { return mEditor; }
  private:
    QgsMapTool* mTool;
    QgsRedliningAttribEditor* mEditor;
  private slots:
    void onClose();
};

template <class T>
class QgsRedliningMapToolT : public T
{
  public:
    QgsRedliningMapToolT( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& flags, const QgsFeature* editFeature = 0, QgsRedliningAttribEditor* editor = 0, bool isArea = false )
        : T( canvas ), mLayer( layer ), mFlags( flags )
    {
      init( editFeature, editor );
      Q_UNUSED( isArea );
    }
    ~QgsRedliningMapToolT()
    {
      delete mBottomBar;
      delete mStandaloneEditor;
    }

  private:
    class AddFeatureCommand;

  private:
    QgsVectorLayer* mLayer;
    QString mFlags;
    QgsRedliningBottomBar* mBottomBar;
    QgsRedliningAttribEditor* mStandaloneEditor;

    void init( const QgsFeature* editFeature, QgsRedliningAttribEditor* editor );
    QgsFeature createFeature() const;
    void onFinished() override;
};

template<>
QgsRedliningMapToolT<QgsMapToolDrawPolyLine>::QgsRedliningMapToolT( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& flags, const QgsFeature* editFeature, QgsRedliningAttribEditor* editor, bool isArea );

class QgsRedliningPointMapTool : public QgsRedliningMapToolT<QgsMapToolDrawPoint>
{
    Q_OBJECT
  private:
    static const int sSizeRatio;

  public:
    QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& symbol, const QgsFeature* editFeature = 0, QgsRedliningAttribEditor* editor = 0 );
    void updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle ) override;
  private:
    QgsRubberBand* mNodeRubberband;
  private slots:
    void updateNodeRubberband();
};

class QgsRedliningRectangleMapTool : public QgsRedliningMapToolT<QgsMapToolDrawRectangle>
{
  public:
    QgsRedliningRectangleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QgsFeature* editFeature = 0, QgsRedliningAttribEditor* editor = 0 )
        : QgsRedliningMapToolT<QgsMapToolDrawRectangle>( canvas, layer, "shape=rectangle", editFeature, editor )
    {
      setMeasurementMode( QgsGeometryRubberBand::MEASURE_RECTANGLE, QGis::Meters );
    }
};

class QgsRedliningPolylineMapTool : public QgsRedliningMapToolT<QgsMapToolDrawPolyLine>
{
  public:
    QgsRedliningPolylineMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, bool closed, const QgsFeature* editFeature = 0, QgsRedliningAttribEditor* editor = 0 )
        : QgsRedliningMapToolT<QgsMapToolDrawPolyLine>( canvas, layer, closed ? "shape=polygon" : "shape=line", editFeature, editor, closed )
    {
      if ( closed )
      {
        setMeasurementMode( QgsGeometryRubberBand::MEASURE_POLYGON, QGis::Meters );
      }
      else
      {
        setMeasurementMode( QgsGeometryRubberBand::MEASURE_LINE_AND_SEGMENTS, QGis::Meters );
      }
    }
};

class QgsRedliningCircleMapTool : public QgsRedliningMapToolT<QgsMapToolDrawCircle>
{
  public:
    QgsRedliningCircleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QgsFeature* editFeature = 0, QgsRedliningAttribEditor* editor = 0 )
        : QgsRedliningMapToolT<QgsMapToolDrawCircle>( canvas, layer, "shape=circle", editFeature, editor )
    {
      setMeasurementMode( QgsGeometryRubberBand::MEASURE_CIRCLE, QGis::Meters );
    }
};

class QgsRedliningEditTextMapTool : public QgsMapToolPan
{
    Q_OBJECT
  public:
    QgsRedliningEditTextMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QgsLabelPosition& label, QgsRedliningAttribEditor* editor = 0 );
    ~QgsRedliningEditTextMapTool();

    void canvasPressEvent( QMouseEvent *e ) override;
    void canvasMoveEvent( QMouseEvent *e ) override;
    void canvasReleaseEvent( QMouseEvent */*e*/ ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    void updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle );

  public slots:
    void undo() { mStateStack->undo(); }
    void redo() { mStateStack->redo(); }

  signals:
    void canUndo( bool );
    void canRedo( bool );

  private:
    enum Status {StatusReady, StatusMoving} mStatus;
    QgsVectorLayer* mLayer;
    QgsLabelPosition mLabel;
    QgsGeometryRubberBand* mRubberBand;
    QgsPoint mPressPos;
    QgsStateStack* mStateStack;
    QgsRedliningBottomBar* mBottomBar;

    struct State : public QgsStateStack::State
    {
      QgsPointV2 pos;
    } mState;

    void showContextMenu( QMouseEvent *e );
    void updateRubberband( const QgsRectangle& rect );

  private slots:
    void update();
    void updateLabelBoundingBox();
    void applyEditorChanges();
};

#endif // QGSMAPTOOLSREDLINING_H
