class QgsMapToolDrawShape : public QgsMapTool
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    enum Status { StatusReady, StatusDrawing, StatusFinished };

protected:
    struct State
    {
      Status status;
    };

    QgsMapToolDrawShape(QgsMapCanvas* canvas, bool isArea , State* initialState);

public:
    ~QgsMapToolDrawShape();
    void activate();
    void deactivate();
    void setShowNodes( bool showNodes );
    void setAllowMultipart( bool multipart );
    void setSnapPoints( bool snapPoints );
    void setShowInputWidget( bool showInput );
    void setResetOnDeactivate( bool resetOnDeactivate );
    void setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits, QgsGeometryRubberBand::AngleUnit angleUnits = QgsGeometryRubberBand::ANGLE_DEGREES );
    QgsGeometryRubberBand* getRubberBand() const;
    Status getStatus() const;

    void canvasPressEvent( QMouseEvent* e );
    void canvasMoveEvent( QMouseEvent* e );
    void canvasReleaseEvent( QMouseEvent* e );
    void keyReleaseEvent( QKeyEvent *e );
    virtual int getPartCount() const = 0;
    virtual QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const = 0;
    void addGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateReferenceSystem& sourceCrs );
    void update();


  public slots:
    void reset();
    void undo();
    void redo();

  signals:
    void cleared();
    void finished();
    void geometryChanged();
    void canUndo( bool );
    void canRedo( bool );

  protected:
    const State* state() const;
    State* cloneState() const;
    virtual State* emptyState() const = 0;
    virtual void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint &/*pos*/ );
    virtual void inputAccepted();
    virtual void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) = 0;
    virtual void initInputWidget();
    virtual void updateInputWidget( const QgsPoint& /*mousePos*/ );

    void moveMouseToPos( const QgsPoint& geoPos );
    void updateState(State* newState, bool mergeable = false);

  protected slots:
    void acceptInput();
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawPoint : public QgsMapToolDrawShape
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawPoint( QgsMapCanvas* canvas );
    int getPartCount() const;
    void getPart( int part, QgsPoint& p ) const;
    void setPart( int part, const QgsPoint& p );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList< QList<QgsPoint> > points;
    };

    const State* state() const;
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawPolyLine : public QgsMapToolDrawShape
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawPolyLine( QgsMapCanvas* canvas, bool closed );
    int getPartCount() const;
    void getPart( int part, QList<QgsPoint>& p ) const;
    void setPart( int part, const QList<QgsPoint>& p );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList< QList<QgsPoint> > points;
    };

    const State* state() const;
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawRectangle : public QgsMapToolDrawShape
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawRectangle( QgsMapCanvas* canvas );
    int getPartCount() const;
    void getPart( int part, QgsPoint& p1, QgsPoint& p2 ) const;
    void setPart( int part, const QgsPoint& p1, const QgsPoint& p2 );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList<QgsPoint> p1;
      QList<QgsPoint> p2;
    };

    const State* state() const;
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawCircle : public QgsMapToolDrawShape
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawCircle( QgsMapCanvas* canvas, bool geodesic = false );
    int getPartCount() const;
    void getPart( int part, QgsPoint& center, double& radius ) const;
    void setPart( int part, const QgsPoint& center, double radius );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    struct State : QgsMapToolDrawShape::State
    {
      QList<QgsPoint> centers;
      QList<QgsPoint> ringPos;
    };

    const State* state() const;
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );

  private slots:
    void centerInputChanged();
    void radiusInputChanged();
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawCircularSector : public QgsMapToolDrawShape
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawCircularSector( QgsMapCanvas* canvas );
    int getPartCount() const;
    void getPart( int part, QgsPoint& center, double& radius, double& startAngle, double& stopAngle ) const;
    void setPart( int part, const QgsPoint& center, double radius, double startAngle, double stopAngle );
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

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
    const State* state() const;
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
};
