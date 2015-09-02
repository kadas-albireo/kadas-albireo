/***************************************************************************
    qgscredentials.cpp -  interface for requesting credentials
    ----------------------
    begin                : February 2010
    copyright            : (C) 2010 by Juergen E. Fischer
    email                : jef at norbit dot de
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscredentials.h"
#include "qgslogger.h"

#include <QTextStream>

QgsCredentials *QgsCredentials::smInstance = 0;

void QgsCredentials::setInstance( QgsCredentials *theInstance )
{
  if ( smInstance )
  {
    QgsDebugMsg( "already registered an instance of QgsCredentials" );
  }

  smInstance = theInstance;
}

QgsCredentials *QgsCredentials::instance()
{
  if ( smInstance )
    return smInstance;

  return new QgsCredentialsConsole();
}

bool QgsCredentials::get( QString realm, QString &username, QString &password, QString message, bool interactive )
{
  QMap< QString, QPair<QString, QString> >::iterator it = mCredentialCache.find( realm );

  // If credentials are cached and not equal to the passed ones, try with those
  if ( it != mCredentialCache.end() && username != it.value().first && password != it.value().second )
  {
    QgsDebugMsg( QString( "using cached credentials realm:%1 username:%2 password:%3" ).arg( realm ).arg( username ).arg( password ) );
    username = it.value().first;
    password = it.value().second;
    return true;
  }
  // Else, if in interactive mode, request credentials from user
  else if ( interactive )
  {
    QgsDebugMsg( "Requesting credentials from user" );
    return request( realm, username, password, message );
  }
  // Else, fail
  else
  {
    return false;
  }
}

void QgsCredentials::put( QString realm, QString username, QString password )
{
  QgsDebugMsg( QString( "inserting realm:%1 username:%2 password:%3" ).arg( realm ).arg( username ).arg( password ) );
  mCredentialCache.insert( realm, QPair<QString, QString>( username, password ) );
}

void QgsCredentials::lock()
{
  mMutex.lock();
}

void QgsCredentials::unlock()
{
  mMutex.unlock();
}


////////////////////////////////
// QgsCredentialsConsole

QgsCredentialsConsole::QgsCredentialsConsole()
{
  setInstance( this );
}

bool QgsCredentialsConsole::request( QString realm, QString &username, QString &password, QString message )
{
  QTextStream in( stdin, QIODevice::ReadOnly );
  QTextStream out( stdout, QIODevice::WriteOnly );

  out << "credentials for " << realm << endl;
  if ( !message.isEmpty() )
    out << "message: " << message << endl;
  out << "username: ";
  in >> username;
  out << "password: ";
  in >> password;

  return true;
}
