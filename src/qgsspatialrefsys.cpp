#include "qgsspatialrefsys.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <sqlite3.h>
#include <qsettings.h>
#include <qapplication.h>
#include <qregexp.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <projects.h>
#include <qdom.h>

#include <qgslayerprojectionselector.h>
#include <qgsproject.h>
#include <qgis.h> //const vals declared here


//gdal and ogr includes (needed for == operator)
#include <ogr_api.h>
#include <ogr_spatialref.h>
#include <cpl_error.h>


//--------------------------

QgsSpatialRefSys::QgsSpatialRefSys() : mMapUnits(QGis::UNKNOWN) {}

QgsSpatialRefSys::QgsSpatialRefSys(QString theWkt) : mMapUnits(QGis::UNKNOWN)
{
  createFromWkt(theWkt);
}

QgsSpatialRefSys::QgsSpatialRefSys(long theSrsId,
                                   QString theDescription,
                                   QString theProjectionAcronym,
                                   QString theEllipsoidAcronym,
                                   QString theProj4String,
                                   long theSRID,
                                   long theEpsg, 
                                   bool theGeoFlag)
    : mMapUnits(QGis::UNKNOWN)
{}

QgsSpatialRefSys::QgsSpatialRefSys(const long theId, SRS_TYPE theType) : mMapUnits(QGis::UNKNOWN)
{
  createFromId(theId, theType);
}

void QgsSpatialRefSys::createFromId(const long theId, SRS_TYPE theType)
{
  switch (theType)
  {
  case QGIS_SRSID:
    createFromSrsId(theId);
    break;
  case POSTGIS_SRID:
    createFromSrid(theId);
    break;
  case EPSG:
    createFromEpsg(theId);
    break;
  default:
    //THIS IS BAD...THIS PART OF CODE SHOULD NEVER BE REACHED...
    std::cout << "Unexpected case reached in " << __FILE__ << " : " << __LINE__ << std::endl;
  };

}

// Assignment operator
QgsSpatialRefSys& QgsSpatialRefSys::operator=(const QgsSpatialRefSys& srs)
{
  if (&srs != this)
  {
    mSrsId = srs.mSrsId;
    mDescription = srs.mDescription;
    mProjectionAcronym = srs.mProjectionAcronym;
    mEllipsoidAcronym = srs.mEllipsoidAcronym;
    mProj4String = srs.mProj4String;
    mGeoFlag = srs.mGeoFlag;
    mMapUnits = srs.mMapUnits;
    mSRID = srs.mSRID;
    mEpsg = srs.mEpsg;
    mIsValidFlag = srs.mIsValidFlag;
  }
  return *this;
}

// Misc helper functions -----------------------


void QgsSpatialRefSys::validate()
{
#ifdef QGISDEBUG
  std::cout << " QgsSpatialRefSys::validate" << std::endl;
#endif
  //dont bother trying to do an initial test with gdal if
  //the proj4String is not even populated
  if (QString::null!=mProj4String && !mProj4String.isEmpty())
  {
    //first of all use gdal to test if this is an ok srs already
    //if not we will prompt the user for and srs
    //then retest using gdal
    //if the retest fails we will then set  this srs to the GEOCS/WGS84 default


    /* Here are the possible OGR error codes :
       typedef int OGRErr;

       #define OGRERR_NONE                0
       #define OGRERR_NOT_ENOUGH_DATA     1    --> not enough data to deserialize
       #define OGRERR_NOT_ENOUGH_MEMORY   2
       #define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3
       #define OGRERR_UNSUPPORTED_OPERATION 4
       #define OGRERR_CORRUPT_DATA        5
       #define OGRERR_FAILURE             6
       #define OGRERR_UNSUPPORTED_SRS     7 */

    //get the wkt into ogr
    //this is really ugly but we need to get a QString to a char**
    const char *mySourceCharArrayPointer = mProj4String.latin1();
    //create the sr and populate it from a wkt proj definition
    OGRSpatialReference myOgrSpatialRef;
    OGRErr myInputResult = myOgrSpatialRef.importFromProj4( mySourceCharArrayPointer );

    if (myInputResult==OGRERR_NONE)
    {
      //srs is valid so nothing more to do...
      createFromProj4(mProj4String);
      return;
    }
  }
  QSettings mySettings;
  QString myDefaultProjectionOption =
    mySettings.readEntry("/Projections/defaultBehaviour");
  if (myDefaultProjectionOption=="prompt")
  {
    //@note this class is not a descendent of QWidget so we cant pass
    //it in the ctor of the layer projection selector

    QgsLayerProjectionSelector * mySelector = new QgsLayerProjectionSelector();
    long myDefaultSRS =
      QgsProject::instance()->readNumEntry("SpatialRefSys","/ProjectSRSID",GEOSRS_ID);
    mySelector->setSelectedSRSID(myDefaultSRS);
    if(mySelector->exec())
    {
      createFromSrsId(mySelector->getCurrentSRSID());
    }
    else
    {
      QApplication::restoreOverrideCursor();
    }
    delete mySelector;
  }
  else if (myDefaultProjectionOption=="useProject")
  {
    // XXX TODO: Change project to store selected CS as 'projectSRS' not 'selectedWKT'
    mProj4String = QgsProject::instance()->readEntry("SpatialRefSys","//ProjectSRSProj4String",GEOPROJ4);
  }
  else ///Projections/defaultBehaviour==useDefault
  {
    // XXX TODO: Change global settings to store default CS as 'defaultSRS' not 'defaultProjectionWKT'
    mProj4String = mySettings.readEntry("/Projections/defaultSRS",GEOPROJ4);
  }

  //
  // This is the second check after the user assigned SRS has been retrieved
  // If it still does not work, we will simply use the QgsSpatialRefSys const GEOPROJ4 for the job
  //

  //this is really ugly but we need to get a QString to a char**
  const char *mySourceCharArrayPointer = mProj4String.latin1();
  //create the sr and populate it from a wkt proj definition
  OGRSpatialReference myOgrSpatialRef;
  OGRErr myInputResult = myOgrSpatialRef.importFromProj4( mySourceCharArrayPointer );

  if (! myInputResult==OGRERR_NONE)
  {
    //default to proj 4..if all else fails we will use that for this srs
    mProj4String = GEOPROJ4;
  }
  createFromProj4(mProj4String);
  return;
}

bool QgsSpatialRefSys::createFromSrid(long theSrid)
{
#ifdef QGISDEBUG
  std::cout << " QgsSpatialRefSys::createFromSrid" << std::endl;
#endif


  // Get the package data path and set the full path name to the sqlite3 spatial reference
  // database.
#if defined(Q_OS_MACX) || defined(WIN32)
  QString PKGDATAPATH = qApp->applicationDirPath() + "/share/qgis";
#endif
  QString myDatabaseFileName = PKGDATAPATH;
  myDatabaseFileName += "/resources/srs.db";


  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  //check the db is available
  myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(myResult == 0);
  }

  /*
    srs_id INTEGER PRIMARY KEY,
    description text NOT NULL,
    projection_acronym text NOT NULL,
    ellipsoid_acronym NOT NULL,
    parameters text NOT NULL,
    srid integer NOT NULL,
    epsg integer NOT NULL,
    is_geo integer NOT NULL);
  */

  QString mySql = "select srs_id,description,projection_acronym,ellipsoid_acronym,parameters,srid,epsg,is_geo from tbl_srs where srid='" + QString::number(theSrid) + "'";
  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK && sqlite3_step(myPreparedStatement) == SQLITE_ROW)
  {
    mSrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,0)).toLong();
    mDescription = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,1));
    mProjectionAcronym = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,2));
    mEllipsoidAcronym = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,3));
    mProj4String = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,4));
    mSRID = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,5)).toLong();
    mEpsg = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,6)).toLong();
    int geo = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,7)).toInt();
    mGeoFlag = (geo == 0 ? false : true);
    setMapUnits();
    mIsValidFlag = true;

  }
  else
  {
#ifdef QGISDEBUG
    std::cout << " QgsSpatialRefSys::createFromSrid failed :  " << mySql.toLocal8Bit().data() << std::endl;
#endif
    mIsValidFlag = false;
  }
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  return mIsValidFlag;
}

bool QgsSpatialRefSys::createFromWkt(QString theWkt)
{
  if (theWkt.isEmpty())
  {
    std::cout << "QgsSpatialRefSys::createFromWkt -- theWkt is uninitialised, operation failed" << std::endl;
    mIsValidFlag = false;
    return false;
  }
#ifdef QGISDEBUG
  std::cout << "QgsSpatialRefSys::createFromWkt(QString theWkt) using: \n" << theWkt.toLocal8Bit().data() << std::endl;
#endif
  //this is really ugly but we need to get a QString to a char**
  const char *myCharArrayPointer = theWkt.latin1(); //Why doesn't it work with toLocal8Bit().data()?
  char *pWkt = (char *)myCharArrayPointer;
  /* Here are the possible OGR error codes :
     typedef int OGRErr;
     #define OGRERR_NONE                0
     #define OGRERR_NOT_ENOUGH_DATA     1    --> not enough data to deserialize 
     #define OGRERR_NOT_ENOUGH_MEMORY   2
     #define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3
     #define OGRERR_UNSUPPORTED_OPERATION 4
     #define OGRERR_CORRUPT_DATA        5
     #define OGRERR_FAILURE             6
     #define OGRERR_UNSUPPORTED_SRS     7 
  */

  OGRSpatialReference myOgrSpatialRef;

  OGRErr myInputResult = myOgrSpatialRef.importFromWkt( &pWkt );
  if (myInputResult != OGRERR_NONE)
  {
    std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<< std::endl;
    std::cout << "QgsSpatialRefSys::createFromWkt(QString theWkt) " << __FILE__ << __LINE__ << std::endl;
    std::cout << "This SRS could *** NOT *** be set from the supplied WKT " << std::endl;
    std::cout << "INPUT: " << std::endl << theWkt.toLocal8Bit().data() << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
    mIsValidFlag = false;
    return false;
  }


  // always morph from esri as it doesn't hurt anything
  myOgrSpatialRef.morphFromESRI();
  // create the proj4 structs needed for transforming
  char *proj4src;
  myOgrSpatialRef.exportToProj4(&proj4src);

  //now that we have the proj4string, delegate to createFromProj4String so
  // that we can try to fill in the remaining class members...
  //create from Proj wil set the isValidFalg
  createFromProj4(QString(proj4src));
  return mIsValidFlag;
  //setMapunits will be called by createfromproj above
}

bool QgsSpatialRefSys::createFromEpsg(long theEpsg)
{
#ifdef QGISDEBUG
  std::cout << " QgsSpatialRefSys::createFromEpsg" << std::endl;
#endif
  // Get the package data path and set the full path name to the sqlite3 spatial reference
  // database.
#if defined(Q_OS_MACX) || defined(WIN32)
  QString PKGDATAPATH = qApp->applicationDirPath() + "/share/qgis";
#endif
  QString myDatabaseFileName = PKGDATAPATH;
  myDatabaseFileName += "/resources/srs.db";


  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  //check the db is available
  myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(myResult == 0);
  }

  /*
    srs_id INTEGER PRIMARY KEY,
    description text NOT NULL,
    projection_acronym text NOT NULL,
    ellipsoid_acronym NOT NULL,
    parameters text NOT NULL,
    srid integer NOT NULL,
    epsg integer NOT NULL,
    is_geo integer NOT NULL);
  */

  QString mySql = "select srs_id,description,projection_acronym,ellipsoid_acronym,parameters,srid,epsg,is_geo from tbl_srs where epsg='" + QString::number(theEpsg) + "'";
  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK && sqlite3_step(myPreparedStatement) == SQLITE_ROW)
  {
    mSrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,0)).toLong();
    mDescription = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,1));
    mProjectionAcronym = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,2));
    mEllipsoidAcronym = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,3));
    mProj4String = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,4));
    mSRID = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,5)).toLong();
    mEpsg = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,6)).toLong();
    int geo = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,7)).toInt();
    mGeoFlag = (geo == 0 ? false : true);
    setMapUnits();
    mIsValidFlag = true;

  }
  else
  {
#ifdef QGISDEBUG
    std::cout << " QgsSpatialRefSys::createFromEpsg failed :  " << mySql.toLocal8Bit().data() << std::endl;
#endif
    mIsValidFlag = false;
  }
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  return mIsValidFlag;
}


bool QgsSpatialRefSys::createFromSrsId (long theSrsId)
{
#ifdef QGISDEBUG
  std::cout << " QgsSpatialRefSys::createFromSrsId" << std::endl;
#endif
  QString myDatabaseFileName;
  //
  // Determine if this is a user projection or a system on
  // user projection defs all have srs_id >= 100000
  //
  if (theSrsId>= USER_PROJECTION_START_ID)
  {
    myDatabaseFileName = QDir::homeDirPath () + "/.qgis/qgis.db";
    QFileInfo myFileInfo;
    myFileInfo.setFile(myDatabaseFileName);
    if ( !myFileInfo.exists( ) )
    {
      mIsValidFlag = false;
      std::cout << " QgsSpatialRefSys::createFromSrid failed :  users qgis.db not found" << std::endl;
      return mIsValidFlag;
    }
  }
  else //must be  a system projection then
  {
    // Get the package data path and set the full path name to the sqlite3 spatial reference
    // database.
#if defined(Q_OS_MACX) || defined(WIN32)
    QString PKGDATAPATH = qApp->applicationDirPath() + "/share/qgis";
#endif
    myDatabaseFileName = PKGDATAPATH;
    myDatabaseFileName += "/resources/srs.db";
  }


  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  //check the db is available
  myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(myResult == 0);
  }

  /*
    srs_id INTEGER PRIMARY KEY,
    description text NOT NULL,
    projection_acronym text NOT NULL,
    ellipsoid_acronym NOT NULL,
    parameters text NOT NULL,
    srid integer NOT NULL,
    epsg integer NOT NULL,
    is_geo integer NOT NULL);
  */

  QString mySql = "select srs_id,description,projection_acronym,ellipsoid_acronym,parameters,srid,epsg,is_geo from tbl_srs where srs_id='" + QString::number(theSrsId) + "'";
  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK && sqlite3_step(myPreparedStatement) == SQLITE_ROW)
  {
    mSrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,0)).toLong();
    mDescription = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,1));
    mProjectionAcronym = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,2));
    mEllipsoidAcronym = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,3));
    mProj4String = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,4));
    mSRID = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,5)).toLong();
    mEpsg = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,6)).toLong();
    int geo = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,7)).toInt();
    mGeoFlag = (geo == 0 ? false : true);
    setMapUnits();
    mIsValidFlag = true;
  }
  else
  {
#ifdef QGISDEBUG
    std::cout << " QgsSpatialRefSys::createFromSrsId failed :  " << mySql.toLocal8Bit().data() << std::endl;
#endif
    mIsValidFlag = false;
  }
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  return mIsValidFlag;
}





bool QgsSpatialRefSys::isValid() const
{
  if (mProj4String.isEmpty())
    return false;

  //this is really ugly but we need to get a QString to a char**
  const char *mySourceCharArrayPointer = mProj4String.latin1();
  //create the sr and populate it from a wkt proj definition
  OGRSpatialReference myOgrSpatialRef;
  OGRErr myResult = myOgrSpatialRef.importFromProj4( mySourceCharArrayPointer );
  if (myResult==OGRERR_NONE)
  {
    //srs is valid so nothing more to do...
    return true;
  }
  else
  {
    return false;
  }
}

bool QgsSpatialRefSys::createFromProj4 (const QString theProj4String)
{

  //
  // Example:
  // +proj=tmerc +lat_0=0 +lon_0=-62 +k=0.999500 +x_0=400000 +y_0=0
  // +ellps=clrk80 +towgs84=-255,-15,71,0,0,0,0 +units=m +no_defs
  //
  mIsValidFlag=false;

  QRegExp myProjRegExp( "\\+proj=\\S+" );
  int myStart= 0;
  int myLength=0;
  myStart = myProjRegExp.search(theProj4String, myStart);
  if (myStart==-1)
  {
    std::cout << "QgsSpatialRefSys::createFromProj4 error proj string supplied has no +proj argument" << std::endl;

    return mIsValidFlag;
  }
  else
  {
    myLength = myProjRegExp.matchedLength();
  }

  mProjectionAcronym = theProj4String.mid(myStart+PROJ_PREFIX_LEN,myLength-PROJ_PREFIX_LEN);

  QRegExp myEllipseRegExp( "\\+ellps=\\S+" );
  myStart= 0;
  myLength=0;
  myStart = myEllipseRegExp.search(theProj4String, myStart);
  if (myStart==-1)
  {
    std::cout << "QgsSpatialRefSys::createFromProj4 error proj string supplied has no +ellps argument" << std::endl;

    return mIsValidFlag;
  }
  else
  {
    myLength = myEllipseRegExp.matchedLength();
  }
  mEllipsoidAcronym = theProj4String.mid(myStart+ELLPS_PREFIX_LEN,myLength-ELLPS_PREFIX_LEN);
  //mproj4string must be set here for the rest of this method to behave in a meaningful way...
  mProj4String = theProj4String;


  /*
  * We try to match the proj string to and srsid using the following logic: 
  *
  * - perform a whole text search on srs name (if not null). The srs name will 
  *   have been set if this method has been delegated to from createFromWkt.
  * Normally we wouldnt expect this to work, but its worth trying first
  * as its quicker than methods below..
  */
  long mySrsId = 0;
  QgsSpatialRefSys::RecordMap myRecord;
  if (!mDescription.stripWhiteSpace ().isEmpty())
  {
     myRecord = getRecord("select * from tbl_srs where where description='" + mDescription.stripWhiteSpace () + "'");
  }
  if (!myRecord.empty())
  {
    mySrsId=myRecord["srs_id"].toLong();
    std::cout << "QgsSpatialRefSys::createFromProj4 Projection Description match search for srsid returned srsid: " << mySrsId << std::endl;
    if (mySrsId > 0)
    {
      createFromSrsId(mySrsId);
    }
  }
  else
  {
    /*
    * - if the above does not match perform a whole text search on proj4 string (if not null)
    */
    std::cout << "QgsSpatialRefSys::createFromProj4 wholetext match on name failed, trying proj4string match" << std::endl;
    myRecord = getRecord("select * from tbl_srs where where parameters='" + mProj4String.stripWhiteSpace () + "'");
    if (!myRecord.empty())
    {
      mySrsId=myRecord["srs_id"].toLong();
      std::cout << "QgsSpatialRefSys::createFromProj4 proj4string match search for srsid returned srsid: " << mySrsId << std::endl;
      if (mySrsId > 0)
      {
        createFromSrsId(mySrsId);
      }
    }

    else
    {
      std::cout << "QgsSpatialRefSys::createFromProj4 globbing search for srsid from this proj string" << std::endl;
      mySrsId = findMatchingProj();
      std::cout << "QgsSpatialRefSys::createFromProj4 globbing search for srsid returned srsid: " << mySrsId << std::endl;
      if (mySrsId > 0)
      {
        createFromSrsId(mySrsId);
      }
    }
  }

  /* If its still empty after all the above steps then all we can do is keep the proj string
  * with what was passed in and hope for the best...If its not empty we can fill other member details in 
    from the record*/
  return mIsValidFlag;
}

QgsSpatialRefSys::RecordMap QgsSpatialRefSys::getRecord(QString theSql)
{

  QString myDatabaseFileName;
  QgsSpatialRefSys::RecordMap myMap;
  QString myFieldName;
  QString myFieldValue;
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;

#ifdef QGISDEBUG
  std::cout << " QgsSpatialRefSys::getRecord...running query:\n"<< theSql.toLocal8Bit().data() << "\n" << std::endl;
  std::cout << " QgsSpatialRefSys::getRecord...trying system srs.db" << std::endl;
#endif
  // Get the package data path and set the full path name to the sqlite3 spatial reference
  // database.
#if defined(Q_OS_MACX) || defined(WIN32)
  QString PKGDATAPATH = qApp->applicationDirPath() + "/share/qgis";
#endif
  myDatabaseFileName = PKGDATAPATH;
  myDatabaseFileName += "/resources/srs.db";


  //check the db is available
  myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(myResult == 0);
  }


  myResult = sqlite3_prepare(myDatabase, theSql.utf8(), theSql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK && sqlite3_step(myPreparedStatement) == SQLITE_ROW)
  {
    int myColumnCount = sqlite3_column_count(myPreparedStatement);
    //loop through each column in the record adding its field name and vvalue to the map
    for (int myColNo=0;myColNo < myColumnCount;myColNo++)
    {
      myFieldName = QString::fromUtf8((char *)sqlite3_column_name(myPreparedStatement,myColNo));
      myFieldValue = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,myColNo));
      myMap[myFieldName]=myFieldValue;
    }
  }
  else
  {
#ifdef QGISDEBUG
    std::cout << " QgsSpatialRefSys::getRecord...trying system users.db" << std::endl;
#endif
    sqlite3_finalize(myPreparedStatement);
    sqlite3_close(myDatabase);

    myDatabaseFileName = QDir::homeDirPath () + "/.qgis/qgis.db";
    QFileInfo myFileInfo;
    myFileInfo.setFile(myDatabaseFileName);
    if ( !myFileInfo.exists( ) )
    {
      std::cout << " QgsSpatialRefSys::getRecord failed :  users qgis.db not found" << std::endl;
      return myMap;
    }

    //check the db is available
    myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
    if(myResult)
    {
      std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
      // XXX This will likely never happen since on open, sqlite creates the
      //     database if it does not exist.
      assert(myResult == 0);
    }


    myResult = sqlite3_prepare(myDatabase, theSql.utf8(), theSql.length(), &myPreparedStatement, &myTail);
    // XXX Need to free memory from the error msg if one is set
    if(myResult == SQLITE_OK && sqlite3_step(myPreparedStatement) == SQLITE_ROW)
    {
      int myColumnCount = sqlite3_column_count(myPreparedStatement);
      //loop through each column in the record adding its field name and vvalue to the map
      for (int myColNo=0;myColNo < myColumnCount;myColNo++)
      {
        myFieldName = QString::fromUtf8((char *)sqlite3_column_name(myPreparedStatement,myColNo));
        myFieldValue = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,myColNo));
        myMap[myFieldName]=myFieldValue;
      }
    }
    else
    {
#ifdef QGISDEBUG
      std::cout << " QgsSpatialRefSys::getRecord failed :  " << theSql.toLocal8Bit().data() << std::endl;
#endif

    }
  }
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);

#ifdef QGISDEBUG
         std::cout << " QgsSpatialRefSys::getRecord retrieved:  " << theSql.toLocal8Bit().data() << std::endl;
        RecordMap::Iterator it;
        for ( it = myMap.begin(); it != myMap.end(); ++it )
       {
            std::cout << it.key().toLocal8Bit().data() << " => " <<  it.data().toLocal8Bit().data() << std::endl;
        }
#endif

  return myMap;



}

// Accessors -----------------------------------
/*! Get the SrsId
 *  @return  long theSrsId The internal sqlite3 srs.db primary key for this srs 
 */
long QgsSpatialRefSys::srsid() const
{
  return mSrsId;
}
/*! Get the Postgis SRID - if possible
 *  @return  long theSRID The internal postgis SRID for this SRS
 */
long QgsSpatialRefSys::srid() const
{

  return mSRID;

}
/*! Get the Description
 * @return  QString the Description A textual description of the srs.
 */
QString QgsSpatialRefSys::description () const
{
  if (mDescription.isNull())
  {
    return "";
  }
  else
  {
    return mDescription;
  }
}
/*! Get the Projection Acronym
 * @return  QString theProjectionAcronym The official proj4 acronym for the projection family
 */
QString QgsSpatialRefSys::projectionAcronym() const
{
  if (mProjectionAcronym.isNull())
  {
    return "";
  }
  else
  {
    return mProjectionAcronym;
  }
}
/*! Get the Ellipsoid Acronym
 * @return  QString theEllipsoidAcronym The official proj4 acronym for the ellipoid
 */
QString QgsSpatialRefSys::ellipsoidAcronym () const
{
  if (mEllipsoidAcronym.isNull())
  {
    return "";
  }
  else
  {
    return mEllipsoidAcronym;
  }
}
/* Get the Proj Proj4String.
 * @return  QString theProj4String Proj4 format specifies that define this srs.
 */
QString QgsSpatialRefSys::proj4String() const
{
  if (mProj4String.isNull())
  {
    return "";
  }
  else
  {
    return mProj4String;
  }
}
/*! Get this Geographic? flag
 * @return  bool theGeoFlag Whether this is a geographic or projected coordinate system
 */
bool QgsSpatialRefSys::geographicFlag () const
{
  return mGeoFlag;
}
/*! Get the units that the projection is in
 * @return QGis::units
 */
QGis::units QgsSpatialRefSys::mapUnits() const
{
  return mMapUnits;
}

/*! Set the postgis srid for this srs
 * @return  long theSRID the Postgis spatial_ref_sys identifier for this srs (defaults to 0)
 */
long QgsSpatialRefSys::postgisSrid () const
{
  return mSRID ;
}
/*! Set the EPSG identifier for this srs
 * @return  long theEpsg the ESPG identifier for this srs (defaults to 0)
 */
long QgsSpatialRefSys::epsg () const
{
  return mEpsg;
}

// Mutators -----------------------------------


void QgsSpatialRefSys::setSrsId(long theSrsId)
{
  mSrsId = theSrsId;
}
void QgsSpatialRefSys::setSrid(long theSrid)
{
  mSRID=theSrid;
}
void QgsSpatialRefSys::setDescription (QString theDescription)
{
  mDescription = theDescription;
}
void QgsSpatialRefSys::setProj4String (QString theProj4String)
{
  mProj4String = theProj4String;
}
void QgsSpatialRefSys::setGeographicFlag (bool theGeoFlag)
{
  mGeoFlag=theGeoFlag;
}
void QgsSpatialRefSys::setEpsg (long theEpsg)
{
  mEpsg=theEpsg;
}
void  QgsSpatialRefSys::setProjectionAcronym(QString theProjectionAcronym)
{
  mProjectionAcronym=theProjectionAcronym;
}
void  QgsSpatialRefSys::setEllipsoidAcronym(QString theEllipsoidAcronym)
{
  mEllipsoidAcronym=theEllipsoidAcronym;
}
/*! Work out the projection units and set the appropriate local variable
 *
 */
void QgsSpatialRefSys::setMapUnits()
{
  if (mProj4String.isEmpty())
  {
    qWarning(QObject::tr("No proj4 projection string. Unable to set map units.").toLocal8Bit().data());
    mMapUnits = QGis::UNKNOWN;
    return;
  }

  char *unitName;
  OGRSpatialReference myOgrSpatialRef;
  myOgrSpatialRef.importFromProj4(mProj4String.latin1());

  // Of interest to us is that this call adds in a unit parameter if
  // one doesn't already exist.
  myOgrSpatialRef.Fixup();

  if (myOgrSpatialRef.IsProjected())
  {
    double toMeter = myOgrSpatialRef.GetLinearUnits(&unitName);
    QString unit(unitName);

    // If the units parameter was created during the Fixup() call
    // above, the name of the units is likely to be 'unknown'. Try to
    // do better than that ... (but perhaps ogr should be enhanced to
    // do this instead?).

    static const double feetToMeter = 0.3048;
    static const double smallNum = 1e-3;

    if (std::abs(toMeter - feetToMeter) < smallNum)
      unit = "Foot";

#ifdef QGISDEBUG
    std::cerr << "Projection has linear units of " << unit.toLocal8Bit().data() << '\n';
#endif

    if (unit == "Meter")
      mMapUnits = QGis::METERS;
    else if (unit == "Foot")
      mMapUnits = QGis::FEET;
    else
    {
      qWarning((QObject::tr("Unsupported map units of ") + unit).toLocal8Bit().data());
      mMapUnits = QGis::UNKNOWN;
    }
  }
  else
  {
    myOgrSpatialRef.GetAngularUnits(&unitName);
    QString unit(unitName);
    if (unit == "degree")
      mMapUnits = QGis::DEGREES;
    else
    {
      qWarning((QObject::tr("Unsupported map units of ") + unit).toLocal8Bit().data());
      mMapUnits = QGis::UNKNOWN;
    }
#ifdef QGISDEBUG
    std::cerr << "Projection has angular units of " << unit.toLocal8Bit().data() << '\n';
#endif

  }
}


/*
*    check if srs is a geocs or a proj cs (using ogr isGeographic)
*   then sequentially walk through the database (first users qgis.db srs tbl then
*   system srs.db tbl), converting each entry into an ogr srs and using isSame
*   or isSameGeocs (essentially calling the == overloaded operator). We'll try to 
*   be smart about this and first parse out the proj and ellpse strings and only 
*   check for a match in entities that have the same ellps and proj entries so 
*   that it doesnt munch yer cpu so much.
*/
long QgsSpatialRefSys::findMatchingProj()
{
#ifdef QGISDEBUG
  std::cout << "QgsSpatialRefSys::findMatchingProj..." << std::endl;
#endif
  if (mEllipsoidAcronym.isNull() ||  mProjectionAcronym.isNull() || mProj4String.isNull())
  {
    std::cout << "QgsSpatialRefSys::findMatchingProj will only work if prj acr ellipsoid acr and proj4string are set!..." << std::endl;
    return 0;
  }

  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;

  // Set up the query to retreive the projection information needed to populate the list
  QString mySql = QString ("select srs_id,parameters from tbl_srs where projection_acronym='" +
                           mProjectionAcronym + "' and ellipsoid_acronym='" + mEllipsoidAcronym + "'");
#ifdef QGISDEBUG
 // std::cout << "QgsSpatialRefSys::findMatchingProj list sql\n" << mySql.toLocal8Bit().data() << std::endl;
 // std::cout << " QgsSpatialRefSys::findMatchingProj...trying system srs.db" << std::endl;
#endif
  // Get the package data path and set the full path name to the sqlite3 spatial reference
  // database.
#if defined(Q_OS_MACX) || defined(WIN32)
  QString PKGDATAPATH = qApp->applicationDirPath() + "/share/qgis";
#endif
  QString myDatabaseFileName = PKGDATAPATH;
  myDatabaseFileName += "/resources/srs.db";


  //check the db is available
  myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "QgsSpatialRefSys::findMatchingProj Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(myResult == 0);
  }

  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {

    while(sqlite3_step(myPreparedStatement) == SQLITE_ROW)
    {
      QString mySrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,0));
      QString myProj4String = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 1));
      if (this->equals(myProj4String))
      {
        std::cout << "QgsSpatialRefSys::findMatchingProj -------> MATCH FOUND in srs.db srsid: " << mySrsId.toLocal8Bit().data() << std::endl;
        // close the sqlite3 statement
        sqlite3_finalize(myPreparedStatement);
        sqlite3_close(myDatabase);
        return mySrsId.toLong();
      }
      else
      {
        //std::cout << " Not matched : " << myProj4String << std::endl;
      }
    }
  }
  //std::cout << "QgsSpatialRefSys::findMatchingProj -------> no match found in srs.db, trying user db now!" << std::endl;
  // close the sqlite3 statement
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  //
  // Try the users db now
  //

  myDatabaseFileName = QDir::homeDirPath () + "/.qgis/qgis.db";
  //check the db is available
  myResult = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &myDatabase);
  if(myResult)
  {
    std::cout <<  "QgsSpatialRefSys::findMatchingProj Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl;
    std::cout <<  "This is a non critical error" << std::endl;
    return 0;
  }

  myResult = sqlite3_prepare(myDatabase, mySql.utf8(), mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {

    while(sqlite3_step(myPreparedStatement) == SQLITE_ROW)
    {
      QString mySrsId = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement,0));
      QString myProj4String = QString::fromUtf8((char *)sqlite3_column_text(myPreparedStatement, 1));
      if (this->equals(myProj4String))
      {
        std::cout << "QgsSpatialRefSys::findMatchingProj -------> MATCH FOUND in user qgis.db srsid: " << mySrsId.toLocal8Bit().data() << std::endl;
        // close the sqlite3 statement
        sqlite3_finalize(myPreparedStatement);
        sqlite3_close(myDatabase);
        return mySrsId.toLong();
      }
      else
      {
        //std::cout << " Not matched : " << myProj4String << std::endl;
      }
    }
  }
  std::cout << "QgsSpatialRefSys::findMatchingProj -------> no match found in user db" << std::endl;

  // close the sqlite3 statement
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  return 0;

}

bool QgsSpatialRefSys::operator==(const QgsSpatialRefSys &theSrs)
{
  //qWarning("QgsSpatialRefSys::operator==(const QgsSpatialRefSys &theSrs) called ");
  //simply delegate to the overloaded == operator  below...
  return this->equals(theSrs.mProj4String);

}

bool QgsSpatialRefSys::equals(QString theProj4CharArray)
{
  //qWarning("QgsSpatialRefSys::operator==(const char *theProj4CharArray) called ");
  bool myMatchFlag = false; //guilty until proven innocent

  /* Here are the possible OGR error codes :
     typedef int OGRErr;

     #define OGRERR_NONE                0
     #define OGRERR_NOT_ENOUGH_DATA     1    --> not enough data to deserialize
     #define OGRERR_NOT_ENOUGH_MEMORY   2
     #define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3
     #define OGRERR_UNSUPPORTED_OPERATION 4
     #define OGRERR_CORRUPT_DATA        5
     #define OGRERR_FAILURE             6
     #define OGRERR_UNSUPPORTED_SRS     7 */

  //get the wkt into ogr
  //this is really ugly but we need to get a QString to a char**
  const char *myCharArrayPointer1 = mProj4String.latin1();

  //note that the proj strings above do not neccessarily need to be exactly the
  //same for the projections they define to be equivalent, which is why I dont just
  //compare the proj parameter strings and return the result


  //create the sr and populate it from a wkt proj definition
  OGRSpatialReference myOgrSpatialRef1;
  OGRSpatialReference myOgrSpatialRef2;
  OGRErr myInputResult1 = myOgrSpatialRef1.importFromProj4(  myCharArrayPointer1 );
  OGRErr myInputResult2 = myOgrSpatialRef2.importFromProj4(  theProj4CharArray.latin1() );

  if (myOgrSpatialRef1.IsGeographic() && myOgrSpatialRef2.IsGeographic())
  {
//    qWarning("QgsSpatialRefSys::operator== srs1 and srs2 are geographic ");
    myMatchFlag = myOgrSpatialRef1.IsSameGeogCS(&myOgrSpatialRef2);
  }
  else if (myOgrSpatialRef1.IsProjected() && myOgrSpatialRef2.IsProjected())
  {
//    qWarning("QgsSpatialRefSys::operator== srs1 and srs2 are projected ");
    myMatchFlag = myOgrSpatialRef1.IsSame(&myOgrSpatialRef2);
  } else {
//    qWarning("QgsSpatialRefSys::operator== srs1 and srs2 are different types ");
    myMatchFlag = false;
  }

  //find out the units:
  /* Not needed anymore here - keeping here as a note because I am gonna use it elsewhere
  const char *myUnitsArrayPointer1;
  const char *myUnitsArrayPointer2;
  OGRErr myUnitsValid1 = myOgrSpatialRef1.GetLinearUnits(&myUnitsArrayPointer1 );
  OGRErr myUnitsValid2 = myOgrSpatialRef2.GetLinearUnits(&myUnitsArrayPointer2 );
  QString myUnitsString1(myUnitsArrayPointer1);
  QString myUnitsString2(myUnitsArrayPointer2);
  */




  //placeholder to be replaced with ogr tests
  if (myMatchFlag)
  {
//    qWarning("QgsSpatialRefSys::operator== result: srs's are equal ");
  }
  else
  {
//    qWarning("QgsSpatialRefSys::operator== result: srs's are not equal ");
  }
  return myMatchFlag;
}

OGRSpatialReference QgsSpatialRefSys::toOgrSrs()
{
  OGRSpatialReference myOgrSpatialRef1;
  OGRErr myInputResult1 = myOgrSpatialRef1.importFromProj4(mProj4String.latin1());
  return myOgrSpatialRef1;
}

bool QgsSpatialRefSys::readXML( QDomNode & theNode )
{
#ifdef QGISDEBUG
  std::cout << "Reading Spatial Ref Sys from xml ------------------------!" << std::endl;
#endif
     QDomNode myNode = theNode.namedItem("proj4");
     QDomElement myElement = myNode.toElement();
     setProj4String(myElement.text());

     myNode = theNode.namedItem("srsid");
     myElement = myNode.toElement();
     setSrsId(myElement.text().toLong());

     myNode = theNode.namedItem("srid");
     myElement = myNode.toElement();
     setSrid(myElement.text().toLong());

     myNode = theNode.namedItem("epsg");
     myElement = myNode.toElement();
     setEpsg(myElement.text().toLong());

     myNode = theNode.namedItem("description");
     myElement = myNode.toElement();
     setDescription(myElement.text());

     myNode = theNode.namedItem("projectionacronym");
     myElement = myNode.toElement();
     setProjectionAcronym(myElement.text());

     myNode = theNode.namedItem("ellipsoidacronym");
     myElement = myNode.toElement();
     setEllipsoidAcronym(myElement.text());
 
     myNode = theNode.namedItem("geographicflag");
     myElement = myNode.toElement();
     if (myElement.text().compare("true"))
     {
       setGeographicFlag(true);
     }
     else
     {
       setGeographicFlag(false);
     }
     //make sure the map units have been set

     setMapUnits();

     //@TODO this srs needs to be validated!!!
     mIsValidFlag=true;//shamelessly hard coded for now
      
#ifdef WIN32
	 return true;
#endif

}

bool QgsSpatialRefSys::writeXML( QDomNode & theNode, QDomDocument & theDoc )
{

  QDomElement myLayerNode = theNode.toElement();
  QDomElement mySrsElement  = theDoc.createElement( "spatialrefsys" );
  
  QDomElement myProj4Element  = theDoc.createElement( "proj4" );
  myProj4Element.appendChild(theDoc.createTextNode( proj4String()));
  mySrsElement.appendChild(myProj4Element);
  
  QDomElement mySrsIdElement  = theDoc.createElement( "srsid" );
  mySrsIdElement.appendChild(theDoc.createTextNode( QString::number(srsid())));
  mySrsElement.appendChild(mySrsIdElement);

  QDomElement mySridElement  = theDoc.createElement( "srid" );
  mySridElement.appendChild(theDoc.createTextNode( QString::number(srid())));
  mySrsElement.appendChild(mySridElement);

  QDomElement myEpsgElement  = theDoc.createElement( "epsg" );
  myEpsgElement.appendChild(theDoc.createTextNode( QString::number(epsg())));
  mySrsElement.appendChild(myEpsgElement);

  QDomElement myDescriptionElement  = theDoc.createElement( "description" );
  myDescriptionElement.appendChild(theDoc.createTextNode( description()));
  mySrsElement.appendChild(myDescriptionElement);

  QDomElement myProjectionAcronymElement  = theDoc.createElement( "projectionacronym" );
  myProjectionAcronymElement.appendChild(theDoc.createTextNode( projectionAcronym()));
  mySrsElement.appendChild(myProjectionAcronymElement);

  QDomElement myEllipsoidAcronymElement  = theDoc.createElement( "ellipsoidacronym" );
  myEllipsoidAcronymElement.appendChild(theDoc.createTextNode( ellipsoidAcronym()));
  mySrsElement.appendChild(myEllipsoidAcronymElement);

  QDomElement myGeographicFlagElement  = theDoc.createElement( "geographicflag" );
  QString myGeoFlagText = "false";
  if (geographicFlag())
  {
    myGeoFlagText="true";
  }
    
  myGeographicFlagElement.appendChild(theDoc.createTextNode( myGeoFlagText ));
  mySrsElement.appendChild(myGeographicFlagElement);

  myLayerNode.appendChild( mySrsElement );
#ifdef WIN32
  return true;
#endif

}



//
// Static helper methods below this point only please!
//


// Returns the whole proj4 string for the selected srsid
//this is a static method!
QString QgsSpatialRefSys::getProj4FromSrsId(const int theSrsId)
{

      QString myDatabaseFileName;
      QString myProjString;
      QString mySql = "select parameters from tbl_srs where srs_id = ";
      mySql += QString::number(theSrsId);

#ifdef QGISDEBUG
      std::cout << "QgsSpatialRefSys::getProj4FromSrsId :  mySrsId = " << theSrsId << std::endl;
      std::cout << "QgsSpatialRefSys::getProj4FromSrsId :  USER_PROJECTION_START_ID = " << USER_PROJECTION_START_ID << std::endl;
      std::cout << "QgsSpatialRefSys::getProj4FromSrsId :Selection sql : " << mySql.toLocal8Bit().data() << std::endl;
#endif
      //
      // Determine if this is a user projection or a system on
      // user projection defs all have srs_id >= 100000
      //
      if (theSrsId >= USER_PROJECTION_START_ID)
      {
        myDatabaseFileName = QDir::homeDirPath () + "/.qgis/qgis.db";
        QFileInfo myFileInfo;
        myFileInfo.setFile(myDatabaseFileName);
        if ( !myFileInfo.exists( ) ) //its unlikely that this condition will ever be reached
        {
          std::cout << " QgsSpatialRefSys::getProj4FromSrsId :  users qgis.db not found" << std::endl;
          return NULL;
        }
      }
      else //must be  a system projection then
      {
#if defined(Q_OS_MACX) || defined(WIN32)
        QString PKGDATAPATH = qApp->applicationDirPath() + "/share/qgis";
#endif
        myDatabaseFileName = PKGDATAPATH;
        myDatabaseFileName += "/resources/srs.db";
      }
      std::cout << "QgsSpatialRefSys::getProj4FromSrsId db = " << myDatabaseFileName.toLocal8Bit().data() << std::endl;


      sqlite3 *db;
      int rc;
      rc = sqlite3_open(myDatabaseFileName.toLocal8Bit().data(), &db);
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

      rc = sqlite3_prepare(db, mySql.utf8(), mySql.length(), &ppStmt, &pzTail);
      // XXX Need to free memory from the error msg if one is set

      if(rc == SQLITE_OK)
      {
        if(sqlite3_step(ppStmt) == SQLITE_ROW)
        {
          myProjString = QString::fromUtf8((char*)sqlite3_column_text(ppStmt, 0));
        }
      }
      // close the statement
      sqlite3_finalize(ppStmt);
      // close the database
      sqlite3_close(db);

      //assert(myProjString.length() > 0);
      return myProjString;
}
