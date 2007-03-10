/***************************************************************************
                    qgsnewconnection.cpp  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
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
#include <iostream>

#include <QSettings>
#include <QMessageBox>

#include "qgsnewconnection.h"
#include "qgscontexthelp.h"
extern "C"
{
#include <libpq-fe.h>
}
QgsNewConnection::QgsNewConnection(QWidget *parent, const QString& connName, Qt::WFlags fl)
: QDialog(parent, fl)
{
  setupUi(this);
  if (!connName.isEmpty())
    {
      // populate the dialog with the information stored for the connection
      // populate the fields with the stored setting parameters
      QSettings settings;

      QString key = "/PostgreSQL/connections/" + connName;
      txtHost->setText(settings.readEntry(key + "/host"));
      txtDatabase->setText(settings.readEntry(key + "/database"));
      QString port = settings.readEntry(key + "/port");
      if(port.length() ==0){
      	port = "5432";
      }
      txtPort->setText(port);
      txtUsername->setText(settings.readEntry(key + "/username"));
      Qt::CheckState s = Qt::Checked;
      if ( ! settings.readBoolEntry(key + "/publicOnly", false))
	s = Qt::Unchecked;
      cb_publicSchemaOnly->setCheckState(s);
      s = Qt::Checked;
      if ( ! settings.readBoolEntry(key + "/geometrycolumnsOnly", false))
	s = Qt::Unchecked;
      cb_geometryColumnsOnly->setCheckState(s);
      // Ensure that cb_plublicSchemaOnly is set correctly
      on_cb_geometryColumnsOnly_clicked();

      if (settings.readEntry(key + "/save") == "true")
        {
          txtPassword->setText(settings.readEntry(key + "/password"));
          chkStorePassword->setChecked(true);
        }
      txtName->setText(connName);
    }
}
/** Autoconnected SLOTS **/
void QgsNewConnection::on_btnOk_clicked()
{
  saveConnection();
}
void QgsNewConnection::on_btnHelp_clicked()
{
 helpInfo();
}
void QgsNewConnection::on_btnConnect_clicked()
{
  testConnection();
}
void QgsNewConnection::on_btnCancel_clicked(){
  // cancel the dialog
  reject();
}
void QgsNewConnection::on_cb_geometryColumnsOnly_clicked()
{
  if (cb_geometryColumnsOnly->checkState() == Qt::Checked)
    cb_publicSchemaOnly->setEnabled(false);
  else
    cb_publicSchemaOnly->setEnabled(true);
}

/** end  Autoconnected SLOTS **/

QgsNewConnection::~QgsNewConnection()
{
}
void QgsNewConnection::testConnection()
{
  // following line uses Qt SQL plugin - currently not used
  // QSqlDatabase *testCon = QSqlDatabase::addDatabase("QPSQL7","testconnection");

  // Need to escape the password to allow for single quotes and backslashes
  QString password = txtPassword->text();
  password.replace('\\', "\\\\");
  password.replace('\'', "\\'");

  QString connInfo =
    "host=" + txtHost->text() + 
    " dbname=" + txtDatabase->text() + 
    " port=" + txtPort->text() +
    " user=" + txtUsername->text() + 
    " password='" + password + "'";
  PGconn *pd = PQconnectdb(connInfo.toLocal8Bit().data());
//  std::cout << pd->ErrorMessage();
  if (PQstatus(pd) == CONNECTION_OK)
    {
      // Database successfully opened; we can now issue SQL commands.
      QMessageBox::information(this, tr("Test connection"), tr("Connection to %1 was successful").arg(txtDatabase->text()));
  } else
    {
      QMessageBox::information(this, tr("Test connection"), tr("Connection failed - Check settings and try again.\n\nExtended error information:\n") + QString(PQerrorMessage(pd)) );
    }
  // free pg connection resources
  PQfinish(pd);


}

void QgsNewConnection::saveConnection()
{
  QSettings settings; 
  QString baseKey = "/PostgreSQL/connections/";
  settings.writeEntry(baseKey + "selected", txtName->text());
  baseKey += txtName->text();
  settings.writeEntry(baseKey + "/host", txtHost->text());
  settings.writeEntry(baseKey + "/database", txtDatabase->text());
  settings.writeEntry(baseKey + "/port", txtPort->text());
  settings.writeEntry(baseKey + "/username", txtUsername->text());
  settings.writeEntry(baseKey + "/password", txtPassword->text());
  settings.writeEntry(baseKey + "/publicOnly", cb_publicSchemaOnly->isChecked());
  settings.writeEntry(baseKey + "/geometryColumnsOnly", cb_geometryColumnsOnly->isChecked());
  if (chkStorePassword->isChecked())
    {
      settings.writeEntry(baseKey + "/save", "true");
  } else
    {
      settings.writeEntry(baseKey + "/save", "false");
    }
  accept();
}
void QgsNewConnection::helpInfo()
{
  QgsContextHelp::run(context_id);
}
/* void QgsNewConnection::saveConnection()
{
	QSettings settings;
	QString baseKey = "/PostgreSQL/connections/";
	baseKey += txtName->text();
	settings.writeEntry(baseKey + "/host", txtHost->text());
	settings.writeEntry(baseKey + "/database", txtDatabase->text());

	settings.writeEntry(baseKey + "/username", txtUsername->text());
	if (chkStorePassword->isChecked()) {
		settings.writeEntry(baseKey + "/password", txtPassword->text());
	} else{
        settings.writeEntry(baseKey + "/password", "");
    }

  accept();
} */
