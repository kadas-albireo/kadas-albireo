/***************************************************************************
               QgsBookmarks.h  - Spatial Bookmarks
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
#ifndef QGSBOOKMARKS_H
#define QGSBOOKMARKS_H
#include "ui_qgsbookmarksbase.h"
#include <QDialog>

class QString;
class QWidget;
class QTreeWidgetItem;
struct sqlite3;
class QgsBookmarks : public QDialog, private Ui::QgsBookmarksBase
{
    Q_OBJECT
  public:
    QgsBookmarks( QWidget *parent = 0, Qt::WFlags fl = 0 );
    ~QgsBookmarks();
    void restorePosition();
  private slots:
    void saveWindowLocation();
    void on_btnDelete_clicked();
    void on_btnZoomTo_clicked();
    void on_lstBookmarks_doubleClicked( QTreeWidgetItem * );
    void helpRequested();
    void refreshBookmarks();
  private:
    QWidget *mParent;
    void initialise();
    int connectDb();
    void zoomToBookmark();
    sqlite3 *db;
    static const int context_id = 151694916;

};
#endif // QGSBOOKMARKS_H

