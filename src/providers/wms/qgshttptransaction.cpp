/***************************************************************************
  qgshttptransaction.cpp  -  Tracks a HTTP request with its response,
                             with particular attention to tracking 
                             HTTP redirect responses
                             -------------------
    begin                : 17 Mar, 2005
    copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au
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

#include <fstream>
#include <iostream>

#include "qgshttptransaction.h"

#include <qapplication.h>
#include <q3url.h>


QgsHttpTransaction::QgsHttpTransaction(QString uri, QString proxyHost, Q_UINT16 proxyPort)
  : httpurl(uri),
    httphost(proxyHost),
    httpport(proxyPort),
    httpresponsecontenttype(0)
{
#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction: constructing." << std::endl;
#endif
  

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction: exiting constructor." << std::endl;
#endif
  
}

QgsHttpTransaction::~QgsHttpTransaction()
{

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction: deconstructing." << std::endl;
#endif

 
}


void QgsHttpTransaction::getAsynchronously()
{

  //TODO
  
}

bool QgsHttpTransaction::getSynchronously(QByteArray &respondedContent, int redirections)
{

  httpredirections = redirections;
  
#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::getSynchronously: Entered." << std::endl;
  std::cout << "QgsHttpTransaction::getSynchronously: Using '" << httpurl.toLocal8Bit().data() << "'." << std::endl;
#endif

  Q3Url qurl(httpurl);
  QString path;
  
  if (httphost.isEmpty())
  {
    // No proxy was specified - connect directly to host in URI
    httphost = qurl.host();
    path = qurl.encodedPathAndQuery();
  }  
  else
  {
    // Proxy -> send complete URL
    path = httpurl;
  }
  http = new Q3Http( httphost, httpport );

#ifdef QGISDEBUG
  qWarning("QgsHttpTransaction::getSynchronously: qurl.host() is '"+qurl.host()+ "'.");
  qWarning("QgsHttpTransaction::getSynchronously: qurl.encodedPathAndQuery() is '"+qurl.encodedPathAndQuery()+"'.");
  std::cout << "path = " << path.ascii() << std::endl;
#endif
  
  httpresponse.truncate(0);
  httpid = http->get( path );
  httpactive = TRUE;
 
  connect(http, SIGNAL( requestStarted ( int ) ), 
          this,      SLOT( dataStarted ( int ) ) );
  
  connect(http, SIGNAL( responseHeaderReceived( const Q3HttpResponseHeader& ) ), 
          this,       SLOT( dataHeaderReceived( const Q3HttpResponseHeader& ) ) );
  
  connect(http,  SIGNAL( readyRead( const Q3HttpResponseHeader& ) ), 
          this, SLOT( dataReceived( const Q3HttpResponseHeader& ) ) );
  
  connect(http, SIGNAL( dataReadProgress ( int, int ) ), 
          this,       SLOT( dataProgress ( int, int ) ) );

  connect(http, SIGNAL( requestFinished ( int, bool ) ), 
          this,      SLOT( dataFinished ( int, bool ) ) );

  connect(http,   SIGNAL( stateChanged ( int ) ), 
          this, SLOT( dataStateChanged ( int ) ) );

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::getSynchronously: Starting get." << std::endl;
#endif

  // A little trick to make this function blocking
  while ( httpactive )
  {
    // Do something else, maybe even network processing events
    qApp->processEvents();
    
    // TODO: Implement a network timeout
  }

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::getSynchronously: Response received." << std::endl;
  
//  QString httpresponsestring(httpresponse);
//  std::cout << "QgsHttpTransaction::getSynchronously: Response received; being '" << httpresponsestring << "'." << std::endl;
#endif
  

  delete http;

  // Do one level of redirection
  // TODO make this recursable
  // TODO detect any redirection loops
  if (!httpredirecturl.isEmpty())
  {
#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::getSynchronously: Starting get of '" << 
               httpredirecturl.toLocal8Bit().data() << "'." << std::endl;
#endif

    QgsHttpTransaction httprecurse(httpredirecturl, httphost, httpport);

    // Do a passthrough for the status bar text
    connect(
            &httprecurse, SIGNAL(setStatus(QString)),
             this,        SIGNAL(setStatus(QString))
           );

    httprecurse.getSynchronously( respondedContent, (redirections + 1) );
    return TRUE;

  }

  respondedContent = httpresponse;
  return TRUE;

}


QString QgsHttpTransaction::responseContentType()  
{
  return httpresponsecontenttype;
}


void QgsHttpTransaction::dataStarted( int id )  
{

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::dataStarted with ID " << id << "." << std::endl;
#endif

 
}


void QgsHttpTransaction::dataHeaderReceived( const Q3HttpResponseHeader& resp )
{

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::dataHeaderReceived: statuscode " << 
    resp.statusCode() << ", reason '" << resp.reasonPhrase().toLocal8Bit().data() << "', content type: '" <<
    resp.value("Content-Type").toLocal8Bit().data() << "'." << std::endl;
#endif

  if (resp.statusCode() == 302) // Redirect
  {
    // Grab the alternative URL 
    // (ref: "http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html")
    httpredirecturl = resp.value("Location");
  }

  httpresponsecontenttype = resp.value("Content-Type");
  
}


void QgsHttpTransaction::dataReceived( const Q3HttpResponseHeader& resp )
{
  // TODO: Match 'resp' with 'http' if we move to multiple http connections

  /* Comment this out for now - leave the coding of progressive rendering to another day.
  char* temp;

  if (  0 < http->readBlock( temp, http->bytesAvailable() )  )
  {
    httpresponse.append(temp);
  }
  */

#ifdef QGISDEBUG
//  std::cout << "QgsHttpTransaction::dataReceived." << std::endl;
//  std::cout << "QgsHttpTransaction::dataReceived: received '" << data << "'."<< std::endl;
#endif
  
}


void QgsHttpTransaction::dataProgress( int done, int total )
{

#ifdef QGISDEBUG
//  std::cout << "QgsHttpTransaction::dataProgress: got " << done << " of " << total << std::endl;
#endif

  QString status;
  
  if (total)
  {
    status = QString("Received %1 of %2 bytes")
                         .arg( done )
                         .arg( total );
  }
  else
  {
    status = QString("Received %1 bytes (total unknown)")
                         .arg( done );
  }

  emit setStatus( status );
}

void QgsHttpTransaction::dataFinished( int id, bool error )  
{

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::dataFinished with ID " << id << "." << std::endl;

  if (error)
  {
    std::cout << "QgsHttpTransaction::dataFinished - however there was an error." << std::endl;
    std::cout << "QgsHttpTransaction::dataFinished - " << http->errorString().toLocal8Bit().data() << std::endl;
  } else
  {
    std::cout << "QgsHttpTransaction::dataFinished - no error." << std::endl;
  }
#endif

  // TODO
  httpresponse = http->readAll();

  httpactive = FALSE;
  
}

void QgsHttpTransaction::dataStateChanged( int state )
{

#ifdef QGISDEBUG
  std::cout << "QgsHttpTransaction::dataStateChanged to " << state << "." << std::endl << "  ";
#endif

  switch (state)
  {
    case Q3Http::Unconnected:
#ifdef QGISDEBUG
      std::cout << "There is no connection to the host." << std::endl;
#endif

      emit setStatus( QString("Not connected") );
      break;

    case Q3Http::HostLookup:
#ifdef QGISDEBUG
      std::cout << "A host name lookup is in progress." << std::endl;
#endif

      emit setStatus( QString("Looking up '%1'")
                         .arg(httphost) );
      break;

    case Q3Http::Connecting:
#ifdef QGISDEBUG
      std::cout << "An attempt to connect to the host is in progress." << std::endl;
#endif

      emit setStatus( QString("Connecting to '%1'")
                         .arg(httphost) );
      break;

    case Q3Http::Sending:
#ifdef QGISDEBUG
      std::cout << "The client is sending its request to the server." << std::endl;
#endif

      emit setStatus( QString("Sending request '%1'")
                         .arg(httpurl) );
      break;

    case Q3Http::Reading:
#ifdef QGISDEBUG
      std::cout << "The client's request has been sent and the client "
                   "is reading the server's response." << std::endl;
#endif

      emit setStatus( QString("Receiving reply") );
      break;

    case Q3Http::Connected:
#ifdef QGISDEBUG
      std::cout << "The connection to the host is open, but the client "
                   "is neither sending a request, nor waiting for a response." << std::endl;
#endif

      emit setStatus( QString("Response is complete") );
      break;

    case Q3Http::Closing:
#ifdef QGISDEBUG
      std::cout << "The connection is closing down, but is not yet closed. "
                   "(The state will be Unconnected when the connection is closed.)" << std::endl;
#endif

      emit setStatus( QString("Closing down connection") );
      break;
  }

}


QString QgsHttpTransaction::errorString()
{
  return mError;
}

// ENDS
