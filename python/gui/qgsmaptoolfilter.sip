class QgsMapToolFilter : public QgsMapTool
{
%TypeHeaderCode
#include <qgsmaptoolfilter.h>
%End
  public:
    enum Mode { Circle, Rect, Poly };

    struct FilterData {
      virtual ~FilterData();
      virtual void updateRubberband(QgsRubberBand* rubberband) = 0;
    };

    struct RectData : QgsMapToolFilter::FilterData {
      QgsPoint p1;
      QgsPoint p2;
      void updateRubberband(QgsRubberBand* rubberband);
    };

    struct CircleData : QgsMapToolFilter::FilterData {
      double radius;
      QgsPoint center;
      void updateRubberband(QgsRubberBand* rubberband);
    };

    struct CircularSectorData : QgsMapToolFilter::CircleData {
      enum Stage { Empty, HaveCenter, HaveArc };
      Stage stage;
      double startAngle;
      double stopAngle;
      void updateRubberband(QgsRubberBand* rubberband);
    };

    QgsMapToolFilter( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand, FilterData* filterData = 0 );

    void canvasPressEvent( QMouseEvent * e );
    void canvasMoveEvent( QMouseEvent * e );
    void canvasReleaseEvent( QMouseEvent * /*e*/ );
    void deactivate();
    bool isEditTool();
};
