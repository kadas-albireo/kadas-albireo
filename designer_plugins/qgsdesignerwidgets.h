#include <qwidgetplugin.h>

class QgsDesignerWidgets : public QWidgetPlugin
{
  Q_OBJECT
public:
    QgsDesignerWidgets();

    QStringList keys() const;
    QWidget* create( const QString &classname, QWidget* parent = 0, const char* name = 0 );
    QString group( const QString& ) const;
    QIcon iconSet( const QString& ) const;
    QString includeFile( const QString& ) const;
    QString toolTip( const QString& ) const;
    QString whatsThis( const QString& ) const;
    bool isContainer( const QString& ) const;
};
