class QgsBottomBar : public QFrame
{
%TypeHeaderCode
#include <qgsbottombar.h>
%End
  public:
    QgsBottomBar( QgsMapCanvas* canvas, const QString& color = "orange" );
    bool eventFilter( QObject* obj, QEvent* event );

  protected:
    void updatePosition();
};
