/***************************************************************************
                          qgsnewogrconnection.cpp  -  description
                             -------------------
    begin                : Mon Jan 2 2009
    copyright            : (C) 2009 by Godofredo Contreras Nava
    email                : frdcn at hotmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id: $ */
#include <iostream>

#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>

#include "qgsnewogrconnection.h"
#include "qgscontexthelp.h"
#include "qgslogger.h"
#include "qgsproviderregistry.h"
#include "qgsogrhelperfunctions.h"
#include <ogr_api.h>
#include <cpl_error.h>


QgsNewOgrConnection::QgsNewOgrConnection( QWidget *parent, const QString& connType, const QString& connName, Qt::WFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );
  connect( buttonBox, SIGNAL( helpRequested() ), this, SLOT( help() ) );
  //add database drivers
  QStringList dbDrivers = QgsProviderRegistry::instance()->databaseDrivers().split( ";" );
  for ( int i = 0; i < dbDrivers.count(); i++ )
  {
    QString dbDrive = dbDrivers.at( i );
    cmbDatabaseTypes->addItem( dbDrive.split( "," ).at( 0 ) );
  }
  txtName->setEnabled( true );
  cmbDatabaseTypes->setEnabled( true );
  if ( !connName.isEmpty() )
  {
    // populate the dialog with the information stored for the connection
    // populate the fields with the stored setting parameters
    QSettings settings;
    QString key = "/" + connType + "/connections/" + connName;
    txtHost->setText( settings.value( key + "/host" ).toString() );
    txtDatabase->setText( settings.value( key + "/database" ).toString() );
    QString port = settings.value( key + "/port" ).toString();
    txtPort->setText( port );
    txtUsername->setText( settings.value( key + "/username" ).toString() );
    if ( settings.value( key + "/save" ).toString() == "true" )
    {
      txtPassword->setText( settings.value( key + "/password" ).toString() );
      chkStorePassword->setChecked( true );
    }
    cmbDatabaseTypes->setCurrentIndex( cmbDatabaseTypes->findText( connType ) );
    txtName->setText( connName );
    txtName->setEnabled( false );
    cmbDatabaseTypes->setEnabled( false );
  }
}

QgsNewOgrConnection::~QgsNewOgrConnection()
{
}

void QgsNewOgrConnection::testConnection()
{
  QString uri;
  uri = createDatabaseURI( cmbDatabaseTypes->currentText(), txtHost->text(),
                           txtDatabase->text(), txtPort->text(),
                           txtUsername->text(), txtPassword->text() );
  QgsDebugMsg( "Connecting using uri = " + uri );
  OGRRegisterAll();
  OGRDataSourceH       poDS;
  OGRSFDriverH         pahDriver;
  CPLErrorReset();
  poDS = OGROpen( QFile::encodeName( uri ).constData(), FALSE, &pahDriver );
  if ( poDS == NULL )
  {
    QMessageBox::information( this, tr( "Test connection" ), tr( "Connection failed - Check settings and try again.\n\nExtended error information:\n%1" ).arg( QString::fromUtf8( CPLGetLastErrorMsg() ) ) );
  }
  else
  {
    QMessageBox::information( this, tr( "Test connection" ), tr( "Connection to %1 was successful" ).arg( uri ) );
    OGRReleaseDataSource( poDS );
  }
}

void QgsNewOgrConnection::saveConnection()
{
  QSettings settings;
  QString baseKey = "/" + cmbDatabaseTypes->currentText() + "/connections/";
  settings.setValue( baseKey + "selected", txtName->text() );
  baseKey += txtName->text();
  settings.setValue( baseKey + "/host", txtHost->text() );
  settings.setValue( baseKey + "/database", txtDatabase->text() );
  settings.setValue( baseKey + "/port", txtPort->text() );
  settings.setValue( baseKey + "/username", txtUsername->text() );
  settings.setValue( baseKey + "/password", chkStorePassword->isChecked() ? txtPassword->text() : "" );
  settings.setValue( baseKey + "/save", chkStorePassword->isChecked() ? "true" : "false" );
}

/** Autoconnected SLOTS **/
void QgsNewOgrConnection::accept()
{
  saveConnection();
  QDialog::accept();
}

void QgsNewOgrConnection::help()
{
  helpInfo();
}

void QgsNewOgrConnection::on_btnConnect_clicked()
{
  testConnection();
}

void QgsNewOgrConnection::helpInfo()
{
  QgsContextHelp::run( context_id );
}

/** end  Autoconnected SLOTS **/









