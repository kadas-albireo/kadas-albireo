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

class QDialogButtonBox;
class QFileSystemModel;
class QModelIndex;
class QTreeView;

class APP_EXPORT QgsProjectTemplateSelectionDialog : public QDialog
{
    Q_OBJECT
  public:
    QgsProjectTemplateSelectionDialog( QWidget* parent = 0 );
    QString run();

  private:
    QFileSystemModel* mModel;
    QTreeView* mTreeView;
    QDialogButtonBox* mButtonBox;

  private slots:
    void itemClicked( const QModelIndex& index );
    void itemDoubleClicked( const QModelIndex& index );

};

#endif // QGSPROJECTTEMPLATESELECTIONDIALOG_H
