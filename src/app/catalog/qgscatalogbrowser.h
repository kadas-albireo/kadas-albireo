/***************************************************************************
 *  qgscatalogbrowser.h                                                    *
 *  -------------------                                                    *
 *  begin                : January, 2016                                   *
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

#ifndef QGSCATALOGBROWSER_H
#define QGSCATALOGBROWSER_H

#include <QStandardItemModel>
#include <QWidget>

class QgsCatalogProvider;
class QgsFilterLineEdit;
class QTreeView;


class APP_EXPORT QgsCatalogBrowser : public QWidget
{
    Q_OBJECT
  public:
    QgsCatalogBrowser( QWidget* parent = 0 );
    void addProvider( QgsCatalogProvider* provider ) { mProviders.append( provider ); }
    void load();
    QStandardItem* addItem( QStandardItem* parent, QString text, bool isLeaf = false, QMimeData* mimeData = 0 );

  private:
    class CatalogModel;
    class TreeFilterProxyModel;

    QgsFilterLineEdit* mFilterLineEdit;
    QTreeView* mTreeView;
    CatalogModel* mCatalogModel;
    TreeFilterProxyModel* mFilterProxyModel;
    QList<QgsCatalogProvider*> mProviders;
    QStandardItem* mLoadingItem;
    int mFinishedProviders;

  private slots:
    void filterChanged( const QString& text );
    void itemDoubleClicked( const QModelIndex& index );
    void providerFinished();
};

#endif // QGSCATALOGBROWSER_H
