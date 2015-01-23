/***************************************************************************
      qgsdatasourceuri.h  -  Structure to contain the component parts
                             of a data source URI
                             -------------------
    begin                : Dec 5, 2004
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdatasourceuri.h"
#include "qgslogger.h"

#include <QStringList>
#include <QRegExp>
#include <QUrl>

QgsDataSourceURI::QgsDataSourceURI()
    : mSSLmode( SSLprefer )
    , mKeyColumn( "" )
    , mUseEstimatedMetadata( false )
    , mSelectAtIdDisabled( false )
    , mWkbType( QGis::WKBUnknown )
{
  // do nothing
}

QgsDataSourceURI::QgsDataSourceURI( QString uri )
    : mSSLmode( SSLprefer )
    , mKeyColumn( "" )
    , mUseEstimatedMetadata( false )
    , mSelectAtIdDisabled( false )
    , mWkbType( QGis::WKBUnknown )
{
  int i = 0;
  while ( i < uri.length() )
  {
    skipBlanks( uri, i );

    if ( uri[i] == '=' )
    {
      QgsDebugMsg( "parameter name expected before =" );
      i++;
      continue;
    }

    int start = i;

    while ( i < uri.length() && uri[i] != '=' && !uri[i].isSpace() )
      i++;

    QString pname = uri.mid( start, i - start );

    skipBlanks( uri, i );

    if ( i == uri.length() || uri[i] != '=' )
    {
      QgsDebugMsg( "= expected after parameter name" );
      return;
    }

    i++;

    if ( pname == "sql" )
    {
      // rest of line is a sql where clause
      skipBlanks( uri, i );
      mSql = uri.mid( i );
      break;
    }
    else
    {
      QString pval = getValue( uri, i );

      if ( pname == "table" )
      {
        if ( uri[i] == '.' )
        {
          i++;

          mSchema = pval;
          mTable = getValue( uri, i );
        }
        else
        {
          mSchema = "";
          mTable = pval;
        }

        if ( uri[i] == '(' )
        {
          i++;

          int start = i;
          QString col;
          while ( i < uri.length() && uri[i] != ')' )
          {
            if ( uri[i] == '\\' )
              i++;
            i++;
          }

          if ( i == uri.length() )
          {
            QgsDebugMsg( "closing parenthesis missing" );
          }

          mGeometryColumn = uri.mid( start, i - start );
          mGeometryColumn.replace( "\\)", ")" );
          mGeometryColumn.replace( "\\\\", "\\" );

          i++;
        }
        else
        {
          mGeometryColumn = QString::null;
        }
      }
      else if ( pname == "key" )
      {
        mKeyColumn = pval;
      }
      else if ( pname == "estimatedmetadata" )
      {
        mUseEstimatedMetadata = pval == "true";
      }
      else if ( pname == "srid" )
      {
        mSrid = pval;
      }
      else if ( pname == "type" )
      {
        QString geomTypeUpper = pval.toUpper();
        if ( geomTypeUpper == "POINT" )
        {
          mWkbType = QGis::WKBPoint;
        }
        else if ( geomTypeUpper == "LINESTRING" || geomTypeUpper == "LINE" )
        {
          mWkbType = QGis::WKBLineString;
        }
        else if ( geomTypeUpper == "POLYGON" )
        {
          mWkbType = QGis::WKBPolygon;
        }
        else if ( geomTypeUpper == "MULTIPOINT" )
        {
          mWkbType = QGis::WKBMultiPoint;
        }
        else if ( geomTypeUpper == "MULTILINESTRING" )
        {
          mWkbType = QGis::WKBMultiLineString;
        }
        else if ( geomTypeUpper == "MULTIPOLYGON" )
        {
          mWkbType = QGis::WKBMultiPolygon;
        }
        else
        {
          mWkbType = QGis::WKBUnknown;
        }
      }
      else if ( pname == "selectatid" )
      {
        mSelectAtIdDisabled = pval == "false";
      }
      else if ( pname == "service" )
      {
        mService = pval;
      }
      else if ( pname == "user" )
      {
        mUsername = pval;
      }
      else if ( pname == "password" )
      {
        mPassword = pval;
      }
      else if ( pname == "connect_timeout" )
      {
        QgsDebugMsg( "connection timeout ignored" );
      }
      else if ( pname == "dbname" )
      {
        mDatabase = pval;
      }
      else if ( pname == "host" )
      {
        mHost = pval;
      }
      else if ( pname == "hostaddr" )
      {
        QgsDebugMsg( "database host ip address ignored" );
      }
      else if ( pname == "port" )
      {
        mPort = pval;
      }
      else if ( pname == "tty" )
      {
        QgsDebugMsg( "backend debug tty ignored" );
      }
      else if ( pname == "options" )
      {
        QgsDebugMsg( "backend debug options ignored" );
      }
      else if ( pname == "sslmode" )
      {
        if ( pval == "disable" )
          mSSLmode = SSLdisable;
        else if ( pval == "allow" )
          mSSLmode = SSLallow;
        else if ( pval == "prefer" )
          mSSLmode = SSLprefer;
        else if ( pval == "require" )
          mSSLmode = SSLrequire;
      }
      else if ( pname == "requiressl" )
      {
        if ( pval == "0" )
          mSSLmode = SSLdisable;
        else
          mSSLmode = SSLprefer;
      }
      else if ( pname == "krbsrvname" )
      {
        QgsDebugMsg( "kerberos server name ignored" );
      }
      else if ( pname == "gsslib" )
      {
        QgsDebugMsg( "gsslib ignored" );
      }
      else
      {
        QgsDebugMsg( "parameter \"" + pname + "\":\"" + pval + "\" added" );
        setParam( pname, pval );
      }
    }
  }
}

QString QgsDataSourceURI::removePassword( const QString& aUri )
{
  QRegExp regexp;
  regexp.setMinimal( true );
  QString safeName( aUri );
  if ( aUri.contains( " password=" ) )
  {
    regexp.setPattern( " password=.* " );
    safeName.replace( regexp, " " );
  }
  else if ( aUri.contains( ",password=" ) )
  {
    regexp.setPattern( ",password=.*," );
    safeName.replace( regexp, "," );
  }
  else if ( aUri.contains( "IDB:" ) )
  {
    regexp.setPattern( " pass=.* " );
    safeName.replace( regexp, " " );
  }
  else if (( aUri.contains( "OCI:" ) )
           || ( aUri.contains( "ODBC:" ) ) )
  {
    regexp.setPattern( "/.*@" );
    safeName.replace( regexp, "/@" );
  }
  else if ( aUri.contains( "SDE:" ) )
  {
    QStringList strlist = aUri.split( "," );
    safeName = strlist[0] + "," + strlist[1] + "," + strlist[2] + "," + strlist[3];
  }
  return safeName;
}

QString QgsDataSourceURI::username() const
{
  return mUsername;
}

void QgsDataSourceURI::setUsername( QString username )
{
  mUsername = username;
}

QString QgsDataSourceURI::service() const
{
  return mService;
}

QString QgsDataSourceURI::host() const
{
  return mHost;
}

QString QgsDataSourceURI::database() const
{
  return mDatabase;
}

QString QgsDataSourceURI::password() const
{
  return mPassword;
}

void QgsDataSourceURI::setPassword( QString password )
{
  mPassword = password;
}

QString QgsDataSourceURI::port() const
{
  return mPort;
}

QgsDataSourceURI::SSLmode QgsDataSourceURI::sslMode() const
{
  return mSSLmode;
}

QString QgsDataSourceURI::schema() const
{
  return mSchema;
}

QString QgsDataSourceURI::table() const
{
  return mTable;
}

QString QgsDataSourceURI::sql() const
{
  return mSql;
}

QString QgsDataSourceURI::geometryColumn() const
{
  return mGeometryColumn;
}

QString QgsDataSourceURI::keyColumn() const
{
  return mKeyColumn;
}

void QgsDataSourceURI::setKeyColumn( QString column )
{
  mKeyColumn = column;
}


void QgsDataSourceURI::setUseEstimatedMetadata( bool theFlag )
{
  mUseEstimatedMetadata = theFlag;
}

bool QgsDataSourceURI::useEstimatedMetadata() const
{
  return mUseEstimatedMetadata;
}

void QgsDataSourceURI::disableSelectAtId( bool theFlag )
{
  mSelectAtIdDisabled = theFlag;
}

bool QgsDataSourceURI::selectAtIdDisabled() const
{
  return mSelectAtIdDisabled;
}

void QgsDataSourceURI::setSql( QString sql )
{
  mSql = sql;
}

void QgsDataSourceURI::clearSchema()
{
  mSchema = "";
}

QString QgsDataSourceURI::escape( const QString &theVal, QChar delim = '\'' ) const
{
  QString val = theVal;

  val.replace( "\\", "\\\\" );
  val.replace( delim, QString( "\\%1" ).arg( delim ) );

  return val;
}

void QgsDataSourceURI::skipBlanks( const QString &uri, int &i )
{
  // skip space before value
  while ( i < uri.length() && uri[i].isSpace() )
    i++;
}

QString QgsDataSourceURI::getValue( const QString &uri, int &i )
{
  skipBlanks( uri, i );

  // Get the parameter value
  QString pval;
  if ( i < uri.length() && ( uri[i] == '\'' || uri[i] == '"' ) )
  {
    QChar delim = uri[i];

    i++;

    // value is quoted
    for ( ;; )
    {
      if ( i == uri.length() )
      {
        QgsDebugMsg( "unterminated quoted string in connection info string" );
        return pval;
      }

      if ( uri[i] == '\\' )
      {
        i++;
        if ( i == uri.length() )
          continue;
        if ( uri[i] != delim && uri[i] != '\\' )
          i--;
      }
      else if ( uri[i] == delim )
      {
        i++;
        break;
      }

      pval += uri[i++];
    }
  }
  else
  {
    // value is not quoted
    while ( i < uri.length() )
    {
      if ( uri[i].isSpace() )
      {
        // end of value
        break;
      }

      if ( uri[i] == '\\' )
      {
        i++;
        if ( i == uri.length() )
          break;
        if ( uri[i] != '\\' && uri[i] != '\'' )
          i--;
      }

      pval += uri[i++];
    }
  }

  skipBlanks( uri, i );

  return pval;
}

QString QgsDataSourceURI::connectionInfo() const
{
  QStringList connectionItems;

  if ( mDatabase != "" )
  {
    connectionItems << "dbname='" + escape( mDatabase ) + "'";
  }

  if ( mService != "" )
  {
    connectionItems << "service='" + escape( mService ) + "'";
  }
  else if ( mHost != "" )
  {
    connectionItems << "host=" + mHost;
  }

  if ( mService.isEmpty() )
  {
    if ( mPort != "" )
      connectionItems << "port=" + mPort;
  }

  if ( mUsername != "" )
  {
    connectionItems << "user='" + escape( mUsername ) + "'";

    if ( mPassword != "" )
    {
      connectionItems << "password='" + escape( mPassword ) + "'";
    }
  }

  if ( mSSLmode == SSLdisable )
    connectionItems << "sslmode=disable";
  else if ( mSSLmode == SSLallow )
    connectionItems << "sslmode=allow";
  else if ( mSSLmode == SSLrequire )
    connectionItems << "sslmode=require";
#if 0
  else if ( mSSLmode == SSLprefer )
    connectionItems << "sslmode=prefer";
#endif

  return connectionItems.join( " " );
}

QString QgsDataSourceURI::uri() const
{
  QString theUri = connectionInfo();

  if ( !mKeyColumn.isEmpty() )
  {
    theUri += QString( " key='%1'" ).arg( escape( mKeyColumn ) );
  }

  if ( mUseEstimatedMetadata )
  {
    theUri += QString( " estimatedmetadata=true" );
  }

  if ( !mSrid.isEmpty() )
  {
    theUri += QString( " srid=%1" ).arg( mSrid );
  }

  if ( mWkbType != QGis::WKBUnknown && mWkbType != QGis::WKBNoGeometry )
  {
    theUri += " type=";

    switch ( mWkbType )
    {
      case QGis::WKBPoint:
        theUri += "POINT";
        break;
      case QGis::WKBLineString:
        theUri += "LINESTRING";
        break;
      case QGis::WKBPolygon:
        theUri += "POLYGON";
        break;
      case QGis::WKBMultiPoint:
        theUri += "MULTIPOINT";
        break;
      case QGis::WKBMultiLineString:
        theUri += "MULTILINESTRING";
        break;
      case QGis::WKBMultiPolygon:
        theUri += "MULTIPOLYGON";
        break;
      case QGis::WKBPoint25D:
        theUri += "POINTM";
        break;
      case QGis::WKBLineString25D:
        theUri += "LINESTRINGM";
        break;
      case QGis::WKBPolygon25D:
        theUri += "POLYGONM";
        break;
      case QGis::WKBMultiPoint25D:
        theUri += "MULTIPOINTM";
        break;
      case QGis::WKBMultiLineString25D:
        theUri += "MULTILINESTRINGM";
        break;
      case QGis::WKBMultiPolygon25D:
        theUri += "MULTIPOLYGONM";
        break;
      case QGis::WKBUnknown:
      case QGis::WKBNoGeometry:
        break;
    }
  }

  if ( mSelectAtIdDisabled )
  {
    theUri += QString( " selectatid=false" );
  }

  for ( QMap<QString, QString>::const_iterator it = mParams.begin(); it != mParams.end(); ++it )
  {
    if ( it.key().contains( "=" ) || it.key().contains( " " ) )
    {
      QgsDebugMsg( QString( "invalid uri parameter %1 skipped" ).arg( it.key() ) );
      continue;
    }

    theUri += " " + it.key() + "='" + escape( it.value() ) + "'";
  }

  QString columnName( mGeometryColumn );
  columnName.replace( "\\", "\\\\" );
  columnName.replace( ")", "\\)" );

  theUri += QString( " table=%1%2 sql=%3" )
            .arg( quotedTablename() )
            .arg( mGeometryColumn.isNull() ? QString() : QString( " (%1)" ).arg( columnName ) )
            .arg( mSql );

  return theUri;
}

QByteArray QgsDataSourceURI::encodedUri() const
{
  QUrl url;
  foreach ( QString key, mParams.uniqueKeys() )
  {
    foreach ( QString value, mParams.values( key ) )
    {
      url.addQueryItem( key, value );
    }
  }
  return url.encodedQuery();
}

void QgsDataSourceURI::setEncodedUri( const QByteArray & uri )
{
  mParams.clear();
  QUrl url;
  url.setEncodedQuery( uri );
  QPair<QString, QString> item;
  foreach ( item, url.queryItems() )
  {
    mParams.insertMulti( item.first, item.second );
  }
}

void QgsDataSourceURI::setEncodedUri( const QString & uri )
{
  setEncodedUri( uri.toAscii() );
}

QString QgsDataSourceURI::quotedTablename() const
{
  if ( !mSchema.isEmpty() )
    return QString( "\"%1\".\"%2\"" )
           .arg( escape( mSchema, '"' ) )
           .arg( escape( mTable, '"' ) );
  else
    return QString( "\"%1\"" )
           .arg( escape( mTable, '"' ) );
}

void QgsDataSourceURI::setConnection( const QString &host,
                                      const QString &port,
                                      const QString &database,
                                      const QString &username,
                                      const QString &password,
                                      SSLmode sslmode )
{
  mHost = host;
  mDatabase = database;
  mPort = port;
  mUsername = username;
  mPassword = password;
  mSSLmode = sslmode;
}

void QgsDataSourceURI::setConnection( const QString &service,
                                      const QString &database,
                                      const QString &username,
                                      const QString &password,
                                      SSLmode sslmode )
{
  mService = service;
  mDatabase = database;
  mUsername = username;
  mPassword = password;
  mSSLmode = sslmode;
}

void QgsDataSourceURI::setDataSource( const QString &schema,
                                      const QString &table,
                                      const QString &geometryColumn,
                                      const QString &sql,
                                      const QString &keyColumn )
{
  mSchema = schema;
  mTable = table;
  mGeometryColumn = geometryColumn;
  mSql = sql;
  mKeyColumn = keyColumn;
}

void QgsDataSourceURI::setDatabase( const QString &database )
{
  mDatabase = database;
}

QGis::WkbType QgsDataSourceURI::wkbType() const
{
  return mWkbType;
}

void QgsDataSourceURI::setWkbType( QGis::WkbType wkbType )
{
  mWkbType = wkbType;
}

QString QgsDataSourceURI::srid() const
{
  return mSrid;
}

void QgsDataSourceURI::setSrid( QString srid )
{
  mSrid = srid;
}

void QgsDataSourceURI::setParam( const QString &key, const QString &value )
{
  // may be multiple
  mParams.insertMulti( key, value );
}

void QgsDataSourceURI::setParam( const QString &key, const QStringList &value )
{
  foreach ( QString val, value )
  {
    mParams.insertMulti( key, val );
  }
}

int QgsDataSourceURI::removeParam( const QString &key )
{
  return mParams.remove( key );
}

QString QgsDataSourceURI::param( const QString &key ) const
{
  return mParams.value( key );
}

QStringList QgsDataSourceURI::params( const QString &key ) const
{
  return mParams.values( key );
}

bool QgsDataSourceURI::hasParam( const QString &key ) const
{
  return mParams.contains( key );
}
