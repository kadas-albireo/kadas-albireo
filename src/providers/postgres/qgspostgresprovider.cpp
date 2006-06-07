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

#include <fstream>
#include <iostream>
#include <cassert>

#include <QStringList>
#include <QApplication>
#include <QMessageBox>
#include <QEvent>
#include <QCustomEvent>
#include <QTextOStream>

// for ntohl
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#include <qgis.h>
#include <qgsfeature.h>
#include <qgsfield.h>
#include <qgsrect.h>
#include <qgsmessageviewer.h>

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
  : QgsVectorDataProvider(uri), geomType(QGis::WKBUnknown)
{
  // assume this is a valid layer until we determine otherwise
  valid = true;
  /* OPEN LOG FILE */

  // Make connection to the data source
  // For postgres, the connection information is passed as a space delimited
  // string:
  //  host=192.168.1.5 dbname=test port=5342 user=gsherman password=xxx table=tablename

  // Strip the table and sql statement name off and store them
  int sqlStart = uri.find(" sql");
  int tableStart = uri.find("table=");
#ifdef QGISDEBUG
  qDebug(  "****************************************");
  qDebug(  "****   Postgresql Layer Creation   *****" );
  qDebug(  "****************************************");
  qDebug(  (const char*)(QString("URI: ") + uri).toLocal8Bit().data() );
  QString msg;

  qDebug(  "tableStart: " + msg.setNum(tableStart) );
  qDebug(  "sqlStart: " + msg.setNum(sqlStart));
#endif 
  mTableName = uri.mid(tableStart + 6, sqlStart - tableStart -6);
  if(sqlStart > -1)
  { 
    sqlWhereClause = uri.mid(sqlStart + 5);
  }
  else
  {
    sqlWhereClause = QString::null;
  }
  QString connInfo = uri.left(uri.find("table="));
#ifdef QGISDEBUG
  qDebug( (const char*)(QString("Table name is ") + mTableName).toLocal8Bit().data());
  qDebug( (const char*)(QString("SQL is ") + sqlWhereClause).toLocal8Bit().data() );
  qDebug( "Connection info is " + connInfo);
#endif
  // calculate the schema if specified
  mSchemaName = "";
  if (mTableName.find(".") > -1) {
    mSchemaName = mTableName.left(mTableName.find("."));
  }
  geometryColumn = mTableName.mid(mTableName.find(" (") + 2);
  geometryColumn.truncate(geometryColumn.length() - 1);
  mTableName = mTableName.mid(mTableName.find(".") + 1, mTableName.find(" (") - (mTableName.find(".") + 1)); 

  // Keep a schema qualified table name for convenience later on.
  if (mSchemaName.length() > 0)
    mSchemaTableName = "\"" + mSchemaName + "\".\"" + mTableName + "\"";
  else
    mSchemaTableName = "\"" + mTableName + "\"";

  /* populate the uri structure */
  mUri.schema = mSchemaName;
  mUri.table = mTableName;
  mUri.geometryColumn = geometryColumn;
  mUri.sql = sqlWhereClause;
  // parse the connection info
  QStringList conParts = QStringList::split(" ", connInfo);
  QStringList parm = QStringList::split("=", conParts[0]);
  if(parm.size() == 2)
  {
    mUri.host = parm[1];
  }
  parm = QStringList::split("=", conParts[1]);
  if(parm.size() == 2)
  {
    mUri.database = parm[1];
  }
  parm = QStringList::split("=", conParts[2]);
  if(parm.size() == 2)
  {
    mUri.port = parm[1];
  }

  parm = QStringList::split("=", conParts[3]);
  if(parm.size() == 2)
  {
    mUri.username = parm[1];
  }
  parm = QStringList::split("=", conParts[4]);
  if(parm.size() == 2)
  {
    mUri.password = parm[1];
  }
  /* end uri structure */

#ifdef QGISDEBUG
  std::cerr << "Geometry column is: " << geometryColumn.toLocal8Bit().data() << std::endl;
  std::cerr << "Schema is: " << mSchemaName.toLocal8Bit().data() << std::endl;
  std::cerr << "Table name is: " << mTableName.toLocal8Bit().data() << std::endl;
#endif
  //QString logFile = "./pg_provider_" + mTableName + ".log";
  //pLog.open((const char *)logFile);
#ifdef QGISDEBUG
  std::cerr << "Opened log file for " << mTableName.toLocal8Bit().data() << std::endl;
#endif
  PGconn *pd = PQconnectdb((const char *) connInfo);
  // check the connection status
  if (PQstatus(pd) == CONNECTION_OK)
  {
    // store the connection for future use
    connection = pd;

    //set client encoding to unicode because QString uses UTF-8 anyway
#ifdef QGISDEBUG
    qWarning("setting client encoding to UNICODE");
#endif
    int errcode=PQsetClientEncoding(connection, "UNICODE");
#ifdef QGISDEBUG
    if(errcode==0)
    {
        qWarning("encoding successfully set");
    }
    else if(errcode==-1)
    {
        qWarning("error in setting encoding");
    }
    else
    {
        qWarning("undefined return value from encoding setting");
    }
#endif

#ifdef QGISDEBUG
    std::cerr << "Checking for select permission on the relation\n";
#endif
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
#ifdef QGISDEBUG
    std::cerr << "Checking for GEOS support" << std::endl;
#endif
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

#ifdef QGISDEBUG
        std::cerr << "Field: " << attnum.toLocal8Bit().data() << " maps to " << i << " " << fieldName.toLocal8Bit().data() << ", " 
          << fieldType.toLocal8Bit().data() << " (" << fldtyp << "),  " << fieldSize.toLocal8Bit().data() << ", "  
          << fieldModifier << std::endl;
#endif
        attributeFieldsIdMap[attnum.toInt()] = i;

        if(fieldName!=geometryColumn)
        {
          attributeFields.push_back(QgsField(fieldName, fieldType, fieldSize.toInt(), fieldModifier));
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
      std::cout << "QgsPostgresProvider: About to touch mExtentThread" << std::endl;
      mExtentThread.setConnInfo( connInfo );
      mExtentThread.setTableName( mTableName );
      mExtentThread.setSqlWhereClause( sqlWhereClause );
      mExtentThread.setGeometryColumn( geometryColumn );
      mExtentThread.setCallback( this );
      std::cout << "QgsPostgresProvider: About to start mExtentThread" << std::endl;
      mExtentThread.start();
      std::cout << "QgsPostgresProvider: Main thread just dispatched mExtentThread" << std::endl;

      std::cout << "QgsPostgresProvider: About to touch mCountThread" << std::endl;
      mCountThread.setConnInfo( connInfo );
      mCountThread.setTableName( mTableName );
      mCountThread.setSqlWhereClause( sqlWhereClause );
      mCountThread.setGeometryColumn( geometryColumn );
      mCountThread.setCallback( this );
      std::cout << "QgsPostgresProvider: About to start mCountThread" << std::endl;
      mCountThread.start();
      std::cout << "QgsPostgresProvider: Main thread just dispatched mCountThread" << std::endl;
#endif
    } 
    else 
    {
      // the table is not a geometry table
      numberFeatures = 0;
      valid = false;
#ifdef QGISDEBUG
      std::cerr << "Invalid Postgres layer" << std::endl;
#endif
    }

    ready = false; // not ready to read yet cuz the cursor hasn't been created

  } else {
    valid = false;
    //--std::cout << "Connection to database failed\n";
  }

  //fill type names into lists
  mNumericalTypes.push_back("double precision");
  mNumericalTypes.push_back("int4");
  mNumericalTypes.push_back("int8");
  mNonNumericalTypes.push_back("text");
  mNonNumericalTypes.push_back("varchar(30)");

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
  std::cout << "QgsPostgresProvider: About to wait for mExtentThread" << std::endl;

  mExtentThread.wait();

  std::cout << "QgsPostgresProvider: Finished waiting for mExtentThread" << std::endl;

  std::cout << "QgsPostgresProvider: About to wait for mCountThread" << std::endl;

  mCountThread.wait();

  std::cout << "QgsPostgresProvider: Finished waiting for mCountThread" << std::endl;

  // Make sure all events from threads have been processed
  // (otherwise they will get destroyed prematurely)
  QApplication::sendPostedEvents(this, QGis::ProviderExtentCalcEvent);
  QApplication::sendPostedEvents(this, QGis::ProviderCountCalcEvent);
#endif
  PQfinish(connection);

  QgsDebugMsg("QgsPostgresProvider: deconstructing.");

  //pLog.flush();
}

QString QgsPostgresProvider::storageType()
{
  return "PostgreSQL database with PostGIS extension";
}

//TODO - we may not need this function - consider removing it from
//       the dataprovider.h interface
/**
 * Get the first feature resutling from a select operation
 * @return QgsFeature
 */
//TODO - this function is a stub and always returns 0
QgsFeature *QgsPostgresProvider::getFirstFeature(bool fetchAttributes)
{
  QgsFeature *f = 0;
  if (valid) {
    //--std::cout << "getting first feature\n";

    f = new QgsFeature();
    /*  f->setGeometry(getGeometryPointer(feat));
        if(fetchAttributes){
        getFeatureAttributes(feat, f);
        } */
  }
  return f;
}

bool QgsPostgresProvider::getNextFeature(QgsFeature &feature, bool fetchAttributes)
{
  return true;
}

/**
 * Get the next feature resutling from a select operation
 * Return 0 if there are no features in the selection set
 * @return QgsFeature
 */
QgsFeature *QgsPostgresProvider::getNextFeature(bool fetchAttributes)
{
  QgsFeature *f = 0;
  
  int row = 0;  // TODO: Make this useful

  if (valid){
    QString fetch = "fetch forward 1 from qgisf";
    queryResult = PQexec(connection, (const char *)fetch);
    //    std::cerr << "Error: " << PQerrorMessage(connection) << std::endl;
    //   std::cerr << "Fetched " << PQntuples(queryResult) << "rows" << std::endl;
    if(PQntuples(queryResult) == 0){
      PQexec(connection, "end work");
      ready = false;
      return 0;
    } 
    //  //--std::cout <<"Raw value of the geometry field: " << PQgetvalue(queryResult,0,PQfnumber(queryResult,"qgs_feature_geometry")) << std::endl;
    //--std::cout << "Length of oid is " << PQgetlength(queryResult,0, PQfnumber(queryResult,"oid")) << std::endl;

    // get the value of the primary key based on type

    int oid = *(int *)PQgetvalue(queryResult, row, PQfnumber(queryResult,primaryKey));
#ifdef QGISDEBUG
    // std::cerr << "OID from database: " << oid << std::endl; 
#endif

    if (swapEndian)
      oid = ntohl(oid); // convert oid to opposite endian

    // oid is the key to be used in fetching attributes if 
    // fetchAttributes = true
#ifdef QGISDEBUG
    //    std::cerr << "Using OID: " << oid << std::endl;
#endif
    f = new QgsFeature(oid);
    if (fetchAttributes)
      getFeatureAttributes(oid, row, f);

    int returnedLength = PQgetlength(queryResult, row, PQfnumber(queryResult,"qgs_feature_geometry"));
    //--std::cerr << __FILE__ << ":" << __LINE__ << " Returned length is " << returnedLength << std::endl;
    if(returnedLength > 0)
    {
      unsigned char *feature = new unsigned char[returnedLength + 1];
      memset(feature, '\0', returnedLength + 1);
      memcpy(feature, PQgetvalue(queryResult, row, PQfnumber(queryResult,"qgs_feature_geometry")), returnedLength);
#ifdef QGISDEBUG
      // a bit verbose
      //int wkbType = *((int *) (feature + 1));
      //std::cout << "WKBtype is: " << wkbType << std::endl;
#endif
      f->setGeometryAndOwnership(feature, returnedLength + 1);
    }
    else
    {
      //--std::cout <<"Couldn't get the feature geometry in binary form" << std::endl;
    }
    
    PQclear(queryResult);
  }
  else
  {
    //--std::cout << "Read attempt on an invalid postgresql data source\n";
  }
  return f;
}

// // TODO: Remove completely (see morb_au)
// QgsFeature* QgsPostgresProvider::getNextFeature(std::list<int> const & attlist)
// {
//   return getNextFeature(attlist, 1);
// }

QgsFeature* QgsPostgresProvider::getNextFeature(std::list<int> const & attlist, int featureQueueSize)
{
  QgsFeature *f = 0;
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
#ifdef QGISDEBUG
        std::cerr << "End of features.\n";
#endif  
        PQexec(connection, "end work");
        ready = false;
        return 0;
      }
      
      for (int row = 0; row < rows; row++)
      {
        int oid = *(int *)PQgetvalue(queryResult, row, PQfnumber(queryResult,primaryKey));
#ifdef QGISDEBUG 
        //  std::cerr << "Primary key type is " << primaryKeyType << std::endl; 
#endif  
	if (swapEndian)
	  oid = ntohl(oid); // convert oid to opposite endian

        f = new QgsFeature(oid);
        if(!attlist.empty())
        {
          getFeatureAttributes(oid, row, f, attlist);
        }
        int returnedLength = PQgetlength(queryResult, row, PQfnumber(queryResult,"qgs_feature_geometry")); 
        if(returnedLength > 0)
        {
          unsigned char *feature = new unsigned char[returnedLength + 1];
          memset(feature, '\0', returnedLength + 1);
          memcpy(feature, PQgetvalue(queryResult, row, PQfnumber(queryResult,"qgs_feature_geometry")), returnedLength); 
    #ifdef QGISDEBUG
          // Too verbose
          //int wkbType = *((int *) (feature + 1));
          //std::cout << "WKBtype is: " << wkbType << std::endl;
    #endif
          f->setGeometryAndOwnership(feature, returnedLength + 1);
    
        }
        else
        {
          //--std::cout <<"Couldn't get the feature geometry in binary form" << std::endl;
        }

#ifdef QGISDEBUG
//      std::cout << "QgsPostgresProvider::getNextFeature: pushing " << f->featureId() << std::endl; 
#endif
  
        mFeatureQueue.push(f);
        
      } // for each row in queue
      
#ifdef QGISDEBUG
//      std::cout << "QgsPostgresProvider::getNextFeature: retrieved batch of features." << std::endl; 
#endif

            
      PQclear(queryResult);
      
    } // if new queue is required
    
    // Now return the next feature from the queue
    
    f = mFeatureQueue.front();
    mFeatureQueue.pop();
    
  }
  else 
  {
    //--std::cout << "Read attempt on an invalid postgresql data source\n";
  }
  
#ifdef QGISDEBUG
//      std::cout << "QgsPostgresProvider::getNextFeature: returning " << f->featureId() << std::endl; 
#endif

  return f;   
}

/**
 * Select features based on a bounding rectangle. Features can be retrieved
 * with calls to getFirstFeature and getNextFeature.
 * @param mbr QgsRect containing the extent to use in selecting features
 */
void QgsPostgresProvider::select(QgsRect * rect, bool useIntersect)
{
  // spatial query to select features

#ifdef QGISDEBUG
  std::cerr << "Selection rectangle is " << *rect << std::endl;
  std::cerr << "Selection polygon is " << rect->asPolygon().toLocal8Bit().data() << std::endl;
#endif

  QString declare = QString("declare qgisf binary cursor for select "
      + primaryKey  
      + ",asbinary(%1,'%2') as qgs_feature_geometry from %3").arg(geometryColumn).arg(endianString()).arg(mSchemaTableName);
#ifdef QGISDEBUG
  std::cout << "Binary cursor: " << declare.toLocal8Bit().data() << std::endl; 
#endif
  if(useIntersect){
    //    declare += " where intersects(" + geometryColumn;
    //    declare += ", GeometryFromText('BOX3D(" + rect->asWKTCoords();
    //    declare += ")'::box3d,";
    //    declare += srid;
    //    declare += "))";

    // Contributed by #qgis irc "creeping"
    // This version actually invokes PostGIS's use of spatial indexes
    declare += " where " + geometryColumn;
    declare += " && setsrid('BOX3D(" + rect->asWKTCoords();
    declare += ")'::box3d,";
    declare += srid;
    declare += ")";
    declare += " and intersects(" + geometryColumn;
    declare += ", setsrid('BOX3D(" + rect->asWKTCoords();
    declare += ")'::box3d,";
    declare += srid;
    declare += "))";
  }else{
    declare += " where " + geometryColumn;
    declare += " && setsrid('BOX3D(" + rect->asWKTCoords();
    declare += ")'::box3d,";
    declare += srid;
    declare += ")";
  }
  if(sqlWhereClause.length() > 0)
  {
    declare += " and (" + sqlWhereClause + ")";
  }

#ifdef QGISDEBUG
  std::cerr << "Selecting features using: " << declare.toLocal8Bit().data() << std::endl;
#endif
  // set up the cursor
  if(ready){
    PQexec(connection, "end work");
  }
  PQexec(connection,"begin work");
  PQexec(connection, (const char *)(declare.utf8()));
  
  // TODO - see if this deallocates member features
  mFeatureQueue.empty();
}


QgsDataSourceURI * QgsPostgresProvider::getURI()
{
  return &mUri;
}


/**
 * Identify features within the search radius specified by rect
 * @param rect Bounding rectangle of search radius
 * @return std::vector containing QgsFeature objects that intersect rect
 */
std::vector<QgsFeature>& QgsPostgresProvider::identify(QgsRect * rect)
{
  features.clear();
  // select the features
  select(rect);

  return features;
}

/* unsigned char * QgsPostgresProvider::getGeometryPointer(OGRFeature *fet){
//  OGRGeometry *geom = fet->GetGeometryRef();
unsigned char *gPtr=0;
// get the wkb representation
gPtr = new unsigned char[geom->WkbSize()];

geom->exportToWkb((OGRwkbByteOrder) endian(), gPtr);
return gPtr;

} */

void QgsPostgresProvider::setExtent( QgsRect* newExtent )
{
  layerExtent.setXmax( newExtent->xMax() );
  layerExtent.setXmin( newExtent->xMin() );
  layerExtent.setYmax( newExtent->yMax() );
  layerExtent.setYmin( newExtent->yMin() );
}

// TODO - make this function return the real extent_
QgsRect *QgsPostgresProvider::extent()
{
  return &layerExtent;      //extent_->MinX, extent_->MinY, extent_->MaxX, extent_->MaxY);
}

/** 
 * Return the feature type
 */
int QgsPostgresProvider::geometryType() const
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
int QgsPostgresProvider::fieldCount() const
{
  return attributeFields.size();
}

/**
 * Fetch attributes for a selected feature
 */
void QgsPostgresProvider::getFeatureAttributes(int key, int &row, QgsFeature *f) {

  QString sql = QString("select * from %1 where %2 = %3").arg(mSchemaTableName).arg(primaryKey).arg(key);

#ifdef QGISDEBUG
  std::cerr << "QgsPostgresProvider::getFeatureAttributes using: " << sql.toLocal8Bit().data() << std::endl; 
#endif
  PGresult *attr = PQexec(connection, (const char *)(sql.utf8()));

  for (int i = 0; i < fieldCount(); i++) {
    QString fld = PQfname(attr, i);
    // Dont add the WKT representation of the geometry column to the identify
    // results
    if(fld != geometryColumn){
      // Add the attribute to the feature
      //QString val = mEncoding->toUnicode(PQgetvalue(attr,0, i));
	QString val = QString::fromUtf8 (PQgetvalue(attr, row, i));
      f->addAttribute(fld, val);
    }
  }
  PQclear(attr);
} 

/**Fetch attributes with indices contained in attlist*/
void QgsPostgresProvider::getFeatureAttributes(int key, int &row,
    QgsFeature *f, 
    std::list<int> const & attlist)
{
  std::list<int>::const_iterator iter;
  for(iter=attlist.begin();iter!=attlist.end();++iter)
  {
    QString sql = QString("select %1 from %2 where %3 = %4")
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
      f->addAttribute(fld, val);
    }
    PQclear(attr);
  }
}

std::vector<QgsField> const & QgsPostgresProvider::fields() const
{
  return attributeFields;
}

void QgsPostgresProvider::reset()
{
  // reset the cursor to the first record
  //--std::cout << "Resetting the cursor to the first record " << std::endl;
  QString declare = QString("declare qgisf binary cursor for select " +
      primaryKey + 
      ",asbinary(%1,'%2') as qgs_feature_geometry from %3").arg(geometryColumn)
    .arg(endianString()).arg(mSchemaTableName);
  if(sqlWhereClause.length() > 0)
  {
    declare += " where " + sqlWhereClause;
  }
  //--std::cout << "Selecting features using: " << declare << std::endl;
#ifdef QGISDEBUG
  std::cerr << "Setting up binary cursor: " << declare.toLocal8Bit().data() << std::endl;
#endif
  // set up the cursor
  PQexec(connection,"end work");

  PQexec(connection,"begin work");
  PQexec(connection, (const char *)(declare.utf8()));
  //--std::cout << "Error: " << PQerrorMessage(connection) << std::endl;
  
  // TODO - see if this deallocates member features
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
  switch ( endian() )
  {
    case QgsDataProvider::NDR : 
      return QString("NDR");
      break;
    case QgsDataProvider::XDR : 
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

#ifdef QGISDEBUG
  std::cerr << "Getting unique index using '" << sql.toLocal8Bit().data() << "'\n";
#endif
  PGresult *pk = executeDbCommand(connection, sql);

#ifdef QGISDEBUG
  std::cerr << "Got " << PQntuples(pk) << " rows.\n";
#endif

  QStringList log;

  // if we got no tuples we ain't got no unique index :)
  if (PQntuples(pk) == 0)
  {
#ifdef QGISDEBUG
    std::cerr << "Relation has no unique index -- "
      << "investigating alternatives\n";
#endif

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
#ifdef QGISDEBUG
      std::cerr << "Relation is a table. Checking to see if it has an "
        << "oid column.\n";
#endif
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
      qWarning("Unexpected relation type of '" + type + "'.");
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
        std::cerr << col.local8Bit().data() << '\n';
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

#ifdef QGISDEBUG
  if (primaryKey.length() > 0)
    std::cerr << "Qgis row key is " << primaryKey.toLocal8Bit().data() << '\n';
  else
    std::cerr << "Qgis row key was not set.\n";
#endif

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
	std::cerr << "Relation " << schemaName.toLocal8Bit().data() << '.' 
                  << tableName.toLocal8Bit().data()
		  << " doesn't exist in the pg_class table. This "
		  << "shouldn't happen and is odd.\n";
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
  for (int i = 0; i < oids.size(); ++i)
  {
    if (suitable.find(oids[i]->first) == suitable.end())
    {
      suitable[oids[i]->first] = oids[i]->second;
#ifdef QGISDEBUG
      std::cerr << "Adding column " << oids[i]->first.toLocal8Bit().data()
        << " as it may be suitable.\n";
#endif
    }
  }

  // Now have a map containing all of the columns in the view that
  // might be suitable for use as the key to the table. Need to choose
  // one thus:
  //
  // If there is more than one suitable column pick one that is
  // indexed, else pick one called 'oid' if it exists, else
  // pick the first one. If there are none we return an empty string. 

  if (suitable.size() == 1)
  {
    if (uniqueData(mSchemaName, mTableName, suitable.begin()->first))
    {
      key = suitable.begin()->first;
#ifdef QGISDEBUG
      std::cerr << "Picked column " << key.toLocal8Bit().data()
                << " as it is the only one that was suitable.\n";
#endif
    }
  }
  else if (suitable.size() > 1)
  {
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
#ifdef QGISDEBUG
        std::cerr << "Picked column '" << key.toLocal8Bit().data()
          << "' because it has an index.\n";
#endif
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
#ifdef QGISDEBUG
        std::cerr << "Picked column " << key.toLocal8Bit().data()
          << " as it is probably the postgresql object id "
          << " column (which contains unique values) and there are no"
          << " columns with indices to choose from\n.";
#endif
      }
      // else choose the first one in the container, ensuring that it
      // contains unique data
      else if (uniqueData(mSchemaName, mTableName, suitable.begin()->first))
      {
        key = suitable.begin()->first;
#ifdef QGISDEBUG
        std::cerr << "Picked column " << key.toLocal8Bit().data()
          << " as it was the first suitable column found"
          << " and there are no"
          << " columns with indices to choose from\n.";
#endif
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
    log.prepend(tr("The view ") + "'" + mSchemaName + mTableName + "'" +
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

  QString sql = "select count(distinct " + colName + ") = count(" +
      colName + ") from \"" + schemaName + "\".\"" + tableName + "\"";

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
  QString sql = "SELECT DISTINCT nv.nspname AS view_schema, v.relname AS view_name, a.attname AS view_column_name, nt.nspname AS table_schema, t.relname AS table_name, a.attname AS column_name, t.relkind as table_type, typ.typname as column_type FROM pg_namespace nv, pg_class v, pg_depend dv, pg_depend dt, pg_class t, pg_namespace nt, pg_attribute a, pg_user u, pg_type typ WHERE nv.oid = v.relnamespace AND v.relkind = 'v'::\"char\" AND v.oid = dv.refobjid AND dv.refclassid = 'pg_class'::regclass::oid AND dv.classid = 'pg_rewrite'::regclass::oid AND dv.deptype = 'i'::\"char\" AND dv.objid = dt.objid AND dv.refobjid <> dt.refobjid AND dt.classid = 'pg_rewrite'::regclass::oid AND dt.refclassid = 'pg_class'::regclass::oid AND dt.refobjid = t.oid AND t.relnamespace = nt.oid AND (t.relkind = 'r'::\"char\" OR t.relkind = 'v'::\"char\") AND t.oid = a.attrelid AND dt.refobjsubid = a.attnum AND nv.nspname NOT IN ('pg_catalog', 'information_schema' ) AND a.atttypid = typ.oid";

  // A structure to store the results of the above sql.
  typedef std::map<QString, TT> columnRelationsType;
  columnRelationsType columnRelations;

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

#ifdef QGISDEBUG
    std::cout << temp.view_schema.local8Bit().data() << "." 
	      << temp.view_name.local8Bit().data() << "."
	      << temp.view_column_name.local8Bit().data() << " <- "
	      << temp.table_schema.local8Bit().data() << "."
	      << temp.table_name.local8Bit().data() << "."
	      << temp.column_name.local8Bit().data() << " is a '"
	      << temp.table_type.local8Bit().data() << "' of type "
	      << temp.column_type.local8Bit().data() << '\n';
#endif
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

    if (ii == columnRelations.end())
      continue;

    int count = 0;
    const int max_loops = 100;

    while (ii->second.table_type != "r" && count < max_loops)
    {
#ifdef QGISDEBUG
      std::cerr << "Searching for the column that " 
		<< ii->second.table_schema.local8Bit().data()  << '.'
		<< ii->second.table_name.local8Bit().data() << "."
		<< ii->second.column_name.local8Bit().data() 
                << " refers to.\n";
#endif

      ii = columnRelations.find(QString(ii->second.table_schema + '.' +
					ii->second.table_name + '.' +
					ii->second.column_name));
      assert(ii != columnRelations.end());
      ++count;
    }

    if (count >= max_loops)
    {
      std::cerr << "  Search for the underlying table.column for view column "
		<< ii->second.table_schema.local8Bit().data() << '.' 
		<< ii->second.table_name.local8Bit().data() << '.'
		<< ii->second.column_name.local8Bit().data() << " failed: exceeded maximum "
		<< "interation limit (" << max_loops << ").\n";
      cols[ii->second.view_column_name] = SRC("","","","");
    }
    else
    {
      cols[ii->second.view_column_name] = 
	SRC(ii->second.table_schema, 
	    ii->second.table_name, 
	    ii->second.column_name, 
            ii->second.column_type);

#ifdef QGISDEBUG
      std::cerr << "  " << PQgetvalue(result, i, 0) << " derives from " 
		<< ii->second.table_schema.local8Bit().data() << "."
		<< ii->second.table_name.local8Bit().data() << "."
		<< ii->second.column_name.local8Bit().data() << '\n';
#endif
    }
  }
  PQclear(result);
}

// Returns the minimum value of an attribute
QString QgsPostgresProvider::minValue(int position){
  // get the field name 
  QgsField fld = attributeFields[position];
  QString sql;
  if(sqlWhereClause.isEmpty())
  {
    sql = QString("select min(%1) from %2").arg(fld.name()).arg(mSchemaTableName);
  }
  else
  {
    sql = QString("select min(%1) from %2").arg(fld.name()).arg(mSchemaTableName)+" where "+sqlWhereClause;
  }
  PGresult *rmin = PQexec(connection,(const char *)(sql.utf8()));
  QString minValue = PQgetvalue(rmin,0,0);
  PQclear(rmin);
  return minValue;
}

// Returns the maximum value of an attribute

QString QgsPostgresProvider::maxValue(int position){
  // get the field name 
  QgsField fld = attributeFields[position];
  QString sql;
  if(sqlWhereClause.isEmpty())
  {
    sql = QString("select max(%1) from %2").arg(fld.name()).arg(mSchemaTableName);
  }
  else
  {
    sql = QString("select max(%1) from %2").arg(fld.name()).arg(mSchemaTableName)+" where "+sqlWhereClause;
  } 
  PGresult *rmax = PQexec(connection,(const char *)(sql.utf8()));
  QString maxValue = PQgetvalue(rmax,0,0);
  PQclear(rmax);
  return maxValue;
}


int QgsPostgresProvider::maxPrimaryKeyValue()
{
  QString sql;

  sql = QString("select max(%1) from %2")
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

bool QgsPostgresProvider::addFeature(QgsFeature* f, int primaryKeyHighWater)
{
#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Entering."
                << "." << std::endl;
#endif
  
  if(f)
  {
    QString insert("INSERT INTO ");
    insert+=mSchemaTableName;
    insert+=" (";

    // add the name of the geometry column to the insert statement
    insert += geometryColumn;

    // add the name of the primary key column to the insert statement
    insert += ",";
    insert += primaryKey;

#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Constructing insert SQL, currently at: " << insert.toLocal8Bit().data()
                << "." << std::endl;
#endif
  
    
    
    //add the names of the other fields to the insert
    std::vector<QgsFeatureAttribute> attributevec=f->attributeMap();
    
#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Got attribute map"
                << "." << std::endl;
#endif
    
    for(std::vector<QgsFeatureAttribute>::iterator it=attributevec.begin();it!=attributevec.end();++it)
    {
      QString fieldname=it->fieldName();

#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Checking field against: " << fieldname.toLocal8Bit().data()
                << "." << std::endl;
#endif

            
      //TODO: Check if field exists in this layer
      // (Sometimes features will have fields that are not part of this layer since
      // they have been pasted from other layers with a different field map)
      bool fieldInLayer = FALSE;
      
      for (std::vector<QgsField>::iterator iter  = attributeFields.begin();
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
           (fieldInLayer)
         )
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
      //Postgis 1.0 wants bytea instead of hex
	QString oct = QString::number((int)geom[i], 8);
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
	insert+=oct;
    }
    insert+="::bytea',"+srid+")";

    //add the primary key value to the insert statement
    insert += ",";
    insert += QString::number(primaryKeyHighWater);

    //add the field values to the insert statement
    for(std::vector<QgsFeatureAttribute>::iterator it=attributevec.begin();it!=attributevec.end();++it)
    {
      QString fieldname=it->fieldName();
#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Checking field name " << fieldname.toLocal8Bit().data()
                << "." << std::endl;
#endif
      
      //TODO: Check if field exists in this layer
      // (Sometimes features will have fields that are not part of this layer since
      // they have been pasted from other layers with a different field map)
      bool fieldInLayer = FALSE;
      
      for (std::vector<QgsField>::iterator iter  = attributeFields.begin();
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
           (fieldInLayer)
         )
      {
        QString fieldvalue = it->fieldValue();
        bool charactertype=false;
        insert+=",";

#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Field is in layer with value " << fieldvalue.toLocal8Bit().data()
                << "." << std::endl;
#endif
        
        //add quotes if the field is a character or date type
      if(fieldvalue != "NULL" && fieldvalue != "DEFAULT")
        {
          for(std::vector<QgsField>::iterator iter=attributeFields.begin();iter!=attributeFields.end();++iter)
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
#ifdef QGISDEBUG
    qWarning("insert statement is: "+insert);
#endif

    //send INSERT statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(insert.utf8()));
    if(result==0)
    {
      QMessageBox::information(0,"INSERT error","An error occured during feature insertion",QMessageBox::Ok);
      return false;
    }
    ExecStatusType message=PQresultStatus(result);
    if(message==PGRES_FATAL_ERROR)
    {
      QMessageBox::information(0,"INSERT error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
      return false;
    }
    
#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Exiting with true."
                << "." << std::endl;
#endif
    return true;
  }
  
#ifdef QGISDEBUG
      std::cout << "QgsPostgresProvider::addFeature: Exiting with false."
                << "." << std::endl;
#endif
  
  return false;
}

QString QgsPostgresProvider::getDefaultValue(const QString& attr, QgsFeature* f)
{
  return "DEFAULT";
}

bool QgsPostgresProvider::deleteFeature(int id)
{
  QString sql("DELETE FROM "+mSchemaTableName+" WHERE "+primaryKey+" = "+QString::number(id));
#ifdef QGISDEBUG
  qWarning("delete sql: "+sql);
#endif

  //send DELETE statement and do error handling
  PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
  if(result==0)
  {
    QMessageBox::information(0,"DELETE error","An error occured during deletion from disk",QMessageBox::Ok);
    return false;
  }
  ExecStatusType message=PQresultStatus(result);
  if(message==PGRES_FATAL_ERROR)
  {
    QMessageBox::information(0,"DELETE error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
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
QString QgsPostgresProvider::postgisVersion(PGconn *connection){
  PGresult *result = PQexec(connection, "select postgis_version()");
  postgisVersionInfo = PQgetvalue(result,0,0);
#ifdef QGISDEBUG
  std::cerr << "PostGIS version info: " << postgisVersionInfo.toLocal8Bit().data() << std::endl;
#endif
  PQclear(result);
  // assume no capabilities
  geosAvailable = false;
  gistAvailable = false;
  projAvailable = false;
  // parse out the capabilities and store them
  QStringList postgisParts = QStringList::split(" ", postgisVersionInfo);
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
  return postgisVersionInfo;
}

bool QgsPostgresProvider::addFeatures(std::list<QgsFeature*> const flist)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");

  int primaryKeyHighWater = maxPrimaryKeyValue();

  for(std::list<QgsFeature*>::const_iterator it=flist.begin();it!=flist.end();++it)
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

bool QgsPostgresProvider::deleteFeatures(std::list<int> const & id)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");
  for(std::list<int>::const_iterator it=id.begin();it!=id.end();++it)
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

bool QgsPostgresProvider::addAttributes(std::map<QString,QString> const & name)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");
  for(std::map<QString,QString>::const_iterator iter=name.begin();iter!=name.end();++iter)
  {
    QString sql="ALTER TABLE "+mSchemaTableName+" ADD COLUMN "+(*iter).first+" "+(*iter).second;
#ifdef QGISDEBUG
    qWarning(sql);
#endif
    //send sql statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
    if(result==0)
    {
      returnvalue=false;
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        QMessageBox::information(0,"ALTER TABLE error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
      } 
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::deleteAttributes(std::set<QString> const & name)
{
  bool returnvalue=true;
  PQexec(connection,"BEGIN");
  for(std::set<QString>::const_iterator iter=name.begin();iter!=name.end();++iter)
  {
    QString sql="ALTER TABLE "+mSchemaTableName+" DROP COLUMN "+(*iter);
#ifdef QGISDEBUG
    qWarning(sql);
#endif
    //send sql statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
    if(result==0)
    {
      returnvalue=false;
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        QMessageBox::information(0,"ALTER TABLE error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
      }
    }
    else
    {
      //delete the attribute from attributeFields
      for(std::vector<QgsField>::iterator it=attributeFields.begin();it!=attributeFields.end();++it)
      {
        if((*it).name()==(*iter))
        {
          attributeFields.erase(it);
          break;
        }
      }
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::changeAttributeValues(std::map<int,std::map<QString,QString> > const & attr_map)
{
  bool returnvalue=true; 
  PQexec(connection,"BEGIN");

  for(std::map<int,std::map<QString,QString> >::const_iterator iter=attr_map.begin();iter!=attr_map.end();++iter)
  {
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

      QString sql="UPDATE "+mSchemaTableName+" SET "+(*siter).first+"="+value+" WHERE " +primaryKey+"="+QString::number((*iter).first);
#ifdef QGISDEBUG
      qWarning(sql);
#endif

      //send sql statement and do error handling
      PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
      if(result==0)
      {
        returnvalue=false;
        ExecStatusType message=PQresultStatus(result);
        if(message==PGRES_FATAL_ERROR)
        {
          QMessageBox::information(0,"UPDATE error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
        }
      }
    }
  }
  PQexec(connection,"COMMIT");
  reset();
  return returnvalue;
}

bool QgsPostgresProvider::changeGeometryValues(std::map<int, QgsGeometry> & geometry_map)
{
#ifdef QGISDEBUG
      std::cerr << "QgsPostgresProvider::changeGeometryValues: entering."
                << std::endl;
#endif
  
  bool returnvalue=true; 
  
  PQexec(connection,"BEGIN");

  for(std::map<int, QgsGeometry>::const_iterator iter  = geometry_map.begin();
                                                 iter != geometry_map.end();
                                               ++iter)
  {

#ifdef QGISDEBUG
      std::cerr << "QgsPostgresProvider::changeGeometryValues: iterating over the map of changed geometries..."
                << std::endl;
#endif
    
    if (iter->second.wkbBuffer())
    {
#ifdef QGISDEBUG
      std::cerr << "QgsPostgresProvider::changeGeometryValues: iterating over feature id "
                << iter->first
                << std::endl;
#endif
    
      QString sql = "UPDATE "+ mSchemaTableName +" SET " + 
                    geometryColumn + "=";
                    
      sql += "GeomFromWKB('";

      //add the wkb geometry to the insert statement
      unsigned char* geom = iter->second.wkbBuffer();
      
      for (int i=0; i < iter->second.wkbSize(); ++i)
      {
        //Postgis 1.0 wants bytea instead of hex
	QString oct = QString::number((int)geom[i], 8);
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
      sql+="::bytea',"+srid+")";
      sql+=" WHERE " +primaryKey+"="+QString::number(iter->first);

#ifdef QGISDEBUG
      qWarning(sql);
#endif

#ifdef QGISDEBUG
      std::cerr << "QgsPostgresProvider::changeGeometryValues: Updating with '"
                << sql.toLocal8Bit().data()
                << "'."
                << std::endl;
#endif
    
      // send sql statement and do error handling
      // TODO: Make all error handling like this one
      PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
      if (result==0)
      {
        QMessageBox::critical(0, "PostGIS error", 
                                 "An error occured contacting the PostgreSQL databse",
                                 QMessageBox::Ok,
                                 Qt::NoButton);
        return false;
      }
      ExecStatusType message=PQresultStatus(result);
      if(message==PGRES_FATAL_ERROR)
      {
        QMessageBox::information(0, "PostGIS error", 
                                 "The PostgreSQL databse returned: "
                                   + QString(PQresultErrorMessage(result)),
                                 QMessageBox::Ok,
                                 Qt::NoButton);
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
#ifdef QGISDEBUG
    qWarning("insert statement is: "+insert);
#endif

    //send INSERT statement and do error handling
    PGresult* result=PQexec(connection, (const char *)(insert.utf8()));
    if(result==0)
    {
      QMessageBox::information(0,"INSERT error","An error occured during feature insertion",QMessageBox::Ok);
      return false;
    }
    ExecStatusType message=PQresultStatus(result);
    if(message==PGRES_FATAL_ERROR)
    {
      QMessageBox::information(0,"INSERT error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
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
#ifdef QGISDEBUG
      qWarning(sql);
#endif

      //send sql statement and do error handling
      PGresult* result=PQexec(connection, (const char *)(sql.utf8()));
      if(result==0)
      {
        returnvalue=false;
        ExecStatusType message=PQresultStatus(result);
        if(message==PGRES_FATAL_ERROR)
        {
          QMessageBox::information(0,"UPDATE error",QString(PQresultErrorMessage(result)),QMessageBox::Ok);
        }
      }
    }
  }
*/
  } // for each feature
  
  PQexec(connection,"COMMIT");
  
  // TODO: Reset Geometry dirty if commit was OK
  
  reset();
  
#ifdef QGISDEBUG
      std::cerr << "QgsPostgresProvider::changeGeometryValues: exiting."
                << std::endl;
#endif
  
  return returnvalue;
}


bool QgsPostgresProvider::supportsSaveAsShapefile() const
{
  return false;
}

int QgsPostgresProvider::capabilities() const
{
  return (
           QgsVectorDataProvider::AddFeatures |
           QgsVectorDataProvider::DeleteFeatures |
           QgsVectorDataProvider::ChangeAttributeValues |
           QgsVectorDataProvider::AddAttributes |
           QgsVectorDataProvider::DeleteAttributes |
           QgsVectorDataProvider::ChangeGeometries
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

  std::cerr << "QgsPostgresProvider: Running SQL: " << 
    sql << std::endl;        
#else                 
  QString sql = "select count(*) from " + mSchemaTableName;

  if(sqlWhereClause.length() > 0)
  {
    sql += " where " + sqlWhereClause;
  }
#endif

  PGresult *result = PQexec(connection, (const char *) (sql.utf8()));

#ifdef QGISDEBUG
  std::cerr << "QgsPostgresProvider: Approximate Number of features as text: " << 
    PQgetvalue(result, 0, 0) << std::endl;
#endif

  numberFeatures = QString(PQgetvalue(result, 0, 0)).toLong();
  PQclear(result);

#ifdef QGISDEBUG
  std::cerr << "QgsPostgresProvider: Approximate Number of features: " << 
    numberFeatures << std::endl;
#endif

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
  sql = "select xmax(extent(" + geometryColumn + ")) as xmax,"
    "xmin(extent(" + geometryColumn + ")) as xmin,"
    "ymax(extent(" + geometryColumn + ")) as ymax," 
    "ymin(extent(" + geometryColumn + ")) as ymin" 
    " from " + mSchemaTableName + "";
#endif

#ifdef QGISDEBUG 
  qDebug((const char*)(QString("QgsPostgresProvider::calculateExtents - Getting approximate extent using: '") + sql + "'").toLocal8Bit().data());
#endif
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

    std::cout << "QgsPostgresProvider: After row " << i << ", extent is: " 
      << layerExtent.xMin() << ", " << layerExtent.yMin() <<
      " " << layerExtent.xMax() << ", " << layerExtent.yMax() << std::endl;

  }

#ifdef QGISDEBUG
  QString xMsg;
  QTextOStream(&xMsg).precision(18);
  QTextOStream(&xMsg).width(18);
  QTextOStream(&xMsg) << "QgsPostgresProvider: Set extents to: " << layerExtent.
    xMin() << ", " << layerExtent.yMin() << " " << layerExtent.xMax() << ", " << layerExtent.yMax();
  std::cerr << xMsg << std::endl;
#endif

  std::cout << "QgsPostgresProvider: Set limit 5 extents to: " 
    << layerExtent.xMin() << ", " << layerExtent.yMin() <<
    " " << layerExtent.xMax() << ", " << layerExtent.yMax() << std::endl;

  // clear query result
  PQclear(result);

#else // non-postgresql threads version

  // get the extents

  QString sql = "select extent(" + geometryColumn + ") from " + 
    mSchemaTableName;
  if(sqlWhereClause.length() > 0)
  {
    sql += " where " + sqlWhereClause;
  }

#if WASTE_TIME
  sql = "select xmax(extent(" + geometryColumn + ")) as xmax,"
    "xmin(extent(" + geometryColumn + ")) as xmin,"
    "ymax(extent(" + geometryColumn + ")) as ymax," 
    "ymin(extent(" + geometryColumn + ")) as ymin" 
    " from " + mSchemaTableName;
#endif

#ifdef QGISDEBUG 
  qDebug((const char*)(QString("+++++++++QgsPostgresProvider::calculateExtents -  Getting extents using schema.table: ") + sql).toLocal8Bit().data());
#endif
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
#ifdef QGISDEBUG
    QString xMsg;
    QTextOStream(&xMsg).precision(18);
    QTextOStream(&xMsg).width(18);
    QTextOStream(&xMsg) << "Set extents to: " << layerExtent.
      xMin() << ", " << layerExtent.yMin() << " " << layerExtent.xMax() << ", " << layerExtent.yMax();
    std::cerr << xMsg.toLocal8Bit().data() << std::endl;
#endif
    // clear query result
    PQclear(result);
  }


#endif

}

/**
 * Event sink for events from threads
 */
void QgsPostgresProvider::customEvent( QCustomEvent * e )
{
  std::cout << "QgsPostgresProvider: received a custom event " << e->type() << std::endl;

  switch ( e->type() )
  {
    case (QEvent::Type) QGis::ProviderExtentCalcEvent:

      std::cout << "QgsPostgresProvider: extent has been calculated" << std::endl;

      // Collect the new extent from the event and set this layer's
      // extent with it.

      setExtent( (QgsRect*) e->data() );


      std::cout << "QgsPostgresProvider: new extent has been saved" << std::endl;

      std::cout << "QgsPostgresProvider: Set extent to: " 
        << layerExtent.xMin() << ", " << layerExtent.yMin() <<
        " " << layerExtent.xMax() << ", " << layerExtent.yMax() << std::endl;

      std::cout << "QgsPostgresProvider: emitting fullExtentCalculated()" << std::endl;

      emit fullExtentCalculated();

      // TODO: Only uncomment this when the overview map canvas has been subclassed
      // from the QgsMapCanvas

      //        std::cout << "QgsPostgresProvider: emitting repaintRequested()" << std::endl;
      //        emit repaintRequested();

      break;

    case (QEvent::Type) QGis::ProviderCountCalcEvent:

      std::cout << "QgsPostgresProvider: count has been calculated" << std::endl;

      QgsProviderCountCalcEvent* e1 = (QgsProviderCountCalcEvent*) e;

      numberFeatures = e1->numberFeatures();

      std::cout << "QgsPostgresProvider: count is " << numberFeatures << std::endl;

      break;
  }

  std::cout << "QgsPostgresProvider: Finished processing custom event " << e->type() << std::endl;

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

#ifdef QGISDEBUG
  std::cerr << "Creating binary cursor" << std::endl;
#endif

  // get the same value using a binary cursor

  PQexec(connection,"begin work");
  QString oidDeclare = QString("declare oidcursor binary cursor for select oid from pg_class where relname = '%1' and relnamespace = (select oid from pg_namespace where nspname = '%2')").arg(mTableName).arg(mSchemaName);
  // set up the cursor
  PQexec(connection, (const char *)oidDeclare);
  QString fetch = "fetch forward 1 from oidcursor";

#ifdef QGISDEBUG
  std::cerr << "Fetching a record and attempting to get check endian-ness" << std::endl;
#endif

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

#ifdef QGISDEBUG
  std::cerr << "Getting geometry column: " << sql.toLocal8Bit().data() << std::endl;
#endif

  PGresult *result = executeDbCommand(connection, sql);

#ifdef QGISDEBUG
  std::cerr << "geometry column query returned " 
	    << PQntuples(result) << std::endl;
  std::cerr << "column number of srid is " 
	    << PQfnumber(result, "srid") << std::endl;
#endif

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
      "srid("         + geometryColumn + "), "
      "geometrytype(" + geometryColumn + ") from " + 
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

#ifdef QGISDEBUG
  if (valid)
    std::cout << "SRID is " << srid.toLocal8Bit().data() << '\n'
              << "type is " << fType.toLocal8Bit().data() << '\n'
              << "Feature type is " << geomType << '\n'
              << "Feature type name is " 
              << QGis::qgisFeatureTypes[geomType] << std::endl;
  else
    std::cout << "Failed to get geometry details for Postgres layer."
              << std::endl;
#endif

  return valid;
}

PGresult* QgsPostgresProvider::executeDbCommand(PGconn* connection, 
                                                const QString& sql)
{
  PGresult *result = PQexec(connection, (const char *) (sql.utf8()));
#ifdef QGISDEBUG
  std::cout << "Executed SQL: " << sql.local8Bit().data() << '\n';
  if (PQresultStatus(result) == PGRES_TUPLES_OK)
    std::cout << "Command was successful.\n";
  else
    std::cout << "Command was unsuccessful. The error message was: "
              << PQresultErrorMessage(result) << ".\n";
#endif
  return result;
}

void QgsPostgresProvider::showMessageBox(const QString& title, 
					 const QString& text)
{
  QgsMessageViewer* message = new QgsMessageViewer();
  message->setCaption(title);
  message->setMessageAsPlainText(text);
  message->exec(); // modal
}

void QgsPostgresProvider::showMessageBox(const QString& title, 
					 const QStringList& text)
{
  showMessageBox(title, text.join("\n"));
}

int QgsPostgresProvider::getSrid()
{
  return srid.toInt();
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

