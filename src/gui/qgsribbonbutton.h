#ifndef QGSRIBBONBUTTON_H
#define QGSRIBBONBUTTON_H

#include <QToolButton>

/**A button which draws icon / text. The icon is above the text at (height - 10) / 2.0 y-position*/
class GUI_EXPORT QgsRibbonButton: public QToolButton
{
  public:
    QgsRibbonButton( QWidget* parent = 0 );
    virtual ~QgsRibbonButton();

  protected:
    virtual void paintEvent( QPaintEvent* e );
    virtual void enterEvent( QEvent* event );
    virtual void leaveEvent( QEvent* event );
    virtual void focusInEvent( QFocusEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
};

#endif // QGSRIBBONBUTTON_H
