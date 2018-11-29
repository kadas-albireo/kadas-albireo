
class QgsCoordinateFormat : public QObject
{
%TypeHeaderCode
#include <qgscoordinateformat.h>
%End
  public:
    enum Format
    {
      Default,
      DegMinSec,
      DegMin,
      DecDeg,
      UTM,
      MGRS
    };

    static QgsCoordinateFormat* instance();
    void getCoordinateDisplayFormat( QgsCoordinateFormat::Format& format, QString& epsg ) const;
    QGis::UnitType getHeightDisplayUnit() const;

    QString getDisplayString( const QgsPoint& p , const QgsCoordinateReferenceSystem &sSrs ) const;
    static QString getDisplayString( const QgsPoint& p , const QgsCoordinateReferenceSystem &sSrs, Format format, const QString& epsg );

    double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QString* errMsg = 0 );
    static double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QGis::UnitType unit, QString* errMsg = 0 );

    QgsPoint parseCoordinate(const QString& text, Format format, bool& valid) const;

  public slots:
    void setCoordinateDisplayFormat( Format format, const QString& epsg );
    void setHeightDisplayUnit( QGis::UnitType heightUnit );

  signals:
    void coordinateDisplayFormatChanged( QgsCoordinateFormat::Format format, const QString& epsg );
    void heightDisplayUnitChanged( QGis::UnitType heightUnit );

private:
    QgsCoordinateFormat();

};

