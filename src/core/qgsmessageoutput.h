/***************************************************************************
    qgsmessageoutput.h  -  interface for showing messages
    ----------------------
    begin                : April 2006
    copyright            : (C) 2006 by Martin Dobias
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


#ifndef QGSMESSAGEOUTPUT_H
#define QGSMESSAGEOUTPUT_H

#include <QString>
#include <QObject>

class QgsMessageOutput;
typedef QgsMessageOutput*( *MESSAGE_OUTPUT_CREATOR )();


/** \ingroup core
 * Interface for showing messages from QGIS in GUI independent way.
 * This class provides abstraction of a dialog for showing output to the user.
 * By default QgsMessageConsoleOutput will be used if not overridden with other
 * message output creator function.

 * QGIS application uses QgsMessageView class for displaying a dialog to the user.

 * Object deletes itself when it's not needed anymore. Children should use
 * signal destroyed() to notify the deletion
*/
class CORE_EXPORT QgsMessageOutput
{
  public:

    //! message can be in plain text or in html format
    enum MessageType { MessageText, MessageHtml };

    //! virtual destructor
    virtual ~QgsMessageOutput();

    //! set message, it won't be displayed until
    virtual void setMessage( const QString& message, MessageType msgType ) = 0;

    //! message to be appended to the current text
    virtual void appendMessage( const QString& message ) = 0;

    //! set title for the messages
    virtual void setTitle( const QString& title ) = 0;

    //! display the message to the user
    virtual void showMessage( bool blocking = true ) = 0;

    //! sets function that will be used to create message output
    static void setMessageOutputCreator( MESSAGE_OUTPUT_CREATOR f );

    //! function that returns new class derived from QgsMessageOutput
    //! (don't forget to delete it then)
    static QgsMessageOutput* createMessageOutput();

  private:

    //! Pointer to the function which creates the class for output
    static MESSAGE_OUTPUT_CREATOR mMessageOutputCreator;
};


/**
\brief Default implementation of message output interface

This class outputs messages to the standard output. Therefore it might
be the right choice for apps without GUI.
*/
class CORE_EXPORT QgsMessageOutputConsole : public QObject, public QgsMessageOutput
{
    Q_OBJECT

  public:

    QgsMessageOutputConsole();

    virtual void setMessage( const QString& message, MessageType msgType );

    virtual void appendMessage( const QString& message );

    virtual void setTitle( const QString& title );

    //! sends the message to the standard output
    virtual void showMessage( bool blocking = true );

  signals:

    //! signals that object will be destroyed and shouldn't be used anymore
    void destroyed();

  private:

    //! stores current message
    QString mMessage;

    //! stores current title
    QString mTitle;

};

#endif
