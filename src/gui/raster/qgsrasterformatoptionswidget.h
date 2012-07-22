#ifndef QGSRASTERFORMATOPTIONSWIDGET_H
#define QGSRASTERFORMATOPTIONSWIDGET_H

#include "ui_qgsrasterformatoptionswidgetbase.h"

//class QgsRasterDataProvider;

class GUI_EXPORT QgsRasterFormatOptionsWidget: public QWidget, private Ui::QgsRasterFormatOptionsWidgetBase
{
    Q_OBJECT

  public:

    QgsRasterFormatOptionsWidget( QWidget* parent = 0, QString format = "GTiff", QString provider = "gdal" );
    ~QgsRasterFormatOptionsWidget();

    void setFormat( QString format );
    void setProvider( QString provider );
    QStringList createOptions() const;
    void showProfileButtons( bool show = true );

  public slots:

    void apply();
    void optionsHelp();
    bool optionsValidate( bool message = true );

  private slots:

    void on_mProfileNewButton_clicked();
    void on_mProfileDeleteButton_clicked();
    void on_mProfileResetButton_clicked();
    void on_mOptionsAddButton_clicked();
    void on_mOptionsDeleteButton_clicked();
    void on_mOptionsLabel_clicked();
    void on_mOptionsLineEdit_editingFinished();
    void optionsTableChanged();
    void optionsTableEnableDeleteButton();
    void updateOptions();

  private:

    QString mFormat;
    QString mProvider;
    QMap< QString, QString> mOptionsMap;
    static QMap< QString, QStringList > mBuiltinProfiles;

    QString settingsKey( QString profile ) const;
    QString currentProfileKey() const;
    QString createOptions( QString profile ) const;
    void deleteCreateOptions( QString profile );
    void setCreateOptions( );
    void setCreateOptions( QString profile, QString options );
    void setCreateOptions( QString profile, QStringList list );
    QStringList profiles() const;
    void updateProfiles();

};

#endif // QGSRASTERLAYERSAVEASDIALOG_H
