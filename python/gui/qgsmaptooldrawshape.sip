class QgsMapToolDrawShape : public QgsMapTool
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawShape( QgsMapCanvas* canvas, bool isArea );
    ~QgsMapToolDrawShape();
    void setAllowMultipart( bool multipart );
    void setMeasurementMode( QgsGeometryRubberBand::MeasurementMode measurementMode, QGis::UnitType displayUnits );
    QgsGeometryRubberBand* getRubberBand() const;

    void canvasPressEvent( QMouseEvent* e );
    void canvasMoveEvent( QMouseEvent* e );
    void canvasReleaseEvent( QMouseEvent* e );
    virtual int getPartCount() const = 0;
    virtual QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const = 0;
    void update();
    void reset();
    enum State { StateReady, StateDrawing, StateFinished };

  signals:
    void finished();

  protected:

    virtual State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button ) = 0;
    virtual void moveEvent( const QgsPoint& pos ) = 0;
    virtual void clear() = 0;
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
    void getPart(int part, QgsPoint& p) const;
    void setPart(int part, const QgsPoint& p);
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint& pos );
    void clear();
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
    void getPart(int part, QList<QgsPoint>& p) const;
    void setPart(int part, const QList<QgsPoint>& p);
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint& pos );
    void clear();
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
    void getPart(int part, QgsPoint& p1, QgsPoint& p2) const;
    void setPart(int part, const QgsPoint& p1, const QgsPoint& p2);
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint& pos );
    void clear();
};

///////////////////////////////////////////////////////////////////////////////

class QgsMapToolDrawCircle : public QgsMapToolDrawShape
{
%TypeHeaderCode
#include <qgsmaptooldrawshape.h>
%End
  public:
    QgsMapToolDrawCircle( QgsMapCanvas* canvas );
    int getPartCount() const;
    void getPart(int part, QgsPoint& center, double& radius) const;
    void setPart(int part, const QgsPoint& center, double radius);
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint& pos );
    void clear();
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
    void getPart(int part, QgsPoint& center, double& radius, double& startAngle, double& stopAngle) const;
    void setPart(int part, const QgsPoint& center, double radius, double startAngle, double stopAngle);
    QgsAbstractGeometryV2* createGeometry( const QgsCoordinateReferenceSystem& targetCrs ) const;

  protected:
    State buttonEvent( const QgsPoint& pos, bool press, Qt::MouseButton button );
    void moveEvent( const QgsPoint& pos );
    void clear();
};
