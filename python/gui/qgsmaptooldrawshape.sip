class QgsMapToolDrawShape : public QgsMapTool
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    enum Status { StatusReady, StatusDrawing, StatusFinished, StatusEditingReady, StatusEditingMoving };

  protected:
    struct State : QgsStateStack::State
    {
      Status status;
    };
    struct EditContext;

    QgsMapToolDrawShape( QgsMapCanvas* canvas, bool isArea, State* initialState );

  public:
    ~QgsMapToolDrawShape();
    void setParentTool( QgsMapTool* tool );
    void activate();
    void deactivate();
    void editGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateReferenceSystem& sourceCrs );
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

    virtual void updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle );

  public slots:
    void reset();
    void undo();
    void redo();
    void update();

  signals:
    void cleared();
    void finished();
    void geometryChanged();
    void canUndo( bool );
    void canRedo( bool );

  protected:
    const State* state() const;
    State* mutableState();
    virtual State* cloneState() const;
    virtual State* emptyState() const = 0;
    virtual void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint &/*pos*/ );
    virtual void inputAccepted();
    virtual void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t ) = 0;
    virtual void initInputWidget();
    virtual void updateInputWidget( const QgsPoint& /*mousePos*/ );
    virtual EditContext* getEditContext( const QgsPoint& pos ) const = 0;
    virtual void edit( EditContext* context, const QgsPoint& pos, const QgsVector& delta ) = 0;
    virtual void addContextMenuActions( const EditContext* /*context*/, QMenu& /*menu*/ ) const;
    virtual void executeContextMenuAction( const EditContext* /*context*/, int /*action*/, const QgsPoint& /*pos*/ );

    void moveMouseToPos( const QgsPoint& geoPos );

    static bool pointInPolygon( const QgsPoint& p, const QList<QgsPoint>& poly );
    static QgsPoint projPointOnSegment( const QgsPoint& p, const QgsPoint& s1, const QgsPoint& s2 );

  protected slots:
    void acceptInput();
    virtual void onFinished();
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
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int index;
    };

    const State* state() const;
    State* mutableState();
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const;
    void edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta );

  private slots:
    void inputChanged();
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
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int part;
      int node;
    };

    const State* state() const;
    State* mutableState();
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const;
    void edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta );
    void addContextMenuActions( const QgsMapToolDrawShape::EditContext* context, QMenu& menu ) const;
    void executeContextMenuAction( const QgsMapToolDrawShape::EditContext* context, int action, const QgsPoint& pos );
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
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int part;
      int point;
    };

    const State* state() const;
    State* mutableState();
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const;
    void edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta );

  private slots:
    void inputChanged();
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
    struct EditContext : QgsMapToolDrawShape::EditContext
    {
      int part;
      int point;
    };

    const State* state() const;
    State* mutableState();
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const;
    void edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta );
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
      // SectorStatus sectorStatus;
      QList<QgsPoint> centers;
      QList<double> radii;
      QList<double> startAngles;
      QList<double> stopAngles;
    };

    const State* state() const;
    State* mutableState();
    State* cloneState() const;
    QgsMapToolDrawShape::State* emptyState() const;
    void buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint &pos );
    void inputAccepted();
    void doAddGeometry( const QgsAbstractGeometryV2* geometry, const QgsCoordinateTransform& t );
    void initInputWidget();
    void updateInputWidget( const QgsPoint& mousePos );
    QgsMapToolDrawShape::EditContext* getEditContext( const QgsPoint& pos ) const;
    void edit( QgsMapToolDrawShape::EditContext* context, const QgsPoint& pos, const QgsVector& delta );
};
