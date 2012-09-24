/***************************************************************************
                         qgsnewvectorlayerdialog.h  -  description
                             -------------------
    begin                : October 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef qgsnewvectorlayerdialog_H
#define qgsnewvectorlayerdialog_H

#include "ui_qgsnewvectorlayerdialogbase.h"
#include "qgisgui.h"
#include "qgscontexthelp.h"

#include "qgis.h"

class GUI_EXPORT QgsNewVectorLayerDialog: public QDialog, private Ui::QgsNewVectorLayerDialogBase
{
    Q_OBJECT

  public:

    // run the dialog, create the layer. Return file name if the creation was successful
    static QString runAndCreateLayer( QWidget* parent = 0, QString* enc = 0 );

    QgsNewVectorLayerDialog( QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags );
    ~QgsNewVectorLayerDialog();
    /**Returns the selected geometry type*/
    QGis::WkbType selectedType() const;
    /**Appends the chosen attribute names and types to at*/
    void attributes( QList< QPair<QString, QString> >& at ) const;
    /**Returns the file format for storage*/
    QString selectedFileFormat() const;
    /**Returns the selected crs id*/
    int selectedCrsId() const;

  protected slots:
    void on_mAddAttributeButton_clicked();
    void on_mRemoveAttributeButton_clicked();
    void on_mTypeBox_currentIndexChanged( int index );
    void on_pbnChangeSpatialRefSys_clicked();
    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }
    void nameChanged( QString );
    void selectionChanged();

  private:
    QPushButton *mOkButton;
    int mCrsId;
};

#endif //qgsnewvectorlayerdialog_H
