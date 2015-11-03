/*
 * Copyright (c) 2013 Raivis Strogonovs
 * http://morf.lv
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
*/

#include "smtp.h"
#include "qgslogger.h"
#include <QSslSocket>
#include <QFile>
#include <QFileInfo>

Smtp::Smtp( const QString &user, const QString &pass, const QString &host, int port, int timeout )
{
  mSocket = new QSslSocket( this );

  connect( mSocket, SIGNAL( readyRead() ), this, SLOT( readyRead() ) );
  connect( mSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SIGNAL( error( QAbstractSocket::SocketError ) ) );
  connect( mSocket, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ), this, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ) );

  mUser = user;
  mPass = pass;
  mHost = host;
  mPort = port;
  mTimeout = timeout;
}

void Smtp::sendMail( const QString &from, const QString &to, const QString &subject, const QString &body, const QString& attachment )
{
  mMessage = "To: " + to + "\n";
  mMessage.append( "From: " + from + "\n" );
  mMessage.append( "Subject: " + subject + "\n" );

  mMessage.append( "MIME-Version: 1.0\n" );
  mMessage.append( "Content-Type: multipart/mixed; boundary=frontier\n\n" );

  mMessage.append( "--frontier\n" );
  mMessage.append( "Content-Type: text/plain\n\n" );
  mMessage.append( body );
  mMessage.append( "\n\n" );

  QFile file( attachment );
  if ( file.exists() )
  {
    if ( !file.open( QIODevice::ReadOnly ) )
    {
      QgsDebugMsg( QString( "Failed to open attachment: %1" ).arg( attachment ) );
    }
    else
    {
      mMessage.append( "--frontier\n" );
      mMessage.append( "Content-Type: application/octet-stream\nContent-Disposition: attachment; filename=" + QFileInfo( file.fileName() ).fileName() + ";\nContent-Transfer-Encoding: base64\n\n" );
      mMessage.append( file.readAll().toBase64() );
      mMessage.append( "\n" );
    }
  }

  mMessage.append( "--frontier--\n" );


  mFrom = from;
  mTo = to;
  state = Init;
  mSocket->connectToHostEncrypted( mHost, mPort );
  if ( !mSocket->waitForConnected( mTimeout ) )
  {
    qDebug() << mSocket->errorString();
  }

  mTextStream = new QTextStream( mSocket );
}

Smtp::~Smtp()
{
  delete mTextStream;
}

void Smtp::readyRead()
{

  qDebug() << "readyRead";
  // SMTP is line-oriented

  QString responseLine;
  do
  {
    responseLine = mSocket->readLine();
    mResponse += responseLine;
  }
  while ( mSocket->canReadLine() && responseLine[3] != ' ' );

  responseLine.truncate( 3 );

  qDebug() << "Server response code:" <<  responseLine;
  qDebug() << "Server response: " << mResponse;

  if ( state == Init && responseLine == "220" )
  {
    // banner was okay, let's go on
    *mTextStream << "EHLO localhost" << "\r\n";
    mTextStream->flush();

    state = HandShake;
  }
  else if ( state == HandShake && responseLine == "250" )
  {
    mSocket->startClientEncryption();
    if ( !mSocket->waitForEncrypted( mTimeout ) )
    {
      qDebug() << mSocket->errorString();
      state = Close;
    }


    //Send EHLO once again but now encrypted

    *mTextStream << "EHLO localhost" << "\r\n";
    mTextStream->flush();
    state = Auth;
  }
  else if ( state == Auth && responseLine == "250" )
  {
    // Trying AUTH
    qDebug() << "Auth";
    *mTextStream << "AUTH LOGIN" << "\r\n";
    mTextStream->flush();
    state = User;
  }
  else if ( state == User && responseLine == "334" )
  {
    //Trying User
    qDebug() << "Username";
    //GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
    //https://developers.google.com/gmail/xoauth2_protocol
    *mTextStream << QByteArray().append( mUser ).toBase64()  << "\r\n";
    mTextStream->flush();

    state = Pass;
  }
  else if ( state == Pass && responseLine == "334" )
  {
    //Trying pass
    qDebug() << "Pass";
    *mTextStream << QByteArray().append( mPass ).toBase64() << "\r\n";
    mTextStream->flush();

    state = Mail;
  }
  else if ( state == Mail && responseLine == "235" )
  {
    // HELO response was okay (well, it has to be)

    //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
    qDebug() << "MAIL FROM:<" << mFrom << ">";
    *mTextStream << "MAIL FROM:<" << mFrom << ">\r\n";
    mTextStream->flush();
    state = Rcpt;
  }
  else if ( state == Rcpt && responseLine == "250" )
  {
    //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
    *mTextStream << "RCPT TO:<" << mTo << ">\r\n"; //r
    mTextStream->flush();
    state = Data;
  }
  else if ( state == Data && responseLine == "250" )
  {

    *mTextStream << "DATA\r\n";
    mTextStream->flush();
    state = Body;
  }
  else if ( state == Body && responseLine == "354" )
  {

    *mTextStream << mMessage << "\r\n.\r\n";
    mTextStream->flush();
    state = Quit;
  }
  else if ( state == Quit && responseLine == "250" )
  {

    *mTextStream << "QUIT\r\n";
    mTextStream->flush();
    // here, we just close.
    state = Close;
    emit status( tr( "Message sent" ) );
  }
  else if ( state == Close )
  {
    deleteLater();
    return;
  }
  else
  {
    // something broke.
    QMessageBox::warning( 0, tr( "Qt Simple SMTP client" ), tr( "Unexpected reply from SMTP server:\n\n" ) + mResponse );
    state = Close;
    emit status( tr( "Failed to send message" ) );
  }
  mResponse = "";
}
