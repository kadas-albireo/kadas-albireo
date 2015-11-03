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

#ifndef SMTP_H
#define SMTP_H

#include <QObject>
#include <QAbstractSocket>

class QTextStream;
class QSslSocket;

class Smtp : public QObject
{
    Q_OBJECT
  public:
    Smtp( const QString &mUser, const QString &mPass,
          const QString &mHost, int mPort = 465, int mTimeout = 30000 );
    ~Smtp();

    void sendMail( const QString &mFrom, const QString &to,
                   const QString &subject, const QString &body,
                   const QString &attachment = QStringList() );

  signals:
    void status( const QString & );
    void error( QAbstractSocket::SocketError );
    void stateChanged( QAbstractSocket::SocketState socketState );

  private slots:
    void readyRead();

  private:
    int mTimeout;
    QString mMessage;
    QTextStream *mTextStream;
    QSslSocket *mSocket;
    QString mFrom;
    QString mTo;
    QString mResponse;
    QString mUser;
    QString mPass;
    QString mHost;
    int mPort;

    enum states
    {
      Tls,
      HandShake,
      Auth,
      User,
      Pass,
      Rcpt,
      Mail,
      Data,
      Init,
      Body,
      Quit,
      Close
    } state;
};

#endif
