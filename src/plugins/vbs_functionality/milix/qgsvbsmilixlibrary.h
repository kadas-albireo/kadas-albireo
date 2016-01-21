/***************************************************************************
 *  qgsvbsmilixlibrary.h                                                   *
 *  -------------------                                                    *
 *  begin                : Sep 29, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSVBSMILIXLIBRARY_H
#define QGSVBSMILIXLIBRARY_H

#include <QDialog>
#include <QIcon>
#include <QModelIndex>
#include <QPointer>

class QgisInterface;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QgsVBSMapToolMilix;
class QgsVBSMilixManager;
class QgsFilterLineEdit;

class QgsVBSMilixLibrary : public QDialog
{
    Q_OBJECT
  public:
    QgsVBSMilixLibrary( QgisInterface *iface, QgsVBSMilixManager *manager, QWidget* parent = 0 );

  private:
    class TreeFilterProxyModel;

    static const int SymbolXmlRole;
    static const int SymbolPointCountRole;

    QgisInterface* mIface;
    QPointer<QgsVBSMapToolMilix> mCurTool;
    QgsVBSMilixManager* mManager;
    QgsFilterLineEdit* mFilterLineEdit;
    QTreeView* mTreeView;
    QStandardItemModel* mGalleryModel;
    TreeFilterProxyModel* mFilterProxyModel;

    void populateLibrary();
    QStandardItem* addItem( QStandardItem* parent, const QString& value, const QIcon &icon = QIcon(), bool isLeaf = false, const QString& symbolXml = QString(), int symbolPointCount = 0 );

  private slots:
    void unsetMilixTool();
    void filterChanged( const QString& text );
    void itemDoubleClicked( QModelIndex index );
};

#endif // QGSVBSMILIXLIBRARY_H
