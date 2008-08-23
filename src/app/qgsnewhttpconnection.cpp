/***************************************************************************
    qgsnewhttpconnection.cpp -  selector for a new HTTP server for WMS, etc.
                             -------------------
    begin                : 3 April 2005
    copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au
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
#include "qgsnewhttpconnection.h"
#include "qgscontexthelp.h"
#include <QSettings>

QgsNewHttpConnection::QgsNewHttpConnection(
  QWidget *parent, const QString& baseKey, const QString& connName, Qt::WFlags fl ):
    QDialog( parent, fl ),
    mBaseKey( baseKey ),
    mOriginalConnName( connName )
{
  setupUi( this );

  if ( !connName.isEmpty() )
  {
    // populate the dialog with the information stored for the connection
    // populate the fields with the stored setting parameters

    QSettings settings;

    QString key = mBaseKey + connName;
    txtName->setText( connName );
    txtUrl->setText( settings.value( key + "/url" ).toString() );
  }
  connect( buttonBox, SIGNAL( helpRequested() ), this, SLOT( helpRequested() ) );
}

QgsNewHttpConnection::~QgsNewHttpConnection()
{
}

void QgsNewHttpConnection::accept()
{
  QSettings settings;
  QString key = mBaseKey + txtName->text();

  //delete original entry first
  if ( !mOriginalConnName.isNull() && mOriginalConnName != key )
  {
    settings.remove( mBaseKey + mOriginalConnName );
  }
  settings.setValue( key + "/url", txtUrl->text().trimmed() );

  QDialog::accept();
}

void QgsNewHttpConnection::helpRequested()
{
  QgsContextHelp::run( context_id );
}
