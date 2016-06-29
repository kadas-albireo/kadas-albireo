/***************************************************************************
                              qgscrscache.cpp
                              ---------------
  begin                : September 6th, 2011
  copyright            : (C) 2011 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscrscache.h"
#include "qgscoordinatetransform.h"
#include "qgsapplication.h"
#include "qgsmessagelog.h"
#include <sqlite3.h>

QgsCoordinateTransformCache::~QgsCoordinateTransformCache()
{
  QHash< QPair< QString, QString >, QgsCoordinateTransform* >::const_iterator tIt = mTransforms.constBegin();
  for ( ; tIt != mTransforms.constEnd(); ++tIt )
  {
    delete tIt.value();
  }

  mTransforms.clear();
}

const QgsCoordinateTransform* QgsCoordinateTransformCache::transform( const QString& srcAuthId, const QString& destAuthId, int srcDatumTransform, int destDatumTransform )
{
  QList< QgsCoordinateTransform* > values =
    mTransforms.values( qMakePair( srcAuthId, destAuthId ) );

  QList< QgsCoordinateTransform* >::const_iterator valIt = values.constBegin();
  for ( ; valIt != values.constEnd(); ++valIt )
  {
    if ( *valIt &&
         ( *valIt )->sourceDatumTransform() == srcDatumTransform &&
         ( *valIt )->destinationDatumTransform() == destDatumTransform )
    {
      return *valIt;
    }
  }

  //not found, insert new value
  const QgsCoordinateReferenceSystem& srcCrs = QgsCRSCache::instance()->crsByAuthId( srcAuthId );
  const QgsCoordinateReferenceSystem& destCrs = QgsCRSCache::instance()->crsByAuthId( destAuthId );
  QgsCoordinateTransform* ct = new QgsCoordinateTransform( srcCrs, destCrs );
  ct->setSourceDatumTransform( srcDatumTransform );
  ct->setDestinationDatumTransform( destDatumTransform );
  ct->initialise();
  mTransforms.insertMulti( qMakePair( srcAuthId, destAuthId ), ct );
  return ct;
}

void QgsCoordinateTransformCache::invalidateCrs( const QString& crsAuthId )
{
  //get keys to remove first
  QHash< QPair< QString, QString >, QgsCoordinateTransform* >::const_iterator it = mTransforms.constBegin();
  QList< QPair< QString, QString > > updateList;

  for ( ; it != mTransforms.constEnd(); ++it )
  {
    if ( it.key().first == crsAuthId || it.key().second == crsAuthId )
    {
      updateList.append( it.key() );
    }
  }

  //and remove after
  QList< QPair< QString, QString > >::const_iterator updateIt = updateList.constBegin();
  for ( ; updateIt != updateList.constEnd(); ++updateIt )
  {
    mTransforms.remove( *updateIt );
  }
}


QgsCRSCache* QgsCRSCache::instance()
{
  static QgsCRSCache mInstance;
  return &mInstance;
}

QgsCRSCache::QgsCRSCache()
{
}

QgsCRSCache::~QgsCRSCache()
{
}

void QgsCRSCache::updateCRSCache( const QString& authid )
{
  QgsCoordinateReferenceSystem s;
  if ( s.createFromOgcWmsCrs( authid ) )
  {
    mCRS.insert( authid, s );
  }
  else
  {
    mCRS.remove( authid );
  }

  QgsCoordinateTransformCache::instance()->invalidateCrs( authid );
}

const QgsCoordinateReferenceSystem& QgsCRSCache::crsByAuthId( const QString& authid )
{
  QHash< QString, QgsCoordinateReferenceSystem >::const_iterator crsIt = mCRS.find( authid );
  if ( crsIt == mCRS.constEnd() )
  {
    QgsCoordinateReferenceSystem s;
    if ( ! s.createFromOgcWmsCrs( authid ) )
    {
      return mCRS.insert( authid, mInvalidCRS ).value();
    }
    return mCRS.insert( authid, s ).value();
  }
  else
  {
    return crsIt.value();
  }
}

const QgsCoordinateReferenceSystem& QgsCRSCache::crsByEpsgId( long epsg )
{
  return crsByAuthId( "EPSG:" + QString::number( epsg ) );
}

const QgsCoordinateReferenceSystem& QgsCRSCache::crsBySrsId( long srsid )
{
  QHash< long, QgsCoordinateReferenceSystem >::const_iterator crsIt = mCRSSrsId.find( srsid );
  if ( crsIt == mCRSSrsId.constEnd() )
  {
    QgsCoordinateReferenceSystem s;
    if ( ! s.createFromSrsId( srsid ) )
    {
      return mCRSSrsId.insert( srsid, mInvalidCRS ).value();
    }
    return mCRSSrsId.insert( srsid, s ).value();
  }
  else
  {
    return crsIt.value();
  }
}

const QgsCoordinateReferenceSystem& QgsCRSCache::crsByProj4( const QString& proj4 )
{
  QHash< QString, QgsCoordinateReferenceSystem >::const_iterator crsIt = mCRSProj4.find( proj4 );
  if ( crsIt == mCRSProj4.constEnd() )
  {
    QgsCoordinateReferenceSystem s;
    if ( ! s.createFromProj4( proj4 ) )
    {
      return mCRSProj4.insert( proj4, mInvalidCRS ).value();
    }
    return mCRSProj4.insert( proj4, s ).value();
  }
  else
  {
    return crsIt.value();
  }
}

const QgsCoordinateReferenceSystem& QgsCRSCache::crsByOgcWms( const QString& ogcwms )
{
  QHash< QString, QgsCoordinateReferenceSystem >::const_iterator crsIt = mCRSOgcWms.find( ogcwms );
  if ( crsIt == mCRSOgcWms.constEnd() )
  {
    QgsCoordinateReferenceSystem s;
    if ( ! s.createFromOgcWmsCrs( ogcwms ) )
    {
      return mCRSOgcWms.insert( ogcwms, mInvalidCRS ).value();
    }
    return mCRSOgcWms.insert( ogcwms, s ).value();
  }
  else
  {
    return crsIt.value();
  }
}

QgsEllipsoidCache* QgsEllipsoidCache::instance()
{
  static QgsEllipsoidCache sInstance;
  return &sInstance;
}

const QgsEllipsoidCache::Params& QgsEllipsoidCache::getParams( const QString& ellipsoid )
{
  QMap<QString, Params>::iterator it = mParams.find( ellipsoid );
  if ( it != mParams.end() )
  {
    return it.value();
  }

  Params params;
  //
  // SQLITE3 stuff - get parameters for selected ellipsoid
  //
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;

  // Continue with PROJ.4 list of ellipsoids.

  //check the db is available
  myResult = sqlite3_open_v2( QgsApplication::srsDbFilePath().toUtf8().data(), &myDatabase, SQLITE_OPEN_READONLY, NULL );
  if ( myResult )
  {
    QgsMessageLog::logMessage( QObject::tr( "Can't open database: %1" ).arg( sqlite3_errmsg( myDatabase ) ) );
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    return mParams.insert( ellipsoid, params ).value();
  }
  // Set up the query to retrieve the projection information needed to populate the ELLIPSOID list
  QString mySql = "select radius, parameter2 from tbl_ellipsoid where acronym='" + ellipsoid + "'";
  myResult = sqlite3_prepare( myDatabase, mySql.toUtf8(), mySql.toUtf8().length(), &myPreparedStatement, &myTail );
  // XXX Need to free memory from the error msg if one is set
  if ( myResult == SQLITE_OK )
  {
    if ( sqlite3_step( myPreparedStatement ) == SQLITE_ROW )
    {
      params.radius = QString(( char * )sqlite3_column_text( myPreparedStatement, 0 ) );
      params.parameter2 = QString(( char * )sqlite3_column_text( myPreparedStatement, 1 ) );
    }
  }
  // close the sqlite3 statement
  sqlite3_finalize( myPreparedStatement );
  sqlite3_close( myDatabase );

  return mParams.insert( ellipsoid, params ).value();
}
