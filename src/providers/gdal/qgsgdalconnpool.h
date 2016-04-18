/***************************************************************************
    qgsgdalconnpool.h
    ---------------------
    begin                : March 2016
    copyright            : (C) 2016 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGDALCONNPOOL_H
#define QGSGDALCONNPOOL_H

#include "qgsconnectionpool.h"
#include <gdal.h>
#include "qgsgdalproviderbase.h"


struct QgsGdalConn
{
  QString path;
  GDALDatasetH ds;
  bool valid;
};

inline QString qgsConnectionPool_ConnectionToName( QgsGdalConn* c )
{
  return c->path;
}

inline void qgsConnectionPool_ConnectionCreate( QString connInfo, QgsGdalConn*& c )
{
  c = new QgsGdalConn;
  c->ds = QgsGdalProviderBase::gdalOpen( TO8F( connInfo ), GA_ReadOnly );
  c->path = connInfo;
  c->valid = true;
}

inline void qgsConnectionPool_ConnectionDestroy( QgsGdalConn* c )
{
  GDALClose( c->ds );
  delete c;
}

inline void qgsConnectionPool_InvalidateConnection( QgsGdalConn* c )
{
  c->valid = false;
}

inline bool qgsConnectionPool_ConnectionIsValid( QgsGdalConn* c )
{
  return c->valid;
}

class QgsGdalConnPoolGroup : public QObject, public QgsConnectionPoolGroup<QgsGdalConn*>
{
    Q_OBJECT

  public:
    QgsGdalConnPoolGroup( QString name ) : QgsConnectionPoolGroup<QgsGdalConn*>( name ) { initTimer( this ); }

  protected slots:
    void handleConnectionExpired() { onConnectionExpired(); }
    void startExpirationTimer() { expirationTimer->start(); }
    void stopExpirationTimer() { expirationTimer->stop(); }

  protected:
    Q_DISABLE_COPY( QgsGdalConnPoolGroup )
};

/** Ogr connection pool - singleton */
class QgsGdalConnPool : public QgsConnectionPool<QgsGdalConn*, QgsGdalConnPoolGroup>
{
  public:
    static QgsGdalConnPool* instance();

  protected:
    Q_DISABLE_COPY( QgsGdalConnPool )

  private:
    QgsGdalConnPool();
    ~QgsGdalConnPool();
};


#endif // QGSGDALCONNPOOL_H
