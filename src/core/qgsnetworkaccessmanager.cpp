/***************************************************************************
                       qgsnetworkaccessmanager.cpp
  This class implements a QNetworkManager with the ability to chain in
  own proxy factories.

                              -------------------
          begin                : 2010-05-08
          copyright            : (C) 2010 by Juergen E. Fischer
          email                : jef at norbit dot de

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgsnetworkaccessmanager.h>

#include <qgsapplication.h>
#include <qgscredentials.h>
#include <qgsmessagelog.h>
#include <qgslogger.h>
#include <qgis.h>
#include "qgsnetworkdiskcache.h"

#include <QAuthenticator>
#include <QUrl>
#include <QSettings>
#include <QTimer>
#include <QNetworkReply>
#include <QThreadStorage>
#include <QSslError>
#include <QMessageBox>

QgsNetworkAccessManager *QgsNetworkAccessManager::smMainNAM = 0;

class QgsNetworkAccessManager::QgsSSLIgnoreList
{
  public:
    static void lock() { instance().mMutex.lock(); }
    static void unlock() { instance().mMutex.unlock(); }
    static QSet<QSslError::SslError>& ignoreErrors() { return instance().mIgnoreErrors; }

  private:
    static QgsSSLIgnoreList& instance()
    {
      static QgsSSLIgnoreList i;
      return i;
    }

    QMutex mMutex;
    QSet<QSslError::SslError> mIgnoreErrors;
};


class QgsNetworkProxyFactory : public QNetworkProxyFactory
{
  public:
    QgsNetworkProxyFactory() {}
    virtual ~QgsNetworkProxyFactory() {}

    virtual QList<QNetworkProxy> queryProxy( const QNetworkProxyQuery & query = QNetworkProxyQuery() ) override
    {
      QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();

      // iterate proxies factories and take first non empty list
      foreach ( QNetworkProxyFactory *f, nam->proxyFactories() )
      {
        QList<QNetworkProxy> systemproxies = f->systemProxyForQuery( query );
        if ( systemproxies.size() > 0 )
          return systemproxies;

        QList<QNetworkProxy> proxies = f->queryProxy( query );
        if ( proxies.size() > 0 )
          return proxies;
      }

      // no proxies from the proxy factor list check for excludes
      if ( query.queryType() != QNetworkProxyQuery::UrlRequest )
        return QList<QNetworkProxy>() << nam->fallbackProxy();

      QString url = query.url().toString();

      foreach ( QString exclude, nam->excludeList() )
      {
        if ( url.startsWith( exclude ) )
        {
          QgsDebugMsg( QString( "using default proxy for %1 [exclude %2]" ).arg( url ).arg( exclude ) );
          return QList<QNetworkProxy>() << QNetworkProxy();
        }
      }

      if ( nam->useSystemProxy() )
      {
        QgsDebugMsg( QString( "requesting system proxy for query %1" ).arg( url ) );
        QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery( query );
        if ( !proxies.isEmpty() )
        {
          QgsDebugMsg( QString( "using system proxy %1:%2 for query" )
                       .arg( proxies.first().hostName() ).arg( proxies.first().port() ) );
          return proxies;
        }
      }

      QgsDebugMsg( QString( "using fallback proxy for %1" ).arg( url ) );
      return QList<QNetworkProxy>() << nam->fallbackProxy();
    }
};

QgsNetworkAccessManager* QgsNetworkAccessManager::instance()
{
  static QThreadStorage<QgsNetworkAccessManager> sInstances;
  QgsNetworkAccessManager *nam = &sInstances.localData();

  if ( nam->thread() == qApp->thread() )
    smMainNAM = nam;

  if ( !nam->mInitialized )
    nam->setupDefaultProxyAndCache();

  return nam;
}

QgsNetworkAccessManager::QgsNetworkAccessManager( QObject *parent )
    : QNetworkAccessManager( parent )
    , mUseSystemProxy( false )
    , mInitialized( false )
{
  setProxyFactory( new QgsNetworkProxyFactory() );
  setCookieJar( new QgsNetworkCookieJar( this ) );
}

QgsNetworkAccessManager::~QgsNetworkAccessManager()
{
}

void QgsNetworkAccessManager::insertProxyFactory( QNetworkProxyFactory *factory )
{
  mProxyFactories.insert( 0, factory );
}

void QgsNetworkAccessManager::removeProxyFactory( QNetworkProxyFactory *factory )
{
  mProxyFactories.removeAll( factory );
}

const QList<QNetworkProxyFactory *> QgsNetworkAccessManager::proxyFactories() const
{
  return mProxyFactories;
}

const QStringList &QgsNetworkAccessManager::excludeList() const
{
  return mExcludedURLs;
}

const QNetworkProxy &QgsNetworkAccessManager::fallbackProxy() const
{
  return mFallbackProxy;
}

void QgsNetworkAccessManager::setFallbackProxyAndExcludes( const QNetworkProxy &proxy, const QStringList &excludes )
{
  QgsDebugMsg( QString( "proxy settings: (type:%1 host: %2:%3, user:%4, password:%5" )
               .arg( proxy.type() == QNetworkProxy::DefaultProxy ? "DefaultProxy" :
                     proxy.type() == QNetworkProxy::Socks5Proxy ? "Socks5Proxy" :
                     proxy.type() == QNetworkProxy::NoProxy ? "NoProxy" :
                     proxy.type() == QNetworkProxy::HttpProxy ? "HttpProxy" :
                     proxy.type() == QNetworkProxy::HttpCachingProxy ? "HttpCachingProxy" :
                     proxy.type() == QNetworkProxy::FtpCachingProxy ? "FtpCachingProxy" :
                     "Undefined" )
               .arg( proxy.hostName() )
               .arg( proxy.port() )
               .arg( proxy.user() )
               .arg( proxy.password().isEmpty() ? "not set" : "set" ) );

  mFallbackProxy = proxy;
  mExcludedURLs = excludes;
}

QNetworkReply *QgsNetworkAccessManager::createRequest( QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData )
{
  QSettings s;

  QNetworkRequest *pReq(( QNetworkRequest * ) &req ); // hack user agent

  QString userAgent = s.value( "/qgis/networkAndProxy/userAgent", "Mozilla/5.0" ).toString();
  if ( !userAgent.isEmpty() )
    userAgent += " ";
  userAgent += QString( "QGIS/%1" ).arg( QGis::QGIS_VERSION );
  pReq->setRawHeader( "User-Agent", userAgent.toUtf8() );

  emit requestAboutToBeCreated( op, req, outgoingData );
  QNetworkReply *reply = QNetworkAccessManager::createRequest( op, req, outgoingData );

  emit requestCreated( reply );

  // abort request, when network timeout happens
  QTimer *timer = new QTimer( reply );
  connect( timer, SIGNAL( timeout() ), this, SLOT( abortRequest() ) );
  timer->setSingleShot( true );
  timer->start( s.value( "/qgis/networkAndProxy/networkTimeout", "20000" ).toInt() );

  connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ), timer, SLOT( start() ) );
  connect( reply, SIGNAL( uploadProgress( qint64, qint64 ) ), timer, SLOT( start() ) );

  return reply;
}

void QgsNetworkAccessManager::abortRequest()
{
  QTimer *timer = qobject_cast<QTimer *>( sender() );
  Q_ASSERT( timer );

  QNetworkReply *reply = qobject_cast<QNetworkReply *>( timer->parent() );
  Q_ASSERT( reply );

  QgsDebugMsg( QString( "Abort [reply:%1]" ).arg(( qint64 ) reply, 0, 16 ) );

  QgsMessageLog::logMessage( tr( "Network request %1 timed out" ).arg( reply->url().toString() ), tr( "Network" ) );

  if ( reply->isRunning() )
    reply->close();

  emit requestTimedOut( reply->request().url() );
}

void QgsNetworkAccessManager::getCredentials( QNetworkReply *reply, QAuthenticator * auth )
{
  QgsCredentials::instance()->lock();
  QString realm = QString( "%1 at %2" ).arg( auth->realm() ).arg( reply->url().host() );
  QString username = auth->user();
  QString password = auth->password();
  bool isGuiThread = thread() == qApp->thread();

  if ( username.isEmpty() && password.isEmpty() && reply->request().hasRawHeader( "Authorization" ) )
  {
    QByteArray header( reply->request().rawHeader( "Authorization" ) );
    if ( header.startsWith( "Basic " ) )
    {
      QByteArray auth( QByteArray::fromBase64( header.mid( 6 ) ) );
      int pos = auth.indexOf( ":" );
      if ( pos >= 0 )
      {
        username = auth.left( pos );
        password = auth.mid( pos + 1 );
      }
    }
  }

  if ( QgsCredentials::instance()->get( realm, username, password, tr( "Authentication required" ), isGuiThread ) )
  {
    auth->setUser( username );
    auth->setPassword( password );
    QgsCredentials::instance()->put( realm, username, password );
  }
  QgsCredentials::instance()->unlock();
}

void QgsNetworkAccessManager::getProxyCredentials( const QNetworkProxy &proxy, QAuthenticator *auth )
{
  QSettings settings;
  if ( !settings.value( "proxy/proxyEnabled", false ).toBool() ||
       settings.value( "proxy/proxyType", "" ).toString() == "DefaultProxy" )
  {
    auth->setUser( "" );
    return;
  }

  QgsCredentials::instance()->lock();
  QString realm = QString( "proxy %1:%2 [%3]" ).arg( proxy.hostName() ).arg( proxy.port() ).arg( auth->realm() );
  QString username = auth->user();
  QString password = auth->password();
  bool isGuiThread = thread() == qApp->thread();

  if ( QgsCredentials::instance()->get( realm, username, password, tr( "Proxy authentication required" ), isGuiThread ) )
  {
    auth->setUser( username );
    auth->setPassword( password );
    QgsCredentials::instance()->put( realm, username, password );
  }
  QgsCredentials::instance()->unlock();
}

#ifndef QT_NO_OPENSSL
void QgsNetworkAccessManager::handleSSLErrors( QNetworkReply *reply, const QList<QSslError> &errors )
{
  QgsSSLIgnoreList::lock();
  QList<QSslError> newErrors;
  foreach ( QSslError error, errors )
  {
    if ( error.error() == QSslError::NoError )
      continue;
    if ( !QgsSSLIgnoreList::ignoreErrors().contains( error.error() ) )
    {
      newErrors.append( error );
    }
  }
  bool isGuiThread = thread() == instance()->thread();
  if ( newErrors.isEmpty() )
  {
    reply->ignoreSslErrors();
  }
  else if ( isGuiThread )
  {
    bool ok = false;
    emit sslErrorsConformationRequired( reply->url(), newErrors, &ok );
    if ( ok )
    {
      foreach ( QSslError error, newErrors )
      {
        QgsSSLIgnoreList::ignoreErrors().insert( error.error() );
      }
      reply->ignoreSslErrors();
    }
  }
  QgsSSLIgnoreList::unlock();
}
#endif

QString QgsNetworkAccessManager::cacheLoadControlName( QNetworkRequest::CacheLoadControl theControl )
{
  switch ( theControl )
  {
    case QNetworkRequest::AlwaysNetwork:
      return "AlwaysNetwork";
      break;
    case QNetworkRequest::PreferNetwork:
      return "PreferNetwork";
      break;
    case QNetworkRequest::PreferCache:
      return "PreferCache";
      break;
    case QNetworkRequest::AlwaysCache:
      return "AlwaysCache";
      break;
    default:
      break;
  }
  return "PreferNetwork";
}

QNetworkRequest::CacheLoadControl QgsNetworkAccessManager::cacheLoadControlFromName( const QString &theName )
{
  if ( theName == "AlwaysNetwork" )
  {
    return QNetworkRequest::AlwaysNetwork;
  }
  else if ( theName == "PreferNetwork" )
  {
    return QNetworkRequest::PreferNetwork;
  }
  else if ( theName == "PreferCache" )
  {
    return QNetworkRequest::PreferCache;
  }
  else if ( theName == "AlwaysCache" )
  {
    return QNetworkRequest::AlwaysCache;
  }
  return QNetworkRequest::PreferNetwork;
}

void QgsNetworkAccessManager::setupDefaultProxyAndCache()
{
  mInitialized = true;
  mUseSystemProxy = false;
  Q_ASSERT( smMainNAM );

  if ( smMainNAM != this )
  {
    connect( this, SIGNAL( requestTimedOut( QUrl ) ),
             smMainNAM, SIGNAL( requestTimedOut( QUrl ) ) );

#ifndef QT_NO_OPENSSL
    connect( this, SIGNAL( sslErrors( QNetworkReply *, const QList<QSslError> & ) ),
             smMainNAM, SIGNAL( sslErrors( QNetworkReply *, const QList<QSslError> & ) ),
             Qt::BlockingQueuedConnection );
#endif
    connect( smMainNAM, SIGNAL( cookieAdded( QByteArray, QUrl ) ), this, SLOT( addCookie( QByteArray, QUrl ) ) );
  }
  connect( this, SIGNAL( authenticationRequired( QNetworkReply *, QAuthenticator * ) ),
           this, SLOT( getCredentials( QNetworkReply, QAuthenticator* ) ) );
  connect( this, SIGNAL( proxyAuthenticationRequired( const QNetworkProxy &, QAuthenticator * ) ),
           this, SLOT( getProxyCredentials( QNetworkProxy, QAuthenticator* ) ) );

  // check if proxy is enabled
  QNetworkProxy proxy;
  QStringList excludes;
  QSettings settings;

  bool proxyEnabled = settings.value( "proxy/proxyEnabled", false ).toBool();
  if ( proxyEnabled )
  {
    excludes = settings.value( "proxy/proxyExcludedUrls", "" ).toString().split( "|", QString::SkipEmptyParts );

    //read type, host, port, user, passw from settings
    QString proxyHost = settings.value( "proxy/proxyHost", "" ).toString();
    int proxyPort = settings.value( "proxy/proxyPort", "" ).toString().toInt();
    QString proxyUser = settings.value( "proxy/proxyUser", "" ).toString();
    QString proxyPassword = settings.value( "proxy/proxyPassword", "" ).toString();

    QString proxyTypeString = settings.value( "proxy/proxyType", "" ).toString();

    if ( proxyTypeString == "DefaultProxy" )
    {
      mUseSystemProxy = true;
      QNetworkProxyFactory::setUseSystemConfiguration( true );
      QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery();
      if ( !proxies.isEmpty() )
      {
        proxy = proxies.first();
      }
      QgsDebugMsg( "setting default proxy" );
    }
    else
    {
      QNetworkProxy::ProxyType proxyType = QNetworkProxy::DefaultProxy;
      if ( proxyTypeString == "Socks5Proxy" )
      {
        proxyType = QNetworkProxy::Socks5Proxy;
      }
      else if ( proxyTypeString == "HttpProxy" )
      {
        proxyType = QNetworkProxy::HttpProxy;
      }
      else if ( proxyTypeString == "HttpCachingProxy" )
      {
        proxyType = QNetworkProxy::HttpCachingProxy;
      }
      else if ( proxyTypeString == "FtpCachingProxy" )
      {
        proxyType = QNetworkProxy::FtpCachingProxy;
      }
      QgsDebugMsg( QString( "setting proxy %1 %2:%3 %4/%5" )
                   .arg( proxyType )
                   .arg( proxyHost ).arg( proxyPort )
                   .arg( proxyUser ).arg( proxyPassword )
                 );
      proxy = QNetworkProxy( proxyType, proxyHost, proxyPort, proxyUser, proxyPassword );
    }
  }

#if QT_VERSION >= 0x40500
  setFallbackProxyAndExcludes( proxy, excludes );

  QgsNetworkDiskCache *newcache = qobject_cast<QgsNetworkDiskCache*>( cache() );
  if ( !newcache )
    newcache = new QgsNetworkDiskCache( this );

  QString cacheDirectory = settings.value( "cache/directory", QgsApplication::qgisSettingsDirPath() + "cache" ).toString();
  if ( cacheDirectory.isEmpty() )
  {
    cacheDirectory = QgsApplication::qgisSettingsDirPath() + "cache";
  }
  qint64 cacheSize = settings.value( "cache/size", 50 * 1024 * 1024 ).toULongLong();
  QgsDebugMsg( QString( "setCacheDirectory: %1" ).arg( cacheDirectory ) );
  QgsDebugMsg( QString( "setMaximumCacheSize: %1" ).arg( cacheSize ) );
  newcache->setCacheDirectory( cacheDirectory );
  newcache->setMaximumCacheSize( cacheSize );
  QgsDebugMsg( QString( "cacheDirectory: %1" ).arg( newcache->cacheDirectory() ) );
  QgsDebugMsg( QString( "maximumCacheSize: %1" ).arg( newcache->maximumCacheSize() ) );

  if ( cache() != newcache )
    setCache( newcache );
#else
  setProxy( proxy );
#endif

  if ( smMainNAM )
  {
    smMainNAM->mCookieLock.lock();
    static_cast<QgsNetworkCookieJar*>( cookieJar() )->setAllCookies( static_cast<QgsNetworkCookieJar*>( smMainNAM->cookieJar() )->allCookies() );
    smMainNAM->mCookieLock.unlock();
  }
}

void QgsNetworkAccessManager::addCookie( const QByteArray &cookie, const QUrl& url )
{
  mCookieLock.lock();
  QNetworkCookieJar* jar = cookieJar();
  jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie ), url );
  mCookieLock.unlock();
  emit cookieAdded( cookie, url );
}

QNetworkRequest QgsNetworkAccessManager::requestWithUserInfo( const QNetworkRequest& req )
{
  // HACK for NTLM SSO: if no authorization info is provided, try setting an
  // empty username and some random password (i.e. sso) as user info to fool
  // QNetworkAccessManager for NTLM SSO authorization, see
  // http://stackoverflow.com/questions/14706851/ntlmv2-authentication-in-qt
  QUrl url = req.url();
  bool trySSO = QSettings().value( "/qgis/networkAndProxy/attemptSSO", false ).toBool();
  if ( trySSO && url.userInfo().isEmpty() && req.rawHeader( "Authorization" ).isEmpty() )
  {
    url.setUserInfo( ":sso" );
  }
  QNetworkRequest adjustedRequest( req );
  adjustedRequest.setUrl( url );
  return adjustedRequest;
}

