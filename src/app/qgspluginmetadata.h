/***************************************************************************
                    qgspluginmetadata.h  -  Metadata class for
                    describing a loaded plugin.
                             -------------------
    begin                : Fri Feb 6 2004
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

#ifndef QGSPLUGINMETADATA_H
#define QGSPLUGINMETADATA_H
#include <QString>
class QgisPlugin;
/**
* \class QgsPluginMetadata
* \brief Stores information about a loaded plugin, including a pointer to
* the instantiated object. This allows the plugin manager to tell the plugin to
* unload itself.
*/
class QgsPluginMetadata
{
  public:
    QgsPluginMetadata( QString _libraryPath, QString _name, QgisPlugin *_plugin, bool _python = false );
    QString name();
    QString library();
    QgisPlugin *plugin();
    bool isPython();
  private:
    QString m_name;
    QString libraryPath;
    QgisPlugin *m_plugin;
    bool m_python;
};
#endif //QGSPLUGINMETADATA_H

