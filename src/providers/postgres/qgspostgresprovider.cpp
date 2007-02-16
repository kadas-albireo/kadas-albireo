/***************************************************************************
  qgspostgresprovider.cpp  -  QGIS data provider for PostgreSQL/PostGIS layers
                             -------------------
    begin                : 2004/01/07
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

/* $Id$ */

// for htonl
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#include <fstream>
#include <iostream>
#include <cassert>

#include <QStringList>
#include <QApplication>
#include <QEvent>
#include <QCustomEvent>
#include <QTextOStream>


#include <qgis.h>
#include <qgsapplication.h>
#include <qgsfeature.h>
#include <qgsfeatureattribute.h>
#include <qgsfield.h>
#include <qgsgeometry.h>
#include <qgsmessageoutput.h>
#include <qgsrect.h>
#include <qgsspatialrefsys.h>

#include "qgsprovidercountcalcevent.h"
#include "qgsproviderextentcalcevent.h"

#include "qgspostgresprovider.h"

#include "qgspostgrescountthread.h"
#include "qgspostgresextentthread.h"

#include "qgspostgisbox3d.h"
#include "qgslogger.h"

#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif

const QString POSTGRES_KEY = "postgres";
const QString POSTGRES_DESCRIPTION = "PostgreSQL/PostGIS data provider";


QgsPostgresProvider::QgsPostgresProvider(QString const & uri)
  : QgsVectorDataProvider(uri),
  geomType(QGis::WKBUnknown),
  gotPostgisVersion(FALSE)
{
  // assume this is a valid layer until we determine otherwise
  valid = true;
  /* OPEN LOG FILE */

  // Make connection to the data source
  // For postgres, the connection information is passed as a space delimited
  // string:
  //  host=192.168.1.5 dbname=test port=5342 user=gsherman password=xxx table=tablename

  QgsDebugMsg("Postgresql Layer Creation");
  QgsDebugMsg("URI: " + uri);

  mUri = QgsDataSourceURI(uri);

  /* populate members from the uri structure */
  mSchemaName = mUri.schema;
  mTableName = mUri.table;
  geometryColumn = mUri.geometryColumn;
  sqlWhereClause = mUri.sql;

  // Keep a schema qualified table name for convenience later on.
  if (mSchemaName.length() > 0)
    mSchemaTableName = "\"" + mSchemaName + "\".\"" + mTableName + "\"";
  else
    mSchemaTableName = "\"" + mTableName + "\"";


  QgsDebugMsg("Table name is " + mTableName);
  QgsDebugMsg("SQL is " + sqlWhereClause);
  QgsDebugMsg("Connection info is " + mUri.connInfo);
  
  QgsDebugMsg("Geometry column is: " + geometryColumn);
  QgsDebugMsg("Schema is: " + mSchemaName);
  QgsDebugMsg("Table name is: " + mTableName);
  
  //QString logFile = "./pg_provider_" + mTableName + ".log";
  //pLog.open((const char *)logFile);
  //QgsDebugMsg("Opened log file for " + mTableName);
  
  PGconn *pd = PQconnectdb((const char *) mUri.connInfo);
  // check the connection status
  if (PQstatus(pd) == CONNECTION_OK)
  {
    // store the connection for future use
    connection = pd;

    //set client encoding to unicode because QString uses UTF-8 anyway
    QgsDebugMsg("setting client encoding to UNICODE");
    
    int errcode=PQsetClientEncoding(connection, "UNICODE");
    
    if(errcode==0) {
      QgsDebugMsg("encoding successfully set");
    } else if(errcode==-1) {
        QgsDebugMsg("error in setting encoding");
    } else {
        QgsDebugMsg("undefined return value from encoding setting");
    }

    QgsDebugMsg("Checking for select permission on the relation\n");
    
    // Check that we can read from the table (i.e., we have
    // select permission).
    QString sql = "select * from " + mSchemaTableName + " limit 1";
    PGresult* testAccess = PQexec(pd, (const char*)(sql.utf8()));
    if (PQresultStatus(testAccess) != PGRES_TUPLES_OK)
    {
      showMessageBox(tr("Unable to access relation"),
          tr("Unable to access the ") + mSchemaTableName + 
          tr(" relation.\nThe error message from the database was:\n") +
          QString(PQresultErrorMessage(testAccess)) + ".\n" + 
          "SQL: " + sql);
      PQclear(testAccess);
      valid = false;
      return;
    }
    PQclear(testAccess);

    /* Check to see if we have GEOS support and if not, warn the user about
       the problems they will see :) */
    QgsDebugMsg("Checking for GEOS support");

    if(!hasGEOS(pd))
    {
      showMessageBox(tr("No GEOS Support!"),
		     tr("Your PostGIS installation has no GEOS support.\n"
			"Feature selection and identification will not "
			"work properly.\nPlease install PostGIS with " 
			"GEOS support (http://geos.refractions.net)"));
    }
    //--std::cout << "Connection to the database was successful\n";

    if (getGeometryDetails()) // gets srid and geometry type
    {
      deduceEndian();
      calculateExtents();
      getFeatureCount();

      // Populate the field vector for this layer. The field vector contains
      // field name, type, length, and precision (if numeric)
      sql = "select * from " + mSchemaTableName + " limit 1";

      PGresult* result = PQexec(pd, (const char *) (sql.utf8()));
      //--std::cout << "Field: Name, Type, Size, Modifier:" << std::endl;
      for (int i = 0; i < PQnfields(result); i++)
      {
        QString fieldName = PQfname(result, i);
        int fldtyp = PQftype(result, i);
        QString typOid = QString().setNum(fldtyp);
        int fieldModifier = PQfmod(result, i);

        sql = "select typelem from pg_type where typelem = " + typOid + " and typlen = -1";
        //  //--std::cout << sql << std::endl;
        PGresult *oidResult = PQexec(pd, (const char *) sql);
        // get the oid of the "real" type
        QString poid = PQgetvalue(oidResult, 0, PQfnumber(oidResult, "typelem"));
        PQclear(oidResult);

        sql = "select typname, typlen from pg_type where oid = " + poid;
        // //--std::cout << sql << std::endl;
        oidResult = PQexec(pd, (const char *) sql);
        QString fieldType = PQgetvalue(oidResult, 0, 0);
        QString fieldSize = PQgetvalue(oidResult, 0, 1);
        PQclear(oidResult);

        sql = "select oid from pg_class where relname = '" + mTableName + "' and relnamespace = ("
	  "select oid from pg_namespace where nspname = '" + mSchemaName + "')";
        PGresult *tresult= PQexec(pd, (const char *)(sql.utf8()));
        QString tableoid = PQgetvalue(tresult, 0, 0);
        PQclear(tresult);

        sql = "select attnum from pg_attribute where attrelid = " + tableoid + " and attname = '" + fieldName + "'";
        tresult = PQexec(pd, (const char *)(sql.utf8()));
        QString attnum = PQgetvalue(tresult, 0, 0);
        PQclear(tresult);

        QgsDebugMsg("Field: " + attnum + " maps to " + QString::number(i) + " " + fieldName + ", " 
          + fieldType + " (" + QString::number(fldtyp) + "),  " + fieldSize + ", " + QString::number(fieldModifier));
        
        attributeFieldsIdMap[attnum.toInt()] = i;

        if(fieldName!=geometryColumn)
        {
          attributeFields.insert(i, QgsField(fieldName, fieldType, fieldSize.toInt(), fieldModifier));
        }
      }
      PQclear(result);

      // set the primary key
      getPrimaryKey();
  
      // Set the postgresql message level so that we don't get the
      // 'there is no transaction in progress' warning.
#ifndef QGISDEBUG
      PQexec(connection, "set client_min_messages to error");
#endif

      // Kick off the long running threads

#ifdef POSTGRESQL_THREADS
      QgsDebugMsg("About to touch mExtentThread");
      mExtentThread.setConnInfo( mUri.connInfo );
      mExtentThread.setTableName( mTableName );
      mExtentThread.setSqlWhereClause( sqlWhereClause );
      mExtentThread.setGeometryColumn( geometryColumn );
      mExtentThread.setCallback( this );
      QgsDebugMsg("About to start mExtentThread");
      mExtentThread.start();
      QgsDebugMsg("Main thread just dispatched mExtentThread");

      QgsDebugMsg("About to touch mCountThread");
      mCountThread.setConnInfo( mUri.connInfo );
      mCountThread.setTableName( mTableName );
      mCountThread.setSqlWhereClause( sqlWhereClause );
      mCountThread.setGeometryColumn( geometryColumn );
      mCountThread.setCallback( this );
      QgsDebugMsg("About to start mCountThread");
      mCountThread.start();
      QgsDebugMsg("Main thread just dispatched mCountThread");
#endif
    } 
    else 
    {
      // the table is not a geometry table
      numberFeatures = 0;
      valid = false;
      
      QgsDebugMsg("Invalid Postgres layer");
    }

    ready = false; // not ready to read yet cuz the cursor hasn't been created

  } else {
    valid = false;
    //--std::cout << "Connection to database failed\n";
  }

  //fill type names into lists
  /* TODO [MD]  
  mNumericalTypes.push_back("double precision");
  mNumericalTypes.push_back("int4");
  mNumericalTypes.push_back("int8");
  mNonNumericalTypes.push_back("text");
  mNonNumericalTypes.push_back("varchar(30)");
  */

  if (primaryKey.isEmpty())
  {
    valid = false;
  }

  // Close the database connection if the layer isn't going to be loaded.
  if (!valid)
    PQfinish(connection);
}

QgsPostgresProvider::~QgsPostgresProvider()
{
#ifdef POSTGRESQL_THREADS
  QgsDebugMsg("About to wait for mExtentThread");

  mExtentThread.wait();

  QgsDebugMsg("Finished waiting for mExtentThread");

  QgsDebugMsg("About to wait for mCountThread");

  mCountThread.wait();

  QgsDebugMsg("Finished waiting for mCountThread");

  // Make sure all events from threads have been processed
  // (otherwise they will get destroyed prematurely)
  QApplication::sendPostedEvents(this, QGis::ProviderExtentCalcEvent);
  QApplication::sendPostedEvents(this, QGis::ProviderCountCalcEvent);
#endif
  PQfinish(connection);

  QgsDebugMsg("deconstructing.");

  //pLog.flush();
}

QString QgsPostgresProvider::storageType()
{
  return "PostgreSQL database with PostGIS extension";
}



bool QgsPostgresProvider::getNextFeature(QgsFeature& feat,
                                         bool fetchGeometry,
                                         QgsAttributeList fetchAttributes,
                                         uint featureQueueSize)
{

  if (valid)
  {
  
    // Top up our queue if it is empty
    if (mFeatureQueue.empty())
    {
    
      if (featureQueueSize < 1)
      {
        featureQueueSize = 1;
      }

      QString fetch = QString("fetch forward %1 from qgisf")
                         .arg(featureQueueSize);
                         
      queryResult = PQexec(connection, (const char *)fetch);
      
      int rows = PQntuples(queryResult);

      if (rows == 0)
      {
        QgsDebugMsg("End of features");
        
        if (ready)
          PQexec(connection, "end work");
        ready = false;
        return false;
      }
      
      for (int row = 0; row < rows; row++)
      {
        int oid = *(int *)PQgetvalue(queryResult, row, PQfnumber(queryResult,"\""+primaryKey+"\""));
        //  QgsDebugMsg("Primary key type is " + QString::number(primaryKeyType)); 
	
        if (swapEndian)
	  oid = ntohl(oid); // convert oid to opposite endian

        // set ID
        feat.setFeatureId(oid);

        // fetch attributes
        if (static_cast<uint>(fetchAttributes.count()) == fieldCount())
          getFeatureAttributes(oid, row, feat); // only one sql query to get all attributes
        else
          getFeatureAttributes(oid, row, feat, fetchAttributes); // ineffective: one sql per attribute
        
        if (fetchGeometry)
        {
          int returnedLength = PQgetlength(queryResult, row, PQfnumber(queryResult,"qgs_feature_geometry")); 
          if(returnedLength > 0)
          {
            unsigned char *feature = new unsigned char[returnedLength + 1];
            memset(feature, '\0', returnedLength + 1);
            memcpy(feature, PQgetvalue(queryResult, row, PQfnumber(queryResult,"qgs_feature_geometry")), returnedLength); 
            
            // Too verbose
            //int wkbType = *((int *) (feature + 1));
            //QgsDebugMsg("WKBtype is: " + QString::number(wkbType));
            
            feat.setGeometryAndOwnership(feature, returnedLength + 1);
      
          }
          else
          {
            //QgsDebugMsg("Couldn't get the feature geometry in binary form");
          }
        }

        //QgsDebugMsg(" pushing " + QString::number(f->featureId()); 
  
        mFeatureQueue.push(feat);
        
      } // for each row in queue
      
      QgsDebugMsg("retrieved batch of features.");
            
      PQclear(queryResult);
      
    } // if new queue is required
    
    // Now return the next feature from the queue
    
    feat = mFeatureQueue.front();
    mFeatureQueue.pop();
    
  }
  else 
  {
    //QgsDebugMsg("Read attempt on an invalid postgresql data source");
    return false;
  }
  
  //QgsDebugMsg("returning feature " + QString::number(feat.featureId())); 

  return true;
}


/**
 * Select features based on a bounding rectangle. Features can be retrieved
 * with calls to getFirstFeature and getNextFeature.
 * @param mbr QgsRect containing the extent to use in selecting features
 */
void QgsPostgresProvider::select(QgsRect rect, bool useIntersect)
{
  // spatial query to select features

  QgsDebugMsg("Selection rectangle is " + rect.stringRep());
  QgsDebugMsg("Selection polygon is " + rect.asPolygon());

  QString declare = QString("declare qgisf binary cursor for select \""
      + primaryKey  
      + "\",asbinary(\"%1\",'%2') as qgs_feature_geometry from %3").arg(geometryColumn).arg(endianString()).arg(mSchemaTableName);
  
  QgsDebugMsg("Binary cursor: " + declare); 
  
  if(useIntersect){
    //    declare += " where intersects(" + geometryColumn;
    //    declare += ", GeometryFromText('BOX3D(" + rect.asWKTCoords();
    //    declare += ")'::box3d,";
    //    declare += srid;
    //    declare += "))";

    // Contributed by #qgis irc "creeping"
    // This version actually invokes PostGIS's use of spatial indexes
    declare += " where " + geometryColumn;
    declare += " && setsrid('BOX3D(" + rect.asWKTCoords();
    declare += ")'::box3d,";
    declare += srid;
    declare += ")";
    declare += " and intersects(" + geometryColumn;
    declare += ", setsrid('BOX3D(" + rect.asWKTCoords();
    declare += ")'::box3d,";
    declare += srid;
    declare += "))";
  }else{
    declare += " where " + geometryColumn;
    declare += " && setsrid('BOX3D(" + rect.asWKTCoords();
    declare += ")'::box3d,";
    declare += srid;
    declare += ")";
  }
  if(sqlWhereClause.length() > 0)
  {
    declare += " and (" + sqlWhereClause + ")";
  }

  QgsDebugMsg("Selecting features using: " + declare);
  
  // set up the cursor
  if(ready){
    PQexec(connection, "end work");
  }
  PQexec(connection,"begin work");
  ready = true;
  PQexec(connection, (const char *)(declare.utf8()));
  
  mFeatureQueue.empty();
}


QgsDataSourceURI& QgsPostgresProvider::getURI()
{
  return mUri;
}


/* unsigned char * QgsPostgresProvider::getGeometryPointer(OGRFeature *fet){
//  OGRGeometry *geom = fet->GetGeometryRef();
unsigned char *gPtr=0;
// get the wkb representation
gPtr = new unsigned char[geom->WkbSize()];

geom->exportToWkb((OGRwkbByteOrder) endian(), gPtr);
return gPtr;

} */

void QgsPostgresProvider::setExtent( QgsRect& newExtent )
{
  layerExtent.setXmax( newExtent.xMax() );
  layerExtent.setXmin( newExtent.xMin() );
  layerExtent.setYmax( newExtent.yMax() );
  layerExtent.setYmin( newExtent.yMin() );
}

// TODO - make this function return the real extent_
QgsRect QgsPostgresProvider::extent()
{
  return layerExtent;      //extent_->MinX, extent_->MinY, extent_->MaxX, extent_->MaxY);
}

/** 
 * Return the feature type
 */
QGis::WKBTYPE QgsPostgresProvider::geometryType() const
{
  return geomType;
}

/** 
 * Return the feature type
 */
long QgsPostgresProvider::featureCount() const
{
  return numberFeatures;
}

/**
 * Return the number of fields
 */
uint QgsPostgresProvider::fieldCount() const
{
  return attributeFields.size();
}

/**
 * Fetch attributes for a selected feature
 */
void QgsPostgresProvider::getFeatureAttributes(int key, int &row, QgsFeature& f)
{

  QString sql = QString("select * from %1 where \"%2\" = %3").arg(mSchemaTableName).arg(primaryKey).arg(key);

  QgsDebugMsg("using: " + sql);
  
  PGresult *attr = PQexec(connection, (const char *)(sql.utf8()));

  for (int i = 0; i < PQnfields(attr); i++) {
    QString fld = PQfname(attr, i);
    // Dont add the WKT representation of the geometry column to the identify
    // results
    if(fld != geometryColumn){
      // Add the attribute to the feature
      //QString val = mEncoding->toUnicode(PQgetvalue(attr,0, i));
	QString val = QString::fromUtf8 (PQgetvalue(attr, row, i));
  f.addAttribute(i, QgsFeatureAttribute(fld, val));
    }
  }
  PQclear(attr);
} 

/**Fetch attributes with indices contained in attlist*/
void QgsPostgresProvider::getFeatureAttributes(int key, int &row,
    QgsFeature &f, const QgsAttributeList& attlist)
{
  int i = 0;
  QgsAttributeList::const_iterator iter;
  for(iter = attlist.begin(); iter != attlist.end(); ++iter, ++i)
  {
    QString sql = QString("select \"%1\" from %2 where \"%3\" = %4")
      .arg(fields()[*iter].name())
      .arg(mSchemaTableName)
      .arg(primaryKey)
      .arg(key);//todo: only query one attribute

    PGresult *attr = PQexec(connection, (const char *)(sql.utf8()));
    QString fld = PQfname(attr, 0);

    // Dont add the WKT representation of the geometry column to the identify
    // results
    if(fld != geometryColumn)
    {
      // Add the attribute to the feature
	QString val = QString::fromUtf8(PQgetvalue(attr, 0, 0));
  f.addAttribute(i, QgsFeatureAttribute(fld, val));
    }
    PQclear(attr);
  }
}


void QgsPostgresProvider::getFeatureGeometry(int key, QgsFeature& f)
{
  if (!valid)
  {
    return;
  }

  QString cursor = QString("declare qgisf binary cursor for "
                           "select asbinary(\"%1\",'%2') from %3 where \"%4\" = %5")
                   .arg(geometryColumn)
                   .arg(endianString())
                   .arg(mSchemaTableName)
                   .arg(primaryKey)
                   .arg(key);

  QgsDebugMsg("using: " + cursor); 
  
  if (ready)
    PQexec(connection, "end work");
  PQexec(connection, "begin work");
  ready = true;
  PQexec(connection, (const char *)(cursor.utf8()));

  QString fetch = "fetch forward 1 from qgisf";
  PGresult *geomResult = PQexec(connection, (const char *)fetch);

  if (PQntuples(geomResult) == 0)
  {
    // Nothing found - therefore nothing to change
    if (ready)
      PQexec(connection,"end work");
    ready = false;
    PQclear(geomResult);
    return;
  }

  int row = 0;

  int returnedLength = PQgetlength(geomResult, row, 0);

  if(returnedLength > 0)
  {
      unsigned char *wkbgeom = new unsigned char[returnedLength];
      memcpy(wkbgeom, PQgetvalue(geomResult, row, 0), returnedLength);
      f.setGeometryAndOwnership(wkbgeom, returnedLength);
  }
  else
  {
    //QgsDebugMsg("Couldn't get the feature geometry in binary form");
  }

  PQclear(geomResult);

  if (ready)
    PQexec(connection,"end work");
  ready = false;

}


const QgsFieldMap & QgsPostgresProvider::fields() const
{
  return attributeFields;
}

void QgsPostgresProvider::reset()
{
  // reset the cursor to the first record
  //QgsDebugMsg("Resetting the cursor to the first record ");
  QString declare = QString("declare qgisf binary cursor for select \"" +
      primaryKey + 
      "\",asbinary(\"%1\",'%2') as qgs_feature_geometry from %3").arg(geometryColumn)
    .arg(endianString()).arg(mSchemaTableName);
  if(sqlWhereClause.length() > 0)
  {
    declare += " where " + sqlWhereClause;
  }
  
  QgsDebugMsg("Setting up binary cursor: " + declare);
  
  // set up the cursor
  if (ready)
    PQexec(connection,"end work");

  PQexec(connection,"begin work");
  PQexec(connection, (const char *)(declare.utf8()));
  //--std::cout << "Error: " << PQerrorMessage(connection) << std::endl;
  
  mFeatureQueue.empty();

  ready = true;
}
/* QString QgsPostgresProvider::getFieldTypeName(PGconn * pd, int oid)
   {
   QString typOid = QString().setNum(oid);
   QString sql = "select typelem from pg_type where typelem = " + typOid + " and typlen = -1";
////--std::cout << sql << std::endl;
PGresult *result = PQexec(pd, (const char *) sql);
// get the oid of the "real" type
QString poid = PQgetvalue(result, 0, PQfnumber(result, "typelem"));
PQclear(result);
sql = "select typname, typlen from pg_type where oid = " + poid;
// //--std::cout << sql << std::endl;
result = PQexec(pd, (const char *) sql);

QString typeName = PQgetvalue(result, 0, 0);
QString typeLen = PQgetvalue(result, 0, 1);
PQclear(result);
typeName += "(" + typeLen + ")";
return typeName;
} */

/** @todo XXX Perhaps this should be promoted to QgsDataProvider? */
QString QgsPostgresProvider::endianString()
{
  switch ( QgsApplication::endian() )
  {
    case QgsApplication::NDR : 
      return QString("NDR");
      break;
    case QgsApplication::XDR : 
      return QString("XDR");
      break;
    default :
      return QString("UNKNOWN");
  }
}

QString QgsPostgresProvider::getPrimaryKey()
{
  // check to see if there is an unique index on the relation, which
  // can be used as a key into the table. Primary keys are always
  // unique indices, so we catch them as well.

  QString sql = "select indkey from pg_index where indisunique = 't' and "
    "indrelid = (select oid from pg_class where relname = '"
    + mTableName + "' and relnamespace = (select oid from pg_namespace where "
    "nspname = '" + mSchemaName + "'))";

  QgsDebugMsg("Getting unique index using '" + sql + "'");
  
  PGresult *pk = executeDbCommand(connection, sql);

  QgsDebugMsg("Got " + QString::number(PQntuples(pk)) + " rows.");

  QStringList log;

  // if we got no tuples we ain't got no unique index :)
  if (PQntuples(pk) == 0)
  {
    QgsDebugMsg("Relation has no unique index -- investigating alternatives");

    // Two options here. If the relation is a table, see if there is
    // an oid column that can be used instead.
    // If the relation is a view try to find a suitable column to use as
    // the primary key.

    sql = "select relkind from pg_class where relname = '" + mTableName + 
      "' and relnamespace = (select oid from pg_namespace where "
      "nspname = '" + mSchemaName + "')";
    PGresult* tableType = executeDbCommand(connection, sql);
    QString type = PQgetvalue(tableType, 0, 0);
    PQclear(tableType);

    primaryKey = "";

    if (type == "r") // the relation is a table
    {
      QgsDebugMsg("Relation is a table. Checking to see if it has an oid column.");
      
      // If there is an oid on the table, use that instead,
      // otherwise give up
      sql = "select attname from pg_attribute where attname = 'oid' and "
	"attrelid = (select oid from pg_class where relname = '" +
	mTableName + "' and relnamespace = (select oid from pg_namespace "
	"where nspname = '" + mSchemaName + "'))";
      PGresult* oidCheck = executeDbCommand(connection, sql);

      if (PQntuples(oidCheck) != 0)
      {
        // Could warn the user here that performance will suffer if
        // oid isn't indexed (and that they may want to add a
        // primary key to the table)
	primaryKey = "oid";
	primaryKeyType = "int4";
      }
      else
      {
        showMessageBox(tr("No suitable key column in table"),
            tr("The table has no column suitable for use as a key.\n\n"
	       "Qgis requires that the table either has a column of type\n"
               "int4 with a unique constraint on it (which includes the\n"
               "primary key) or has a PostgreSQL oid column.\n"));
      }
      PQclear(oidCheck);
    }
    else if (type == "v") // the relation is a view
    {
      // Have a poke around the view to see if any of the columns
      // could be used as the primary key.
      tableCols cols;
      // Given a schema.view, populate the cols variable with the
      // schema.table.column's that underly the view columns.
      findColumns(cols);
      // From the view columns, choose one for which the underlying
      // column is suitable for use as a key into the view.
      primaryKey = chooseViewColumn(cols);
    }
    else
      QgsDebugMsg("Unexpected relation type of '" + type + "'.");
  }
  else // have some unique indices on the table. Now choose one...
  {
    // choose which (if more than one) unique index to use
    std::vector<std::pair<QString, QString> > suitableKeyColumns;
    for (int i = 0; i < PQntuples(pk); ++i)
    {
      QString col = PQgetvalue(pk, i, 0);
      QStringList columns = QStringList::split(" ", col);
      if (columns.count() == 1)
      {
	// Get the column name and data type
	sql = "select attname, pg_type.typname from pg_attribute, pg_type where "
	  "atttypid = pg_type.oid and attnum = " +
	  col + " and attrelid = (select oid from pg_class where " +
	  "relname = '" + mTableName + "' and relnamespace = (select oid "
	  "from pg_namespace where nspname = '" + mSchemaName + "'))";
	PGresult* types = executeDbCommand(connection, sql);

	assert(PQntuples(types) > 0); // should never happen

	QString columnName = PQgetvalue(types, 0, 0);
	QString columnType = PQgetvalue(types, 0, 1);

	if (columnType != "int4")
	  log.append(tr("The unique index on column") + 
                     " '" + columnName + "' " +
		     tr("is unsuitable because Qgis does not currently support"
                        " non-int4 type columns as a key into the table.\n"));
	else
	  suitableKeyColumns.push_back(std::make_pair(columnName, columnType));
	  
	PQclear(types);
      }
      else
      {
	sql = "select attname from pg_attribute, pg_type where "
	  "atttypid = pg_type.oid and attnum in (" +
	  col.replace(" ", ",") 
          + ") and attrelid = (select oid from pg_class where " +
	  "relname = '" + mTableName + "' and relnamespace = (select oid "
	  "from pg_namespace where nspname = '" + mSchemaName + "'))";
	PGresult* types = executeDbCommand(connection, sql);
        QString colNames;
        int numCols = PQntuples(types);
        for (int j = 0; j < numCols; ++j)
        {
          if (j == numCols-1)
            colNames += tr("and ");
          colNames += "'" + QString(PQgetvalue(types, j, 0)) 
            + (j < numCols-2 ? "', " : "' ");
        }

	log.append(tr("The unique index based on columns ") + colNames + 
		   tr(" is unsuitable because Qgis does not currently support"
                      " multiple columns as a key into the table.\n"));
      }
    }

    // suitableKeyColumns now contains the name of columns (and their
    // data type) that
    // are suitable for use as a key into the table. If there is
    // more than one we need to choose one. For the moment, just
    // choose the first in the list.

    if (suitableKeyColumns.size() > 0)
    {
      primaryKey = suitableKeyColumns[0].first;
      primaryKeyType = suitableKeyColumns[0].second;
    }
    else
    {
      // If there is an oid on the table, use that instead,
      // otherwise give up
      sql = "select attname from pg_attribute where attname = 'oid' and "
	"attrelid = (select oid from pg_class where relname = '" +
	mTableName + "' and relnamespace = (select oid from pg_namespace "
	"where nspname = '" + mSchemaName + "'))";
      PGresult* oidCheck = executeDbCommand(connection, sql);

      if (PQntuples(oidCheck) != 0)
      {
	primaryKey = "oid";
	primaryKeyType = "int4";
      }
      else
      {
	log.prepend("There were no columns in the table that were suitable "
		   "as a qgis key into the table (either a column with a "
		   "unique index and type int4 or a PostgreSQL oid column.\n");
      }
      PQclear(oidCheck);
    }

    // Either primaryKey has been set by the above code, or it
    // hasn't. If not, present some info to the user to give them some
    // idea of why not.
    if (primaryKey.isEmpty())
    {
      // Give some info to the user about why things didn't work out.
      valid = false;
      showMessageBox(tr("Unable to find a key column"), log);
    }
  }
  PQclear(pk);

  if (primaryKey.length() > 0) {
    QgsDebugMsg("Qgis row key is " + primaryKey);
  } else {
    QgsDebugMsg("Qgis row key was not set.");
  }

  return primaryKey;
}

// Given the table and column that each column in the view refers to,
// choose one. Prefers column with an index on them, but will
// otherwise choose something suitable.

QString QgsPostgresProvider::chooseViewColumn(const tableCols& cols)
{
  // For each relation name and column name need to see if it
  // has unique constraints on it, or is a primary key (if not,
  // it shouldn't be used). Should then be left with one or more
  // entries in the map which can be used as the key.

  QString sql, key;
  QStringList log;
  tableCols suitable;
  // Cache of relation oid's
  std::map<QString, QString> relOid;

  std::vector<tableCols::const_iterator> oids;
  tableCols::const_iterator iter = cols.begin();
  for (; iter != cols.end(); ++iter)
  {
    QString viewCol   = iter->first;
    QString schemaName = iter->second.schema;
    QString tableName = iter->second.relation;
    QString tableCol  = iter->second.column;
    QString colType   = iter->second.type;

    // Get the oid from pg_class for the given schema.relation for use
    // in subsequent queries.
    sql = "select oid from pg_class where relname = '" + tableName +
      "' and relnamespace = (select oid from pg_namespace where "
      " nspname = '" + schemaName + "')";
    PGresult* result = PQexec(connection, (const char*)(sql.utf8()));
    QString rel_oid;
    if (PQntuples(result) == 1)
    {
      rel_oid = PQgetvalue(result, 0, 0);
      // Keep the rel_oid for use later one.
      relOid[viewCol] = rel_oid;
    }
    else
      {
        QgsDebugMsg("Relation " + schemaName + "." + tableName +
                 " doesn't exist in the pg_class table."
                 "This shouldn't happen and is odd.");
	assert(0);
      }
    PQclear(result);
      
    // This sql returns one or more rows if the column 'tableCol' in 
    // table 'tableName' and schema 'schemaName' has one or more
    // columns that satisfy the following conditions:
    // 1) the column has data type of int4.
    // 2) the column has a unique constraint or primary key constraint
    //    on it.
    // 3) the constraint applies just to the column of interest (i.e.,
    //    it isn't a constraint over multiple columns.
    sql = "select * from pg_constraint where conkey[1] = "
      "(select attnum from pg_attribute where attname = '" + tableCol + "' "
      "and attrelid = " + rel_oid + ")"
      "and conrelid = " + rel_oid + " "
      "and (contype = 'p' or contype = 'u') "
      "and array_dims(conkey) = '[1:1]'";

    result = PQexec(connection, (const char*)(sql.utf8()));
    if (PQntuples(result) == 1 && colType == "int4")
      suitable[viewCol] = iter->second;

    QString details = "'" + viewCol + "'" + tr(" derives from ") 
      + "'" + schemaName + "." + tableName + "." + tableCol + "' ";

    if (PQntuples(result) == 1 && colType == "int4")
    {
      details += tr("and is suitable.");
    }
    else
    {
      details += tr("and is not suitable ");
      details += "(" + tr("type is ") + colType;
      if (PQntuples(result) == 1)
        details += tr(" and has a suitable constraint)");
      else
        details += tr(" and does not have a suitable constraint)");
    }

    log << details;

    PQclear(result);
    if (tableCol == "oid")
      oids.push_back(iter);
  }

  // 'oid' columns in tables don't have a constraint on them, but
  // they are useful to consider, so add them in if not already
  // here.
  for (uint i = 0; i < oids.size(); ++i)
  {
    if (suitable.find(oids[i]->first) == suitable.end())
    {
      suitable[oids[i]->first] = oids[i]->second;
      
      QgsDebugMsg("Adding column " + oids[i]->first + " as it may be suitable.");
    }
  }

  // Now have a map containing all of the columns in the view that
  // might be suitable for use as the key to the table. Need to choose
  // one thus:
  //
  // If there is more than one suitable column pick one that is
  // indexed, else pick one called 'oid' if it exists, else
  // pick the first one. If there are none we return an empty string. 

  // Search for one with an index
  tableCols::const_iterator i = suitable.begin();
  for (; i != suitable.end(); ++i)
  {
    // Get the relation oid from our cache.
    QString rel_oid = relOid[i->first];
    // And see if the column has an index
    sql = "select * from pg_index where indrelid = " + rel_oid +
      " and indkey[0] = (select attnum from pg_attribute where "
      "attrelid = " +	rel_oid + " and attname = '" + i->second.column + "')";
    PGresult* result = PQexec(connection, (const char*)(sql.utf8()));

    if (PQntuples(result) > 0 && uniqueData(mSchemaName, mTableName, i->first))
    { // Got one. Use it.
      key = i->first;
      
      QgsDebugMsg("Picked column '" + key + "' because it has an index.");
      break;
    }
    PQclear(result);
  }

  if (key.isEmpty())
  {
    // If none have indices, choose one that is called 'oid' (if it
    // exists). This is legacy support and could be removed in
    // future. 
    i = suitable.find("oid");
    if (i != suitable.end() && uniqueData(mSchemaName, mTableName, i->first))
    {
      key = i->first;
      
      QgsDebugMsg("Picked column " + key +
                  " as it is probably the postgresql object id "
                  " column (which contains unique values) and there are no"
                  " columns with indices to choose from.");
    }
    // else choose the first one in the container that has unique data
    else
    {
      tableCols::const_iterator i = suitable.begin();
      for (; i != suitable.end(); ++i)
      {
        if (uniqueData(mSchemaName, mTableName, i->first))
        {
          key = i->first;
          
          QgsDebugMsg("Picked column " + key +
                      " as it was the first suitable column found"
                      " with unique data and were are no"
                      " columns with indices to choose from");
          break;
        }
        else
        {
          log << QString(tr("Note: ") + "'" + i->first + "' "
                         + tr("initially appeared suitable but does not "
                              "contain unique data, so is not suitable.\n"));
        }
      }
    }
  }

  if (key.isEmpty())
  {
    valid = false;
    // Successive prepends means that the text appears in the dialog
    // box in the reverse order to that seen here.
    log.prepend(tr("The view you selected has the following columns, none "
                   "of which satisfy the above conditions:"));
    log.prepend(tr("Qgis requires that the view has a column that can be used "
                   "as a unique key. Such a column should be derived from "
                   "a table column of type int4 and be a primary key, "
                   "have a unique constraint on it, or be a PostgreSQL "
                   "oid column. To improve "
                   "performance the column should also be indexed.\n"));
    log.prepend(tr("The view ") + "'" + mSchemaName + '.' + mTableName + "' " +
                tr("has no column suitable for use as a unique key.\n"));
    showMessageBox(tr("No suitable key column in view"), log);
  }

  return key;
}

bool QgsPostgresProvider::uniqueData(QString schemaName, 
				     QString tableName, QString colName)
{
  // Check to see if the given column contains unique data

  bool isUnique = false;

  QString sql = "select count(distinct \"" + colName + "\") = count(\"" +
      colName + "\") from \"" + schemaName + "\".\"" + tableName + "\"";

  PGresult* unique = PQexec(connection, (const char*) (sql.utf8()));

  if (PQntuples(unique) == 1)
      if (strncmp(PQgetvalue(unique, 0, 0),"t", 1) == 0)
	isUnique = true;

    PQclear(unique);

  return isUnique;
}

// This function will return in the cols variable the 
// underlying view and columns for each column in
// mSchemaName.mTableName.

void QgsPostgresProvider::findColumns(tableCols& cols)
{
  // This sql is derived from the one that defines the view
  // 'information_schema.view_column_usage' in PostgreSQL, with a few
  // mods to suit our purposes. 
  QString sql = ""
    "SELECT DISTINCT "
    "	nv.nspname AS view_schema, "
    "	v.relname AS view_name, "
    "	a.attname AS view_column_name, "
    "	nt.nspname AS table_schema, "
    "	t.relname AS table_name, "
    "	a.attname AS column_name, "
    "	t.relkind as table_type, "
    "	typ.typname as column_type "
    "FROM "
    "	pg_namespace nv, "
    "	pg_class v, "
    "	pg_depend dv,"
    "	pg_depend dt, "
    "	pg_class t, "
    "	pg_namespace nt, "
    "	pg_attribute a,"
    "	pg_user u, "
    "	pg_type typ "
    "WHERE "
    "	nv.oid = v.relnamespace AND "
    "	v.relkind = 'v'::\"char\" AND "
    "	v.oid = dv.refobjid AND "
    "	dv.refclassid = 'pg_class'::regclass::oid AND "
    "	dv.classid = 'pg_rewrite'::regclass::oid AND "
    "	dv.deptype = 'i'::\"char\" AND "
    "	dv.objid = dt.objid AND "
    "	dv.refobjid <> dt.refobjid AND "
    "	dt.classid = 'pg_rewrite'::regclass::oid AND "
    "	dt.refclassid = 'pg_class'::regclass::oid AND "
    "	dt.refobjid = t.oid AND "
    "	t.relnamespace = nt.oid AND "
    "	(t.relkind = 'r'::\"char\" OR t.relkind = 'v'::\"char\") AND "
    "	t.oid = a.attrelid AND "
    "	dt.refobjsubid = a.attnum AND "
    "	nv.nspname NOT IN ('pg_catalog', 'information_schema' ) AND "
    "	a.atttypid = typ.oid";

  // A structure to store the results of the above sql.
  typedef std::map<QString, TT> columnRelationsType;
  columnRelationsType columnRelations;

  // A structure to cache the query results that return the view 
  // definition. 
  typedef QMap<QString, QString> viewDefCache; 
  viewDefCache viewDefs; 

  PGresult* result = PQexec(connection, (const char*)(sql.utf8()));
  // Store the results of the query for convenient access 

  for (int i = 0; i < PQntuples(result); ++i)
  {
    TT temp;
    temp.view_schema      = PQgetvalue(result, i, 0);
    temp.view_name        = PQgetvalue(result, i, 1);
    temp.view_column_name = PQgetvalue(result, i, 2);
    temp.table_schema     = PQgetvalue(result, i, 3);
    temp.table_name       = PQgetvalue(result, i, 4);
    temp.column_name      = PQgetvalue(result, i, 5);
    temp.table_type       = PQgetvalue(result, i, 6);
    temp.column_type      = PQgetvalue(result, i, 7);

    // BUT, the above SQL doesn't always give the correct value for the view 
    // column name (that's because that information isn't available directly 
    // from the database), mainly when the view column name has been renamed 
    // using 'AS'. To fix this we need to look in the view definition and 
    // adjust the view column name if necessary. 

    
    QString viewQuery = "SELECT definition FROM pg_views "
      "WHERE schemaname = '" + temp.view_schema + "' AND "
      "viewname = '" + temp.view_name + "'";
 
    // Maintain a cache of the above SQL.
    QString viewDef;
    if (!viewDefs.contains(viewQuery))
    {
      PGresult* r = PQexec(connection, (const char*)(viewQuery.utf8()));
      if (PQntuples(r) > 0)
        viewDef = PQgetvalue(r, 0, 0);
      else
        QgsDebugMsg("Failed to get view definition for " + temp.view_schema + "." + temp.view_name);
      viewDefs[viewQuery] = viewDef;
    }

    viewDef = viewDefs.value(viewQuery);

    // Now pick the view definiton apart, looking for
    // temp.column_name to the left of an 'AS'.

    // This regular expression needs more testing. Since the view
    // definition comes from postgresql and has been 'standardised', we
    // don't need to deal with everything that the user could put in a view
    // definition. Does the regexp have to deal with the schema??
    if (!viewDef.isEmpty())
    {
      QRegExp s(".* \"?" + QRegExp::escape(temp.table_name) +
                "\"?\\.\"?" + QRegExp::escape(temp.column_name) +
                "\"? AS \"?(\\w+)\"?,* .*");
 
      QgsDebugMsg(viewQuery + "\n" + viewDef + "\n" + s.pattern());

      if (s.indexIn(viewDef) != -1)
      {
        temp.view_column_name = s.cap(1);
        //std::cerr<<__FILE__<<__LINE__<<' '<<temp.view_column_name.toLocal8Bit().data()<<'\n';
      }
    }

    QgsDebugMsg(temp.view_schema + "." 
              + temp.view_name + "." 
              + temp.view_column_name + " <- " 
              + temp.table_schema + "." 
              + temp.table_name + "." 
              + temp.column_name + " is a '" 
              + temp.table_type + "' of type " 
              + temp.column_type);

    columnRelations[temp.view_schema + '.' +
		    temp.view_name + '.' +
		    temp.view_column_name] = temp;
  }
  PQclear(result);

  // Loop over all columns in the view in question. 

  sql = "SELECT pg_namespace.nspname || '.' || "
    "pg_class.relname || '.' || pg_attribute.attname "
    "FROM pg_attribute, pg_class, pg_namespace "
    "WHERE pg_class.relname = '" + mTableName + "' "
    "AND pg_namespace.nspname = '" + mSchemaName + "' "
    "AND pg_attribute.attrelid = pg_class.oid "
    "AND pg_class.relnamespace = pg_namespace.oid";

  result = PQexec(connection, (const char*)(sql.utf8()));

  // Loop over the columns in mSchemaName.mTableName and find out the
  // underlying schema, table, and column name.
  for (int i = 0; i < PQntuples(result); ++i)
  {
    columnRelationsType::const_iterator 
      ii = columnRelations.find(PQgetvalue(result, i, 0));
    columnRelationsType::const_iterator start_iter = ii;

    if (ii == columnRelations.end())
      continue;

    int count = 0;
    const int max_loops = 100;

    while (ii->second.table_type != "r" && count < max_loops)
    {
      QgsDebugMsg("Searching for the column that " +
		  ii->second.table_schema + '.'+
		  ii->second.table_name + "." +
                  ii->second.column_name + " refers to.");

      columnRelationsType::const_iterator 
        jj = columnRelations.find(QString(ii->second.table_schema + '.' +
                                          ii->second.table_name + '.' +
                                          ii->second.column_name));

      if (jj == columnRelations.end())
      {
        QgsDebugMsg("WARNING: Failed to find the column that " +
                    ii->second.table_schema + "." +
                    ii->second.table_name + "." +
                    ii->second.column_name + " refers to.");
      break;
      }

      ii = jj;
      ++count;
    }

    if (count >= max_loops)
    {
      QgsDebugMsg("Search for the underlying table.column for view column " +
		  ii->second.table_schema + "." + 
		  ii->second.table_name + "." +
		  ii->second.column_name + " failed: exceeded maximum "
		  "interation limit (" + QString::number(max_loops) + ")");

      cols[ii->second.view_column_name] = SRC("","","","");
    }
    else if (ii != columnRelations.end())
    {
      cols[start_iter->second.view_column_name] = 
	SRC(ii->second.table_schema, 
	    ii->second.table_name, 
	    ii->second.column_name, 
            ii->second.column_type);

      QgsDebugMsg( QString(PQgetvalue(result, i, 0)) + " derives from " +
		   ii->second.table_schema + "." +
		   ii->second.table_name + "." +
		   ii->second.column_name);
    }
  }
  PQclear(result);
}

// Returns the minimum value of an attribute
QString QgsPostgresProvider::minValue(uint position){
  // get the field name 
  QgsField fld = attributeFields[position];
  QString sql;
  if(sqlWhereClause.isEmpty())
  {
    sql = QString("select min(\"%1\") from %2").arg(fld.name()).arg(mSchemaTableName);
  }
  else
  {
    sql = QString("select min(\"%1\") from %2").arg(fld.name()).arg(mSchemaTableName)+" where "+sqlWhereClause;
  }
  PGresult *rmin = PQexec(connection,(const char *)(sql.utf8()));
  QString minValue = PQgetvalue(rmin,0,0);
  PQclear(rmin);
  return minValue;
}

// Returns the maximum value of an attribute

QString QgsPostgresProvider::maxValue(uint position){
  // get the field name 
  QgsField fld = attributeFields[position];
  QString sql;
  if(sqlWhereClause.isEmpty())
  {
    sql = QString("select max(\"%1\") from %2").arg(fld.name()).arg(mSchemaTableName);
  }
  else
  {
    sql = QString("select max(\"%1\") from %2").arg(fld.name()).arg(mSchemaTableName)+" where "+sqlWhereClause;
  } 
  PGresult *rmax = PQexec(connection,(const char *)(sql.utf8()));
  QString maxValue = PQgetvalue(rmax,0,0);
  PQclear(rmax);
  return maxValue;
}


int QgsPostgresProvider::maxPrimaryKeyValue()
{
  QString sql;

  sql = QString("select max(\"%1\") from %2")
           .arg(primaryKey)
           .arg(mSchemaTableName);

  PGresult *rmax = PQexec(connection,(const char *)(sql.utf8()));
  QString maxValue = PQgetvalue(rmax,0,0);
  PQclear(rmax);

  return maxValue.toInt();
}


bool QgsPostgresProvider::isValid(){
  return valid;
}

bool QgsPostgresProvider::addFeature(QgsFeature& f, int primaryKeyHighWater)
{
   QgsDebugMsg("Entering.");
  
    // Determine which insertion method to use for WKB
    // PostGIS 1.0+ uses BYTEA
    // earlier versions use HEX
    bool useWkbHex(FALSE);

    if (!gotPostgisVersion)
    {
      postgisVersion(connection);
    }

    QgsDebugMsg("PostGIS version is  major: "  + QString::number(postgisVersionMajor) +
                ", minor: " + QString::number(postgisVersionMinor));

    if (postgisVersionMajor < 1)
    {
      useWkbHex = TRUE;
    }

    // Start building insert string
    QString insert("INSERT INTO ");
    insert+=mSchemaTableName;
    insert+=" (";

    // add the name of the geometry column to the insert statement
    insert += "\"" + geometryColumn;

    // add the name of the primary key column to the insert statement
    insert += "\",\"";
    insert += primaryKey + "\"";

    QgsDebugMsg("Constructing insert SQL, currently at: " + insert);
    
    //add the names of the other fields to the insert
    const QgsAttributeMap& attributevec = f.attributeMap();
    
    QgsDebugMsg("Got attribute map.");
    
    for(QgsAttributeMap::const_iterator it = attributevec.begin(); it != attributevec.end(); ++it)
    {
      QString fieldname=it->fieldName();

      QgsDebugMsg("Checking field against: " + fieldname);
            
      //TODO: Check if field exists in this layer
      // (Sometimes features will have fields that are not part of this layer since
      // they have been pasted from other layers with a different field map)
      bool fieldInLayer = FALSE;
      
      for (QgsFieldMap::iterator iter  = attributeFields.begin();
                                 iter != attributeFields.end();
                               ++iter)
      {
        if ( iter->name() == it->fieldName() )
        {
          fieldInLayer = TRUE;
          break;
        }
      }                                         
      
      if (
           (fieldname != geometryColumn) &&
           (fieldname != primaryKey) &&
           (!(it->fieldValue().isEmpty())) && 
           (fieldInLayer)
         )
      {
        insert+=",\"";
        insert+=fieldname +"\"";
      }
    }

    insert+=") VALUES (GeomFromWKB('";

    // Add the WKB geometry to the INSERT statement
    QgsGeometry* geometry = f.geometry();
    unsigned char* geom = geometry->wkbBuffer();
    for (uint i=0; i < geometry->wkbSize(); ++i)
    {
      if (useWkbHex)
      {
        // PostGIS < 1.0 wants hex
        QString hex = QString::number((int) geom[i], 16).upper();

        if (hex.length() == 1)
        {
          hex = "0" + hex;
        }

        insert += hex;
      }
      else
      {
        // Postgis 1.0 wants bytea
	QString oct = QString::number((int) geom[i], 8);

	if(oct.length()==3)
	{
	    oct="\\\\"+oct;
	}
	else if(oct.length()==1)
	{
	    oct="\\\\00"+oct;
	}
	else if(oct.length()==2)
	{
	    oct="\\\\0"+oct; 
	}

	insert += oct;
      }
    }

    if (useWkbHex)
    {
      insert += "',"+srid+")";
    }
    else
    {
      insert += "::bytea',"+srid+")";
    }

    //add the primary key value to the insert statement
    insert += ",";
    insert += QString::number(primaryKeyHighWater);

    //add the field values to the insert statement
    for(QgsAttributeMap::const_iterator it=attributevec.begin();it!=attributevec.end();++it)
    {
      QString fieldname=it->fieldName();
      
      QgsDebugMsg("Checking field name " + fieldname);
      
      //TODO: Check if field exists in this layer
      // (Sometimes features will have fields that are not part of this layer since
      // they have been pasted from other layers with a different field map)
      bool fieldInLayer = FALSE;
      
      for (QgsFieldMap::iterator iter  = attributeFields.begin();
                                 iter != attributeFields.end();
                               ++iter)
      {
        if ( iter->name() == it->fieldName() )
        {
          fieldInLayer = TRUE;
          break;
        }
      }                                         
      
      if (
           (fieldname != geometryColumn) &&
           (fieldname != primaryKey) &&
           (!(it->fieldValue().isEmpty())) && 
           (fieldInLayer)
         )
      {
        QString fieldvalue = it->fieldValue();
        bool charactertype=false;
        insert+=",";

        QgsDebugMsg("Field is in layer with value " + fieldvalue);
        
        //add quotes if the field is a character or date type and not
        //the postgres provided default value
      if(fieldvalue != "NULL" && fieldvalue != getDefaultValue(it->fieldName(), f))
        {
          for(QgsFieldMap::iterator iter=attributeFields.begin();iter!=attributeFields.end();++iter)
          {
            if(iter->name()==it->fieldName())
            {
              if (
                  iter->type().contains("char",false) > 0 || 
                  iter->type() == "text"                  ||
                  iter->type() == "date"                  ||
                  iter->type() == "interval"              ||
                  iter->type().contains("time",false) > 0      // includes time and timestamp
                 )
              {
                charactertype=true;
                break; // no need to continue with this loop
              }
            }
          }
        }

        // important: escape quotes in field value
        fieldvalue.replace("'", "''");

        if(charactertype)
        {
          insert+="'";
        }
        insert+=fieldvalue;
        if(charactertype)
        {
          insert+="'";
        }
      }
    }

    insert+=")";
    QgsDebugMsg("insert statement is: "+insert);

    //send INSERT statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(insert.utf8()));
    if(result==0)
    {
      showMessageBox(tr("INSERT error"),tr("An error occured during feature insertion"));
      return false;
    }
    ExecStatusType message=PQresultStatus(result);
    if(message==PGRES_FATAL_ERROR)
    {
      showMessageBox(tr("INSERT error"),QString(PQresultErrorMessage(result)));
      return false;
    }
    
    QgsDebugMsg("Exiting with true.");
    return true;
}

QString QgsPostgresProvider::getDefaultValue(const QString& attr, QgsFeature& f)
{
  // Get the default column value from the Postgres information
  // schema. If there is no default we return an empty string.

  // Maintaining a cache of the results of this query would be quite
  // simple and if this query is called lots, could save some time.

  QString sql("SELECT column_default FROM "
              "information_schema.columns WHERE "
              "column_default IS NOT NULL AND "
              "table_schema = '" + mSchemaName + "' AND "
              "table_name = '" + mTableName + "' AND "
              "column_name = '" + attr + "'");

  QString defaultValue("");

  PGresult* result = PQexec(connection, (const char*)(sql.utf8()));

  if (PQntuples(result) == 1)
    defaultValue = PQgetvalue(result, 0, 0);

  PQclear(result);

  return defaultValue;
}

bool QgsPostgresProvider::deleteFeature(int id)
{
  QString sql("DELETE FROM "+mSchemaTableName+" WHERE \""+primaryKey+"\" = "+QString::number(id));
  
  QgsDebugMsg("delete sql: "+sql);

  //send DELETE statement and do error handling
  PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
  if(result==0)
  {
    showMessageBox(tr("DELETE error"),tr("An error occured during deletion from disk"));
    return false;
  }
  ExecStatusType message=PQresultStatus(result);
  if(message==PGRES_FATAL_ERROR)
  {
    showMessageBox(tr("DELETE error"),QString(PQresultErrorMessage(result)));
    return false;
  }

  return true;
}

/**
 * Check to see if GEOS is available
 */
bool QgsPostgresProvider::hasGEOS(PGconn *connection){
  // make sure info is up to date for the current connection
  postgisVersion(connection);
  // get geos capability
  return geosAvailable;
}

/* Functions for determining available features in postGIS */
QString QgsPostgresProvider::postgisVersion(PGconn *connection)
{
  PGresult *result = PQexec(connection, "select postgis_version()");
  postgisVersionInfo = PQgetvalue(result,0,0);
  
  QgsDebugMsg("PostGIS version info: " + postgisVersionInfo);
  
  PQclear(result);

  QStringList postgisParts = QStringList::split(" ", postgisVersionInfo);

  // Get major and minor version
  QStringList postgisVersionParts = QStringList::split(".", postgisParts[0]);

  postgisVersionMajor = postgisVersionParts[0].toInt();
  postgisVersionMinor = postgisVersionParts[1].toInt();

  // assume no capabilities
  geosAvailable = false;
  gistAvailable = false;
  projAvailable = false;

  // parse out the capabilities and store them
  QStringList geos = postgisParts.grep("GEOS");
  if(geos.size() == 1){
    geosAvailable = (geos[0].find("=1") > -1);  
  }
  QStringList gist = postgisParts.grep("STATS");
  if(gist.size() == 1){
    gistAvailable = (geos[0].find("=1") > -1);
  }
  QStringList proj = postgisParts.grep("PROJ");
  if(proj.size() == 1){
    projAvailable = (proj[0].find("=1") > -1);
  }

  gotPostgisVersion = TRUE;

  return postgisVersionInfo;
}

bool QgsPostgresProvider::addFeatures(QgsFeatureList & flist)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");

  int primaryKeyHighWater = maxPrimaryKeyValue();

  for(QgsFeatureList::iterator it=flist.begin();it!=flist.end();++it)
  {
    primaryKeyHighWater++;
    if(!addFeature(*it, primaryKeyHighWater))
    {
      returnvalue=false;
      // TODO: exit loop here?
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::deleteFeatures(const QgsFeatureIds & id)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");
  for(QgsFeatureIds::const_iterator it=id.begin();it!=id.end();++it)
  {
    if(!deleteFeature(*it))
    {
      returnvalue=false;
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::addAttributes(const QgsNewAttributesMap & name)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");
  for(QgsNewAttributesMap::const_iterator iter=name.begin();iter!=name.end();++iter)
  {
    QString sql="ALTER TABLE "+mSchemaTableName+" ADD COLUMN "+iter.key()+" "+iter.value();
    
    QgsDebugMsg(sql);
    
    //send sql statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
    if(result==0)
    {
      returnvalue=false;
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        showMessageBox("ALTER TABLE error",QString(PQresultErrorMessage(result)));
      } 
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::deleteAttributes(const QgsAttributeIds& name)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");
  for(QgsAttributeIds::const_iterator iter=name.begin();iter!=name.end();++iter)
  {
    QString column = attributeFields[*iter].name();
    QString sql="ALTER TABLE "+mSchemaTableName+" DROP COLUMN "+column;
    
    QgsDebugMsg(sql);
    
    //send sql statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
    if(result==0)
    {
      returnvalue=false;
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        showMessageBox("ALTER TABLE error",QString(PQresultErrorMessage(result)));
      }
    }
    else
    {
      //delete the attribute from attributeFields
      attributeFields.remove(*iter);
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::changeAttributeValues(const QgsChangedAttributesMap & attr_map)
{
  bool returnvalue=true; 
  PQexec(connection,"BEGIN");

  // cycle through the features
  for(QgsChangedAttributesMap::const_iterator iter=attr_map.begin();iter!=attr_map.end();++iter)
  {
    int fid = iter.key();
    const QgsAttributeMap& attrs = iter.value();

    // cycle through the changed attributes of the feature
    for(QgsAttributeMap::const_iterator siter = attrs.begin(); siter != attrs.end(); ++siter)
    {
      QString val = siter->fieldValue();

      // escape quotes
      val.replace("'", "''");
       
      QString sql="UPDATE "+mSchemaTableName+" SET "+siter->fieldName()+"='"+val+"' WHERE \"" +primaryKey+"\"="+QString::number(fid);
      QgsDebugMsg(sql);

      // s end sql statement and do error handling
      // TODO: Make all error handling like this one
      PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
      if (result==0)
      {
        showMessageBox(tr("PostGIS error"),
                       tr("An error occured contacting the PostgreSQL databse"));
        return false;
      }
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        showMessageBox(tr("PostGIS error"),tr("The PostgreSQL databse returned: ")
                        + QString(PQresultErrorMessage(result))
                        + "\n" + tr("When trying: ") + sql);
        return false;
      }

    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::changeGeometryValues(QgsGeometryMap & geometry_map)
{
  QgsDebugMsg("entering.");

  bool returnvalue = true;

  // Determine which insertion method to use for WKB
  // PostGIS 1.0+ uses BYTEA
  // earlier versions use HEX
  bool useWkbHex(FALSE);

  if (!gotPostgisVersion)
  {
    postgisVersion(connection);
  }

  QgsDebugMsg("PostGIS version is  major: " + QString::number(postgisVersionMajor) +
              ", minor: " + QString::number(postgisVersionMinor));

  if (postgisVersionMajor < 1)
  {
    useWkbHex = TRUE;
  }

  // Start the PostGIS transaction

  PQexec(connection,"BEGIN");

  for(QgsGeometryMap::const_iterator iter  = geometry_map.begin();
                                             iter != geometry_map.end();
                                           ++iter)
  {

    QgsDebugMsg("iterating over the map of changed geometries...");
    
    if (iter->wkbBuffer())
    {
      QgsDebugMsg("iterating over feature id " + QString::number(iter.key()));
    
      QString sql = "UPDATE "+ mSchemaTableName +" SET \"" + 
                    geometryColumn + "\"=";

      sql += "GeomFromWKB('";

      // Add the WKB geometry to the UPDATE statement
      unsigned char* geom = iter->wkbBuffer();
      for (uint i=0; i < iter->wkbSize(); ++i)
      {
        if (useWkbHex)
        {
          // PostGIS < 1.0 wants hex
          QString hex = QString::number((int) geom[i], 16).upper();

          if (hex.length() == 1)
          {
            hex = "0" + hex;
          }

          sql += hex;
        }
        else
        {
          // Postgis 1.0 wants bytea
          QString oct = QString::number((int) geom[i], 8);

          if(oct.length()==3)
          {
              oct="\\\\"+oct;
          }
          else if(oct.length()==1)
          {
              oct="\\\\00"+oct;
          }
          else if(oct.length()==2)
          {
              oct="\\\\0"+oct; 
          }

          sql += oct;
        }
      }

      if (useWkbHex)
      {
        sql += "',"+srid+")";
      }
      else
      {
        sql += "::bytea',"+srid+")";
      }

      sql += " WHERE \"" +primaryKey+"\"="+QString::number(iter.key());

      QgsDebugMsg("Updating with: " + sql);
    
      // send sql statement and do error handling
      // TODO: Make all error handling like this one
      PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
      if (result==0)
      {
        showMessageBox(tr("PostGIS error"), tr("An error occured contacting the PostgreSQL databse"));
        return false;
      }
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        showMessageBox(tr("PostGIS error"), tr("The PostgreSQL databse returned: ")
                                   + QString(PQresultErrorMessage(result))
                                   + "\n" + tr("When trying: ") + sql);
        return false;
      }
                       
    } // if (*iter)

/*  
  
    if(f)
  {
    QString insert("INSERT INTO \"");
    insert+=tableName;
    insert+="\" (";

    //add the name of the geometry column to the insert statement
    insert+=geometryColumn;//first the geometry

    //add the names of the other fields to the insert
    std::vector<QgsFeatureAttribute> attributevec=f->attributeMap();
    for(std::vector<QgsFeatureAttribute>::iterator it=attributevec.begin();it!=attributevec.end();++it)
    {
      QString fieldname=it->fieldName();
      if(fieldname!=geometryColumn)
      {
        insert+=",";
        insert+=fieldname;
      }
    }

    insert+=") VALUES (GeomFromWKB('";

    //add the wkb geometry to the insert statement
    unsigned char* geom=f->getGeometry();
    for(int i=0;i<f->getGeometrySize();++i)
    {
      QString hex=QString::number((int)geom[i],16).upper();
      if(hex.length()==1)
      {
        hex="0"+hex;
      }
      insert+=hex;
    }
    insert+="',"+srid+")";

    //add the field values to the insert statement
    for(std::vector<QgsFeatureAttribute>::iterator it=attributevec.begin();it!=attributevec.end();++it)
    {
      if(it->fieldName()!=geometryColumn)
      {
        QString fieldvalue=it->fieldValue();
        bool charactertype=false;
        insert+=",";

        //add quotes if the field is a characted type
        if(fieldvalue!="NULL")
        {
          for(std::vector<QgsField>::iterator iter=attributeFields.begin();iter!=attributeFields.end();++iter)
          {
            if(iter->name()==it->fieldName())
            {
              if(iter->type().contains("char",false)>0||iter->type()=="text")
              {
                charactertype=true;
              }
            }
          }
        }

        if(charactertype)
        {
          insert+="'";
        }
        insert+=fieldvalue;
        if(charactertype)
        {
          insert+="'";
        }
      }
    }

    insert+=")";
    QgsDebugMsg("insert statement is: "+insert);

    //send INSERT statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(insert.utf8()));
    if(result==0)
    {
      QMessageBox::information(0,"INSERT error","An error occured during feature insertion");
      return false;
    }
    ExecStatusType message=PQresultStatus(result);
    if(message==PGRES_FATAL_ERROR)
    {
      QMessageBox::information(0,"INSERT error",QString(PQresultErrorMessage(result)));
      return false;
    }
    return true;
  }
  return false;

  
  
  
  
  
  
  
  
    for(std::map<QString,QString>::const_iterator siter=(*iter).second.begin();siter!=(*iter).second.end();++siter)
    {
      QString value=(*siter).second;

      //find out, if value contains letters and quote if yes
      bool text=false;
      for(int i=0;i<value.length();++i)
      {
        if(value[i].isLetter())
        {
          text=true;
        }
      }
      if(text)
      {
        value.prepend("'");
        value.append("'");
      }

      QString sql="UPDATE \""+tableName+"\" SET "+(*siter).first+"="+value+" WHERE " +primaryKey+"="+QString::number((*iter).first);
      QgsDebugMsg(sql);

      //send sql statement and do error handling
      PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
      if(result==0)
      {
        returnvalue=false;
        ExecStatusType message=PQresultStatus(result);
        if(message==PGRES_FATAL_ERROR)
        {
          QMessageBox::information(0,"UPDATE error",QString(PQresultErrorMessage(result)));
        }
      }
    }
  }
*/
  } // for each feature
  
  PQexec(connection,"COMMIT");
  
  // TODO: Reset Geometry dirty if commit was OK
  
  reset();
  
  QgsDebugMsg("exiting.");
  
  return returnvalue;
}


int QgsPostgresProvider::capabilities() const
{
  return (
           QgsVectorDataProvider::AddFeatures |
           QgsVectorDataProvider::DeleteFeatures |
           QgsVectorDataProvider::ChangeAttributeValues |
           QgsVectorDataProvider::AddAttributes |
           QgsVectorDataProvider::DeleteAttributes |
           QgsVectorDataProvider::ChangeGeometries |
           QgsVectorDataProvider::SelectGeometryAtId
         );
}

void QgsPostgresProvider::setSubsetString(QString theSQL)
{
  sqlWhereClause=theSQL;
  // Update datasource uri too
  mUri.sql=theSQL;
  // Update yet another copy of the uri. Why are there 3 copies of the
  // uri? Perhaps this needs some rationalisation.....
  setDataSourceUri(mUri.text());

  // need to recalculate the number of features...
  getFeatureCount();
  calculateExtents();

}

long QgsPostgresProvider::getFeatureCount()
{
  // get total number of features

  // First get an approximate count; then delegate to
  // a thread the task of getting the full count.

#ifdef POSTGRESQL_THREADS
  QString sql = "select reltuples from pg_catalog.pg_class where relname = '" + 
    tableName + "'";

  QgsDebugMsg("Running SQL: " + sql);
#else                 
  QString sql = "select count(*) from " + mSchemaTableName + "";

  if(sqlWhereClause.length() > 0)
  {
    sql += " where " + sqlWhereClause;
  }
#endif

  PGresult *result = PQexec(connection, (const char *) (sql.utf8()));

  QgsDebugMsg("Approximate Number of features as text: " +
              QString(PQgetvalue(result, 0, 0)));

  numberFeatures = QString(PQgetvalue(result, 0, 0)).toLong();
  PQclear(result);

  QgsDebugMsg("Approximate Number of features: " + QString::number(numberFeatures));

  return numberFeatures;
}

// TODO: use the estimateExtents procedure of PostGIS and PostgreSQL 8
// This tip thanks to #qgis irc nick "creeping"
void QgsPostgresProvider::calculateExtents()
{
#ifdef POSTGRESQL_THREADS
  // get the approximate extent by retreiving the bounding box
  // of the first few items with a geometry

  QString sql = "select box3d(" + geometryColumn + ") from " 
    + mSchemaTableName + " where ";

  if(sqlWhereClause.length() > 0)
  {
    sql += "(" + sqlWhereClause + ") and ";
  }

  sql += "not IsEmpty(" + geometryColumn + ") limit 5";


#if WASTE_TIME
  sql = "select xmax(extent(\"" + geometryColumn + "\")) as xmax,"
    "xmin(extent(\"" + geometryColumn + "\")) as xmin,"
    "ymax(extent(\"" + geometryColumn + "\")) as ymax," 
    "ymin(extent(\"" + geometryColumn + "\")) as ymin" 
    " from " + mSchemaTableName;
#endif

  QgsDebugMsg("Getting approximate extent using: '" + sql + "'");
  
  PGresult *result = PQexec(connection, (const char *) (sql.utf8()));

  // TODO: Guard against the result having no rows

  for (int i = 0; i < PQntuples(result); i++)
  {
    std::string box3d = PQgetvalue(result, i, 0);

    if (0 == i)
    {
      // create the initial extent
      layerExtent = QgsPostGisBox3d(box3d);
    }
    else
    {
      // extend the initial extent
      QgsPostGisBox3d b = QgsPostGisBox3d(box3d);

      layerExtent.combineExtentWith( &b );
    }

    QgsDebugMsg("After row " + QString::number(i) +
                ", extent is: " + layerExtent.stringRep());
  }

  // clear query result
  PQclear(result);

#else // non-postgresql threads version

  // get the extents

  QString sql = "select extent(\"" + geometryColumn + "\") from " + 
    mSchemaTableName;
  if(sqlWhereClause.length() > 0)
  {
    sql += " where " + sqlWhereClause;
  }

#if WASTE_TIME
  sql = "select xmax(extent(\"" + geometryColumn + "\")) as xmax,"
    "xmin(extent(\"" + geometryColumn + "\")) as xmin,"
    "ymax(extent(\"" + geometryColumn + "\")) as ymax," 
    "ymin(extent(\"" + geometryColumn + "\")) as ymin" 
    " from " + mSchemaTableName;
#endif

  QgsDebugMsg("Getting extents using schema.table: " + sql);
  
  PGresult *result = PQexec(connection, (const char *) (sql.utf8()));
  Q_ASSERT(PQntuples(result) == 1);
  std::string box3d = PQgetvalue(result, 0, 0);

  if (box3d != "")
  {
    std::string s;

    box3d = box3d.substr(box3d.find_first_of("(")+1);
    box3d = box3d.substr(box3d.find_first_not_of(" "));
    s = box3d.substr(0, box3d.find_first_of(" "));
    double minx = strtod(s.c_str(), NULL);

    box3d = box3d.substr(box3d.find_first_of(" ")+1);
    s = box3d.substr(0, box3d.find_first_of(" "));
    double miny = strtod(s.c_str(), NULL);

    box3d = box3d.substr(box3d.find_first_of(",")+1);
    box3d = box3d.substr(box3d.find_first_not_of(" "));
    s = box3d.substr(0, box3d.find_first_of(" "));
    double maxx = strtod(s.c_str(), NULL);

    box3d = box3d.substr(box3d.find_first_of(" ")+1);
    s = box3d.substr(0, box3d.find_first_of(" "));
    double maxy = strtod(s.c_str(), NULL);

    layerExtent.setXmax(maxx);
    layerExtent.setXmin(minx);
    layerExtent.setYmax(maxy);
    layerExtent.setYmin(miny);

    // clear query result
    PQclear(result);
  }

#endif

// #ifdef QGISDEBUG
//   QString xMsg;
//   QTextOStream(&xMsg).precision(18);
//   QTextOStream(&xMsg).width(18);
//   QTextOStream(&xMsg) << "QgsPostgresProvider: Set extents to: " << layerExtent.
//     xMin() << ", " << layerExtent.yMin() << " " << layerExtent.xMax() << ", " << layerExtent.yMax();
//   std::cerr << xMsg << std::endl;
// #endif
  QgsDebugMsg("Set extents to: " + layerExtent.stringRep());
}

/**
 * Event sink for events from threads
 */
void QgsPostgresProvider::customEvent( QCustomEvent * e )
{
  QgsDebugMsg("received a custom event " + QString::number(e->type()));

  switch ( e->type() )
  {
    case (QEvent::Type) QGis::ProviderExtentCalcEvent:

      QgsDebugMsg("extent has been calculated");

      // Collect the new extent from the event and set this layer's
      // extent with it.

      {
        QgsRect* r = (QgsRect*) e->data();
        setExtent( *r );
      }

      QgsDebugMsg("new extent has been saved");

      QgsDebugMsg("Set extent to: " + layerExtent.stringRep());

      QgsDebugMsg("emitting fullExtentCalculated()");

      emit fullExtentCalculated();

      // TODO: Only uncomment this when the overview map canvas has been subclassed
      // from the QgsMapCanvas

      //        QgsDebugMsg("emitting repaintRequested()");
      //        emit repaintRequested();

      break;

    case (QEvent::Type) QGis::ProviderCountCalcEvent:

      QgsDebugMsg("count has been calculated");

      numberFeatures = ((QgsProviderCountCalcEvent*) e)->numberFeatures();

      QgsDebugMsg("count is " + QString::number(numberFeatures));

      break;
      
    default:
      // do nothing
      break;
  }

  QgsDebugMsg("Finished processing custom event " + QString::number(e->type()));

}


bool QgsPostgresProvider::deduceEndian()
{
  // need to store the PostgreSQL endian format used in binary cursors
  // since it appears that starting with
  // version 7.4, binary cursors return data in XDR whereas previous versions
  // return data in the endian of the server

  QString firstOid = "select oid from pg_class where relname = '" + 
    mTableName + "' and relnamespace = (select oid from pg_namespace where nspname = '"
    + mSchemaName + "')";
  PGresult * oidResult = PQexec(connection, (const char*)(firstOid.utf8()));
  // get the int value from a "normal" select
  QString oidValue = PQgetvalue(oidResult,0,0);
  PQclear(oidResult);

  QgsDebugMsg("Creating binary cursor");

  // get the same value using a binary cursor

  PQexec(connection,"begin work");
  QString oidDeclare = QString("declare oidcursor binary cursor for select oid from pg_class where relname = '%1' and relnamespace = (select oid from pg_namespace where nspname = '%2')").arg(mTableName).arg(mSchemaName);
  // set up the cursor
  PQexec(connection, (const char *)oidDeclare);
  QString fetch = "fetch forward 1 from oidcursor";

  QgsDebugMsg("Fetching a record and attempting to get check endian-ness");

  PGresult *fResult = PQexec(connection, (const char *)fetch);
  PQexec(connection, "end work");
  swapEndian = true;
  if(PQntuples(fResult) > 0){
    // get the oid value from the binary cursor
    int oid = *(int *)PQgetvalue(fResult,0,0);

    //--std::cout << "Got oid of " << oid << " from the binary cursor" << std::endl;
    //--std::cout << "First oid is " << oidValue << std::endl;
    // compare the two oid values to determine if we need to do an endian swap
    if(oid == oidValue.toInt())
      swapEndian = false;

    PQclear(fResult);
  }
  return swapEndian;
}

bool QgsPostgresProvider::getGeometryDetails()
{
  QString fType("");
  srid = "";
  valid = false;
  QStringList log;

  QString sql = "select f_geometry_column,type,srid from geometry_columns"
    " where f_table_name='" + mTableName + "' and f_geometry_column = '" + 
    geometryColumn + "' and f_table_schema = '" + mSchemaName + "'";

  QgsDebugMsg("Getting geometry column: " + sql);

  PGresult *result = executeDbCommand(connection, sql);

  QgsDebugMsg("geometry column query returned " + QString(PQntuples(result)));
  QgsDebugMsg("column number of srid is " + QString::number(PQfnumber(result, "srid")));

  if (PQntuples(result) > 0)
  {
    srid = PQgetvalue(result, 0, PQfnumber(result, "srid"));
    fType = PQgetvalue(result, 0, PQfnumber(result, "type"));
    PQclear(result);
  }
  else
  {
    // Didn't find what we need in the geometry_columns table, so
    // get stuff from the relevant column instead. This may (will?) 
    // fail if there is no data in the relevant table.
    PQclear(result); // for the query just before the if() statement
    sql = "select "
      "srid(\""         + geometryColumn + "\"), "
      "geometrytype(\"" + geometryColumn + "\") from " + 
      mSchemaTableName + " limit 1";

    result = executeDbCommand(connection, sql);

    if (PQntuples(result) > 0)
    {
      srid = PQgetvalue(result, 0, PQfnumber(result, "srid"));
      fType = PQgetvalue(result, 0, PQfnumber(result, "geometrytype"));
    }
    PQclear(result);
  }

  if (!srid.isEmpty() && !fType.isEmpty())
  {
    valid = true;
    if (fType == "POINT")
      {
	geomType = QGis::WKBPoint;
      }
    else if(fType == "MULTIPOINT")
      {
	geomType = QGis::WKBMultiPoint;
      }
    else if(fType == "LINESTRING")
      {
	geomType = QGis::WKBLineString;
      }
    else if(fType == "MULTILINESTRING")
      {
	geomType = QGis::WKBMultiLineString;
      }
    else if (fType == "POLYGON")
      {
	geomType = QGis::WKBPolygon;
      }
    else if(fType == "MULTIPOLYGON")
      {
	geomType = QGis::WKBMultiPolygon;
      }
    else
    {
      showMessageBox(tr("Unknown geometry type"), 
	tr("Column ") + geometryColumn + tr(" in ") +
	   mSchemaTableName + tr(" has a geometry type of ") +
           fType + tr(", which Qgis does not currently support."));
      valid = false;
    }
  }
  else // something went wrong...
  {
    log.prepend(tr("Qgis was unable to determine the type and srid of "
		   "column " + geometryColumn + tr(" in ") +
		   mSchemaTableName + 
		   tr(". The database communication log was:\n")));
    showMessageBox(tr("Unable to get feature type and srid"), log);
  }

  if (valid)
  {
    QgsDebugMsg("SRID is " + srid);
    QgsDebugMsg("type is " + fType);
    QgsDebugMsg("Feature type is " + QString::number(geomType));
    QgsDebugMsg("Feature type name is " + QString(QGis::qgisFeatureTypes[geomType]));
  }
  else
  {
    QgsDebugMsg("Failed to get geometry details for Postgres layer.");
  }

  return valid;
}

PGresult* QgsPostgresProvider::executeDbCommand(PGconn* connection, 
                                                const QString& sql)
{
  PGresult *result = PQexec(connection, (const char *) (sql.utf8()));
  
  QgsDebugMsg("Executed SQL: " + sql);
  if (PQresultStatus(result) == PGRES_TUPLES_OK) {
    QgsDebugMsg("Command was successful.");
  } else {
    QgsDebugMsg("Command was unsuccessful. The error message was: "
                + QString( PQresultErrorMessage(result) ) );
  }
  return result;
}

void QgsPostgresProvider::showMessageBox(const QString& title, 
					 const QString& text)
{
  QgsMessageOutput* message = QgsMessageOutput::createMessageOutput();
  message->setTitle(title);
  message->setMessage(text, QgsMessageOutput::MessageText);
  message->showMessage();
}

void QgsPostgresProvider::showMessageBox(const QString& title, 
					 const QStringList& text)
{
  showMessageBox(title, text.join("\n"));
}


void QgsPostgresProvider::setSRS(const QgsSpatialRefSys& theSRS)
{
  // TODO implement [MD]
}

QgsSpatialRefSys QgsPostgresProvider::getSRS()
{
  QgsSpatialRefSys srs;
  srs.createFromSrid(srid.toInt());
  return srs;
}

QString QgsPostgresProvider::subsetString()
{
  return sqlWhereClause;
}

PGconn * QgsPostgresProvider::pgConnection()
{
  return connection;
}

QString QgsPostgresProvider::getTableName()
{
  return mTableName;
}


size_t QgsPostgresProvider::layerCount() const
{
    return 1;                   // XXX need to return actual number of layers
} // QgsPostgresProvider::layerCount()



QString  QgsPostgresProvider::name() const
{
    return POSTGRES_KEY;
} //  QgsPostgresProvider::name()



QString  QgsPostgresProvider::description() const
{
    return POSTGRES_DESCRIPTION;
} //  QgsPostgresProvider::description()




/**
 * Class factory to return a pointer to a newly created 
 * QgsPostgresProvider object
 */
QGISEXTERN QgsPostgresProvider * classFactory(const QString *uri)
{
  return new QgsPostgresProvider(*uri);
}
/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey()
{
  return  POSTGRES_KEY;
}
/**
 * Required description function 
 */
QGISEXTERN QString description()
{
    return POSTGRES_DESCRIPTION;
} 
/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider(){
  return true;
}

