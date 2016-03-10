
class QgsMapToolPan : QgsMapTool
{
%TypeHeaderCode
#include <qgsmaptoolpan.h>
%End

  public:
    //! constructor
    QgsMapToolPan( QgsMapCanvas* canvas, bool allowItemInteraction = true );

    //! Overridden mouse double click event
    virtual void canvasDoubleClickEvent(QMouseEvent *e);

    //! Overridden mouse press event
    virtual void canvasPressEvent( QMouseEvent * e );

    //! Overridden mouse move event
    virtual void canvasMoveEvent( QMouseEvent * e );

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QMouseEvent * e );

    virtual bool isTransient();
};

