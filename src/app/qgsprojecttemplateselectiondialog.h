/***************************************************************************
    qgsprojecttemplateselectiondialog.h
    -----------------------------------
  begin                : January 2016
  copyright            : (C) 2016 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPROJECTTEMPLATESELECTIONDIALOG_H
#define QGSPROJECTTEMPLATESELECTIONDIALOG_H

#include <QDialog>
#include <ui_qgsprojecttemplateselectiondialog.h>

class QDialogButtonBox;
class QFileSystemModel;
class QModelIndex;
class QTreeView;

class APP_EXPORT QgsProjectTemplateSelectionDialog : public QDialog, Ui::QgsProjectTemplateSelectionDialogBase
{
    Q_OBJECT
  public:
    QgsProjectTemplateSelectionDialog( QWidget* parent = 0 );

  private:
    QFileSystemModel* mModel;
    QAbstractButton* mCreateButton;

  private slots:
    void itemClicked( const QModelIndex& index );
    void itemDoubleClicked( const QModelIndex& index );
    void radioChanged();
    void createProject();

};

#endif // QGSPROJECTTEMPLATESELECTIONDIALOG_H
