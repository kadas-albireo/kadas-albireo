/***************************************************************************
 *  qgsmilxio.h                                                            *
 *  -----------                                                            *
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

#include "qgisinterface.h"
#include "qgscrscache.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessagebar.h"
#include "qgsmilxio.h"
#include "qgsmilxannotationitem.h"
#include "qgsmilxlayer.h"
#include "layertree/qgslayertreeview.h"
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QVBoxLayout>
#include <quazip/quazipfile.h>

bool QgsMilXIO::save( QgisInterface* iface )
{
  QDialog layerSelectionDialog( iface->mainWindow() );
  layerSelectionDialog.setWindowTitle( tr( "Export MilX layers" ) );
  layerSelectionDialog.setLayout( new QVBoxLayout() );
  layerSelectionDialog.layout()->addWidget( new QLabel( tr( "Select MilX layers to export" ) ) );
  QListWidget* layerListWidget = new QListWidget();
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      QListWidgetItem* item = new QListWidgetItem( layer->name() );
      item->setData( Qt::UserRole, layer->id() );
      item->setCheckState( iface->mapCanvas()->layers().contains( layer ) ? Qt::Checked : Qt::Unchecked );
      layerListWidget->addItem( item );
    }
  }
  layerSelectionDialog.layout()->addWidget( layerListWidget );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  layerSelectionDialog.layout()->addWidget( bbox );
  connect( bbox, SIGNAL( accepted() ), &layerSelectionDialog, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), &layerSelectionDialog, SLOT( reject() ) );
  if ( layerSelectionDialog.exec() == QDialog::Rejected )
  {
    return false;
  }

  QStringList exportLayers;
  for ( int i = 0, n = layerListWidget->count(); i < n; ++i )
  {
    QListWidgetItem* item = layerListWidget->item( i );
    if ( item->checkState() == Qt::Checked )
    {
      exportLayers.append( item->data( Qt::UserRole ).toString() );
    }
  }

  QStringList versionTags, versionNames;
  MilXClient::getSupportedLibraryVersionTags( versionTags, versionNames );
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
    delete dev;
    delete zip;
    iface->messageBar()->pushMessage( tr( "Export Failed" ), tr( "Failed to open the output file for writing." ), QgsMessageBar::CRITICAL, 5 );
    return false;
  }

  QStringList exportMessages;

  QDomDocument doc;
  doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
  QDomElement milxDocumentEl = doc.createElement( "MilXDocument_Layer" );
  milxDocumentEl.setAttribute( "xmlns", "http://gs-soft.com/MilX/V3.1" );
  milxDocumentEl.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  doc.appendChild( milxDocumentEl );

  QString verTag; MilXClient::getCurrentLibraryVersionTag( verTag );
  QDomElement milxVersionEl = doc.createElement( "MssLibraryVersionTag" );
  milxVersionEl.appendChild( doc.createTextNode( verTag ) );
  milxDocumentEl.appendChild( milxVersionEl );

  foreach ( const QString& layerId, exportLayers )
  {
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerId );
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      static_cast<QgsMilXLayer*>( layer )->exportToMilxly( milxDocumentEl, versionTag, exportMessages );
    }
  }
  dev->write( doc.toString().toUtf8() );

  delete dev;
  delete zip;
  iface->messageBar()->pushMessage( tr( "Export Completed" ), "", QgsMessageBar::INFO, 5 );
  if ( !exportMessages.isEmpty() )
  {
    showMessageDialog( tr( "Export Messages" ), tr( "The following messages were emitted while exporting:" ), exportMessages.join( "\n" ) );
  }
  return true;
}

bool QgsMilXIO::load( QgisInterface* iface )
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
    iface->messageBar()->pushMessage( tr( "Import Failed" ), tr( "Failed to open the output file for reading." ), QgsMessageBar::CRITICAL, 5 );
    return false;
  }

  QDomDocument doc;
  doc.setContent( dev->readAll() );
  delete dev;

  QDomElement milxDocumentEl = doc.firstChildElement( "MilXDocument_Layer" );
  QDomElement milxVersionEl = milxDocumentEl.firstChildElement( "MssLibraryVersionTag" );
  QString fileMssVer = milxVersionEl.text();

  QString verTag;
  MilXClient::getCurrentLibraryVersionTag( verTag );
  if ( fileMssVer > verTag )
  {
    QString errorMsg = tr( "The file was created by a newer MSS library version." );
    iface->messageBar()->pushMessage( tr( "Import Failed" ), errorMsg, QgsMessageBar::CRITICAL, 5 );
    return false;
  }

  QDomNodeList milxLayerEls = milxDocumentEl.elementsByTagName( "MilXLayer" );
  QStringList importMessages;
  QString errorMsg;
  QList<QgsMilXLayer*> importedLayers;
  for ( int iLayer = 0, nLayers = milxLayerEls.count(); iLayer < nLayers; ++iLayer )
  {
    QDomElement milxLayerEl = milxLayerEls.at( iLayer ).toElement();
    QgsMilXLayer* layer = new QgsMilXLayer( iface->layerTreeView()->menuProvider() );
    if ( !layer->importMilxly( milxLayerEl, fileMssVer, errorMsg, importMessages ) )
    {
      break;
    }
    importedLayers.append( layer );
  }

  if ( errorMsg.isEmpty() )
  {
    foreach ( QgsMilXLayer* layer, importedLayers )
    {
      QgsMapLayerRegistry::instance()->addMapLayer( layer );
    }
    iface->messageBar()->pushMessage( tr( "Import Completed" ), "", QgsMessageBar::INFO, 5 );
    if ( !importMessages.isEmpty() )
    {
      showMessageDialog( tr( "Import Messages" ), tr( "The following messages were emitted while importing:" ), importMessages.join( "\n" ) );
    }
  }
  else
  {
    qDeleteAll( importedLayers );
    iface->messageBar()->pushMessage( tr( "Import Failed" ), errorMsg, QgsMessageBar::CRITICAL, 5 );
  }
  return true;
}

void QgsMilXIO::showMessageDialog( const QString& title, const QString& body, const QString& messages )
{
  QDialog dialog;
  dialog.setWindowTitle( title );
  dialog.setLayout( new QVBoxLayout );
  dialog.layout()->addWidget( new QLabel( body ) );
  QPlainTextEdit* textEdit = new QPlainTextEdit( messages );
  textEdit->setReadOnly( true );
  dialog.layout()->addWidget( textEdit );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Close );
  dialog.layout()->addWidget( bbox );
  connect( bbox, SIGNAL( accepted() ), &dialog, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), &dialog, SLOT( reject() ) );
  dialog.exec();
}
