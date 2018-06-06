/***************************************************************************
                              qgscrssync.cpp
                              --------------
  begin                : September 20th, 2017
  copyright            : (C) 2017 by Sandro Mani
  email                : manisandro at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgis.h"
#include "qgslogger.h"
#include "qgsapplication.h"

#include <QFile>
#include <QString>
#include <QTextStream>

#include <sqlite3.h>
#include <proj_api.h>
#include <ogr_srs_api.h>
#include <cpl_error.h>
#include <cpl_conv.h>
#include <cpl_csv.h>

namespace QgsCrsSync
{

// adapted from gdal/ogr/ogr_srs_dict.cpp
  static bool loadWkts( QHash<int, QString> &wkts, const char *filename )
  {
    qDebug( "Loading %s", filename );
    const char *pszFilename = CPLFindFile( "gdal", filename );
    if ( !pszFilename )
      return false;

    QFile csv( pszFilename );
    if ( !csv.open( QIODevice::ReadOnly ) )
      return false;

    QTextStream lines( &csv );

    for ( ;; )
    {
      QString line = lines.readLine();
      if ( line.isNull() )
        break;

      if ( line.startsWith( '#' ) )
      {
        continue;
      }
      else if ( line.startsWith( "include " ) )
      {
        if ( !loadWkts( wkts, line.mid( 8 ).toUtf8() ) )
          break;
      }
      else
      {
        int pos = line.indexOf( "," );
        if ( pos < 0 )
          return false;

        bool ok;
        int epsg = line.left( pos ).toInt( &ok );
        if ( !ok )
          return false;

        wkts.insert( epsg, line.mid( pos + 1 ) );
      }
    }

    csv.close();

    return true;
  }


  static bool loadIDs( QHash<int, QString> &wkts )
  {
    OGRSpatialReferenceH crs = OSRNewSpatialReference( NULL );

    foreach ( QString csv, QStringList() << "gcs.csv" << "pcs.csv" << "vertcs.csv" << "compdcs.csv" << "geoccs.csv" )
    {
      QString filename = CPLFindFile( "gdal", csv.toUtf8() );

      QFile f( filename );
      if ( !f.open( QIODevice::ReadOnly ) )
        continue;

      QTextStream lines( &f );
      int l = 0, n = 0;

      lines.readLine();
      for ( ;; )
      {
        l++;
        QString line = lines.readLine();
        if ( line.isNull() )
          break;

        int pos = line.indexOf( "," );
        if ( pos < 0 )
          continue;

        bool ok;
        int epsg = line.left( pos ).toInt( &ok );
        if ( !ok )
          continue;

        // some CRS are known to fail (see http://trac.osgeo.org/gdal/ticket/2900)
        if ( epsg == 2218 || epsg == 2221 || epsg == 2296 || epsg == 2297 || epsg == 2298 || epsg == 2299 || epsg == 2300 || epsg == 2301 || epsg == 2302 ||
             epsg == 2303 || epsg == 2304 || epsg == 2305 || epsg == 2306 || epsg == 2307 || epsg == 2963 || epsg == 2985 || epsg == 2986 || epsg == 3052 ||
             epsg == 3053 || epsg == 3139 || epsg == 3144 || epsg == 3145 || epsg == 3173 || epsg == 3295 || epsg == 3993 || epsg == 4087 || epsg == 4088 ||
             epsg == 5017 || epsg == 5221 || epsg == 5224 || epsg == 5225 || epsg == 5514 || epsg == 5515 || epsg == 5516 || epsg == 5819 || epsg == 5820 ||
             epsg == 5821 || epsg == 32600 || epsg == 32663 || epsg == 32700 )
          continue;

        if ( OSRImportFromEPSG( crs, epsg ) != OGRERR_NONE )
        {
          qDebug( "EPSG %d: not imported", epsg );
          continue;
        }

        char *wkt = 0;
        if ( OSRExportToWkt( crs, &wkt ) != OGRERR_NONE )
        {
          qWarning( "EPSG %d: not exported to WKT", epsg );
          continue;
        }

        wkts.insert( epsg, wkt );
        n++;

        CPLFree( wkt );
      }

      f.close();

      qDebug( "Loaded %d/%d from %s", n, l, filename.toUtf8().constData() );
    }

    OSRDestroySpatialReference( crs );

    return true;
  }

  static QString quotedValue( QString value )
  {
    value.replace( "'", "''" );
    return value.prepend( "'" ).append( "'" );
  }

  static bool syncDatumTransform( const QString& dbPath )
  {
    const char *filename = CSVFilename( "datum_shift.csv" );
    FILE *fp = VSIFOpen( filename, "rb" );
    if ( !fp )
    {
      return false;
    }

    char **fieldnames = CSVReadParseLine( fp );

    // "SEQ_KEY","COORD_OP_CODE","SOURCE_CRS_CODE","TARGET_CRS_CODE","REMARKS","COORD_OP_SCOPE","AREA_OF_USE_CODE","AREA_SOUTH_BOUND_LAT","AREA_NORTH_BOUND_LAT","AREA_WEST_BOUND_LON","AREA_EAST_BOUND_LON","SHOW_OPERATION","DEPRECATED","COORD_OP_METHOD_CODE","DX","DY","DZ","RX","RY","RZ","DS","PREFERRED"

    struct
    {
      const char *src;
      const char *dst;
      int idx;
    } map[] =
    {
      // { "SEQ_KEY", "", -1 },
      { "SOURCE_CRS_CODE", "source_crs_code", -1 },
      { "TARGET_CRS_CODE", "target_crs_code", -1 },
      { "REMARKS", "remarks", -1 },
      { "COORD_OP_SCOPE", "scope", -1 },
      { "AREA_OF_USE_CODE", "area_of_use_code", -1 },
      // { "AREA_SOUTH_BOUND_LAT", "", -1 },
      // { "AREA_NORTH_BOUND_LAT", "", -1 },
      // { "AREA_WEST_BOUND_LON", "", -1 },
      // { "AREA_EAST_BOUND_LON", "", -1 },
      // { "SHOW_OPERATION", "", -1 },
      { "DEPRECATED", "deprecated", -1 },
      { "COORD_OP_METHOD_CODE", "coord_op_method_code", -1 },
      { "DX", "p1", -1 },
      { "DY", "p2", -1 },
      { "DZ", "p3", -1 },
      { "RX", "p4", -1 },
      { "RY", "p5", -1 },
      { "RZ", "p6", -1 },
      { "DS", "p7", -1 },
      { "PREFERRED", "preferred", -1 },
      { "COORD_OP_CODE", "coord_op_code", -1 },
    };

    QString update = "UPDATE tbl_datum_transform SET ";
    QString insert, values;

    int n = CSLCount( fieldnames );

    int idxid = -1, idxrx = -1, idxry = -1, idxrz = -1, idxmcode = -1;
    for ( unsigned int i = 0; i < sizeof( map ) / sizeof( *map ); i++ )
    {
      bool last = i == sizeof( map ) / sizeof( *map ) - 1;

      map[i].idx = CSLFindString( fieldnames, map[i].src );
      if ( map[i].idx < 0 )
      {
        qWarning( "field %s not found", map[i].src );
        CSLDestroy( fieldnames );
        VSIFClose( fp );
        return false;
      }

      if ( strcmp( map[i].src, "COORD_OP_CODE" ) == 0 )
        idxid = i;
      if ( strcmp( map[i].src, "RX" ) == 0 )
        idxrx = i;
      if ( strcmp( map[i].src, "RY" ) == 0 )
        idxry = i;
      if ( strcmp( map[i].src, "RZ" ) == 0 )
        idxrz = i;
      if ( strcmp( map[i].src, "COORD_OP_METHOD_CODE" ) == 0 )
        idxmcode = i;

      if ( i > 0 )
      {
        insert += ",";
        values += ",";

        if ( last )
        {
          update += " WHERE ";
        }
        else
        {
          update += ",";
        }
      }

      update += QString( "%1=%%2" ).arg( map[i].dst ).arg( i + 1 );

      insert += map[i].dst;
      values += QString( "%%1" ).arg( i + 1 );
    }

    insert = "INSERT INTO tbl_datum_transform(" + insert + ") VALUES (" + values + ")";

    QgsDebugMsgLevel( QString( "insert:%1" ).arg( insert ), 4 );
    QgsDebugMsgLevel( QString( "update:%1" ).arg( update ), 4 );

    CSLDestroy( fieldnames );

    Q_ASSERT( idxid >= 0 );
    Q_ASSERT( idxrx >= 0 );
    Q_ASSERT( idxry >= 0 );
    Q_ASSERT( idxrz >= 0 );

    sqlite3 *db;
    int openResult = sqlite3_open( dbPath.toUtf8().constData(), &db );
    if ( openResult != SQLITE_OK )
    {
      VSIFClose( fp );
      return false;
    }

    if ( sqlite3_exec( db, "BEGIN TRANSACTION", 0, 0, 0 ) != SQLITE_OK )
    {
      qCritical( "Could not begin transaction: %s [%s]\n", QgsApplication::srsDbFilePath().toLocal8Bit().constData(), sqlite3_errmsg( db ) );
      sqlite3_close( db );
      VSIFClose( fp );
      return false;
    }

    QStringList v;
    v.reserve( sizeof( map ) / sizeof( *map ) );

    while ( true )
    {
      char **values = CSVReadParseLine( fp );
      if ( !values )
        break;

      v.clear();

      if ( CSLCount( values ) < n )
      {
        qWarning( "Only %d columns", CSLCount( values ) );
        continue;
      }

      for ( unsigned int i = 0; i < sizeof( map ) / sizeof( *map ); i++ )
      {
        int idx = map[i].idx;
        Q_ASSERT( idx != -1 );
        Q_ASSERT( idx < n );
        v.insert( i, *values[ idx ] ? quotedValue( values[idx] ) : "NULL" );
      }

      //switch sign of rotation parameters. See http://trac.osgeo.org/proj/wiki/GenParms#towgs84-DatumtransformationtoWGS84
      if ( v.at( idxmcode ).compare( QString( "'9607'" ) ) == 0 )
      {
        v[ idxmcode ] = "'9606'";
        v[ idxrx ] = "'" + qgsDoubleToString( -( v[ idxrx ].remove( "'" ).toDouble() ) ) + "'";
        v[ idxry ] = "'" + qgsDoubleToString( -( v[ idxry ].remove( "'" ).toDouble() ) ) + "'";
        v[ idxrz ] = "'" + qgsDoubleToString( -( v[ idxrz ].remove( "'" ).toDouble() ) ) + "'";
      }

      //entry already in db?
      sqlite3_stmt *stmt;
      QString cOpCode;
      QString sql = QString( "SELECT coord_op_code FROM tbl_datum_transform WHERE coord_op_code=%1" ).arg( v[ idxid ] );
      int prepareRes = sqlite3_prepare( db, sql.toLatin1(), sql.size(), &stmt, NULL );
      if ( prepareRes != SQLITE_OK )
        continue;

      if ( sqlite3_step( stmt ) == SQLITE_ROW )
      {
        cOpCode = ( const char * ) sqlite3_column_text( stmt, 0 );
      }
      sqlite3_finalize( stmt );

      sql = cOpCode.isEmpty() ? insert : update;
      for ( int i = 0; i < v.size(); i++ )
      {
        sql = sql.arg( v[i] );
      }

      if ( sqlite3_exec( db, sql.toUtf8(), 0, 0, 0 ) != SQLITE_OK )
      {
        qCritical( "SQL: %s", sql.toUtf8().constData() );
        qCritical( "Error: %s", sqlite3_errmsg( db ) );
      }
    }

    if ( sqlite3_exec( db, "COMMIT", 0, 0, 0 ) != SQLITE_OK )
    {
      qCritical( "Could not commit transaction: %s [%s]\n", QgsApplication::srsDbFilePath().toLocal8Bit().constData(), sqlite3_errmsg( db ) );
      sqlite3_close( db );
      VSIFClose( fp );
      return false;
    }

    sqlite3_close( db );
    VSIFClose( fp );
    return true;
  }

  int syncDb()
  {
    QString dbFilePath = QgsApplication::srsDbFilePath();
    syncDatumTransform( dbFilePath );

    int inserted = 0, updated = 0, deleted = 0, errors = 0;

    qDebug( "Load srs db from: %s", QgsApplication::srsDbFilePath().toLocal8Bit().constData() );

    sqlite3 *database;
    if ( sqlite3_open( dbFilePath.toUtf8().constData(), &database ) != SQLITE_OK )
    {
      qCritical( "Could not open database: %s [%s]\n", QgsApplication::srsDbFilePath().toLocal8Bit().constData(), sqlite3_errmsg( database ) );
      return -1;
    }

    if ( sqlite3_exec( database, "BEGIN TRANSACTION", 0, 0, 0 ) != SQLITE_OK )
    {
      qCritical( "Could not begin transaction: %s [%s]\n", QgsApplication::srsDbFilePath().toLocal8Bit().constData(), sqlite3_errmsg( database ) );
      return -1;
    }

    // fix up database, if not done already //
    if ( sqlite3_exec( database, "alter table tbl_srs add noupdate boolean", 0, 0, 0 ) == SQLITE_OK )
      ( void )sqlite3_exec( database, "update tbl_srs set noupdate=(auth_name='EPSG' and auth_id in (5513,5514,5221,2065,102067,4156,4818))", 0, 0, 0 );

    ( void )sqlite3_exec( database, "UPDATE tbl_srs SET srid=141001 WHERE srid=41001 AND auth_name='OSGEO' AND auth_id='41001'", 0, 0, 0 );

    OGRSpatialReferenceH crs = OSRNewSpatialReference( NULL );
    const char *tail;
    sqlite3_stmt *select;
    char *errMsg = NULL;

    QString proj4;
    QString sql;
    QHash<int, QString> wkts;
    loadIDs( wkts );
    loadWkts( wkts, "epsg.wkt" );

    qDebug( "%d WKTs loaded", wkts.count() );

    for ( QHash<int, QString>::const_iterator it = wkts.constBegin(); it != wkts.constEnd(); ++it )
    {
      QByteArray ba( it.value().toUtf8() );
      char *psz = ba.data();
      OGRErr ogrErr = OSRImportFromWkt( crs, &psz );
      if ( ogrErr != OGRERR_NONE )
        continue;

      if ( OSRExportToProj4( crs, &psz ) != OGRERR_NONE )
        continue;

      proj4 = psz;
      proj4 = proj4.trimmed();

      CPLFree( psz );

      if ( proj4.isEmpty() )
        continue;

      sql = QString( "SELECT parameters,noupdate FROM tbl_srs WHERE auth_name='EPSG' AND auth_id='%1'" ).arg( it.key() );
      if ( sqlite3_prepare( database, sql.toLatin1(), sql.size(), &select, &tail ) != SQLITE_OK )
      {
        qCritical( "Could not prepare: %s [%s]\n", sql.toLatin1().constData(), sqlite3_errmsg( database ) );
        continue;
      }

      QString srsProj4;
      if ( sqlite3_step( select ) == SQLITE_ROW )
      {
        srsProj4 = ( const char * ) sqlite3_column_text( select, 0 );

        if ( QString::fromUtf8(( char * )sqlite3_column_text( select, 1 ) ).toInt() != 0 )
          continue;
      }

      sqlite3_finalize( select );

      if ( !srsProj4.isEmpty() )
      {
        if ( proj4 != srsProj4 )
        {
          errMsg = NULL;
          sql = QString( "UPDATE tbl_srs SET parameters=%1 WHERE auth_name='EPSG' AND auth_id=%2" ).arg( quotedValue( proj4 ) ).arg( it.key() );

          if ( sqlite3_exec( database, sql.toUtf8(), 0, 0, &errMsg ) != SQLITE_OK )
          {
            qCritical( "Could not execute: %s [%s/%s]\n",
                       sql.toLocal8Bit().constData(),
                       sqlite3_errmsg( database ),
                       errMsg ? errMsg : "(unknown error)" );
            errors++;
          }
          else
          {
            updated++;
            QgsDebugMsgLevel( QString( "SQL: %1\n OLD:%2\n NEW:%3" ).arg( sql ).arg( srsProj4 ).arg( proj4 ), 3 );
          }
        }
      }
      else
      {
        QRegExp projRegExp( "\\+proj=(\\S+)" );
        if ( projRegExp.indexIn( proj4 ) < 0 )
        {
          QgsDebugMsg( QString( "EPSG %1: no +proj argument found [%2]" ).arg( it.key() ).arg( proj4 ) );
          continue;
        }

        QRegExp ellipseRegExp( "\\+ellps=(\\S+)" );
        QString ellps;
        if ( ellipseRegExp.indexIn( proj4 ) >= 0 )
        {
          ellps = ellipseRegExp.cap( 1 );
        }

        QString name( OSRIsGeographic( crs ) ? OSRGetAttrValue( crs, "GEOCS", 0 ) : OSRGetAttrValue( crs, "PROJCS", 0 ) );
        if ( name.isEmpty() )
          name = QObject::tr( "Imported from GDAL" );

        sql = QString( "INSERT INTO tbl_srs(description,projection_acronym,ellipsoid_acronym,parameters,srid,auth_name,auth_id,is_geo,deprecated) VALUES (%1,%2,%3,%4,%5,'EPSG',%5,%6,0)" )
              .arg( quotedValue( name ) )
              .arg( quotedValue( projRegExp.cap( 1 ) ) )
              .arg( quotedValue( ellps ) )
              .arg( quotedValue( proj4 ) )
              .arg( it.key() )
              .arg( OSRIsGeographic( crs ) );

        errMsg = NULL;
        if ( sqlite3_exec( database, sql.toUtf8(), 0, 0, &errMsg ) == SQLITE_OK )
        {
          inserted++;
        }
        else
        {
          qCritical( "Could not execute: %s [%s/%s]\n",
                     sql.toLocal8Bit().constData(),
                     sqlite3_errmsg( database ),
                     errMsg ? errMsg : "(unknown error)" );
          errors++;

          if ( errMsg )
            sqlite3_free( errMsg );
        }
      }
    }

    sql = "DELETE FROM tbl_srs WHERE auth_name='EPSG' AND NOT auth_id IN (";
    QString delim;
    foreach ( int i, wkts.keys() )
    {
      sql += delim + QString::number( i );
      delim = ",";
    }
    sql += ") AND NOT noupdate";

    if ( sqlite3_exec( database, sql.toUtf8(), 0, 0, 0 ) == SQLITE_OK )
    {
      deleted = sqlite3_changes( database );
    }
    else
    {
      errors++;
      qCritical( "Could not execute: %s [%s]\n",
                 sql.toLocal8Bit().constData(),
                 sqlite3_errmsg( database ) );
    }

#if !defined(PJ_VERSION) || PJ_VERSION!=470
    sql = QString( "select auth_name,auth_id,parameters from tbl_srs WHERE auth_name<>'EPSG' AND NOT deprecated AND NOT noupdate" );
    if ( sqlite3_prepare( database, sql.toLatin1(), sql.size(), &select, &tail ) == SQLITE_OK )
    {
      while ( sqlite3_step( select ) == SQLITE_ROW )
      {
        const char *auth_name = ( const char * ) sqlite3_column_text( select, 0 );
        const char *auth_id   = ( const char * ) sqlite3_column_text( select, 1 );
        const char *params    = ( const char * ) sqlite3_column_text( select, 2 );

        QString input = QString( "+init=%1:%2" ).arg( QString( auth_name ).toLower() ).arg( auth_id );
        projPJ pj = pj_init_plus( input.toLatin1() );
        if ( !pj )
        {
          input = QString( "+init=%1:%2" ).arg( QString( auth_name ).toUpper() ).arg( auth_id );
          pj = pj_init_plus( input.toLatin1() );
        }

        if ( pj )
        {
          char *def = pj_get_def( pj, 0 );
          if ( def )
          {
            proj4 = def;
            pj_dalloc( def );

            input.prepend( ' ' ).append( ' ' );
            if ( proj4.startsWith( input ) )
            {
              proj4 = proj4.mid( input.size() );
              proj4 = proj4.trimmed();
            }

            if ( proj4 != params )
            {
              sql = QString( "UPDATE tbl_srs SET parameters=%1 WHERE auth_name=%2 AND auth_id=%3" )
                    .arg( quotedValue( proj4 ) )
                    .arg( quotedValue( auth_name ) )
                    .arg( quotedValue( auth_id ) );

              if ( sqlite3_exec( database, sql.toUtf8(), 0, 0, &errMsg ) == SQLITE_OK )
              {
                updated++;
                QgsDebugMsgLevel( QString( "SQL: %1\n OLD:%2\n NEW:%3" ).arg( sql ).arg( params ).arg( proj4 ), 3 );
              }
              else
              {
                qCritical( "Could not execute: %s [%s/%s]\n",
                           sql.toLocal8Bit().constData(),
                           sqlite3_errmsg( database ),
                           errMsg ? errMsg : "(unknown error)" );
                errors++;
              }
            }
          }
          else
          {
            QgsDebugMsg( QString( "could not retrieve proj string for %1 from PROJ" ).arg( input ) );
          }
        }
        else
        {
          QgsDebugMsgLevel( QString( "could not retrieve crs for %1 from PROJ" ).arg( input ), 3 );
        }

        pj_free( pj );
      }
    }
    else
    {
      errors++;
      qCritical( "Could not execute: %s [%s]\n",
                 sql.toLocal8Bit().constData(),
                 sqlite3_errmsg( database ) );
    }
#endif

    OSRDestroySpatialReference( crs );

    if ( sqlite3_exec( database, "COMMIT", 0, 0, 0 ) != SQLITE_OK )
    {
      qCritical( "Could not commit transaction: %s [%s]\n", QgsApplication::srsDbFilePath().toLocal8Bit().constData(), sqlite3_errmsg( database ) );
      return -1;
    }

    sqlite3_close( database );

    qWarning( "CRS update (inserted:%d updated:%d deleted:%d errors:%d)", inserted, updated, deleted, errors );

    if ( errors > 0 )
      return -errors;
    else
      return updated + inserted;
  }

}
