/***************************************************************************
 *   Copyright (C) 2005 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
/* $Id$ */
#include "qgsprojectionselector.h"

//standard includes
#include <iostream>
#include <cassert>
#include <sqlite3.h>

//qgis includes
#include "qgis.h" //magick numbers here
#include "qgsapplication.h"
#include "qgslogger.h"

//qt includes
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QHeaderView>
#include <QResizeEvent>

QgsProjectionSelector::QgsProjectionSelector(QWidget* parent,
                                             const char * name,
                                             Qt::WFlags fl)
    : QWidget(parent, fl),
      mProjListDone(FALSE),
      mUserProjListDone(FALSE),
      mSRSNameSelectionPending(FALSE),
      mSRSIDSelectionPending(FALSE)

{
  setupUi(this);
  connect(lstCoordinateSystems, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
      this, SLOT(coordinateSystemSelected(QTreeWidgetItem*)));
  connect(leSearch, SIGNAL(returnPressed()), pbnFind, SLOT(animateClick()));

  // Get the full path name to the sqlite3 spatial reference database.
  mSrsDatabaseFileName = QgsApplication::srsDbFilePath();
  lstCoordinateSystems->header()->setResizeMode(1,QHeaderView::Stretch);
}


QgsProjectionSelector::~QgsProjectionSelector()
{}


void QgsProjectionSelector::resizeEvent ( QResizeEvent * theEvent )
{

  lstCoordinateSystems->header()->resizeSection(0,(theEvent->size().width()-120));
  lstCoordinateSystems->header()->resizeSection(1,120);
}

void QgsProjectionSelector::showEvent ( QShowEvent * theEvent )
{
  // ensure the projection list view is actually populated
  // before we show this widget

  if (!mProjListDone)
  {
    applyProjList(&mCrsFilter);
  }

  if (!mUserProjListDone)
  {
    applyUserProjList(&mCrsFilter);
  }

  // check if a paricular projection is waiting
  // to be pre-selected, and if so, to select it now.
  if (mSRSNameSelectionPending)
  {
    applySRSNameSelection();
  }
  if (mSRSIDSelectionPending)
  {
    applySRSIDSelection();
  }

  // Pass up the inheritance heirarchy
  QWidget::showEvent(theEvent);
}

QString QgsProjectionSelector::ogcWmsCrsFilterAsSqlExpression(QSet<QString> * crsFilter)
{
  QString sqlExpression = "1";             // it's "SQL" for "true"
  QStringList epsgParts = QStringList();

  if (!crsFilter)
  {
    return sqlExpression;
  }

  /*
     Ref: WMS 1.3.0, section 6.7.3 "Layer CRS":

     Every Layer CRS has an identifier that is a character string. Two types of
     Layer CRS identifiers are permitted: "label" and "URL" identifiers:

     Label: The identifier includes a namespace prefix, a colon, a numeric or
        string code, and in some instances a comma followed by additional
        parameters. This International Standard defines three namespaces: 
        CRS, EPSG and AUTO2 [...]

     URL: The identifier is a fully-qualified Uniform Resource Locator that 
        references a publicly-accessible file containing a definition of the CRS
        that is compliant with ISO 19111.
  */

  // iterate through all incoming CRSs

  QSet<QString>::const_iterator i = crsFilter->begin();
  while (i != crsFilter->end())
  {
    QStringList parts = i->split(":");

    if (parts.at(0) == "EPSG")
    {
      epsgParts.push_back( parts.at(1) );
    }

    ++i;
  }

//   for ( int i = 0; i < crsFilter->size(); i++ ) 
//   {
//     QStringList parts = crsFilter->at(i).split(":");
// 
//     if (parts.at(0) == "EPSG")
//     {
//       epsgParts.push_back( parts.at(1) );
//     }
//   }

  if (epsgParts.size())
  {
    sqlExpression = "epsg in (";
    sqlExpression += epsgParts.join(",");
    sqlExpression += ")";
  }

  QgsDebugMsg("exiting with '" + sqlExpression + "'.");

  return sqlExpression;
}


void QgsProjectionSelector::setSelectedSRSName(QString theSRSName)
{
  mSRSNameSelection = theSRSName;
  mSRSNameSelectionPending = TRUE;
  mSRSIDSelectionPending = FALSE;  // only one type can be pending at a time

  if (isVisible())
  {
    applySRSNameSelection();
  }
  // else we will wait for the projection selector to
  // become visible (with the showEvent()) and set the
  // selection there
}


void QgsProjectionSelector::setSelectedSRSID(long theSRSID)
{
  mSRSIDSelection = theSRSID;
  mSRSIDSelectionPending = TRUE;
  mSRSNameSelectionPending = FALSE;  // only one type can be pending at a time

  if (isVisible())
  {
    applySRSIDSelection();
  }
  // else we will wait for the projection selector to
  // become visible (with the showEvent()) and set the
  // selection there
}


void QgsProjectionSelector::applySRSNameSelection()
{
  if (
      (mSRSNameSelectionPending) &&
      (mProjListDone) &&
      (mUserProjListDone)
     )
  {
    //get the srid given the wkt so we can pick the correct list item
    QgsDebugMsg("called with " + mSRSNameSelection);
    QList<QTreeWidgetItem*> nodes = lstCoordinateSystems->findItems(mSRSNameSelection, Qt::MatchExactly|Qt::MatchRecursive, 0);
  
    if (nodes.count() > 0)
    {
      lstCoordinateSystems->setCurrentItem(nodes.first());
      lstCoordinateSystems->scrollToItem(nodes.first());
    }
    else // unselect the selected item to avoid confusing the user
    { 
      lstCoordinateSystems->clearSelection();
      teProjection->setText("");
    }

    mSRSNameSelectionPending = FALSE;
  }
}

void QgsProjectionSelector::applySRSIDSelection()
{
  if (
      (mSRSIDSelectionPending) &&
      (mProjListDone) &&
      (mUserProjListDone)
     )
  {
    QString mySRSIDString = QString::number(mSRSIDSelection);
  
    QList<QTreeWidgetItem*> nodes = lstCoordinateSystems->findItems(mySRSIDString, Qt::MatchExactly|Qt::MatchRecursive, 1);
  
    if (nodes.count() > 0)
    {
      lstCoordinateSystems->setCurrentItem(nodes.first());
      lstCoordinateSystems->scrollToItem(nodes.first());
    }
    else // unselect the selected item to avoid confusing the user
    { 
      lstCoordinateSystems->clearSelection();
      teProjection->setText("");
    }

    mSRSIDSelectionPending = FALSE;
  }
}


//note this line just returns the projection name!
QString QgsProjectionSelector::getSelectedName()
{
  // return the selected wkt name from the list view
  QTreeWidgetItem *lvi = lstCoordinateSystems->currentItem();
  if(lvi)
  {
    return lvi->text(0);
  }
  else
  {
    return QString::null;
  }
}
// Returns the whole proj4 string for the selected projection node
QString QgsProjectionSelector::getCurrentProj4String()
{
  // Only return the projection if there is a node in the tree
  // selected that has an srid. This prevents error if the user
  // selects a top-level node rather than an actual coordinate
  // system
  //
  // Get the selected node
  QTreeWidgetItem *myItem = lstCoordinateSystems->currentItem();
  if(myItem)
  {

    if(myItem->text(1).length() > 0)
    {
      QString myDatabaseFileName;
      QString mySrsId = myItem->text(1);

      QgsDebugMsg("mySrsId = " + mySrsId);
      QgsDebugMsg("USER_PROJECTION_START_ID = " + QString::number(USER_PROJECTION_START_ID));
      //
      // Determine if this is a user projection or a system on
      // user projection defs all have srs_id >= 100000
      //
      if (mySrsId.toLong() >= USER_PROJECTION_START_ID)
      {
        myDatabaseFileName = QgsApplication::qgisUserDbFilePath();
        QFileInfo myFileInfo;
        myFileInfo.setFile(myDatabaseFileName);
        if ( !myFileInfo.exists( ) ) //its unlikely that this condition will ever be reached
        {
          QgsDebugMsg("users qgis.db not found");
          return QString("");
        }
        else
        {
          QgsDebugMsg("users qgis.db found");
        }
      }
      else //must be  a system projection then
      {
        myDatabaseFileName =  mSrsDatabaseFileName;
      }
      QgsDebugMsg("db = " + myDatabaseFileName);


      sqlite3 *db;
      int rc;
      rc = sqlite3_open(myDatabaseFileName.toUtf8().data(), &db);
      if(rc)
      {
        QgsLogger::warning("Can't open database: " + QString(sqlite3_errmsg(db)));
        // XXX This will likely never happen since on open, sqlite creates the
        //     database if it does not exist.
        assert(rc == 0);
      }
      // prepare the sql statement
      const char *pzTail;
      sqlite3_stmt *ppStmt;
      QString sql = "select parameters from tbl_srs where srs_id = ";
      sql += mySrsId;
      
      QgsDebugMsg("Selection sql: " + sql);

      rc = sqlite3_prepare(db, sql.utf8(), sql.length(), &ppStmt, &pzTail);
      // XXX Need to free memory from the error msg if one is set
      QString myProjString;
      if(rc == SQLITE_OK)
      {
        if(sqlite3_step(ppStmt) == SQLITE_ROW)
        {
          myProjString = QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 0));
        }
      }
      // close the statement
      sqlite3_finalize(ppStmt);
      // close the database
      sqlite3_close(db);
#ifdef QGISDEBUG
      std::cout << "Item selected : " << myItem->text(0).toLocal8Bit().data() << std::endl;
      std::cout << "Item selected full string : " << myProjString.toLocal8Bit().data() << std::endl;
#endif
      assert(myProjString.length() > 0);
      return myProjString;
    }
    else
    {
      // No node is selected, return null
      return QString("");
    }
  }
  else
  {
    // No node is selected, return null
    return QString("");
  }

}

long QgsProjectionSelector::getCurrentLongAttribute(QString attributeName)
{
  // Only return the attribute if there is a node in the tree
  // selected that has an srs_id.  This prevents error if the user
  // selects a top-level node rather than an actual coordinate
  // system
  //
  // Get the selected node
  QTreeWidgetItem *lvi = lstCoordinateSystems->currentItem();
  if(lvi)
  {
    // Make sure the selected node is a srs and not a top-level projection node
    if(lvi->text(1).length() > 0)
    {
      QString myDatabaseFileName;
      //
      // Determine if this is a user projection or a system on
      // user projection defs all have srs_id >= 100000
      //
      if (lvi->text(1).toLong() >= USER_PROJECTION_START_ID)
      {
        myDatabaseFileName = QgsApplication::qgisUserDbFilePath();
        QFileInfo myFileInfo;
        myFileInfo.setFile(myDatabaseFileName);
        if ( !myFileInfo.exists( ) )
        {
          std::cout << " QgsSpatialRefSys::createFromSrid failed :  users qgis.db not found" << std::endl;
          return 0;
        }
      }
      else //must be  a system projection then
      {
        myDatabaseFileName=mSrsDatabaseFileName;
      }
      //
      // set up the database
      // XXX We could probabaly hold the database open for the life of this object,
      // assuming that it will never be used anywhere else. Given the low overhead,
      // opening it each time seems to be a reasonable approach at this time.
      sqlite3 *db;
      int rc;
      rc = sqlite3_open(myDatabaseFileName.toUtf8().data(), &db);
      if(rc)
      {
        std::cout <<  "Can't open database: " <<  sqlite3_errmsg(db) << std::endl;
        // XXX This will likely never happen since on open, sqlite creates the
        //     database if it does not exist.
        assert(rc == 0);
      }
      // prepare the sql statement
      const char *pzTail;
      sqlite3_stmt *ppStmt;
      QString sql = "select ";
      sql += attributeName;
      sql += " from tbl_srs where srs_id = ";
      sql += lvi->text(1);

#ifdef QGISDEBUG
      std::cout << "Finding selected attribute using : " <<  sql.toLocal8Bit().data() << std::endl;
#endif
      rc = sqlite3_prepare(db, sql.utf8(), sql.length(), &ppStmt, &pzTail);
      // XXX Need to free memory from the error msg if one is set
      QString mySrid;
      if(rc == SQLITE_OK)
      {
        // get the first row of the result set
        if(sqlite3_step(ppStmt) == SQLITE_ROW)
        {
          // get the wkt
          mySrid = QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 0));
        }
      }
      // close the statement
      sqlite3_finalize(ppStmt);
      // close the database
      sqlite3_close(db);
      // return the srs wkt
      return mySrid.toLong();
    }
  }

  // No node is selected, return null
  return 0;
}


long QgsProjectionSelector::getCurrentSRID()
{
  return getCurrentLongAttribute("srid");
}


long QgsProjectionSelector::getCurrentEpsg()
{
  return getCurrentLongAttribute("epsg");
}


long QgsProjectionSelector::getCurrentSRSID()
{
  QTreeWidgetItem* item = lstCoordinateSystems->currentItem();

  if(item != NULL && item->text(1).length() > 0)
  {
    return lstCoordinateSystems->currentItem()->text(1).toLong();
  }
  else
  {
    return 0;
  }
}


void QgsProjectionSelector::setOgcWmsCrsFilter(QSet<QString> crsFilter)
{
  mCrsFilter = crsFilter;
  mProjListDone = false;
  mUserProjListDone = false;
  lstCoordinateSystems->clear();
}


void QgsProjectionSelector::applyUserProjList(QSet<QString> * crsFilter)
{
#ifdef QGISDEBUG
  std::cout << "Fetching user projection list..." << std::endl;
#endif

  // convert our Coordinate Reference System filter into the SQL expression
  QString sqlFilter = ogcWmsCrsFilterAsSqlExpression(crsFilter);

  // User defined coordinate system node
  // Make in an italic font to distinguish them from real projections
  mUserProjList = new QTreeWidgetItem(lstCoordinateSystems,QStringList("User Defined Coordinate Systems"));

  QFont fontTemp = mUserProjList->font(0);
  fontTemp.setItalic(TRUE);
  mUserProjList->setFont(0, fontTemp);

  //determine where the user proj database lives for this user. If none is found an empty
  //now only will be shown
  QString myDatabaseFileName = QgsApplication::qgisUserDbFilePath();
  // first we look for ~/.qgis/qgis.db
  // if it doesnt exist we copy it in from the global resources dir
  QFileInfo myFileInfo;
  myFileInfo.setFile(myDatabaseFileName);
  //return straight away if the user has not created any custom projections
  if ( !myFileInfo.exists( ) )
  {
#ifdef QGISDEBUG
    std::cout << "Users qgis.db not found...skipping" << std::endl;
#endif

    mUserProjListDone = TRUE;
    return;
  }

  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  //check the db is available
  myResult = sqlite3_open(QString(myDatabaseFileName).toUtf8().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist. But we checked earlier for its existance
    //     and aborted in that case. This is because we may be runnig from read only
    //     media such as live cd and dont want to force trying to create a db.
    assert(myResult == 0);
  }

  // Set up the query to retreive the projection information needed to populate the list
  QString mySql = "select description, srs_id, is_geo, name, parameters from vw_srs ";
  mySql += "where ";
  mySql += sqlFilter;

#ifdef QGISDEBUG
  std::cout << "User projection list sql" << mySql.toLocal8Bit().data() << std::endl;
#endif
  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    QTreeWidgetItem *newItem;
    while(sqlite3_step(myPreparedStatement) == SQLITE_ROW)
    {
      newItem = new QTreeWidgetItem(mUserProjList, QStringList(QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,0))));
      // display the qgis srs_id in the second column of the list view
      newItem->setText(1,QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 1)));
    }
  }
  // close the sqlite3 statement
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);

  mUserProjListDone = TRUE;
}

void QgsProjectionSelector::applyProjList(QSet<QString> * crsFilter)
{
  // convert our Coordinate Reference System filter into the SQL expression
  QString sqlFilter = ogcWmsCrsFilterAsSqlExpression(crsFilter);

  // Create the top-level nodes for the list view of projections
  // Make in an italic font to distinguish them from real projections
  //
  // Geographic coordinate system node
  mGeoList = new QTreeWidgetItem(lstCoordinateSystems,QStringList("Geographic Coordinate Systems"));

  QFont fontTemp = mGeoList->font(0);
  fontTemp.setItalic(TRUE);
  mGeoList->setFont(0, fontTemp);

  // Projected coordinate system node
  mProjList = new QTreeWidgetItem(lstCoordinateSystems,QStringList("Projected Coordinate Systems"));

  fontTemp = mProjList->font(0);
  fontTemp.setItalic(TRUE);
  mProjList->setFont(0, fontTemp);

  //bail out in case the projections db does not exist
  //this is neccessary in case the pc is running linux with a
  //read only filesystem because otherwise sqlite will try
  //to create the db file on the fly

  QFileInfo myFileInfo;
  myFileInfo.setFile(mSrsDatabaseFileName);
  if ( !myFileInfo.exists( ) )
  {
    mProjListDone = TRUE;
    return;
  }

  // open the database containing the spatial reference data
  sqlite3 *db;
  int rc;
  rc = sqlite3_open(mSrsDatabaseFileName.toUtf8().data(), &db);
  if(rc)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(db) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(rc == 0);
  }
  // prepare the sql statement
  const char *pzTail;
  sqlite3_stmt *ppStmt;
  // get total count of records in the projection table
  QString sql = "select count(*) from tbl_srs";

  rc = sqlite3_prepare(db, sql.utf8(), sql.length(), &ppStmt, &pzTail);
  assert(rc == SQLITE_OK);
  sqlite3_step(ppStmt);

#ifdef QGISDEBUG
  int myEntriesCount = sqlite3_column_int(ppStmt, 0);
  std::cout << "Projection entries found in srs.db: " << myEntriesCount << std::endl;
#endif
  sqlite3_finalize(ppStmt);

  // Set up the query to retreive the projection information needed to populate the list
  //note I am giving the full field names for clarity here and in case someown
  //changes the underlying view TS
  sql = "select description, srs_id, is_geo, name, parameters from vw_srs ";
  sql += "where ";
  sql += sqlFilter;
  sql += " order by name, description";

#ifdef QGISDEBUG
  std::cout << "SQL for projection list:\n" << sql.toLocal8Bit().data() << std::endl;
#endif
  rc = sqlite3_prepare(db, sql.utf8(), sql.length(), &ppStmt, &pzTail);
  // XXX Need to free memory from the error msg if one is set
  if(rc == SQLITE_OK)
  {
#ifdef QGISDEBUG
    std::cout << "SQL for projection list executed ok..."  << std::endl;
#endif

    QTreeWidgetItem *newItem;
    // Cache some stuff to speed up creating of the list of projected
    // spatial reference systems
    QString previousSrsType("");
    QTreeWidgetItem* previousSrsTypeNode = NULL;

    while(sqlite3_step(ppStmt) == SQLITE_ROW)
    {
      // check to see if the srs is geographic
      int isGeo = sqlite3_column_int(ppStmt, 2);
      if(isGeo)
      {
        // this is a geographic coordinate system
        // Add it to the tree
        newItem = new QTreeWidgetItem(mGeoList, QStringList(QString::fromUtf8((char *)sqlite3_column_text(ppStmt,0))));

        // display the qgis srs_id in the second column of the list view
        newItem->setText(1,QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 1)));
      }
      else
      {
        // This is a projected srs

	QTreeWidgetItem *node;
	QString srsType = QString::fromUtf8((char*)sqlite3_column_text(ppStmt, 3));
        // Find the node for this type and add the projection to it
        // If the node doesn't exist, create it
	if (srsType == previousSrsType)
	{
	  node = previousSrsTypeNode;
	}
	else
	{ // Different from last one, need to search
	  QList<QTreeWidgetItem*> nodes = lstCoordinateSystems->findItems(srsType,Qt::MatchExactly|Qt::MatchRecursive,0);
	  if(nodes.count() == 0)
	  {
	    // the node doesn't exist -- create it
            // Make in an italic font to distinguish them from real projections
	    node = new QTreeWidgetItem(mProjList, QStringList(srsType));

            QFont fontTemp = node->font(0);
            fontTemp.setItalic(TRUE);
            node->setFont(0, fontTemp);
	  }
	  else
	  {
	    node = nodes.first();
	  }
	  // Update the cache.
	  previousSrsType = srsType;
	  previousSrsTypeNode = node;
	}
        // add the item, setting the projection name in the first column of the list view
        newItem = new QTreeWidgetItem(node, QStringList(QString::fromUtf8((char *)sqlite3_column_text(ppStmt,0))));
        // set the srs_id in the second column on the list view
        newItem->setText(1,QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 1)));
      }
      //Only enable thse lines temporarily if you want to generate a script
      //to update proj an ellipoid fields in the srs.db
      //updateProjAndEllipsoidAcronyms(QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 1)).toLong(),
      //                               QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 4)))  ;
    }
  }
#ifdef QGISDEBUG
  std::cout << "Size of projection list widget : " << sizeof(*lstCoordinateSystems) << std::endl;
#endif
  // close the sqlite3 statement
  sqlite3_finalize(ppStmt);
  // close the database
  sqlite3_close(db);

  mProjListDone = TRUE;
}

//this is a little helper function to populate the (well give you a sql script to populate)
//the projection_acronym and ellipsoid_acronym fields in the srs.db backend
//To cause it to be run, uncomment or add the line:
//      updateProjAndEllipsoidAcronyms(QString::fromUtf8((char *)sqlite3_column_text(ppStmt, 1)).toLong(),
//                                     QString:.fromUtf8((char *)sqlite3_column_text(ppStmt, 4)))  ;
//to the above method. NOTE it will cause a huge slow down in population of the proj selector dialog so
//remember to disable it again!
void QgsProjectionSelector::updateProjAndEllipsoidAcronyms(int theSrsid,QString theProj4String)
{


  //temporary hack
  QFile myFile( "/tmp/srs_updates.sql" );
  myFile.open(  QIODevice::WriteOnly | QIODevice::Append );
  QTextStream myStream( &myFile );

  QRegExp myProjRegExp( "proj=[a-zA-Z]* " );
  int myStart= 0;
  int myLength=0;
  myStart = myProjRegExp.search(theProj4String, myStart);
  QString myProjectionAcronym;
  if (myStart==-1)
  {
    std::cout << "proj string supplied has no +proj argument" << std::endl;
    myProjectionAcronym = "";
  }
  else
  {
    myLength = myProjRegExp.matchedLength();
    myProjectionAcronym = theProj4String.mid(myStart+PROJ_PREFIX_LEN,myLength-(PROJ_PREFIX_LEN+1));//+1 for space
  }


  QRegExp myEllipseRegExp( "ellps=[a-zA-Z0-9\\-]* " );
  myStart= 0;
  myLength=0;
  myStart = myEllipseRegExp.search(theProj4String, myStart);
  QString myEllipsoidAcronym;
  if (myStart==-1)
  {
    std::cout << "proj string supplied has no +ellps argument" << std::endl;
    myEllipsoidAcronym="";
  }
  else
  {
    myLength = myEllipseRegExp.matchedLength();
    myEllipsoidAcronym = theProj4String.mid(myStart+ELLPS_PREFIX_LEN,myLength-(ELLPS_PREFIX_LEN+1));
  }


  //now create the update statement
  QString mySql = "update tbl_srs set projection_acronym='" + myProjectionAcronym +
                  "', ellipsoid_acronym='" + myEllipsoidAcronym + "' where " +
                  "srs_id=" + QString::number(theSrsid)+";";


  //tmporary hack
  myStream << mySql << "\n";
  myFile.close();
  //std::cout

}

// New coordinate system selected from the list
void QgsProjectionSelector::coordinateSystemSelected( QTreeWidgetItem * theItem)
{
  // If the item has children, it's not an end node in the tree, and
  // hence is just a grouping thingy, not an actual SRS.
  if (theItem != NULL && theItem->childCount() == 0)
  {
    // Found a real SRS
    QString myDescription = tr("QGIS SRSID: ") + QString::number(getCurrentSRSID()) +"\n";
    myDescription        += tr("PostGIS SRID: ") + QString::number(getCurrentSRID()) +"\n";
    emit sridSelected(QString::number(getCurrentSRSID()));
    QString myProjString = getCurrentProj4String();
    if (!myProjString.isNull())
    {
      myDescription+=(myProjString);
    }

    lstCoordinateSystems->scrollToItem(theItem);
    teProjection->setText(myDescription);
  }
  else
  {
    // Not an SRS - remove the highlight so the user doesn't get too confused
    lstCoordinateSystems->setItemSelected(theItem, FALSE);  // TODO - make this work.
    teProjection->setText("");
  }
}

void QgsProjectionSelector::on_pbnFind_clicked()
{

#ifdef QGISDEBUG
  std::cout << "pbnFind..." << std::endl;
#endif

  QString mySearchString(stringSQLSafe(leSearch->text()));
  // Set up the query to retreive the projection information needed to populate the list
  QString mySql;
  if (radSRID->isChecked())
  {
    mySql= "select srs_id from tbl_srs where srid=" + mySearchString;
  }
  else if (radEPSGID->isChecked())
  {
    mySql= "select srs_id from tbl_srs where epsg=" + mySearchString;
  }
  else if (radName->isChecked()) //name search
  {
    //we need to find what the largest srsid matching our query so we know whether to
    //loop backto the beginning
    mySql= "select srs_id from tbl_srs where description like '%" + mySearchString +"%'" +
           " order by srs_id desc limit 1";
    long myLargestSrsId = getLargestSRSIDMatch(mySql);
#ifdef QGISDEBUG
    std::cout << "Largest SRSID" << myLargestSrsId << std::endl;
#endif
    //a name search is ambiguous, so we find the first srsid after the current seelcted srsid
    // each time the find button is pressed. This means we can loop through all matches.
    if (myLargestSrsId <= getCurrentSRSID())
    {
      //roll search around to the beginning
      mySql= "select srs_id from tbl_srs where description like '%" + mySearchString +"%'" +
             " order by srs_id limit 1";
    }
    else
    {
      // search ahead of the current postion
      mySql= "select srs_id from tbl_srs where description like '%" + mySearchString +"%'" +
             " and srs_id > " + QString::number(getCurrentSRSID()) + " order by srs_id limit 1";
    }
  }
  else //qgis srsid
  {
    //no need to try too look up srsid in db as user has already entered it!
    setSelectedSRSID(mySearchString.toLong());
    return;
  }
#ifdef QGISDEBUG
  std::cout << " Search sql: " << mySql.toLocal8Bit().data() << std::endl;
#endif

  //
  // Now perform the actual search
  //

  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  //check the db is available
  myResult = sqlite3_open(mSrsDatabaseFileName.toUtf8().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist. But we checked earlier for its existance
    //     and aborted in that case. This is because we may be runnig from read only
    //     media such as live cd and dont want to force trying to create a db.
    assert(myResult == 0);
  }

  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    myResult = sqlite3_step(myPreparedStatement);
    if (myResult == SQLITE_ROW)
    {  
      QString mySrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 0));
      setSelectedSRSID(mySrsId.toLong());
      // close the sqlite3 statement
      sqlite3_finalize(myPreparedStatement);
      sqlite3_close(myDatabase);
      return;
    }
  }
  //search the users db
  QString myDatabaseFileName = QgsApplication::qgisUserDbFilePath();
  QFileInfo myFileInfo;
  myFileInfo.setFile(myDatabaseFileName);
  if ( !myFileInfo.exists( ) ) //its not critical if this happens
  {
    qDebug(myDatabaseFileName);
    qDebug("User db does not exist");
    return ;
  }
  myResult = sqlite3_open(myDatabaseFileName.toUtf8().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open * user * database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    //no need for assert because user db may not have been created yet
    return;
  }
  
  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    myResult = sqlite3_step(myPreparedStatement);
    if (myResult == SQLITE_ROW)
    {  
      QString mySrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 0));
      setSelectedSRSID(mySrsId.toLong());
      // close the sqlite3 statement
      sqlite3_finalize(myPreparedStatement);
      sqlite3_close(myDatabase);
    }
  }
}

long QgsProjectionSelector::getLargestSRSIDMatch(QString theSql)
{
  long mySrsId =0;
  //
  // Now perform the actual search
  //

  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;

  // first we search the users db as any srsid there will be definition be greater than in sys db

  //check the db is available
  QString myDatabaseFileName = QgsApplication::qgisUserDbFilePath();
  QFileInfo myFileInfo;
  myFileInfo.setFile(myDatabaseFileName);
  if ( myFileInfo.exists( ) ) //only bother trying to open if the file exists
  {
    myResult = sqlite3_open(myDatabaseFileName.toUtf8().data(), &myDatabase);
    if(myResult)
    {
      std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
      // XXX This will likely never happen since on open, sqlite creates the
      //     database if it does not exist. But we checked earlier for its existance
      //     and aborted in that case. This is because we may be runnig from read only
      //     media such as live cd and dont want to force trying to create a db.

    }
    else
    {
      myResult = sqlite3_prepare(myDatabase, theSql.utf8(), theSql.length(), &myPreparedStatement, &myTail);
      // XXX Need to free memory from the error msg if one is set
      if(myResult == SQLITE_OK)
      {
        myResult = sqlite3_step(myPreparedStatement);
        if (myResult == SQLITE_ROW)
        {  
          QString mySrsIdString = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 0));
          mySrsId = mySrsIdString.toLong();
          // close the sqlite3 statement
          sqlite3_finalize(myPreparedStatement);
          sqlite3_close(myDatabase);
          return mySrsId;
        }
      }
    }
  }
  
  //only bother looking in srs.db if it wasnt found above

  myResult = sqlite3_open(mSrsDatabaseFileName.toUtf8().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open * user * database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    //no need for assert because user db may not have been created yet
    return 0;
  }

  myResult = sqlite3_prepare(myDatabase, theSql.utf8(), theSql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    myResult = sqlite3_step(myPreparedStatement);
    if (myResult == SQLITE_ROW)
    {  
      QString mySrsIdString = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 0));
      mySrsId =  mySrsIdString.toLong();
      // close the sqlite3 statement
      sqlite3_finalize(myPreparedStatement);
      sqlite3_close(myDatabase);
    }
  }
  return mySrsId;
}
/*!
* \brief Make the string safe for use in SQL statements.
*  This involves escaping single quotes, double quotes, backslashes,
*  and optionally, percentage symbols.  Percentage symbols are used
*  as wildcards sometimes and so when using the string as part of the
*  LIKE phrase of a select statement, should be escaped.
* \arg const QString in The input string to make safe.
* \return The string made safe for SQL statements.
*/
const QString QgsProjectionSelector::stringSQLSafe(const QString theSQL)
{
	QString myRetval = theSQL;
	myRetval.replace("\\","\\\\");
	myRetval.replace('\"',"\\\"");
	myRetval.replace("\'","\\'");
	myRetval.replace("%","\\%");
	return myRetval;
 }
