/***************************************************************************
                          qgspluginmanager.h
               Plugin manager for loading/unloading QGIS plugins
                             -------------------
    begin                : 2004-02-12
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
//  *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSPLUGINMANAGER_H
#define QGSPLUGINMANAGER_H
#include <vector>
#include <QMap>
#include <QString>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QHeaderView>
#include "ui_qgspluginmanagerbase.h"
#include "qgsoptionsdialogbase.h"
#include "qgisgui.h"
#include "qgscontexthelp.h"
#include "qgspythonutils.h"
#include "qgspluginsortfilterproxymodel.h"



/*!
 * \brief Plugin manager for loading/unloading plugins
@author Gary Sherman
*/
class QgsPluginManager : public QgsOptionsDialogBase, private Ui::QgsPluginManagerBase
{
    Q_OBJECT
  public:
    //! Constructor
    QgsPluginManager( QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags );
    //! Destructor
    ~QgsPluginManager();

    //! Save pointer for python utils - necessary for unloading Python plugins
    void setPythonUtils( QgsPythonUtils* pythonUtils );

    //! Load selected plugin
    void loadPlugin( QString id );
    //! Unload unselected plugin
    void unloadPlugin( QString id );
    //! Get description of C++ plugins (name, etc)
    void getCppPluginDescriptions();

    //! Repopulate the plugin list model
    void reloadModelData();
    //! Populate the html browser widget with plugin details
    void showPluginDetails( QStandardItem * item );

    //! Remove python plugins from the metadata registry (c++ plugins stay)
    void clearPythonPluginMetadata();

    //! Add a single plugin to the map of plugin metadata
    void addPluginMetadata( QString key, QMap<QString,QString> metadata );

    //! Get metadata of given plugin
    QMap<QString,QString> * pluginMetadata( QString key );

    //! Switch to one of vertical tabs by API
    void selectTabItem( int idx );

    //! Clear the repository listWidget
    void clearRepositoryList();

    //! Add repository to the repository listWidget
    void addToRepositoryList( QMap<QString,QString> repository );

// //     //! Set vwPlugins table GUI
// //     void setTable();
// //     //! Get description of Pythin plugins (does nothing when python is disabled)
// //     void getPythonPluginDescriptions();
// //     //! Resize columns to contents
// //     void resizeColumnsToContents();
// //     //! Sort model by column ascending
// //     void sortModel( int );
// //     //! Check whether plugin installer is available (and tries to load it if it's disabled)
// //     bool checkForPluginInstaller();

  public slots:
    //! Load selected plugins and close the dialog  (called when the "Close" button clicked)
    void reject();
    //! Update stacket widget (called from the vertical list item)
    void setCurrentTab( int idx );
    //! Updates current tab tile of according to current filters
    void updateTabTitle();
    //! Handle plugin selection
    void currentPluginChanged( const QModelIndex & theIndex );
    //! Load/unload plugin when checkbox state changed
    void pluginItemChanged( QStandardItem * item );
    //! Display details of inactive item too
    void on_vwPlugins_clicked( const QModelIndex & );
    //! Disable/enable plugin after double click
    void on_vwPlugins_doubleClicked( const QModelIndex & index );
    //! Update the filter when user changes the filter expression
    void on_leFilter_textChanged( QString theText );
    //! Set filter mode to filter by name
    void on_rbFilterNames_toggled( bool checked );
    //! Set filter mode to filter by description
    void on_rbFilterDescriptions_toggled( bool checked );
    //! Set filter mode to filter by tags
    void on_rbFilterTags_toggled( bool checked );
    //! Set filter mode to filter by autor
    void on_rbFilterAuthors_toggled( bool checked );
    //! Upgrade all upgradeable plugins
    void on_buttonUpgradeAll_clicked( );
    //! Install selected plugin
    void on_buttonInstall_clicked( );
    //! Uninstall selected plugin
    void on_buttonUninstall_clicked( );

    //! Enable/disable buttons according to selected repository
    void on_treeRepositories_itemSelectionChanged( );
    //! Edit selected repository
    void on_treeRepositories_doubleClicked( QModelIndex );
    //! Define new repository connection
    void on_buttonAddRep_clicked( );
    //! Edit selected repository connection
    void on_buttonEditRep_clicked( );
    //! Delete selected repository connection
    void on_buttonDeleteRep_clicked( );
    //! Refresh all repositories
    void on_buttonRefreshRepos_clicked( );

    //! Reload plugin metadata registry after allowing/disallowing experimental plugins
    void on_ckbExperimental_toggled( bool state );

    //! Open help browser
    void on_buttonBox_helpRequested( ) { QgsContextHelp::run( metaObject()->className() ); }

// // //     //! Select all plugins by setting their checkbox on
// // //     void selectAll();
// // //     //! Clear all selections by clearing the plugins checkbox
// // //     void clearAll();
// // //     //! Show the plugin installer
// // //     void showPluginInstaller();

// // //     //! query the metadata registry about some frequently used state questions
// // //     bool pluginState( QString key, QString question );

    //! Overwrite QgsOptionsDialogBase method to prevent changing the tab list from the stacked widget
    void optionsStackedWidget_CurrentChanged( int indx ) { Q_UNUSED( indx ) }

  private:

    //! Return true if given plugin is currently present in QgsPluginRegistry
    bool isPluginLoaded( QString key );

    //! Return true if there are upgradeable plugins in the registry
    bool hasUpgradeablePlugins( );

    //! Return true if there are new plugins in the registry
    bool hasNewPlugins( );

    //! Return true if there are invalid plugins in the registry
    bool hasInvalidPlugins( );

    QStandardItemModel *mModelPlugins;
    QgsPluginSortFilterProxyModel * mModelProxy;

    QgsPythonUtils* mPythonUtils;

    QMap< QString, QMap< QString,QString > > mPlugins;

    QString mCurrentlyDisplayedPlugin;

    QList<int> checkingOnStartIntervals;
};

#endif
