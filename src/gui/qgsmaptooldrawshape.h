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
#include <QPointer>

class QgsMapToolDrawShapeInputWidget;
class QgsMapToolDrawShapeInputField;

class GUI_EXPORT QgsMapToolDrawShape : public QgsMapTool
{
    Q_OBJECT
  public:
    enum State { StateReady, StateDrawing, StateFinished };

    QgsMapToolDrawShape( QgsMapCanvas* canvas, bool isArea );
    ~QgsMapToolDrawShape();
    void activate() override;
    void deactivate() override;
    void setShowNodes( bool showNodes );
    void setAllowMultipart( bool multipart ) { mMultipart = multipart; }
    void setSnapPoints( bool snapPoints ) { mSnapPoints = snapPoints; }
    void setShowInputWidget( bool showInput ) { mShowInput = showInput; }
    void setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits, QgsGeometryRubberBand::AngleUnit angleUnits = QgsGeometryRubberBand::ANGLE_DEGREES );
    QgsGeometryRubberBand* getRubberBand() const { return mRubberBand; }
    bool isFinished() const { return mState == StateFinished; }

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

  signals:
    void cleared();
    void finished();
    void geometryChanged();

  protected:
    State mState;
    bool mIsArea;
    bool mMultipart;
    bool mSnapPoints;
    bool mShowInput;
    QgsGeometryRubberBand* mRubberBand;
    QgsMapToolDrawShapeInputWidget* mInputWidget;

    virtual State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint& pos ) = 0;
    virtual void clear() = 0;
    virtual void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) = 0;
    virtual void initInputWidget() {}
    virtual void updateInputWidget( const QgsPoint& /*mousePos*/ ) {}
    virtual State inputAccepted() { return mState; }
    void moveMouseToPos( const QgsPoint& geoPos );

  protected slots:
    void acceptInput();

  private:
    QgsPoint transformPoint( const QPoint& p );
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawPoint : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawPoint( QgsMapCanvas* canvas );
    int getPartCount() const override { return mPoints.size(); }
    void getPart( int part, QgsPoint& p ) const { p = mPoints[part].front(); }
    void setPart( int part, const QgsPoint& p ) { mPoints[part].front() = p; }
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;

  protected:
    QList< QList<QgsPoint> > mPoints;
    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    State inputAccepted() override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawPolyLine : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawPolyLine( QgsMapCanvas* canvas, bool closed );
    int getPartCount() const override { return mPoints.size(); }
    void getPart( int part, QList<QgsPoint>& p ) const { p = mPoints[part]; }
    void setPart( int part, const QList<QgsPoint>& p ) { mPoints[part] = p; }
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;

  protected:
    QList< QList<QgsPoint> > mPoints;
    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    State inputAccepted() override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawRectangle : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawRectangle( QgsMapCanvas* canvas );
    int getPartCount() const override { return mP1.size(); }
    void getPart( int part, QgsPoint& p1, QgsPoint& p2 ) const { p1 = mP1[part]; p2 = mP2[part]; }
    void setPart( int part, const QgsPoint& p1, const QgsPoint& p2 ) { mP1[part] = p1; mP2[part] = p2; }
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;

  protected:
    QList<QgsPoint> mP1, mP2;
    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    State inputAccepted() override;

  private slots:
    void inputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class GUI_EXPORT QgsMapToolDrawCircle : public QgsMapToolDrawShape
{
    Q_OBJECT
  public:
    QgsMapToolDrawCircle( QgsMapCanvas* canvas );
    int getPartCount() const override { return mCenters.size(); }
    void getPart( int part, QgsPoint& center, double& radius ) const { center = mCenters[part]; radius = mRadii[part]; }
    void setPart( int part, const QgsPoint& center, double radius ) { mCenters[part] = center; mRadii[part] = radius; }
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;

  protected:
    QList<QgsPoint> mCenters;
    QList<double> mRadii;
    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;
    QPointer<QgsMapToolDrawShapeInputField> mREdit;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    State inputAccepted() override;

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
    int getPartCount() const override { return mCenters.size(); }
    void getPart( int part, QgsPoint& center, double& radius, double& startAngle, double& stopAngle ) const
    {
      center = mCenters[part]; radius = mRadii[part]; startAngle = mStartAngles[part]; stopAngle = mStopAngles[part];
    }
    void setPart( int part, const QgsPoint& center, double radius, double startAngle, double stopAngle )
    {
      mCenters[part] = center; mRadii[part] = radius; mStartAngles[part] = startAngle; mStopAngles[part] = stopAngle;
    }
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const override;
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) override;

  protected:
    enum Stage { HaveNothing, HaveCenter, HaveRadius } mSectorStage;
    QList<QgsPoint> mCenters;
    QList<double> mRadii;
    QList<double> mStartAngles;
    QList<double> mStopAngles;
    QPointer<QgsMapToolDrawShapeInputField> mXEdit;
    QPointer<QgsMapToolDrawShapeInputField> mYEdit;
    QPointer<QgsMapToolDrawShapeInputField> mREdit;
    QPointer<QgsMapToolDrawShapeInputField> mA1Edit;
    QPointer<QgsMapToolDrawShapeInputField> mA2Edit;

    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) override;
    void moveEvent( const QgsPoint& pos ) override;
    void clear() override;
    void initInputWidget() override;
    void updateInputWidget( const QgsPoint& mousePos ) override;
    State inputAccepted() override;

  private slots:
    void centerInputChanged();
    void arcInputChanged();
};

#endif // QGSMAPTOOLDRAWSHAPE_H
