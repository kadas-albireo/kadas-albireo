/***************************************************************************
                          qgsvectorlayersaveasdialog.h
 Dialog to select destination, type and crs to save as ogr layers
                             -------------------
    begin                : Mon Mar 22 2010
    copyright            : (C) 2010 by Juergen E. Fischer
    email                : jef at norbit dot de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id:$ */
#ifndef QGSVECTORLAYERSAVEASDIALOG_H
#define QGSVECTORLAYERSAVEASDIALOG_H

#include <ui_qgsvectorlayersaveasdialogbase.h>
#include <QDialog>
#include "qgscontexthelp.h"

/**
 *  Class to select destination file, type and CRS for ogr layrs
 */
class QgsVectorLayerSaveAsDialog : public QDialog, private Ui::QgsVectorLayerSaveAsDialogBase
{
    Q_OBJECT

  public:
    QgsVectorLayerSaveAsDialog( QWidget* parent = 0,  Qt::WFlags fl = 0 );
    ~QgsVectorLayerSaveAsDialog();

    QString format() const;
    QString encoding() const;
    QString filename() const;
    long crs() const;

  private slots:
    void on_mFormatComboBox_currentIndexChanged( int idx );
    void on_browseFilename_clicked();
    void on_browseCRS_clicked();
    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }
    void accept();

  private:
    long mCRS;
};

#endif // QGSVECTORLAYERSAVEASDIALOG_H
