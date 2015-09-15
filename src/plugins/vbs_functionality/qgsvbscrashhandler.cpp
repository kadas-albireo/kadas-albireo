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
#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <SMTPEmail/SmtpMime>
#include <QSettings>

#ifdef __MSC_VER
#include <windows.h>
#else
#include <csignal>
#endif

QgsVBSCrashHandler::QgsVBSCrashHandler( const QString& coredumpFile, const QString& coredumpError, QWidget *parent ):
    QDialog( parent ), mCoredumpFile( coredumpFile ), mCoredumpError( coredumpError )
{
  setWindowTitle( tr( "Crash Handler" ) );

  setLayout( new QVBoxLayout() );
  QLabel* label = new QLabel( tr( "The application has crashed. Apologies for the inconvenience. Please describe what you were doing in the field below and submit the error report." ) );
  label->setWordWrap( true );
  layout()->addWidget( label );
  mTextEditUserMessage = new QPlainTextEdit();
  layout()->addWidget( mTextEditUserMessage );
  layout()->addWidget( new QLabel( QString( tr( "Version code: %1" ) ).arg( QGis::QGIS_DEV_VERSION ) ) );
  mButtonSubmit = new QPushButton( tr( "Submit" ) );
  mButtonClose = new QPushButton( tr( "Close" ) );
  QDialogButtonBox* buttonBox = new QDialogButtonBox();
  buttonBox->addButton( mButtonSubmit, QDialogButtonBox::ActionRole );
  buttonBox->addButton( mButtonClose, QDialogButtonBox::RejectRole );
  layout()->addWidget( buttonBox );

  connect( mButtonClose, SIGNAL( clicked( bool ) ), this, SLOT( reject() ) );
  connect( mButtonSubmit, SIGNAL( clicked( bool ) ), this, SLOT( submitCrashReport() ) );
}

void QgsVBSCrashHandler::submitCrashReport()
{
  setEnabled( false );
  QApplication::setOverrideCursor( Qt::BusyCursor );
  QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
  QSettings settings;
  QString server = settings.value( "/vbsfunctionality/crashreporter/server", "" ).toString();
  int serverPort = settings.value( "/vbsfunctionality/crashreporter/serverPort", "" ).toInt();
  QString fromEmail = settings.value( "/vbsfunctionality/crashreporter/from_email", "" ).toString();
  QByteArray fromEmailPwd  = settings.value( "/vbsfunctionality/crashreporter/from_email_pwd", "" ).toByteArray();
  QString toEmail = settings.value( "/vbsfunctionality/crashreporter/to_email", "" ).toString();

  SmtpClient smtp( server, serverPort, SmtpClient::SslConnection );
  smtp.setUser( fromEmail );
  smtp.setPassword( QByteArray::fromBase64( fromEmailPwd ) );

  EmailAddress sender( fromEmail, "QGIS KADAS Crash Reporter" );
  EmailAddress to( toEmail, toEmail );

  MimeMessage message;
  message.setSender( &sender );
  message.addRecipient( &to );
  message.setSubject( "QGIS KADAS Crash Report" );

  QString dumpError;
  if ( !mCoredumpError.isEmpty() )
  {
    dumpError += "\n\nCoredump generation failed: " + mCoredumpError;
  }

  MimeText text;
  text.setText(
    QString( "QGIS Version: %1\n\nMessage:\n%2%3" )
    .arg( QGis::QGIS_DEV_VERSION ).arg( mTextEditUserMessage->toPlainText() ).arg( dumpError )
  );
  message.addPart( &text );

  QScopedPointer<MimeAttachment> attachment;
  if ( !mCoredumpError.isEmpty() )
  {
    attachment.reset( new MimeAttachment( new QFile( mCoredumpFile ) ) );
    attachment->setContentType( "application/octet-stream" );
    message.addPart( attachment.data() );
  }

  QString errorMsg;
  if ( !smtp.connectToHost() )
  {
    errorMsg = tr( "Failed to connect to mail server." );
  }
  else if ( !smtp.login() )
  {
    errorMsg = tr( "Login failed." );
  }
  else if ( !smtp.sendMail( message ) )
  {
    errorMsg = tr( "Failed to send email." );
  }
  QApplication::restoreOverrideCursor();
  setEnabled( true );

  if ( !errorMsg.isEmpty() )
  {
    QMessageBox::warning( this, tr( "Submit failed" ), tr( "The error report could not be submitted:\n%1\n\nCheck your network connectivity and try again." ).arg( errorMsg ) );
  }
  else
  {
    QMessageBox::information( this, tr( "Submit succeeded" ), tr( "The error report has been successfully submitted." ) );
    accept();
  }
}

#ifdef __MSC_VER
LONG WINAPI QgsVBSCrashHandler::crashHandler( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
  QString dumpName = QDir::toNativeSeparators(
                       QString( "%1\\qgis-%2-%3-%4-%5.dmp" )
                       .arg( QDir::tempPath() )
                       .arg( QDateTime::currentDateTime().toString( "yyyyMMdd-hhmmss" ) )
                       .arg( GetCurrentProcessId() )
                       .arg( GetCurrentThreadId() )
                       .arg( QGis::QGIS_DEV_VERSION )
                     );

  QString errorMsg;
  HANDLE hDumpFile = CreateFile( dumpName.toLocal8Bit(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0 );
  if ( hDumpFile != INVALID_HANDLE_VALUE )
  {
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;
    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = ExceptionInfo;
    ExpParam.ClientPointers = TRUE;

    if ( !MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, ExceptionInfo ? &ExpParam : NULL, NULL, NULL ) )
    {
      errorMsg = QObject::tr( "Writing of minidump to %1 failed: %2" ).arg( dumpName ).arg( GetLastError(), 0, 16 );
    }

    CloseHandle( hDumpFile );
  }
  else
  {
    errorMsg = QObject::tr( "Creation of minidump to %1 failed: %2" ).arg( dumpName ).arg( GetLastError(), 0, 16 );
  }

  QgsVBSCrashHandler( dumpName, errorMsg ).exec();

  return EXCEPTION_EXECUTE_HANDLER;
}
#else
void QgsVBSCrashHandler::crashHandler( int sig )
{
  std::signal( sig, 0 );
  QgsVBSCrashHandler( "", "Not implemented" ).exec();
  abort();
}
#endif

void QgsVBSCrashHandler::installCrashHandler()
{
#ifdef __MSC_VER
  SetUnhandledExceptionFilter( crashHandler );
#else
  std::signal( SIGQUIT, crashHandler );
  std::signal( SIGILL, crashHandler );
  std::signal( SIGFPE, crashHandler );
  std::signal( SIGSEGV, crashHandler );
  std::signal( SIGBUS, crashHandler );
  std::signal( SIGSYS, crashHandler );
  std::signal( SIGTRAP, crashHandler );
  std::signal( SIGXCPU, crashHandler );
  std::signal( SIGXFSZ, crashHandler );
  std::signal( SIGABRT, crashHandler );
#endif
}
