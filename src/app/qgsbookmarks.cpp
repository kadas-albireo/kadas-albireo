/***************************************************************************
               QgsBookmarks.cpp  - Spatial Bookmarks
                             -------------------
    begin                : 2005-04-23
    copyright            : (C) 2005 Gary Sherman
    email                : sherman at mrcc dot com
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
#include "qgsbookmarks.h"
#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgscontexthelp.h"
#include "qgsmapcanvas.h"
#include "qgslogger.h"
 
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

//standard includes
#include <iostream>
#include <cassert>
#include <sqlite3.h>
#include <fstream>

QgsBookmarks::QgsBookmarks(QWidget *parent, Qt::WFlags fl)
  : QDialog(parent, fl),
  mParent(parent)
{
  setupUi(this);
  connect(btnClose, SIGNAL(clicked()), this, SLOT(reject()));

  // user database is created at QGIS startup in QgisApp::createDB
  // we just check whether there is our database [MD]
  QFileInfo myFileInfo;
  myFileInfo.setFile(QgsApplication::qgisSettingsDirPath());
  if ( !myFileInfo.exists( ) )
  {
#ifdef QGISDEBUG 
    std::cout << "The qgis.db does not exist" << std::endl; 
#endif 
  }
  
  // Note proper queens english on next line
  initialise();

  // connect the slot up to catch when a new bookmark is added
  connect(mParent, SIGNAL(bookmarkAdded()), this, SLOT(refreshBookmarks()));
}

// Destructor
QgsBookmarks::~QgsBookmarks()
{
}

void QgsBookmarks::refreshBookmarks()
{
  lstBookmarks->clear();
  initialise();
}
// Initialise the bookmark tree from the database
void QgsBookmarks::initialise()
{
  int rc = connectDb();
  if(rc == SQLITE_OK)
  {
    // prepare the sql statement
    const char *pzTail;
    sqlite3_stmt *ppStmt;
    QString sql = "select * from tbl_bookmarks";

    rc = sqlite3_prepare(db, sql.utf8(), sql.length(), &ppStmt, &pzTail);
    // XXX Need to free memory from the error msg if one is set
    if(rc == SQLITE_OK)
    {
      // get the first row of the result set
      while(sqlite3_step(ppStmt) == SQLITE_ROW)
      {
        QString name = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 1));
        //        sqlite3_bind_parameter_index(ppStmt, "name"));
        //QgsDebugMsg("Bookmark name: " + name.toLocal8Bit().data()); 
        Q3ListViewItem *lvi = new Q3ListViewItem(lstBookmarks, name);
        // set the project name
        lvi->setText(1, QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 2))); 
        // get the extents
        QString xMin = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 3));
        QString yMin = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 4));
        QString xMax = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 5));
        QString yMax = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 6));
        // set the extents
        lvi->setText(2, xMin + ", " + yMin + ", " + xMax + ", " + yMax); 
        // set the id
        lvi->setText(3, QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 0)));
      }
    }
    else
    {
      // XXX query failed -- warn the user some how
      std::cout << "Failed to get bookmarks: " << sqlite3_errmsg(db) << std::endl; 
    }
    // close the statement
    sqlite3_finalize(ppStmt);
    // close the database
    sqlite3_close(db);
    // return the srs wkt

  }
}


void QgsBookmarks::on_btnDelete_clicked()
{
  // get the current item
  Q3ListViewItem *lvi = lstBookmarks->currentItem();
  if(lvi)
  {
    // make sure the user really wants to delete this bookmark
    if(0 == QMessageBox::information(this,tr("Really Delete?"),
          tr("Are you sure you want to delete the ") + lvi->text(0) +
          tr(" bookmark?"), tr("&Yes"), tr("&No"), QString::null, 0, 1))  
    {
      // remove it from the listview
      lstBookmarks->takeItem(lvi);
      // delete it from the database
      int rc = connectDb();
      if(rc == SQLITE_OK)
      {
        char *errmsg;
        // build the sql statement
        QString sql = "delete from tbl_bookmarks where bookmark_id = " + lvi->text(3);
        rc = sqlite3_exec(db, sql.utf8(), NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
          // XXX Provide popup message on failure?
          QMessageBox::warning(this, tr("Error deleting bookmark"),
                               tr("Failed to delete the ") + 
                               lvi->text(0) +
                               tr(" bookmark from the database. The "
                                  "database said:\n") + QString(errmsg),
                               QMessageBox::Ok, QMessageBox::NoButton);
          sqlite3_free(errmsg);
        }
        // close the database
        sqlite3_close(db);
      }
    }
  }
}

void QgsBookmarks::on_btnZoomTo_clicked()
{
  zoomToBookmark();
}

void QgsBookmarks::on_lstBookmarks_doubleClicked(Q3ListViewItem *lvi)
{
  zoomToBookmark();
}

void QgsBookmarks::zoomToBookmark()
{
	// Need to fetch the extent for the selected bookmark and then redraw
	// the map
  // get the current item
  Q3ListViewItem *lvi = lstBookmarks->currentItem();
  if(!lvi)
  {
      return;
  }
  // get the extent from the database
  int rc = connectDb();
  if(rc == SQLITE_OK)
  {
    sqlite3_stmt *ppStmt;
    const char *pzTail;
    // build the sql statement
    QString sql = "select xmin, ymin, xmax, ymax from tbl_bookmarks where bookmark_id = " + lvi->text(3);
    rc = sqlite3_prepare(db, sql.utf8(), sql.length(), &ppStmt, &pzTail);
    if(rc == SQLITE_OK)
    {
      if(sqlite3_step(ppStmt) == SQLITE_ROW){
        // get the extents from the resultset
        QString xmin  = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 0));
        QString ymin  = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 1));
        QString xmax  = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 2));
        QString ymax  = QString::fromUtf8((const char *)sqlite3_column_text(ppStmt, 3));
        // set the extent to the bookmark
        dynamic_cast<QgisApp*>(mParent)->setExtent(QgsRect(xmin.toDouble(),
              ymin.toDouble(),
              xmax.toDouble(),
              ymax.toDouble()));
        // redraw the map
        dynamic_cast<QgisApp*>(mParent)->getMapCanvas()->refresh();


      }
    }

    // close the statement
    sqlite3_finalize(ppStmt);
    // close the database
    sqlite3_close(db);
  }
  
}

int QgsBookmarks::connectDb()
{

  int rc;
  rc = sqlite3_open(QgsApplication::qgisUserDbFilePath(), &db);
  if(rc)
  {
    std::cout <<  "Can't open database: " <<  sqlite3_errmsg(db) << std::endl;

    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert(rc == 0);
  }
  return rc;
}

void QgsBookmarks::on_btnHelp_clicked()
{
  QgsContextHelp::run(context_id);
}
