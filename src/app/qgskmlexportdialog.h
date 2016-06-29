#ifndef QGSKMLEXPORTDIALOG_H
#define QGSKMLEXPORTDIALOG_H

#include "ui_qgskmlexportdialogbase.h"

class QgsMapLayer;

class QgsKMLExportDialog: public QDialog, private Ui::QgsKMLExportDialogBase
{
    Q_OBJECT
  public:

    enum ExportFormat
    {
      KML = 0,
      KMZ
    };

    QgsKMLExportDialog( const QStringList layerIds, QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsKMLExportDialog();

    QString saveFile() const;
    QList<QgsMapLayer*> selectedLayers() const;
    bool exportAnnotations() const;

    QgsKMLExportDialog::ExportFormat exportFormat() const;

  private slots:
    void on_mFileSelectionButton_clicked();
    void on_mFormatComboBox_currentIndexChanged( const QString& text );

  private:
    QgsKMLExportDialog();
    void insertAvailableLayers();
    void deactivateNonVectorLayers();
    void activateAllLayers();

    QStringList mLayerIds;
};

#endif // QGSKMLEXPORTDIALOG_H
