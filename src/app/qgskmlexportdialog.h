/***************************************************************************
 *  qgskmlexportdialog.h                                                   *
 *  -----------                                                            *
 *  begin                : October 2015                                    *
 *  copyright            : (C) 2015 by Marco Hugentobler / Sourcepole AG   *
 *  email                : marco@sourcepole.ch                             *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSKMLEXPORTDIALOG_H
#define QGSKMLEXPORTDIALOG_H

#include "qgskmlexport.h"
#include "ui_qgskmlexportdialogbase.h"

class QgsMapLayer;

class QgsKMLExportDialog: public QDialog, private Ui::QgsKMLExportDialogBase
{
    Q_OBJECT
  public:
    QgsKMLExportDialog( const QList<QgsMapLayer *> &activeLayers, QWidget * parent = 0, Qt::WindowFlags f = 0 );
    QString getFilename() const { return mFileLineEdit->text(); }
    bool getExportAnnotations() const { return mAnnotationsCheckBox->isChecked(); }
    QList<QgsMapLayer*> getSelectedLayers() const;

  private slots:
    void selectFile();
};

#endif // QGSKMLEXPORTDIALOG_H
