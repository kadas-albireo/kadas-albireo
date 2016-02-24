/***************************************************************************
 *  qgsvbsmilixio.h                                                        *
 *  -------------------                                                    *
 *  begin                : February 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscrscache.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessagebar.h"
#include "qgsvbsmilixio.h"
#include "qgsvbsmilixannotationitem.h"
#include "qgsvbsmilixlayer.h"
#include <QDomDocument>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QVBoxLayout>
#include <quazip/quazipfile.h>

bool QgsVBSMilixIO::save( QgsMessageBar* messageBar, QgsVBSMilixLayer* layer )
{
  QStringList versionTags, versionNames;
  VBSMilixClient::getLibraryVersionTags( versionTags, versionNames );
  QStringList filters;
  foreach ( const QString& versionName, versionNames )
  {
    filters.append( tr( "Compressed MilX Layer [%1] (*.milxlyz)" ).arg( versionName ) );
    filters.append( tr( "MilX Layer [%1] (*.milxly)" ).arg( versionName ) );
  }

  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString selectedFilter;

  QString filename = QFileDialog::getSaveFileName( 0, tr( "Select Output" ), lastProjectDir, filters.join( ";;" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return false;
  }
  if ( selectedFilter == filters[0] && !filename.endsWith( ".milxlyz", Qt::CaseInsensitive ) )
  {
    filename += ".milxlyz";
  }
  else if ( selectedFilter == filters[1] && !filename.endsWith( ".milxly", Qt::CaseInsensitive ) )
  {
    filename += ".milxly";
  }
  QString versionTag = versionTags[static_cast< QList<QString> >( filters ).indexOf( selectedFilter ) / 2];

  QIODevice* dev = 0;
  QuaZip* zip = 0;
  if ( selectedFilter == filters[0] )
  {
    zip = new QuaZip( filename );
    zip->open( QuaZip::mdCreate );
    dev = new QuaZipFile( zip );
    static_cast<QuaZipFile*>( dev )->open( QIODevice::WriteOnly, QuaZipNewInfo( "Layer.milxly" ) );
  }
  else
  {
    dev = new QFile( filename );
    dev->open( QIODevice::WriteOnly );
  }
  if ( !dev->isOpen() )
  {
    delete zip;
    delete dev;
    messageBar->pushMessage( tr( "Export Failed" ), tr( "Failed to open the output file for writing." ), QgsMessageBar::CRITICAL, 5 );
    return false;
  }

  QStringList exportMessages;
  layer->exportToMilxly( dev, versionTag, exportMessages );
  delete zip;
  delete dev;
  messageBar->pushMessage( tr( "Export Completed" ), "", QgsMessageBar::INFO, 5 );
  if ( !exportMessages.isEmpty() )
  {
    showMessageDialog( tr( "Export Messages" ), tr( "The following messages were emitted while exporting:" ), exportMessages.join( "\n" ) );
  }
  return true;
}

bool QgsVBSMilixIO::load( QgsMessageBar *messageBar )
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString filter = tr( "MilX Layer Files (*.milxly *.milxlyz)" );
  QString filename = QFileDialog::getOpenFileName( 0, tr( "Select Milx Layer File" ), lastProjectDir, filter );
  if ( filename.isEmpty() )
  {
    return false;
  }

  QIODevice* dev = 0;
  if ( filename.endsWith( ".milxlyz", Qt::CaseInsensitive ) )
  {
    dev = new QuaZipFile( filename, "Layer.milxly" );
  }
  else
  {
    dev = new QFile( filename );
  }
  if ( !dev->open( QIODevice::ReadOnly ) )
  {
    delete dev;
    messageBar->pushMessage( tr( "Import Failed" ), tr( "Failed to open the output file for reading." ), QgsMessageBar::CRITICAL, 5 );
    return false;
  }

  QgsVBSMilixLayer* layer = new QgsVBSMilixLayer();
  QString errorMsg;
  QStringList importMessages;
  bool success = layer->importMilxly( dev, errorMsg, importMessages );
  delete dev;

  if ( success )
  {
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    messageBar->pushMessage( tr( "Import Completed" ), "", QgsMessageBar::INFO, 5 );
    if ( !importMessages.isEmpty() )
    {
      showMessageDialog( tr( "Import Messages" ), tr( "The following messages were emitted while importing:" ), importMessages.join( "\n" ) );
    }
  }
  else
  {
    delete layer;
    messageBar->pushMessage( tr( "Import Failed" ), errorMsg, QgsMessageBar::CRITICAL, 5 );
  }
  return true;
}

void QgsVBSMilixIO::showMessageDialog( const QString& title, const QString& body, const QString& messages )
{
  QDialog dialog;
  dialog.setWindowTitle( title );
  dialog.setLayout( new QVBoxLayout );
  dialog.layout()->addWidget( new QLabel( body ) );
  QPlainTextEdit* textEdit = new QPlainTextEdit( messages );
  textEdit->setReadOnly( true );
  dialog.layout()->addWidget( textEdit );
  dialog.exec();
}
