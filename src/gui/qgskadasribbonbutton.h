#ifndef QGSKADASRIBBONBUTTON_H
#define QGSKADASRIBBONBUTTON_H

#include <QAbstractButton>

/**A button which draws icon / text. The icon is above the text at (height - 10) / 2.0 y-position*/
class GUI_EXPORT QgsKadasRibbonButton: public QAbstractButton
{
    public:
        QgsKadasRibbonButton( QWidget* parent = 0 );
        virtual ~QgsKadasRibbonButton();

    protected:
        virtual void paintEvent( QPaintEvent* e );
        virtual void enterEvent( QEvent* event );
        virtual void leaveEvent( QEvent* event );
        virtual void focusInEvent( QFocusEvent* event );
        virtual void focusOutEvent( QFocusEvent* event );
};

#endif // QGSKADASRIBBONBUTTON_H
