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
#include "qgsstatestack.h"
#include <QPointer>

class QgsFloatingInputWidget;
class QgsFloatingInputWidgetField;

class GUI_EXPORT QgsMapToolDrawShape : public QgsMapTool
{
    Q_OBJECT
  public:
    enum Status { StatusReady, StatusDrawing, StatusFinished, StatusEditingReady, StatusEditingMoving };

  protected:
    struct State : QgsStateStack::State
    {
      Status status;
    };
    struct EditContext
    {
      virtual ~EditContext() {}
      bool move = false;
    };

    QgsMapToolDrawShape( QgsMapCanvas* canvas, bool isArea, State* initialState );

  public:
    ~QgsMapToolDrawShape();
    void setParentTool( QgsMapTool* tool ) { mParentTool = tool; }
    void activate() override;
    void deactivate() override;
    void editGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateReferenceSystem& sourceCrs );
    void setAllowMultipart( bool multipart ) { mMultipart = multipart; }
    void setSnapPoints( bool snapPoints ) { mSnapPoints = snapPoints; }
    void setShowInputWidget( bool showInput ) { mShowInput = showInput; }
    void setResetOnDeactivate( bool resetOnDeactivate ) { mResetOnDeactivate = resetOnDeactivate; }
    void setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits, QgsGeometryRubberBand::AngleUnit angleUnits = QgsGeometryRubberBand::ANGLE_DEGREES, QgsGeometryRubberBand::AzimuthNorth azimuthNorth = QgsGeometryRubberBand::AZIMUTH_NORTH_GEOGRAPHIC );
    QgsGeometryRubberBand* getRubberBand() const { return mRubberBand; }
    Status getStatus() const { return state()->status; }

    void canvasPressEvent( QMouseEvent* e ) override;
    void canvasMoveEvent( QMouseEvent* e ) override;
    void canvasReleaseEvent( QMouseEvent* e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    virtual int getPartCount() const = 0;
    virtual QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const = 0;
    void addGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateReferenceSystem& sourceCrs );

    virtual void updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle );

  public slots:
    void reset();
    void undo() { mStateStack.undo(); }
    void redo() { mStateStack.redo(); }
    void update();

  signals:
    void cleared();
    void finished();
    void geometryChanged();
    void canUndo( bool );
    void canRedo( bool );

  protected:
    bool mIsArea;
    bool mMultipart;
    QPointer<QgsGeometryRubberBand> mRubberBand;
    QgsFloatingInputWidget* mInputWidget;
    QgsStateStack mStateStack;

    const State* state() const { return static_cast<const State*>( mStateStack.state() ); }
    State* mutableState() { return static_cast<State*>( mStateStack.mutableState() ); }
    const EditContext* currentEditContext() const { return mEditContext; }
    virtual State* cloneState() const { return new State( *state() ); }
    virtual State* emptyState() const = 0;
    virtual void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint &/*pos*/ ) { }
    virtual void inputAccepted() { }
    virtual void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) = 0;
    virtual void initInputWidget() {}
    virtual void updateInputWidget( const QgsPoint& /*mousePos*/ ) {}
    virtual void updateInputWidget( const EditContext* /*context*/ ) {}
    virtual EditContext* getEditContext( const QgsPoint& pos ) const = 0;
    virtual void edit( const EditContext* context, const QgsPoint& pos, const QgsVector& delta ) = 0;
    virtual void addContextMenuActions( const EditContext* /*context*/, QMenu& /*menu*/ ) const {}
    virtual void executeContextMenuAction( const EditContext* /*context*/, int /*action*/, const QgsPoint& /*pos*/ ) {}

    void moveMouseToPos( const QgsPoint& geoPos );

    static bool pointInPolygon( const QgsPoint& p, const QList<QgsPoint>& poly );
    static QgsPoint projPointOnSegment( const QgsPoint& p, const QgsPoint& s1, const QgsPoint& s2 );

  protected slots:
    void acceptInput();
    void deleteShape();

  private:
    bool mSnapPoints;
    bool mShowInput;
    bool mResetOnDeactivate;
    bool mIgnoreNextMoveEvent;
    QgsPoint mLastEditPos;
    EditContext* mEditContext;
    QgsMapTool* mParentTool;

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
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList< QList<QgsPoint> > points;
    };
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int index;
    };

    QPointer<QgsFloatingInputWidgetField> mXEdit;
    QPointer<QgsFloatingInputWidgetField> mYEdit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( QgsMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    void updateInputWidget( const QgsMapToolDrawShape::EditContext* context ) override;
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const override;
    void edit( const QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta ) override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawPolyLine : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawPolyLine( QgsMapCanvas* canvas, bool closed, bool geodesic = false );
    int getPartCount() const override { return state()->points.size(); }
    void getPart( int part, QList<QgsPoint>& p ) const { p = state()->points[part]; }
    void setPart( int part, const QList<QgsPoint>& p );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList< QList<QgsPoint> > points;
    };
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int part;
      int node;
    };

    bool mGeodesic;
    QgsDistanceArea mDa;
    QPointer<QgsFloatingInputWidgetField> mXEdit;
    QPointer<QgsFloatingInputWidgetField> mYEdit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( QgsMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    void updateInputWidget( const QgsMapToolDrawShape::EditContext* context ) override;
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const override;
    void edit( const QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta ) override;
    void addContextMenuActions( const QgsMapToolDrawShape::EditContext* context, QMenu& menu ) const override;
    void executeContextMenuAction( const QgsMapToolDrawShape::EditContext* context, int action, const QgsPoint& pos ) override;

  private slots:
    void inputChanged();

  private:
    enum ContextMenuActions {DeleteNode, AddNode, ContinueGeometry};
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
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList<QgsPoint> p1, p2;
    };
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int part;
      int point;
    };

    QPointer<QgsFloatingInputWidgetField> mXEdit;
    QPointer<QgsFloatingInputWidgetField> mYEdit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( QgsMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    void updateInputWidget( const QgsMapToolDrawShape::EditContext* context ) override;
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const override;
    void edit( const QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta ) override;

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
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList<QgsPoint> centers;
      QList<QgsPoint> ringPos;
    };
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int part;
      int point;
    };

    friend class GeodesicCircleMeasurer;
    bool mGeodesic;
    QgsDistanceArea mDa;
    QPointer<QgsFloatingInputWidgetField> mXEdit;
    QPointer<QgsFloatingInputWidgetField> mYEdit;
    QPointer<QgsFloatingInputWidgetField> mREdit;
    mutable QVector<int> mPartMap;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( QgsMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    void updateInputWidget( const QgsMapToolDrawShape::EditContext* context ) override;
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const override;
    void edit( const QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta ) override;

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
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs, QList<QgsVertexId>* hiddenNodes = 0 ) const override;

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

    QPointer<QgsFloatingInputWidgetField> mXEdit;
    QPointer<QgsFloatingInputWidgetField> mYEdit;
    QPointer<QgsFloatingInputWidgetField> mREdit;
    QPointer<QgsFloatingInputWidgetField> mA1Edit;
    QPointer<QgsFloatingInputWidgetField> mA2Edit;

    const State* state() const { return static_cast<const State*>( QgsMapToolDrawShape::state() ); }
    State* mutableState() { return static_cast<State*>( QgsMapToolDrawShape::mutableState() ); }
    State* cloneState() const override { return new State( *state() ); }
    QgsMapToolDrawShape::State* emptyState() const override;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint &pos ) override;
    void inputAccepted() override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    void updateInputWidget( const QgsMapToolDrawShape::EditContext* context ) override;
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const override;
    void edit( const QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta ) override;

  private slots:
    void centerInputChanged();
    void arcInputChanged();
};

#endif // QGSMAPTOOLDRAWSHAPE_H
