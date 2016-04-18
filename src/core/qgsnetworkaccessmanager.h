/***************************************************************************
                          qgsnetworkaccessmanager.h  -  description
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

#ifndef QGSNETWORKACCESSMANAGER_H
#define QGSNETWORKACCESSMANAGER_H

#include <QList>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>

#include "qgssingleton.h"

/*
 * \class QgsNetworkAccessManager
 * \brief network access manager for QGIS
 * \ingroup core
 * \since 1.5
 *
 * This class implements the QGIS network access manager.  It's a singleton
 * that can be used across QGIS.
 *
 * Plugins can insert proxy factories and thereby redirect requests to
 * individual proxies.
 *
 * If no proxy factories are there or none returns a proxy for an URL a
 * fallback proxy can be set.  There's also a exclude list that defines URLs
 * that the fallback proxy should not be used for, then no proxy will be used.
 *
 */
class CORE_EXPORT QgsNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

  public:
    //! returns a pointer to the single instance
    // and creates that instance on the first call.
    static QgsNetworkAccessManager* instance();

    QgsNetworkAccessManager( QObject *parent = 0 );

    //! destructor
    ~QgsNetworkAccessManager();

    //! insert a factory into the proxy factories list
    void insertProxyFactory( QNetworkProxyFactory *factory );

    //! remove a factory from the proxy factories list
    void removeProxyFactory( QNetworkProxyFactory *factory );

    //! retrieve proxy factory list
    const QList<QNetworkProxyFactory *> proxyFactories() const;

    //! retrieve fall back proxy (for urls that no factory returned proxies for)
    const QNetworkProxy &fallbackProxy() const;

    //! retrieve exclude list (urls shouldn't use the fallback proxy)
    const QStringList &excludeList() const;

    //! set fallback proxy and URL that shouldn't use it.
    void setFallbackProxyAndExcludes( const QNetworkProxy &proxy, const QStringList &excludes );

    //! Get name for QNetworkRequest::CacheLoadControl
    static QString cacheLoadControlName( QNetworkRequest::CacheLoadControl theControl );

    //! Get QNetworkRequest::CacheLoadControl from name
    static QNetworkRequest::CacheLoadControl cacheLoadControlFromName( const QString &theName );

    //! Setup the NAM according to the user's settings
    void setupDefaultProxyAndCache();

    //! return whether the system proxy should be used
    bool useSystemProxy() { return mUseSystemProxy; }

    QNetworkReply *head( const QNetworkRequest &request )
    {
      return QNetworkAccessManager::head( requestWithUserInfo( request ) );
    }
    QNetworkReply *get( const QNetworkRequest &request )
    {
      return QNetworkAccessManager::get( requestWithUserInfo( request ) );
    }
    QNetworkReply *post( const QNetworkRequest &request, QIODevice *data )
    {
      return QNetworkAccessManager::post( requestWithUserInfo( request ), data );
    }
    QNetworkReply *post( const QNetworkRequest &request, const QByteArray &data )
    {
      return QNetworkAccessManager::post( requestWithUserInfo( request ), data );
    }
    QNetworkReply *post( const QNetworkRequest &request, QHttpMultiPart *multiPart )
    {
      return QNetworkAccessManager::post( requestWithUserInfo( request ), multiPart );
    }
    QNetworkReply *put( const QNetworkRequest &request, QIODevice *data )
    {
      return QNetworkAccessManager::put( requestWithUserInfo( request ), data );
    }
    QNetworkReply *put( const QNetworkRequest &request, const QByteArray &data )
    {
      return QNetworkAccessManager::put( requestWithUserInfo( request ), data );
    }
    QNetworkReply *put( const QNetworkRequest &request, QHttpMultiPart *multiPart )
    {
      return QNetworkAccessManager::put( requestWithUserInfo( request ), multiPart );
    }
    QNetworkReply *deleteResource( const QNetworkRequest &request )
    {
      return QNetworkAccessManager::deleteResource( requestWithUserInfo( request ) );
    }
    QNetworkReply *sendCustomRequest( const QNetworkRequest &request, const QByteArray &verb, QIODevice *data = 0 )
    {
      return QNetworkAccessManager::sendCustomRequest( requestWithUserInfo( request ), verb, data );
    }

  signals:
    void requestAboutToBeCreated( QNetworkAccessManager::Operation, const QNetworkRequest &, QIODevice * );
    void requestCreated( QNetworkReply * );
    void requestTimedOut( QNetworkReply * );
    void sslErrorsConformationRequired( const QUrl& url, const QList<QSslError> &errors, bool* ok );

  private slots:
    void abortRequest();
    void getCredentials( QNetworkReply *reply, QAuthenticator * auth );
    void getProxyCredentials( const QNetworkProxy &proxy, QAuthenticator * auth );
#ifndef QT_NO_OPENSSL
    void handleSSLErrors( QNetworkReply *reply, const QList<QSslError> &errors );
#endif

  protected:
    virtual QNetworkReply *createRequest( QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0 ) override;

  private:
    class QgsSSLIgnoreList;

    QList<QNetworkProxyFactory*> mProxyFactories;
    QNetworkProxy mFallbackProxy;
    QStringList mExcludedURLs;
    bool mUseSystemProxy;
    bool mInitialized;
    static QgsNetworkAccessManager *smMainNAM;

    QNetworkRequest requestWithUserInfo( const QNetworkRequest& req );
};

#endif // QGSNETWORKACCESSMANAGER_H

