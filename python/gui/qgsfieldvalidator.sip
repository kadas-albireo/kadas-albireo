
class QgsFieldValidator : QValidator
{
%TypeHeaderCode
#include <qgsfieldvalidator.h>
%End

  public:
    QgsFieldValidator( QObject *parent, const QgsField &field, QString defaultValue, QString dateFormat = "yyyy-MM-dd" );
    ~QgsFieldValidator();

    virtual State validate(QString & /In,Out/, int & /In,Out/) const = 0;
    virtual void fixup(QString &input /In,Out/) const;

    QString dateFormat() const;
};
