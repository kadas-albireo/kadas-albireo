#ifndef QGSKMLEXPORTDIALOG_H
#define QGSKMLEXPORTDIALOG_H

#include "ui_qgskmlexportdialogbase.h"

class QgsMapLayer;

class QgsKMLExportDialog: public QDialog, private Ui::QgsKMLExportDialogBase
{
    Q_OBJECT
  public:
    QgsKMLExportDialog( const QStringList layerIds, QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsKMLExportDialog();

    QString saveFile() const;
    QList<QgsMapLayer*> selectedLayers() const;
    bool visibleExtentOnly() const;

  private slots:
    void on_mFileSelectionButton_clicked();

  private:
    QgsKMLExportDialog();
    void insertAvailableLayers();

    QStringList mLayerIds;
};

#endif // QGSKMLEXPORTDIALOG_H
