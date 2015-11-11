class QgsMapToolFilter : public QgsMapTool
{
%TypeHeaderCode
#include <qgsmaptoolfilter.h>
%End
  public:
    enum Mode { Circle, Rect, Poly };

    QgsMapToolFilter( QgsMapCanvas* canvas, Mode mode, QgsRubberBand* rubberBand );

    void canvasPressEvent( QMouseEvent * e );
    void canvasMoveEvent( QMouseEvent * e );
    void canvasReleaseEvent( QMouseEvent * /*e*/ );
    void deactivate();
    bool isEditTool();
};
