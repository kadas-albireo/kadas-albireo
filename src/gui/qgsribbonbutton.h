#ifndef QGSRIBBONBUTTON_H
#define QGSRIBBONBUTTON_H

#include <QToolButton>

/**A button which draws icon / text. The icon is above the text at (height - 10) / 2.0 y-position*/
class GUI_EXPORT QgsRibbonButton: public QToolButton
{
    Q_OBJECT
  public:
    QgsRibbonButton( QWidget* parent = 0 );
    virtual ~QgsRibbonButton();

  signals:
    void contextMenuRequested( QPoint pos );

  protected:
    virtual void paintEvent( QPaintEvent* e );
    virtual void enterEvent( QEvent* event );
    virtual void leaveEvent( QEvent* event );
    virtual void focusInEvent( QFocusEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void contextMenuEvent( QContextMenuEvent* event );
};

#endif // QGSRIBBONBUTTON_H
