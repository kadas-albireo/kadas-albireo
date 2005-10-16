/***************************************************************************
                          qgsoptions.cpp
                    Set user options and preferences
                             -------------------
    begin                : May 28, 2004
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
#include <qsettings.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qfiledialog.h>
#include <qradiobutton.h>
#include <qapplication.h>
#include <qtextbrowser.h>
#include <sqlite3.h>
#include <cassert>
#include "qgsoptions.h"
#include "qgisapp.h"
#include "qgssvgcache.h"
#include "qgslayerprojectionselector.h"
/**
 * \class QgsOptions - Set user options and preferences
 * Constructor
 */
QgsOptions::QgsOptions(QWidget *parent, const char *name, bool modal) :
  QgsOptionsBase(parent, name, modal)
{
  qparent = parent;
  // read the current browser and set it
  QSettings settings;
  QString browser = settings.readEntry("/qgis/browser");
  cmbBrowser->setCurrentText(browser);
  // set the show splash option
  int identifyValue = settings.readNumEntry("/qgis/map/identifyRadius");
  spinBoxIdentifyValue->setValue(identifyValue);
  bool hideSplashFlag = false;
  if (settings.readEntry("/qgis/hideSplash")=="true")
  {
    hideSplashFlag =true;
  }
  cbxHideSplash->setChecked(hideSplashFlag);

  // set the current theme
  cmbTheme->setCurrentText(settings.readEntry("/qgis/theme"));
  // set the SVG oversampling factor
  spbSVGOversampling->setValue(QgsSVGCache::instance().getOversampling());
  // set the display update threshold
  spinBoxUpdateThreshold->setValue(settings.readNumEntry("/qgis/map/updateThreshold"));
  //set the default projection behaviour radio buttongs
  if (settings.readEntry("/qgis/projections/defaultBehaviour")=="prompt")
  {
    radPromptForProjection->setChecked(true);
  }
  else if (settings.readEntry("/qgis/projections/defaultBehaviour")=="useProject")
  {
    radUseProjectProjection->setChecked(true);
  }
  else //useGlobal
  {
    radUseGlobalProjection->setChecked(true);
  }
  mGlobalSRSID = settings.readNumEntry("/qgis/projections/defaultProjectionSRSID");
  //! @todo changes this control name in gui to txtGlobalProjString
  txtGlobalWKT->setText(QString::number(mGlobalSRSID));
  
  // populate combo box with ellipsoids
  mQGisSettingsDir = QDir::homeDirPath () + "/.qgis/";
  getEllipsoidList();
  QString myEllipsoidId = settings.readEntry("/qgis/measure/ellipsoid", "WGS84");
  cmbEllipsoid->setCurrentText(getEllipsoidName(myEllipsoidId));
}

//! Destructor
QgsOptions::~QgsOptions(){}

void QgsOptions::themeChanged(const QString &newThemeName)
{
  // Slot to change the theme as user scrolls through the choices
  QString newt = newThemeName;
  ((QgisApp*)qparent)->setTheme(newt);
}
QString QgsOptions::theme()
{
  // returns the current theme (as selected in the cmbTheme combo box)
  return cmbTheme->currentText();
}

void QgsOptions::saveOptions()
{
  QSettings settings;
  settings.writeEntry("/qgis/browser", cmbBrowser->currentText());
  settings.writeEntry("/qgis/map/identifyRadius", spinBoxIdentifyValue->value());
  settings.writeEntry("/qgis/hideSplash",cbxHideSplash->isChecked());
  settings.writeEntry("/qgis/new_layers_visible",!chkAddedVisibility->isChecked());
  if(cmbTheme->currentText().length() == 0)
  {
    settings.writeEntry("/qgis/theme", "default");
  }else{
    settings.writeEntry("/qgis/theme",cmbTheme->currentText());
  }
  settings.writeEntry("/qgis/map/updateThreshold", spinBoxUpdateThreshold->value());
  QgsSVGCache::instance().setOversampling(spbSVGOversampling->value());
  settings.writeEntry("/qgis/svgoversampling", spbSVGOversampling->value());
  //check behaviour so default projection when new layer is added with no
  //projection defined...
  if (radPromptForProjection->isChecked())
  {
    //
    settings.writeEntry("/qgis/projections/defaultBehaviour", "prompt");
  }
  else if(radUseProjectProjection->isChecked())
  {
    //
    settings.writeEntry("/qgis/projections/defaultBehaviour", "useProject");
  }
  else //assumes radUseGlobalProjection is checked
  {
    //
    settings.writeEntry("/qgis/projections/defaultBehaviour", "useGlobal");
  }
  settings.writeEntry("/qgis/projections/defaultProjectionSRSID",(int)mGlobalSRSID);

  settings.writeEntry("/qgis/measure/ellipsoid", getEllipsoidAcronym(cmbEllipsoid->currentText()));
  
  //all done
  accept();
}


void QgsOptions::cbxHideSplash_toggled( bool )
{

}
void QgsOptions::addTheme(QString item)
{
  cmbTheme->insertItem(item);
}



void QgsOptions::setCurrentTheme()
{
  QSettings settings;
  cmbTheme->setCurrentText(settings.readEntry("/qgis/theme","default"));
}

void QgsOptions::findBrowser()
{
  QString filter;
#ifdef WIN32
  filter = "Applications (*.exe)";
#else
  filter = "All Files (*)";
#endif
  QString browser = QFileDialog::getOpenFileName(
          "./",
          filter, 
          this,
          "open file dialog",
          "Choose a browser" );
  if(browser.length() > 0)
  {
    cmbBrowser->setCurrentText(browser);
  }
}


void QgsOptions::pbnSelectProjection_clicked()
{
  QSettings settings;
  QgsLayerProjectionSelector * mySelector = new QgsLayerProjectionSelector(this);
  mySelector->setSelectedSRSID(mGlobalSRSID);
  if(mySelector->exec())
  {
#ifdef QGISDEBUG
    std::cout << "------ Global Default Projection Selection Set ----------" << std::endl;
#endif
    mGlobalSRSID = mySelector->getCurrentSRSID();  
    //! @todo changes this control name in gui to txtGlobalProjString
    txtGlobalWKT->setText(mySelector->getCurrentProj4String());
#ifdef QGISDEBUG
    std::cout << "------ Global Default Projection now set to ----------\n" << mGlobalSRSID << std::endl;
#endif
  }
  else
  {
#ifdef QGISDEBUG
    std::cout << "------ Global Default Projection Selection change cancelled ----------" << std::endl;
#endif
    QApplication::restoreOverrideCursor();
  }

}
// Return state of the visibility flag for newly added layers. If

bool QgsOptions::newVisible()
{
  return !chkAddedVisibility->isChecked();
}

void QgsOptions::getEllipsoidList()
{
  // (copied from qgscustomprojectiondialog.cpp)

  // 
  // Populate the ellipsoid combo
  // 
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  //check the db is available
  myResult = sqlite3_open(QString(mQGisSettingsDir+"qgis.db").latin1(), &myDatabase);
  if(myResult) 
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl; 
    // XXX This will likely never happen since on open, sqlite creates the 
    //     database if it does not exist.
    assert(myResult == 0);
  }

  // Set up the query to retreive the projection information needed to populate the ELLIPSOID list
  QString mySql = "select * from tbl_ellipsoid order by name";
  myResult = sqlite3_prepare(myDatabase, (const char *)mySql, mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    while(sqlite3_step(myPreparedStatement) == SQLITE_ROW)
    {
      cmbEllipsoid->insertItem((char *)sqlite3_column_text(myPreparedStatement,1));
    }
  }
  // close the sqlite3 statement
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
}

QString QgsOptions::getEllipsoidAcronym(QString theEllipsoidName)
{
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  QString       myName;
  //check the db is available
  myResult = sqlite3_open(QString(mQGisSettingsDir+"qgis.db").latin1(), &myDatabase);
  if(myResult) 
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl; 
    // XXX This will likely never happen since on open, sqlite creates the 
    //     database if it does not exist.
    assert(myResult == 0);
  }
  // Set up the query to retreive the projection information needed to populate the ELLIPSOID list
  QString mySql = "select acronym from tbl_ellipsoid where name='" + theEllipsoidName + "'";
  myResult = sqlite3_prepare(myDatabase, (const char *)mySql, mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    sqlite3_step(myPreparedStatement) == SQLITE_ROW;
    myName = QString((char *)sqlite3_column_text(myPreparedStatement,0));
  }
  // close the sqlite3 statement
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  return myName;

}

QString QgsOptions::getEllipsoidName(QString theEllipsoidAcronym)
{
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  QString       myName;
  //check the db is available
  myResult = sqlite3_open(QString(mQGisSettingsDir+"qgis.db").latin1(), &myDatabase);
  if(myResult) 
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(myDatabase) << std::endl; 
    // XXX This will likely never happen since on open, sqlite creates the 
    //     database if it does not exist.
    assert(myResult == 0);
  }
  // Set up the query to retreive the projection information needed to populate the ELLIPSOID list
  QString mySql = "select name from tbl_ellipsoid where acronym='" + theEllipsoidAcronym + "'";
  myResult = sqlite3_prepare(myDatabase, (const char *)mySql, mySql.length(), &myPreparedStatement, &myTail);
  // XXX Need to free memory from the error msg if one is set
  if(myResult == SQLITE_OK)
  {
    sqlite3_step(myPreparedStatement) == SQLITE_ROW;
    myName = QString((char *)sqlite3_column_text(myPreparedStatement,0));
  }
  // close the sqlite3 statement
  sqlite3_finalize(myPreparedStatement);
  sqlite3_close(myDatabase);
  return myName;

}
