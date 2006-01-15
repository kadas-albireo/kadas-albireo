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
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */
#ifndef QGSPLUGINMANAGER_H
#define QGSPLUGINMANAGER_H
#include <vector>
#include "ui_qgspluginmanagerbase.h"
#include "qgisgui.h"

class QgsPluginItem;
/*!
 * \brief Plugin manager for loading/unloading plugins
@author Gary Sherman
*/
class QgsPluginManager : public QDialog, private Ui::QgsPluginManagerBase
{
  Q_OBJECT
  public:
    //! Constructor
    QgsPluginManager(QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags);
    //! Destructor
    ~QgsPluginManager();
    //! Get description of plugins (name, etc)
    void getPluginDescriptions();
    //! Unload the selected plugins
    void unload();
    //! Gets the selected plugins
    std::vector<QgsPluginItem> getSelectedPlugins();
    public slots:
    //! Load selected plugins and close the dialog
    void on_btnOk_clicked();
    //! Select all plugins by setting their checkbox on
    void on_btnSelectAll_clicked();
    //! Clear all selections by clearing the plugins checkbox
    void on_btnClearAll_clicked();
    //! Browse to a location (directory) containing QGIS plugins
    void on_btnBrowse_clicked();
    //! Close the dialog
    void on_btnClose_clicked();

};

#endif
