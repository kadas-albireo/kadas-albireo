class QgsCoordinateInput : public QWidget
{
%TypeHeaderCode
#include <qgscoordinateinput.h>
%End
public:
  QgsCoordinateInput(QWidget* parent);

  const QgsPoint& getCoordinate() const;
  const QgsCoordinateReferenceSystem& getCrs() const;
  bool isEmpty() const;

  void setCoordinate(const QgsPoint& coo, const QgsCoordinateReferenceSystem& crs);

signals:
  void coordinateEdited();
  void coordinateChanged();

};

