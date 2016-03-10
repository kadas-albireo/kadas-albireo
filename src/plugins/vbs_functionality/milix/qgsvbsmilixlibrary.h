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
#include <QThread>

class QgisInterface;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QgsMapLayer;
class QModelIndex;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QgsFilterLineEdit;
class QgsVBSMilixLibraryLoader;

class QgsVBSMilixLibrary : public QDialog
{
    Q_OBJECT
  public:
    QgsVBSMilixLibrary( QgisInterface *iface, QWidget* parent = 0 );
    ~QgsVBSMilixLibrary();
    void autocreateLayer();

  public slots:
    void updateLayers();

  private:
    class TreeFilterProxyModel;
    friend class QgsVBSMilixLibraryLoader;

    static const int SymbolXmlRole;
    static const int SymbolMilitaryNameRole;
    static const int SymbolPointCountRole;
    static const int SymbolVariablePointsRole;

    QgisInterface* mIface;
    QgsVBSMilixLibraryLoader* mLoader;
    QgsFilterLineEdit* mFilterLineEdit;
    QTreeView* mTreeView;
    QStandardItemModel* mGalleryModel;
    QStandardItemModel* mLoadingModel;
    TreeFilterProxyModel* mFilterProxyModel;
    QComboBox* mLayersCombo;

  private slots:
    void filterChanged( const QString& text );
    void itemClicked( QModelIndex index );
    void loaderFinished();
    QStandardItem* addItem( QStandardItem* parent, const QString& value, const QImage &image = QImage(), bool isLeaf = false, const QString& symbolXml = QString(), const QString &symbolMilitaryName = QString(), int symbolPointCount = 0, bool symbolHasVariablePoints = false );
    void setCurrentLayer( int idx );
    void setCurrentLayer( QgsMapLayer* layer );
    void addMilXLayer();
    void manageSymbolPick( int symbolIdx );
};


class QgsVBSMilixLibraryLoader : public QThread
{
    Q_OBJECT
  public:
    QgsVBSMilixLibraryLoader( QgsVBSMilixLibrary* library, QObject* parent = 0 ) : QThread( parent ), mLibrary( library ) {}

  private:
    QgsVBSMilixLibrary* mLibrary;

    void run() override;
    QStandardItem* addItem( QStandardItem* parent, const QString& value, const QImage &image = QImage(), bool isLeaf = false, const QString& symbolXml = QString(), const QString &symbolMilitaryName = QString(), int symbolPointCount = 0, bool symbolHasVariablePoints = false );
};

#endif // QGSVBSMILIXLIBRARY_H
