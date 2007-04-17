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
/* $Id$ */

#ifndef QGSDATASOURCEURI_H
#define QGSDATASOURCEURI_H

#include <QString> 

/** 
\struct QgsDataSourceURI 
\brief Structure for storing the component parts of a PostgreSQL/RDBMS datasource URI.
           
 This structure stores the database connection information, including host, database,
  user name, password, schema, password, and sql where clause
 */
class CORE_EXPORT QgsDataSourceURI
{

public:
  
  //! default constructor
  QgsDataSourceURI();
   
  //! constructor which parses input URI
  QgsDataSourceURI(QString uri);
   
  //! All in a single string
  QString text() const;
  
  //! Set all connection related members at once
  void setConnection(const QString& aHost,
                     const QString& aPort,
                     const QString& aDatabase,
                     const QString& aUsername,
                     const QString& aPassword);
  
  //! Set all data source related members at once
  void setDataSource(const QString& aSchema,
                     const QString& aTable,
                     const QString& aGeometryColumn,
                     const QString& aSql = QString());
  
  /* data */

  //! host name
  QString host;
  //! database name
  QString database;
  //! port the database server listens on
  QString port;
  //! schema
  QString schema;
  //! spatial table
  QString table;
  //! geometry column
  QString geometryColumn;
  //! SQL where clause used to limit features returned from the layer
  QString sql;
  //! username 
  QString username;
  //! password
  QString password;
  
  //! whole connection info (host, db, port, name, pass)
  QString connInfo;

};

#endif //QGSDATASOURCEURI_H

