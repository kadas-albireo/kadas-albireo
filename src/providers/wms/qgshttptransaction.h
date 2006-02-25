/***************************************************************************
  qgshttptransaction.h  -  Tracks a HTTP request with its response,
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

#ifndef QGSHTTPTRANSACTION_H
#define QGSHTTPTRANSACTION_H

#include <q3http.h>
#include <qstring.h>

/**

  \brief  HTTP request/response manager that is redirect-aware.
 
  This class extends the Qt QHttp concept by being able to recognise
  and respond to redirection responses (e.g. HTTP code 302)
  
  
  TODO: Make it work
  
*/

class QgsHttpTransaction : public QObject
{
  
  Q_OBJECT

public:
  /**
  * Constructor.
  */
  QgsHttpTransaction( QString uri, QString proxyHost = 0, Q_UINT16 proxyPort = 80 );

  //! Destructor
  virtual ~QgsHttpTransaction();
  
  void getAsynchronously();
  
  //! Gets the response synchronously.  Note that signals will still be emitted
  //! while in this function.

  /*!
      The function returns FALSE if there is an error while getting the response.
      \param[out] respondedContent is replaced with the new content.
      \param[in]  redirections     is used to measure how many http redirections we've been through.
                                   Clients typically don't need to set this.
   */
  bool getSynchronously(QByteArray &respondedContent, int redirections = 0);

  QString responseContentType();

  /**
   * If an operation returns 0 (e.g. getSynchronously()), this function
   * returns the text of the error associated with the failure.
   * Interactive users of this provider can then, for example,
   * call a QMessageBox to display the contents.
   */
  QString errorString();


public slots:

  void dataStarted( int id );
  
  void dataHeaderReceived( const Q3HttpResponseHeader& resp );
  
  void dataReceived( const Q3HttpResponseHeader& resp );

  void dataProgress( int done, int total );

  void dataFinished( int id, bool error );

  void dataStateChanged( int state );

  
signals:

    /** \brief emit a signal to notify of a progress event */
    void setProgress(int theProgress, int theTotalSteps);

    /** \brief emit a signal to be caught by qgisapp and display a msg on status bar */
    void setStatus(QString theStatusQString);


private:
  
  /**
   * Indicates the associated QHttp object
   *
   * \note  We tried to use this as a plain QHttp object
   *        but strange things were happening with the signals -
   *        therefore we use the "pointer to" instead.
   */
  Q3Http* http;

  /**
   * Indicates the QHttp ID
   */
  int httpid;
  
  /**
   * Indicates if the transaction is in progress
   */
  bool httpactive;

  /*
   * Indicates the response from the QHttp 
   */ 
  QByteArray httpresponse;
  
  /*
   * Indicates the content type of the response from the QHttp 
   */ 
  QString    httpresponsecontenttype;
    
  /**
   * The original URL requested for this transaction
   */
  QString httpurl;
  
  /**
   * The host being used for this transaction
   */
  QString httphost;

  /**
   * The port being used for this transaction
   */
  Q_UINT16 httpport;
  
  /**
   * If not empty, indicates that the QHttp is a redirect
   * to the contents of this variable
   */
  QString httpredirecturl;

  /**
   * Number of http redirections this transaction has been
   * subjected to.
   *
   * TODO: Use this as part of a redirection loop detector
   *
   */
  int httpredirections;

  /**
   * The error message associated with the last HTTP error.
   */
  QString mError;

};

#endif

// ENDS
