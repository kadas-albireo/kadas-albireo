/***************************************************************************
 *  qgsvbscrashhandler.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Sep 15, 2015                                    *
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

#include "qgsvbscrashhandler.h"
#include "qgis.h"
#include "qgsproject.h"
#include "qgslogger.h"
#ifdef _MSC_VER
#include <CrashRpt.h>
#endif

#ifdef _MSC_VER
// Define the callback function that will be called on crash
int CALLBACK CrashCallback(CR_CRASH_CALLBACK_INFO* pInfo)
{
  // TODO: Attempt to save the project
  // QgsProject::instance()->

  // Return CR_CB_DODEFAULT to generate error report
  return CR_CB_DODEFAULT;
}
#endif


QgsVBSCrashHandler::QgsVBSCrashHandler()
	: mHandlerInstalled(false)
{
#ifdef _MSC_VER
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO)); 
    info.cb = sizeof(CR_INSTALL_INFO);
    info.pszAppName = _strdup(QString("QGIS %1").arg(QGis::QGIS_RELEASE_NAME).toLocal8Bit().data());
    info.pszAppVersion = _strdup(QString("%1 (%2)").arg(QGis::QGIS_VERSION).arg(QGis::QGIS_DEV_VERSION).toLocal8Bit().data());
	info.pszUrl = _strdup(QString("http://www.sourcepole.com").toLocal8Bit().data());
	info.pszEmailTo = _strdup(QString("smani@sourcepole.ch").toLocal8Bit().data());
	info.pszEmailSubject = _strdup(QString("QGIS %1 Error Report").arg(QGis::QGIS_RELEASE_NAME).toLocal8Bit().data());
    info.dwFlags = 0;
    info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS; // Install all available exception handlers.
    info.dwFlags |= CR_INST_APP_RESTART; // Restart on crash
	info.dwFlags |= CR_INST_AUTO_THREAD_HANDLERS; // Automatically install handlers to threads
	info.dwFlags |= CR_INST_SHOW_ADDITIONAL_INFO_FIELDS;
    info.uPriorities[CR_HTTP] = CR_NEGATIVE_PRIORITY; // Disabled
    info.uPriorities[CR_SMTP] = 1;
    info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY; // Disabled
 
    int nResult;
	nResult = crInstall(&info);
 
    if(nResult!=0)
	{
        TCHAR buff[512];
        crGetLastErrorMsg(buff, sizeof(buff));
		QgsDebugMsg(QString("Failed to install crash reporter: %1").arg(QString::fromLocal8Bit(buff, sizeof(buff))));
        return;
    }
	else
	{
		QgsDebugMsg("Crash reporter installed");
		crSetCrashCallback(CrashCallback, 0);
		mHandlerInstalled = true;
	}
#endif
}

QgsVBSCrashHandler::~QgsVBSCrashHandler()
{
#ifdef _MSC_VER
	if(mHandlerInstalled)
		crUninstall();
#endif
}

