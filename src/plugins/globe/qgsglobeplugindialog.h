/***************************************************************************
    qgsglobeplugindialog.h - settings dialog for the globe plugin
     --------------------------------------
    Date                 : 11-Nov-2010
    Copyright            : (C) 2010 by Marco Bernasocchi
    Email                : marco at bernawebdesign.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSGLOBEPLUGINDIALOG_H
#define QGSGLOBEPLUGINDIALOG_H

#include <ui_qgsglobeplugindialog.h>
#include <QDialog>

class GlobePlugin;
class QgsVectorLayer;

class QgsGlobePluginDialog: public QDialog, private Ui::QgsGlobePluginDialogGuiBase
{
    Q_OBJECT
  public:
    QgsGlobePluginDialog( QWidget * parent = 0, Qt::WFlags fl = 0 );

    struct ElevationDataSource
    {
      QString uri;
      QString type;
      bool operator==( const ElevationDataSource& other ) { return uri == other.uri && type == other.type; }
      bool operator!=( const ElevationDataSource& other ) { return uri != other.uri || type != other.type; }
    };
    void readProjectSettings();

    QString getBaseLayerUrl() const;
    bool getSkyEnabled() const;
    QDateTime getSkyDateTime() const;
    bool getSkyAutoAmbience() const;
    double getSkyMinAmbient() const;
    float getScrollSensitivity() const;
    bool getInvertScrollWheel() const;
    QList<ElevationDataSource> getElevationDataSources() const;
    double getVerticalScale() const;
    bool getFrustumHighlighting() const;
    bool getFeatureIdenification() const;

  signals:
    void settingsApplied();

  private:
    void restoreSavedSettings();
    void writeProjectSettings();
    QString validateElevationResource( QString type, QString uri );
    void moveRow( QTableWidget* widget, int direction );

  private slots:
    void apply();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    //STEREO
    void on_comboBoxStereoMode_currentIndexChanged( int index );
    void on_pushButtonStereoResetDefaults_clicked();

    //ELEVATION
    void on_comboBoxElevationLayerType_currentIndexChanged( QString type );
    void on_pushButtonElevationLayerBrowse_clicked();
    void on_pushButtonElevationLayerAdd_clicked();
    void on_pushButtonElevationLayerRemove_clicked();
    void on_pushButtonElevationLayerUp_clicked();
    void on_pushButtonElevationLayerDown_clicked();

    //MAP
    void on_comboBoxBaseLayer_currentIndexChanged( int index );
};

#endif // QGSGLOBEPLUGINDIALOG_H
