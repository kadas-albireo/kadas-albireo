/***************************************************************************
                              qgsmapserverlogger.cpp
                              ----------------------
  begin                : July 04, 2006
  copyright            : (C) 2006 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmapserverlogger.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

QgsMapServerLogger* QgsMapServerLogger::mInstance = 0;

QgsMapServerLogger::QgsMapServerLogger()
{

}
QgsMapServerLogger::~QgsMapServerLogger()
{
  delete mInstance;
}

QgsMapServerLogger* QgsMapServerLogger::instance()
{
  if ( mInstance == 0 )
  {
    mInstance = new QgsMapServerLogger();
  }
  return mInstance;
}

int QgsMapServerLogger::setLogFilePath( const QString& path )
{
  mLogFile.setFileName( path );
  if ( mLogFile.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append ) )
  {
    mTextStream.setDevice( &mLogFile );
    return 0;
  }
  return 1;
}

void QgsMapServerLogger::printMessage( const QString& message )
{
  if ( !mLogFile.isOpen() )
  {
#ifdef _MSC_VER
    ::OutputDebugString( message .toLocal8Bit() );
    ::OutputDebugString( "\n" );
#endif
    return;
  }

  mTextStream << message << endl;
}

void QgsMapServerLogger::printChar( QChar c )
{
  if ( !mLogFile.isOpen() )
    return;

  mTextStream << c;
}

#ifdef QGSMSDEBUG
void QgsMapServerLogger::printMessage( const char *file, const char *function, int line, const QString& message )
{
#if defined(_MSC_VER)
  printMessage( QString( "%1(%2): (%3) %4" ).arg( file ).arg( line ).arg( function ).arg( message ) );
#else
  printMessage( QString( "%1: %2: (%3) %4" ).arg( file ).arg( line ).arg( function ).arg( message ) );
#endif
}
#endif
