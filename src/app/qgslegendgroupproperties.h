#ifndef QGSLEGENDGROUPPROPERTIES_H
#define QGSLEGENDGROUPPROPERTIES_H

#include "ui_qgslegendgrouppropertiesbase.h"
#include <QDialog>

class QgsLayerTreeGroup;

class QgsLegendGroupProperties: public QDialog, private Ui::QgsLegendGroupPropertiesBase
{
    Q_OBJECT
  public:
    QgsLegendGroupProperties( QgsLayerTreeGroup* legendGroup, QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsLegendGroupProperties();

  private slots:
    void apply();

  private:
    QgsLayerTreeGroup* mLegendGroup;
};

#endif // QGSLEGENDGROUPDIALOG_H
