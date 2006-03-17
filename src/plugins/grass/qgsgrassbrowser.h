/***************************************************************************
                              qgsgrasstree.h 
                             -------------------
    begin                : February, 2006
    copyright            : (C) 2006 by Radim Blazek
    email                : radim.blazek@gmail.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSGRASSBROWSER_H
#define QGSGRASSBROWSER_H

#include <QMainWindow>
class QSplitter;
class QAction;
class QTreeView;
class QTextBrowser;
class QDirModel;

#include "qgisiface.h"
#include "qgsgrassmodel.h"

/*! \class QgsGrassBrowser
 *  \brief Model representing GRASS location structure.
 */
class QgsGrassBrowser: public QMainWindow
{
    Q_OBJECT;

public:
    //! Constructor
    QgsGrassBrowser ( QgisIface *iface, QWidget * parent = 0, Qt::WFlags f = 0  );

    //! Destructor
    ~QgsGrassBrowser();

public slots:
    // Add selected map to canvas
    void addMap();

    // Copy selected map
    void copyMap();

    // Rename selected map
    void renameMap();

    // Delete selected map
    void deleteMap();

    // Set current region to selected map
    void setRegion();

    // Refresh model
    void refresh();

    // Called when tree selection changes
    void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
    void currentChanged(const QModelIndex & current, const QModelIndex & previous);

    // Double click
    void doubleClicked(const QModelIndex & index);

signals:
    // emited when something in GRASS Tools changed region
    void regionChanged();

private:
    QgisIface *mIface;
    
    //! Current GISBASE
    QString mGisbase;

    //! Current LOCATION_NAME
    QString mLocation;

    // ! Data model
    QgsGrassModel *mModel;

    QSplitter *mSplitter;

    QTreeView *mTree;

    QTextBrowser *mTextBrowser;

    //! Actions
    QAction *mActionAddMap;
    QAction *mActionDeleteMap;
    QAction *mActionCopyMap;
    QAction *mActionRenameMap;
    QAction *mActionSetRegion;
    QAction *mActionRefresh;
};

#endif // QGSGRASSBROWSER_H
