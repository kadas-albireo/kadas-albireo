/***************************************************************************
 *  qgsiamauth.h                                                           *
 *  ------------                                                           *
 *  begin                : March 2016                                      *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#include "qgsiamauth.h"
#include "qgisinterface.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsmessagebar.h"
#include "iamauth_plugin.h"
#include "webaxwidget.h"

#include <ActiveQt/QAxObject>
#include <QDialog>
#include <QMessageBox>
#include <QNetworkCookieJar>
#include <QSslConfiguration>
#include <QStackedLayout>
#include <QSettings>
#include <QToolButton>
#include <QUuid>
#include <QWebView>
#include <QWebPage>

StackedDialog::StackedDialog( QWidget* parent, Qt::WindowFlags flags )
    : QDialog( parent, flags )
{
  mLayout = new QStackedLayout();
  setLayout( mLayout );
}

void StackedDialog::pushWidget( QWidget *widget )
{
  mLayout->setCurrentIndex( mLayout->addWidget( widget ) );
}

void StackedDialog::popWidget( QWidget *widget )
{
  if ( mLayout->currentWidget() == widget )
  {
    mLayout->setCurrentIndex( mLayout->currentIndex() - 1 );
  }
  mLayout->removeWidget( widget );
  widget->deleteLater();
}

QgsIAMAuth::QgsIAMAuth( QgisInterface * theQgisInterface )
    : QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType )
    , mQGisIface( theQgisInterface ), mLoginButton( 0 ), mLoginDialog( 0 )
{
}

void QgsIAMAuth::initGui()
{
  mLoginButton = qobject_cast<QToolButton*>( mQGisIface->findObject( "mLoginButton" ) );
  if ( mLoginButton )
  {
    connect( mLoginButton, SIGNAL( clicked( bool ) ), this, SLOT( performLogin() ) );
  }
}

void QgsIAMAuth::unload()
{
  disconnect( mLoginButton, SIGNAL( clicked( bool ) ), this, SLOT( performLogin() ) );
  mLoginButton = 0;
  delete mLoginDialog;
  mLoginDialog = 0;
}

void QgsIAMAuth::performLogin()
{
  mLoginDialog = new StackedDialog( mQGisIface->mainWindow() );
  mLoginDialog->setWindowTitle( tr( "eIAM Authentication" ) );
  mLoginDialog->resize( 1000, 480 );

  WebAxWidget* webWidget = new WebAxWidget();
  webWidget->setControl( QString::fromUtf8( "{8856F961-340A-11D0-A96B-00C04FD705A2}" ) );
  webWidget->dynamicCall( "Navigate(const QString&)", QSettings().value( "iamauth/loginurl", QString( "" ) ).toString() );
  connect( webWidget, SIGNAL( NavigateComplete( QString ) ), this, SLOT( checkLoginComplete( QString ) ) );
  connect( webWidget, SIGNAL( NewWindow3( IDispatch**, bool&, uint, QString, QString ) ), this, SLOT( handleNewWindow( IDispatch**, bool&, uint, QString, QString ) ) );
  connect( webWidget, SIGNAL( WindowClosing( bool, bool& ) ), this, SLOT( handleWindowClose( bool, bool& ) ) );
  mLoginDialog->pushWidget( webWidget );
  mLoginDialog->exec();
}

void QgsIAMAuth::checkLoginComplete( QString /*addr*/ )
{
  if ( mLoginDialog )
  {
    WebAxWidget* webWidget = static_cast<WebAxWidget*>( QObject::sender() );
    QUrl url( webWidget->dynamicCall( "LocationURL()" ).toString() );
    QUrl baseUrl;
    baseUrl.setScheme( url.scheme() );
    baseUrl.setHost( url.host() );
    QAxObject* document = webWidget->querySubObject( "Document()" );
    QStringList cookies = document->property( "cookie" ).toString().split( QRegExp( "\\s*;\\s*" ) );
    foreach ( const QString& cookie, cookies )
    {
      QStringList pair = cookie.split( "=" );
      if ( !pair.isEmpty() && pair.first() == "esri_auth" )
      {
//        QMessageBox::information( mLoginDialog, tr( "Authentication successful" ), QString( "Url: %1\nCookie: %2" ).arg( baseUrl.toString() ).arg( cookie ) );
        mQGisIface->messageBar()->pushMessage( tr( "Authentication successful" ), QgsMessageBar::INFO, 5 );
        mLoginDialog->accept();
        mLoginDialog->deleteLater();
        mLoginDialog = 0;
        QNetworkCookieJar* jar = QgsNetworkAccessManager::instance()->cookieJar();
        QStringList cookieUrls = QSettings().value( "iamauth/cookieurls", QString( "" ) ).toString().split( ";" );
        foreach ( const QString& url, cookieUrls )
        {
          jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie.toLocal8Bit() ), url );
        }
        QToolButton* loginButton = qobject_cast<QToolButton*>( mQGisIface->findObject( "mRefreshCatalogButton" ) );
        if ( loginButton )
        {
          loginButton->click();
        }
        /*
        QWebView* view = new QWebView();
        QWebPage* page = new QWebPage();
        page->setNetworkAccessManager( QgsNetworkAccessManager::instance() );
        view->setPage( page );
        QNetworkRequest req;
        req.setUrl( QUrl( "https://www.arcgis.com/sharing/rest/search" ) );
        QSslConfiguration conf = req.sslConfiguration();
        conf.setPeerVerifyMode( QSslSocket::VerifyNone );
        req.setSslConfiguration( conf );
        view->load( req );
        view->show();
        view->setAttribute( Qt::WA_DeleteOnClose );*/
      }
    }
  }
}

void QgsIAMAuth::handleNewWindow( IDispatch** ppDisp, bool& cancel, uint /*dwFlags*/, QString /*bstrUrlContext*/, QString /*bstrUrl*/ )
{
  if ( mLoginDialog )
  {
    WebAxWidget* webWidget = new WebAxWidget;
    webWidget->setControl( QString::fromUtf8( "{8856F961-340A-11D0-A96B-00C04FD705A2}" ) );
    connect( webWidget, SIGNAL( NavigateComplete( QString ) ), this, SLOT( checkLoginComplete( QString ) ) );
    connect( webWidget, SIGNAL( NewWindow3( IDispatch**, bool&, uint, QString, QString ) ), this, SLOT( handleNewWindow( IDispatch**, bool&, uint, QString, QString ) ) );
    connect( webWidget, SIGNAL( WindowClosing( bool, bool& ) ), this, SLOT( handleWindowClose( bool, bool& ) ) );
    IDispatch* appDisp;
    QAxObject* appDispObj = webWidget->querySubObject( "Application()" );
    appDispObj->queryInterface( IID_IDispatch, ( void** )&appDisp );
    *ppDisp = appDisp;
    mLoginDialog->pushWidget( webWidget );
  }
}

void QgsIAMAuth::handleWindowClose( bool /*isChild*/, bool& /*cancel*/ )
{
  if ( mLoginDialog )
  {
    mLoginDialog->popWidget( qobject_cast<QWidget*>( QObject::sender() ) );
  }
}
