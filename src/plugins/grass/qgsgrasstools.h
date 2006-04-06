//Added by qt3to4:
#include <QCloseEvent>
/***************************************************************************
                              qgsgrasstools.h 
                             -------------------
    begin                : March, 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSGRASSTOOLS_H
#define QGSGRASSTOOLS_H

class QCloseEvent;
class QString;
class Q3ListView;
class Q3ListViewItem;
class QDomNode;
class QDomElement;

// Must be here, so that it is included to moc file
#include "qgisapp.h"
#include "qgisiface.h"

class QgsGrassProvider;
class QgsGrassBrowser;
#include <QDialog>
#include <QTabBar>
class QSize;

class QgsGrassToolsTabWidget: public QTabWidget
{
    Q_OBJECT;

public:
    //! Constructor
    QgsGrassToolsTabWidget ( QWidget * parent = 0 );

    //! Destructor
    ~QgsGrassToolsTabWidget();

    QSize iconSize();
};

/*! \class QgsGrassTools
 *  \brief Interface to GRASS modules.
 *
 */
class QgsGrassTools: public QDialog
{
    Q_OBJECT;

public:
    //! Constructor
    QgsGrassTools ( QgisApp *qgisApp, QgisIface *iface, 
	           QWidget * parent = 0, const char * name = 0, Qt::WFlags f = 0 );

    //! Destructor
    ~QgsGrassTools();

    //! Recursively add sections and modules to the list view
    //  If parent is 0, the modules are added to mModulesListView root
    void addModules ( Q3ListViewItem *parent, QDomElement &element );

    //! Returns application directory
    QString appDir();

public slots:
    //! Load configuration from file
    bool loadConfig(QString filePath);
    
    //! Close
    void close ( void);

    //! Close event
    void closeEvent(QCloseEvent *e);

    //! Restore window position 
    void restorePosition();

    //! Save window position 
    void saveWindowLocation();

    //! Module in list clicked
    void moduleClicked ( Q3ListViewItem * item );

    //! Current mapset changed
    void mapsetChanged();

    // Emits regionChanged
    void emitRegionChanged();

    //! Close open tabs with tools
    void closeTools();

signals:
    void regionChanged();

private:
    //! QGIS application
    QgisApp *mQgisApp;

    //! Pointer to the QGIS interface object
    QgisIface *mIface;

    //! Pointer to canvas
    QgsMapCanvas *mCanvas;

    //! QGIS directory
    QString mAppDir;

    //! Browser
    QgsGrassBrowser *mBrowser;

    QgsGrassToolsTabWidget *mTabWidget;
    Q3ListView *mModulesListView;
};

#endif // QGSGRASSTOOLS_H
