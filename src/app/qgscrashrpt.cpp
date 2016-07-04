/***************************************************************************
 *  qgscrashrpt.cpp                                                        *
 *  ---------------                                                        *
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

#include "qgscrashrpt.h"
#include "qgis.h"
#include "qgsproject.h"
#include "qgslogger.h"
#ifdef _MSC_VER
#include <CrashRpt.h>
#endif
#include <QSettings>

#ifdef _MSC_VER
// Define the callback function that will be called on crash
int CALLBACK CrashCallback( CR_CRASH_CALLBACK_INFO* pInfo )
{
  // TODO: Attempt to save the project
  // QgsProject::instance()->

  // Return CR_CB_DODEFAULT to generate error report
  return CR_CB_DODEFAULT;
}
#endif


QgsCrashRpt::QgsCrashRpt()
    : mHandlerInstalled( false )
{
}

QgsCrashRpt::~QgsCrashRpt()
{
#ifdef _MSC_VER
  if ( mHandlerInstalled )
    crUninstall();
#endif
}

bool QgsCrashRpt::install()
{
#ifdef _MSC_VER
  QString submitUrl = QSettings().value( "/Qgis/crashrpt_url" ).toString();
  if ( submitUrl.isEmpty() )
  {
    QgsDebugMsg( "Failed to install crash reporter: submit url is empty" );
    return false;
  }
  CR_INSTALL_INFO info;
  memset( &info, 0, sizeof( CR_INSTALL_INFO ) );
  info.cb = sizeof( CR_INSTALL_INFO );
  info.pszAppName = _strdup( QString( "%1" ).arg( QGis::QGIS_FULL_RELEASE_NAME ).toLocal8Bit().data() );
  info.pszAppVersion = _strdup( QString( "%1 (%2)" ).arg( QGis::QGIS_BUILD_DATE ).arg( QGis::QGIS_DEV_VERSION ).toLocal8Bit().data() );
  info.pszUrl = _strdup( submitUrl.toLocal8Bit().data() );
  info.dwFlags = 0;
  info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS; // Install all available exception handlers.
  info.dwFlags |= CR_INST_AUTO_THREAD_HANDLERS; // Automatically install handlers to threads
  info.dwFlags |= CR_INST_SHOW_ADDITIONAL_INFO_FIELDS;
  info.dwFlags |= CR_INST_SEND_QUEUED_REPORTS;
  info.uPriorities[CR_HTTP] = 1;
  info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY; // Disabled
  info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY; // Disabled

  int nResult;
  nResult = crInstall( &info );

  if ( nResult != 0 )
  {
    TCHAR buff[512];
    crGetLastErrorMsg( buff, sizeof( buff ) );
    QgsDebugMsg( QString( "Failed to install crash reporter: %1" ).arg( QString::fromLocal8Bit( buff, sizeof( buff ) ) ) );
    return false;
  }
  else
  {
    QgsDebugMsg( "Crash reporter installed" );
    crSetCrashCallback( CrashCallback, 0 );
    mHandlerInstalled = true;
    return true;
  }
#endif
  return false;
}
