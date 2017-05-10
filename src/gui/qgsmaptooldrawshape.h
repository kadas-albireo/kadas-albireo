/***************************************************************************
    qgsmaptooldrawshape.h  -  Generic map tool for drawing shapes
    -------------------------------------------------------------
  begin                : Nov 26, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLDRAWSHAPE_H
#define QGSMAPTOOLDRAWSHAPE_H

#include "qgsmaptool.h"
#include "qgsgeometryrubberband.h"
#include "qgsundostack.h"
#include <QPointer>

class QgsMapToolDrawShapeInputWidget;
class QgsMapToolDrawShapeInputField;

class GUI_EXPORT QgsMapToolDrawShape : public QgsMapTool
{
    Q_OBJECT
  public:
    enum Status { StatusReady, StatusDrawing, StatusFinished };

  protected:
    struct State
    {
      Status status;
    };

    QgsMapToolDrawShape( QgsMapCanvas* canvas, bool isArea, State* initialState );

  public:
    ~QgsMapToolDrawShape();
    void activate() override;
    void deactivate() override;
    void setShowNodes( bool showNodes );
    void setAllowMultipart( bool multipart ) { mMultipart = multipart; }
    void setSnapPoints( bool snapPoints ) { mSnapPoints = snapPoints; }
    void setShowInputWidget( bool showInput ) { mShowInput = showInput; }
    void setResetOnDeactivate( bool resetOnDeactivate ) { mResetOnDeactivate = resetOnDeactivate; }
    void setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits, QgsGeometryRubberBand::AngleUnit angleUnits = QgsGeometryRubberBand::ANGLE_DEGREES );
    QgsGeometryRubberBand* getRubberBand() const { return mRubberBand; }
    Status getStatus() const { return mState->status; }

    void canvasPressEvent( QMouseEvent* e ) override;
    void canvasMoveEvent( QMouseEvent* e ) override;
    void canvasReleaseEvent( QMouseEvent* e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;
    virtual int getPartCount() const = 0;
    virtual QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const = 0;
    void addGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateReferenceSystem& sourceCrs );
    void update();


  public slots:
    void reset();
    void undo() { mUndoStack.undo(); }
    void redo() { mUndoStack.redo(); }

  signals:
    void cleared();
    void finished();
    void geometryChanged();
    void canUndo( bool );
    void canRedo( bool );

  protected:
    class StateChangeCommand : public QgsUndoCommand
    {
      public:
        StateChangeCommand( QgsMapToolDrawShape* tool, State* newState, bool compress );
        void undo() override;
        void redo() override;
        bool compress() const override { return mCompress; }
      private:
        QgsMapToolDrawShape* mTool;
        QSharedPointer<QgsMapToolDrawShape::State> mPrevState, mNextState;
        bool mCompress;
    };
    friend class StateChangeCommand;

    bool mIsArea;
    bool mMultipart;
    QPointer<QgsGeometryRubberBand> mRubberBand;
    QgsMapToolDrawShapeInputWidget* mInputWidget;
    QgsUndoStack mUndoStack;

    const State* state() const { return mState.data(); }
    State* modifyableState() { return mState.data(); }
    State* cloneState() const { return new State( *state() ); }
    virtual State* emptyState() const = 0;
    virtual void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint &/*pos*/ ) { }
    virtual void inputAccepted() { }
    virtual void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) = 0;
    virtual void initInputWidget() {}
    virtual void updateInputWidget( const QgsPoint& /*mousePos*/ ) {}

    void moveMouseToPos( const QgsPoint& geoPos );
    void updateState( State* newState, bool mergeable = false );

  protected slots:
    void acceptInput();

  private:
    bool mSnapPoints;
    bool mShowInput;
    bool mResetOnDeactivate;
    bool mIgnoreNextMoveEvent;
    QSharedPointer<State> mState;

    QgsPoint transformPoint( const QPoint& p );
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawPoint : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawPoint( QgsMapCanvas* canvas );
    int getPartCount() const override { return state()->points.size(); }
    void getPart( int part, QgsPoint& p ) const { p = state()->points[part].front(); }
    void setPart( int part, const QgsPoint& p );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList< QList<QgsPoint> > points;
    };

    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* modifyableState() { return static_cast<State*>( QgsMapToolDrawShape::modifyableState() ); }
    State* cloneState() const { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawPolyLine : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawPolyLine( QgsMapCanvas* canvas, bool closed );
    int getPartCount() const override { return state()->points.size(); }
    void getPart( int part, QList<QgsPoint>& p ) const { p = state()->points[part]; }
    void setPart( int part, const QList<QgsPoint>& p );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList< QList<QgsPoint> > points;
    };

    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* modifyableState() { return static_cast<State*>( QgsMapToolDrawShape::modifyableState() ); }
    State* cloneState() const { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawRectangle : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawRectangle( QgsMapCanvas* canvas );
    int getPartCount() const override { return state()->p1.size(); }
    void getPart( int part, QgsPoint& p1, QgsPoint& p2 ) const
    {
      p1 = state()->p1[part];
      p2 = state()->p2[part];
    }
    void setPart( int part, const QgsPoint& p1, const QgsPoint& p2 );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList<QgsPoint> p1, p2;
    };

    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* modifyableState() { return static_cast<State*>( QgsMapToolDrawShape::modifyableState() ); }
    State* cloneState() const { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawCircle : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawCircle( QgsMapCanvas* canvas, bool geodesic = false );
    int getPartCount() const override { return state()->centers.size(); }
    void getPart( int part, QgsPoint& center, double& radius ) const;
    void setPart( int part, const QgsPoint& center, double radius );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList<QgsPoint> centers;
      QList<QgsPoint> ringPos;
    };

    friend class GeodesicCircleMeasurer;
    bool mGeodesic;
    QgsDistanceArea mDa;
    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;
    QPointer<QgsMapToolDrawShapeInputField> mREdit;
    mutable QVector<int> mPartMap;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* modifyableState() { return static_cast<State*>( QgsMapToolDrawShape::modifyableState() ); }
    State* cloneState() const { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;

  private slots:
    void centerInputChanged();
    void radiusInputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawCircularSector : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawCircularSector( QgsMapCanvas* canvas );
    int getPartCount() const override { return state()->centers.size(); }
    void getPart( int part, QgsPoint& center, double& radius, double& startAngle, double& stopAngle ) const
    {
      center = state()->centers[part];
      radius = state()->radii[part];
      startAngle = state()->startAngles[part];
      stopAngle = state()->stopAngles[part];
    }
    void setPart( int part, const QgsPoint& center, double radius, double startAngle, double stopAngle );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;

  protected:
    enum SectorStatus { HaveNothing, HaveCenter, HaveRadius };
    struct State : QgsMapToolDrawShape::State
    {
      SectorStatus sectorStatus;
      QList<QgsPoint> centers;
      QList<double> radii;
      QList<double> startAngles;
      QList<double> stopAngles;
    };

    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;
    QPointer<QgsMapToolDrawShapeInputField> mREdit;
    QPointer<QgsMapToolDrawShapeInputField> mA1Edit;
    QPointer<QgsMapToolDrawShapeInputField> mA2Edit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* modifyableState() { return static_cast<State*>( QgsMapToolDrawShape::modifyableState() ); }
    State* cloneState() const { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;

  private slots:
    void centerInputChanged();
    void arcInputChanged();
};

#endif // QGSMAPTOOLDRAWSHAPE_H
