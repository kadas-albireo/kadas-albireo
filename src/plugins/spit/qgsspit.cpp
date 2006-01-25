/***************************************************************************
                         qgsspit.cpp  -  description
                            -------------------
   begin                : Fri Dec 19 2003
   copyright            : (C) 2003 by Denis Antipov
   email                :
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

#include <q3table.h>
#include <QMessageBox>
#include <QComboBox>
#include <QFileDialog>
#include <q3progressdialog.h>
#include <q3memarray.h>
#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QPixmap>

#include <iostream>

#include "qgspgutil.h"
#include "qgsspit.h"
#include "qgsconnectiondialog.h"
#include "qgseditreservedwordsdialog.h"
#include "qgsmessageviewer.h"
#include "spiticon.xpm"
#include "spit_icons.h"

// Qt implementation of alignment() + changed the numeric types to be shown on the left as well
int Q3TableItem::alignment() const
{
  bool num;
  bool ok1 = FALSE, ok2 = FALSE;
  ( void ) txt.toInt( &ok1 );
  if ( !ok1 )
    ( void ) txt.toDouble( &ok2 );
  num = ok1 || ok2;

  return ( num ? Qt::AlignLeft : Qt::AlignLeft ) | Qt::AlignVCenter;
}

QgsSpit::QgsSpit( QWidget *parent, Qt::WFlags fl ) : QDialog( parent, fl )
{
  setupUi(this);
  QPixmap icon;
  icon = QPixmap( spitIcon );
  setIcon( icon );

  // Set up the table column headers
  tblShapefiles->setColumnCount(5);
  QStringList headerText;
  headerText << tr("File Name") << tr("Feature Class") << tr("Features") 
	     << tr("DB Relation Name") << tr("Schema");
  tblShapefiles->setHorizontalHeaderLabels(headerText);
  tblShapefiles->resizeColumnsToContents();

  //  tblShapefiles->verticalHeader()->hide();
  //  //tblShapefiles->adjustColumn(3);
  //  tblShapefiles->setLeftMargin(0);

  populateConnectionList();
  defSrid = -1;
  defGeom = "the_geom";
  total_features = 0;
  //setFixedSize(QSize(605, 612));

  chkUseDefaultSrid->setChecked( true );
  chkUseDefaultGeom->setChecked( true );
  useDefaultSrid();
  useDefaultGeom();

  schema_list << "public";
  gl_key = "/PostgreSQL/connections/";
  getSchema();
  // init the geometry type list
}

QgsSpit::~QgsSpit()
{}

void QgsSpit::populateConnectionList()
{
  QSettings settings("QuantumGIS", "qgis");
  QStringList keys = settings.subkeyList( "/PostgreSQL/connections" );
  QStringList::Iterator it = keys.begin();
  cmbConnections->clear();
  while ( it != keys.end() )
  {
    cmbConnections->insertItem( *it );
    ++it;
  }
}

void QgsSpit::newConnection()
{
  QgsConnectionDialog * con = new QgsConnectionDialog( this, "New Connection" );

  if ( con->exec() )
  {
    populateConnectionList();
    getSchema();
  }
}

void QgsSpit::editConnection()
{
  QgsConnectionDialog * con = new QgsConnectionDialog( this, cmbConnections->currentText() );
  if ( con->exec() )
  {
    con->saveConnection();
    getSchema();
  }
}

void QgsSpit::removeConnection()
{
  QSettings settings("QuantumGIS", "qgis");
  QString key = "/PostgreSQL/connections/" + cmbConnections->currentText();
  QString msg = tr("Are you sure you want to remove the [") + cmbConnections->currentText() + tr("] connection and all associated settings?");
  int result = QMessageBox::information( this, tr("Confirm Delete"), msg, tr("Yes"), tr("No") );
  if ( result == 0 )
  {
    settings.removeEntry( key + "/host" );
    settings.removeEntry( key + "/database" );
    settings.removeEntry( key + "/port" );
    settings.removeEntry( key + "/username" );
    settings.removeEntry( key + "/password" );
    settings.removeEntry( key + "/save" );

    cmbConnections->removeItem( cmbConnections->currentItem() );
  }
}

void QgsSpit::addFile()
{
  QString error1 = "";
  QString error2 = "";
  bool exist;
  bool is_error = false;
  QSettings settings("QuantumGIS", "qgis");

  QStringList files = QFileDialog::getOpenFileNames(this,
                        "Add Shapefiles",
                        settings.readEntry( "/Plugin-Spit/last_directory" ),
                        "Shapefiles (*.shp);;All files (*.*)" );
  if ( files.size() > 0 )
  {
    // Save the directory for future use
    QFileInfo fi( files[ 0 ] );
    settings.writeEntry( "/Plugin-Spit/last_directory", fi.dirPath( true ) );
  }
  // Process the files
  for ( QStringList::Iterator it = files.begin(); it != files.end(); ++it )
  {
    exist = false;
    is_error = false;

    // Check to ensure that we don't insert the same file twice
    QList<QTableWidgetItem*> items = tblShapefiles->findItems(*it, 
							Qt::MatchExactly);
    if (items.count() > 0)
    {
      exist = true;
    }

    if ( !exist )
    {
      // check other files: file.dbf and file.shx
      QString name = *it;
      if ( !QFile::exists( name.left( name.length() - 3 ) + "dbf" ) )
      {
        is_error = true;
      }
      else if ( !QFile::exists( name.left( name.length() - 3 ) + "shx" ) )
      {
        is_error = true;
      }

      if ( !is_error )
      {
        QgsShapeFile * file = new QgsShapeFile( name );
        if ( file->is_valid() )
        {
          /* XXX getFeatureClass actually does a whole bunch
           * of things and is probably better named 
           * something else
           */
          QString featureClass = file->getFeatureClass();
          fileList.push_back( file );

	  QTableWidgetItem *filenameItem       = new QTableWidgetItem( name );
	  QTableWidgetItem *featureClassItem   = new QTableWidgetItem( featureClass );
	  QTableWidgetItem *featureCountItem   = new QTableWidgetItem( QString( "%1" ).arg( file->getFeatureCount() ) );
          // Sanitize the relation name to make it pg friendly
          QString relName = file->getTable().replace(QRegExp("\\s"), "_");
	  QTableWidgetItem *dbRelationNameItem = new QTableWidgetItem( relName );
          //Q3ComboTableItem* schema = new Q3ComboTableItem( tblShapefiles, schema_list );
	  QTableWidgetItem *schemaItem         = new QTableWidgetItem();

	  // All items are editable except for these two
	  filenameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	  featureCountItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

          int row = tblShapefiles->rowCount();
          tblShapefiles->insertRow( row );
	  tblShapefiles->setItem( row, ColFILENAME, filenameItem );
	  tblShapefiles->setItem( row, ColFEATURECLASS, featureClassItem );
	  tblShapefiles->setItem( row, ColFEATURECOUNT, featureCountItem );
	  tblShapefiles->setItem( row, ColDBRELATIONNAME, dbRelationNameItem );

	    // FIXME
	    //Q3ComboTableItem* schema = new Q3ComboTableItem( tblShapefiles, schema_list );
	    //schema->setCurrentItem( cmbSchema->currentText() );
	  tblShapefiles->setItem( row, ColDBSCHEMA, schemaItem );

          total_features += file->getFeatureCount();

          // check for postgresql reserved words
          // First get an instance of the PG utility class
          QgsPgUtil *pgu = QgsPgUtil::instance();
          bool hasReservedWords = false;
          // if a reserved word is found, set the flag so the "adjustment"
          // dialog can be presented to the user
	  hasReservedWords = false;
          for ( int i = 0; i < file->column_names.size(); i++ )
          {
            if ( pgu->isReserved( file->column_names[ i ] ) )
            {
              hasReservedWords = true;
            }
          }
          // Why loop through all of them and then turn around and test
          // the flag? Because if there are reserved words, we want to
          // add all columns to the listview so the user can have the
          // opportunity to change them.
          if ( hasReservedWords )
          {
            // show the dialog for adjusting reserved words. Reserved
            // words are displayed differently so they are easy to spot
            QgsEditReservedWordsDialog * srw = new QgsEditReservedWordsDialog( this );
            srw->setCaption( file->getTable().upper() + tr(" - Edit Column Names") );
            // load the reserved words list
            srw->setReservedWords( pgu->reservedWords() );
            // load the columns and set their status
            for ( int i = 0; i < file->column_names.size(); i++ )
            {
              srw->addColumn( file->column_names[ i ],
                              pgu->isReserved( file->column_names[ i ] ), i );
            }
            if ( srw->exec() )
            {
              // get the new column specs from the listview control
              // and replace the existing column spec for the shapefile
              file->setColumnNames( srw->columnNames() );
            }
          }


        }
        else
        {
          error1 += name + "\n";
          is_error = true;
          delete file;
        }
      }
      else
      {
        error2 += name + "\n";
      }
    }
  }

  if ( error1 != "" || error2 != "" )
  {
    QString message = tr("The following Shapefile(s) could not be loaded:\n\n");
    if ( error1 != "" )
    {
      error1 += "----------------------------------------------------------------------------------------";
      error1 += "\n" + tr("REASON: File cannot be opened") + "\n\n";
    }
    if ( error2 != "" )
    {
      error2 += "----------------------------------------------------------------------------------------";
      error2 += "\n" + tr("REASON: One or both of the Shapefile files (*.dbf, *.shx) missing") + "\n\n";
    }
    QgsMessageViewer * e = new QgsMessageViewer( this );
    e->setMessage( message + error1 + error2 );
    e->exec();
  }

  /*
    not necessary (set elsewhere)
  for (int i = 0; i < tblShapefiles->numCols(); i++)
  {
    tblShapefiles->adjustColumn( i );
  } 
  */ 
//   tblShapefiles->adjustColumn( 0 );
//   tblShapefiles->adjustColumn( 1 );
//   tblShapefiles->adjustColumn( 2 );
//   tblShapefiles->adjustColumn( 3 );
//   tblShapefiles->adjustColumn( 4 );
//   tblShapefiles->setCurrentCell( -1, 0 );
}

void QgsSpit::removeFile()
{
  std::vector <int> temp;
  for ( int n = 0; n < tblShapefiles->rowCount(); n++ )
    if ( tblShapefiles->isItemSelected( tblShapefiles->item( n, 0 ) ) )
    {
      for ( std::vector<QgsShapeFile *>::iterator vit = fileList.begin(); vit != fileList.end(); vit++ )
      {
        if ( ( *vit ) ->getName() == tblShapefiles->item( n, 0 )->text() )
        {
          total_features -= ( *vit ) ->getFeatureCount();
          fileList.erase( vit );
          break;
        }
      }
      temp.push_back( n );
    }
  Q3MemArray<int> array( temp.size() );
  for ( int i = 0; i < temp.size(); i++ )
    tblShapefiles->removeRow( temp[ i ] );

  tblShapefiles->setCurrentCell( -1, 0 );
}

void QgsSpit::removeAllFiles()
{
  tblShapefiles->clear();
  fileList.clear();
  total_features = 0;
}

void QgsSpit::useDefaultSrid()
{
  if ( chkUseDefaultSrid->isChecked() )
  {
    defaultSridValue = spinSrid->value();
    spinSrid->setValue( defSrid );
    spinSrid->setEnabled( false );
  }
  else
  {
    spinSrid->setEnabled( true );
    spinSrid->setValue( defaultSridValue );
  }
}

void QgsSpit::useDefaultGeom()
{
  if ( chkUseDefaultGeom->isChecked() )
  {
    defaultGeomValue = txtGeomName->text();
    txtGeomName->setText( defGeom );
    txtGeomName->setEnabled( false );
  }
  else
  {
    txtGeomName->setEnabled( true );
    txtGeomName->setText( defaultGeomValue );
  }
}

// TODO: make translation of helpinfo
void QgsSpit::helpInfo()
{
  QString message = tr("General Interface Help:") + "\n\n";
  message += QString(
               tr("PostgreSQL Connections:") + "\n" ) + QString(
               "----------------------------------------------------------------------------------------\n" ) + QString(
               tr("[New ...] - create a new connection") + "\n" ) + QString(
               tr("[Edit ...] - edit the currently selected connection") + "\n" ) + QString(
               tr("[Remove] - remove the currently selected connection") + "\n" ) + QString(
               tr("-you need to select a connection that works (connects properly) in order to import files") + "\n" ) + QString(
               tr("-when changing connections Global Schema also changes accordingly") + "\n\n" ) + QString(
               tr("Shapefile List:") + "\n" ) + QString(
               "----------------------------------------------------------------------------------------\n" ) + QString(
               tr("[Add ...] - open a File dialog and browse to the desired file(s) to import") + "\n" ) + QString(
               tr("[Remove] - remove the currently selected file(s) from the list") + "\n" ) + QString(
               tr("[Remove All] - remove all the files in the list") + "\n" ) + QString(
               tr("[SRID] - Reference ID for the shapefiles to be imported") + "\n" ) + QString(
               tr("[Use Default (SRID)] - set SRID to -1") + "\n" ) + QString(
               tr("[Geometry Column Name] - name of the geometry column in the database") + "\n" ) + QString(
               tr("[Use Default (Geometry Column Name)] - set column name to \'the_geom\'") + "\n" ) + QString(
               tr("[Glogal Schema] - set the schema for all files to be imported into") + "\n\n" ) + QString(
               "----------------------------------------------------------------------------------------\n" ) + QString(
               tr("[Import] - import the current shapefiles in the list") + "\n" ) + QString(
               tr("[Quit] - quit the program\n") ) + QString(
               tr("[Help] - display this help dialog") + "\n\n" );
  QgsMessageViewer * e = new QgsMessageViewer( this );
  e->setMessage( message );
  e->exec();
}

PGconn* QgsSpit::checkConnection()
{
  QSettings settings("QuantumGIS", "qgis");
  PGconn * pd;
  bool result = true;
  QString connName = cmbConnections->currentText();
  if ( connName.isEmpty() )
  {
    QMessageBox::warning( this, tr("Import Shapefiles"), tr("You need to specify a Connection first") );
    result = false;
  }
  else
  {
    QString connInfo = "host=" + settings.readEntry( gl_key + connName + "/host" ) +
                       " dbname=" + settings.readEntry( gl_key + connName + "/database" ) +
                       " port=" + settings.readEntry( gl_key + connName + "/port" ) +
                       " user=" + settings.readEntry( gl_key + connName + "/username" ) +
                       " password=" + settings.readEntry( gl_key + connName + "/password" );
    pd = PQconnectdb( ( const char * ) connInfo );

    if ( PQstatus( pd ) != CONNECTION_OK )
    {
      QMessageBox::warning( this, tr("Import Shapefiles"), tr("Connection failed - Check settings and try again") );
      result = false;
    }
  }
  if ( result )
    return pd;
  else
    return NULL;
}

void QgsSpit::getSchema()
{
  QSettings settings("QuantumGIS", "qgis");
  schema_list.clear();
  schema_list << "public";
  PGconn* pd = checkConnection();
  if ( pd != NULL )
  {
    QString connName = cmbConnections->currentText();
    QString user = settings.readEntry( gl_key + connName + "/username" );
    QString schemaSql = QString( "select nspname from pg_namespace,pg_user where nspowner = usesysid and usename = '%1'" ).arg( user );
    PGresult *schemas = PQexec( pd, ( const char * ) schemaSql );
    // get the schema names
    if ( PQresultStatus( schemas ) == PGRES_TUPLES_OK )
    {
      for ( int i = 0; i < PQntuples( schemas ); i++ )
      {
        if ( QString( PQgetvalue( schemas, i, 0 ) ) != "public" )
          schema_list << QString( PQgetvalue( schemas, i, 0 ) );
      }
    }
    PQclear( schemas );
  }

  // update the schemas in the combo of all the shapefiles
  for ( int i = 0; i < tblShapefiles->rowCount(); i++ )
  {
    // FIXME update the widget with latest schema stuff
    /*
    Q3ComboTableItem* temp_schemas = new Q3ComboTableItem( tblShapefiles, schema_list );
    temp_schemas->setCurrentItem( "public" );
    tblShapefiles->setItem( i, 4, temp_schemas );
    */
  }

  cmbSchema->clear();
  cmbSchema->insertStringList( schema_list );
  cmbSchema->setCurrentText( "public" );
}

void QgsSpit::updateSchema()
{
  for ( int i = 0; i < tblShapefiles->rowCount(); i++ )
  {
    /*
    // FIXME update the widget with latest schema stuff
    tblShapefiles->clearCell( i, 4 );
    Q3ComboTableItem* temp_schemas = new Q3ComboTableItem( tblShapefiles, schema_list );
    temp_schemas->setCurrentItem( cmbSchema->currentText() );
    tblShapefiles->setItem( i, 4, temp_schemas );
    */

  }
}

void QgsSpit::import()
{
  tblShapefiles->setCurrentCell( -1, 0 );

  QString connName = cmbConnections->currentText();
  QSettings settings("QuantumGIS", "qgis");
  bool cancelled = false;
  PGconn* pd = checkConnection();
  QString query;

  if ( total_features == 0 )
  {
    QMessageBox::warning( this, tr("Import Shapefiles"), 
                         tr("You need to add shapefiles to the list first") );
  }
  else if ( pd != NULL )
  {
    PGresult * res;
    Q3ProgressDialog * pro = new Q3ProgressDialog( tr("Importing files"), 
                            tr("Cancel"), total_features, 
                            this, tr("Progress"), true );
    pro->setProgress( 0 );
    pro->setAutoClose( true );
    pro->show();
    qApp->processEvents();

    for ( int i = 0; i < fileList.size() ; i++ )
    {
      QString error = tr("Problem inserting features from file:") + "\n" + 
                      tblShapefiles->item( i, ColFILENAME )->text();
                      
      // if a name starts with invalid character
      if ( ! tblShapefiles->item( i, ColDBRELATIONNAME )->text()[ 0 ].isLetter() )
      {
        QMessageBox::warning( pro, tr("Import Shapefiles"), 
                             error + "\n" + tr("Invalid table name.") );
        pro->setProgress( pro->progress() 
                        + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }

      // if no fields detected
      if ( ( fileList[ i ] ->column_names ).size() == 0 )
      {
        QMessageBox::warning( pro, tr("Import Shapefiles"), 
                            error + "\n" + tr("No fields detected.") );
        pro->setProgress( pro->progress() + 
                          tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }

      // duplicate field check
      std::vector<QString> names_copy = fileList[ i ] ->column_names;
      std::cerr << "Size of names_copy before sort: " << names_copy.size() << std::endl;
      QString dupl = "";
      std::sort( names_copy.begin(), names_copy.end() );
      std::cerr << "Size of names_copy after sort: " << names_copy.size() << std::endl;

      for ( int k = 1; k < names_copy.size(); k++ )
      {
        std::cerr << "USING :" << names_copy[ k ].toLocal8Bit().data() << " index " << k << std::endl;
        qWarning( "Checking to see if " + names_copy[ k ] + " == " + names_copy[ k - 1 ] );
        if ( names_copy[ k ] == names_copy[ k - 1 ] )
          dupl += names_copy[ k ] + "\n";
      }
      // if duplicate field names exist
      if ( dupl != "" )
      {
        QMessageBox::warning( pro, tr("Import Shapefiles"), error +
                              "\n" + tr("The following fields are duplicates:") + "\n" + dupl );
        pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }

      // Check and set destination table 
      fileList[ i ] ->setTable( tblShapefiles->item( i, ColDBRELATIONNAME )->text() );
      pro->setLabelText( tr("Importing files") + "\n" 
                      + tblShapefiles->item(i, ColFILENAME)->text() );
      bool rel_exists1 = false;
      bool rel_exists2 = false;
      query = "SELECT f_table_name FROM geometry_columns WHERE f_table_name=\'" 
              + tblShapefiles->item( i, ColDBRELATIONNAME )->text() +
              "\' AND f_table_schema=\'" 
              + tblShapefiles->item( i, ColDBSCHEMA )->text() + "\'";
      res = PQexec( pd, ( const char * ) query );
      rel_exists1 = ( PQntuples( res ) > 0 );
      if ( PQresultStatus( res ) != PGRES_TUPLES_OK )
      {
        qWarning( PQresultErrorMessage( res ) );
        QMessageBox::warning( pro, tr("Import Shapefiles"), error );
        pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }
      else
      {
        PQclear( res );
      }

      query = "SELECT tablename FROM pg_tables WHERE tablename=\'" + tblShapefiles->item( i, ColDBRELATIONNAME )->text() + "\' AND schemaname=\'" +
              tblShapefiles->item( i, ColDBSCHEMA )->text() + "\'";
      res = PQexec( pd, ( const char * ) query );
      qWarning( query );
      rel_exists2 = ( PQntuples( res ) > 0 );
      if ( PQresultStatus( res ) != PGRES_TUPLES_OK )
      {
        qWarning( PQresultErrorMessage( res ) );
        QMessageBox::warning( pro, tr("Import Shapefiles"), error );
        pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }
      else
      {
        PQclear( res );
      }

      // begin session
      query = "BEGIN";
      res = PQexec( pd, query );
      if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
      {
        qWarning( PQresultErrorMessage( res ) );
        QMessageBox::warning( pro, tr("Import Shapefiles"), error );
        pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }
      else
      {
        PQclear( res );
      }

      query = "SET SEARCH_PATH TO \'";
      if ( tblShapefiles->item( i, ColDBSCHEMA )->text() == "public" )
        query += "public\'";
      else
        query += tblShapefiles->item( i, ColDBSCHEMA )->text() + "\', \'public\'";
      res = PQexec( pd, query );
      qWarning( query );
      if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
      {
        qWarning( PQresultErrorMessage( res ) );
        qWarning( PQresStatus( PQresultStatus( res ) ) );
        QMessageBox::warning( pro, tr("Import Shapefiles"), error );
        pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        continue;
      }
      else
      {
        PQclear( res );
      }

      QMessageBox *del_confirm;
      if ( rel_exists1 || rel_exists2 )
      {
        del_confirm = new QMessageBox( tr("Import Shapefiles - Relation Exists"),
                                       tr("The Shapefile:" ) + "\n" + tblShapefiles->item( i, 0 )->text() + "\n" + tr("will use [") +
                                       tblShapefiles->item( i, ColDBRELATIONNAME )->text() + tr("] relation for its data,") + "\n" + tr("which already exists and possibly contains data.") + "\n" +
                                       tr("To avoid data loss change the \"DB Relation Name\"") + "\n" + tr("for this Shapefile in the main dialog file list.") + "\n\n" +
                                       tr("Do you want to overwrite the [") + tblShapefiles->item( i, ColDBRELATIONNAME )->text() + tr("] relation?"),
                                       QMessageBox::Warning,
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape,
                                       Qt::NoButton, 
                                       this
//                                       tr("Relation Exists") 
                                     );

        if ( del_confirm->exec() == QMessageBox::Yes )
        {
          if ( rel_exists2 )
          {
            query = "DROP TABLE " + tblShapefiles->item( i, ColDBRELATIONNAME )->text();
            qWarning( query );
            res = PQexec( pd, ( const char * ) query );
            if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
              qWarning( PQresultErrorMessage( res ) );
              QMessageBox::warning( pro, tr("Import Shapefiles"), error );
              pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
              continue;
            }
            else
            {
              PQclear( res );
            }
          }

          if ( rel_exists1 )
          {
            /*query = "SELECT DropGeometryColumn(\'"+QString(settings.readEntry(key + "/database"))+"\', \'"+
              fileList[i]->getTable()+"\', \'"+txtGeomName->text()+"')";*/
            query = "DELETE FROM geometry_columns WHERE f_table_schema=\'" + tblShapefiles->item( i, ColDBSCHEMA )->text() + "\' AND " +
                    "f_table_name=\'" + tblShapefiles->item( i, ColDBRELATIONNAME )->text() + "\'";
            qWarning( query );
            res = PQexec( pd, ( const char * ) query );
            if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
              qWarning( PQresultErrorMessage( res ) );
              QMessageBox::warning( pro, tr("Import Shapefiles"), error );
              pro->setProgress( pro->progress() + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
              continue;
            }
            else
            {
              PQclear( res );
            }
          }
        }
        else
        {
          query = "ROLLBACK";
          res = PQexec( pd, query );
          if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
          {
            qWarning( PQresultErrorMessage( res ) );
            QMessageBox::warning( pro, tr("Import Shapefiles"), error );
          }
          else
          {
            PQclear( res );
          }
          pro->setProgress( pro->progress() + tblShapefiles->item( i, 2 )->text().toInt() );
          continue;
        }
      }

      // importing file here
      int temp_progress = pro->progress();
      cancelled = false;
      if ( fileList[ i ] ->insertLayer( settings.readEntry( gl_key + connName + "/database" ), tblShapefiles->item( i, ColDBSCHEMA )->text(),
                                        txtGeomName->text(), QString( "%1" ).arg( spinSrid->value() ), pd, pro, cancelled ) && !cancelled )
      { // if file has been imported successfully
        query = "COMMIT";
        res = PQexec( pd, query );
        if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
        {
          qWarning( PQresultErrorMessage( res ) );
          QMessageBox::warning( pro, tr("Import Shapefiles"), error );
          continue;
        }
        else
        {
          PQclear( res );
        }

        // remove file
        for ( int j = 0; j < tblShapefiles->rowCount(); j++ )
        {
          if ( tblShapefiles->item( j, ColFILENAME )->text() == QString( fileList[ i ] ->getName() ) )
          {
            tblShapefiles->selectRow( j );
            removeFile();
            i--;
            break;
          }
        }

      }
      else if ( !cancelled )
      { // if problem importing file occured
        pro->setProgress( temp_progress + tblShapefiles->item( i, ColFEATURECOUNT )->text().toInt() );
        QMessageBox::warning( this, tr("Import Shapefiles"), error );
        query = "ROLLBACK";
        res = PQexec( pd, query );
        if ( PQresultStatus( res ) != PGRES_COMMAND_OK )
        {
          qWarning( PQresultErrorMessage( res ) );
          QMessageBox::warning( pro, tr("Import Shapefiles"), error );
        }
        else
        {
          PQclear( res );
        }
      }
      else
      { // if import was actually cancelled
        delete pro;
        break;
      }
    }
    delete pro;
  }
  PQfinish( pd );
}
void QgsSpit::editColumns( int row, int col, int button, const QPoint &mousePos )
{
  // get the shapefile - table row maps directly to the fileList array
  QgsPgUtil * pgu = QgsPgUtil::instance();
  // show the dialog for adjusting reserved words. Reserved
  // words are displayed differently so they are easy to spot
  QgsEditReservedWordsDialog *srw = new QgsEditReservedWordsDialog( this );
  srw->setCaption( fileList[ row ] ->getTable().upper() + tr(" - Edit Column Names") );
  // set the description to indicate that we are editing column names, not
  // necessarily dealing with reserved words (although that is a possibility too)
  srw->setDescription(tr("Use the table below to edit column names. Make sure that none of the columns are named using a PostgreSQL reserved word"));
  // load the reserved words list
  srw->setReservedWords( pgu->reservedWords() );
  // load the columns and set their status
  for ( int i = 0; i < fileList[ row ] ->column_names.size(); i++ )
  {
    srw->addColumn( fileList[ row ] ->column_names[ i ],
                    pgu->isReserved( fileList[ row ] ->column_names[ i ] ), i );
  }
  if ( srw->exec() )
  {
    // get the new column specs from the listview control
    // and replace the existing column spec for the shapefile
    fileList[ row ] ->setColumnNames( srw->columnNames() );
  }

}

void QgsSpit::editShapefile(int row, int col, int button, const QPoint& mousePos)
{
  // FIXME Is this necessary any more?
  /*
  if (ColFEATURECLASS == col || ColDBRELATIONNAME == col)
  {
    tblShapefiles->editCell(row, col, FALSE);
  }
  */
}
