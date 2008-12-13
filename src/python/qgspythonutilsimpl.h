/***************************************************************************
    qgspythonutilsimpl.h - routines for interfacing Python
    ---------------------
    begin                : May 2008
    copyright            : (C) 2008 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */


#ifndef QGSPYTHONUTILSIMPL_H
#define QGSPYTHONUTILSIMPL_H

#include "qgspythonutils.h"

// forward declaration for PyObject
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif


class QgsPythonUtilsImpl : public QgsPythonUtils
{
  public:

    QgsPythonUtilsImpl();

    virtual ~QgsPythonUtilsImpl();

    /* general purpose functions */

    //! initialize python and import bindings
    void initPython( QgisInterface* interface );

    //! close python interpreter
    void exitPython();

    //! returns true if python support is ready to use (must be inited first)
    bool isEnabled();

    //! returns path where QGIS python stuff is located
    QString pythonPath();

    //! run a statement (wrapper for PyRun_String)
    //! this command is more advanced as enables error checking etc.
    //! when an exception is raised, it shows dialog with exception details
    //! @return true if no error occured
    bool runString( const QString& command, QString msgOnError = QString() );

    //! run a statement, error reporting is not done
    //! @return true if no error occured
    bool runStringUnsafe( const QString& command );

    bool evalString( const QString& command, QString& result );

    //! @return object's type name as a string
    QString getTypeAsString( PyObject* obj );

    //! get information about error to the supplied arguments
    //! @return false if there was no python error
    bool getError( QString& errorClassName, QString& errorText );

    //! get variable from main dictionary
    QString getVariableFromMain( QString name );

    /* python console related functions */

    //! change displayhook and excepthook
    //! our hooks will just save the result to special variables
    //! and those can be used in the program
    void installConsoleHooks();

    //! get back to the original settings (i.e. write output to stdout)
    void uninstallConsoleHooks();

    //! get result from the last statement as a string
    QString getResult();

    /* plugins related functions */

    //! return current path for python plugins
    QString pluginsPath();

    //! return current path for home directory python plugins
    QString homePluginsPath();

    //! return list of all available python plugins
    QStringList pluginList();

    //! load python plugin (import)
    bool loadPlugin( QString packageName );

    //! start plugin: add to active plugins and call initGui()
    bool startPlugin( QString packageName );

    //! helper function to get some information about plugin
    //! @param function one of these strings: name, tpye, version, description
    QString getPluginMetadata( QString pluginName, QString function );

    //! unload plugin
    bool unloadPlugin( QString packageName );

  protected:

    void installErrorHook();

    void uninstallErrorHook();

    QString getTraceback();

    //! reference to module __main__
    PyObject* mMainModule;

    //! dictionary of module __main__
    PyObject* mMainDict;

    //! flag determining that python support is enabled
    bool mPythonEnabled;
};


#endif
