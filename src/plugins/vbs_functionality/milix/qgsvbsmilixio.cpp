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

#include "qgsmessagebar.h"
#include "qgsvbsmilixio.h"
#include "qgsvbsmilixannotationitem.h"
#include "qgsvbsmilixmanager.h"
#include <QDomDocument>
#include <QFileDialog>
#include <QSettings>
#include <quazip/quazipfile.h>

bool QgsVBSMilixIO::save( QgsVBSMilixManager* manager, QgsMessageBar* messageBar )
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString selectedFilter;
  QStringList filters = QStringList() << tr( "Compressed MilX Layer (*.milxlyz)" ) << tr( "MilX Layer (*.milxly)" );
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

  QDomDocument xml;
  xml.appendChild( xml.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
  QDomElement milxDocumentEl = xml.createElement( "MilXDocument_Layer" );
  milxDocumentEl.setAttribute( "xmlns", "http://gs-soft.com/MilX/V3.1" );
  milxDocumentEl.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  xml.appendChild( milxDocumentEl );

  QDomElement milxVersionEl = xml.createElement( "MssLibraryVersionTag" );
  milxVersionEl.appendChild( xml.createTextNode( VBSMilixClient::libraryVersionTag() ) );
  milxDocumentEl.appendChild( milxVersionEl );

  QDomElement milxLayerEl = xml.createElement( "MilXLayer" );
  milxDocumentEl.appendChild( milxLayerEl );

  QDomElement milxLayerNameEl = xml.createElement( "Name" );
  milxLayerNameEl.appendChild( xml.createTextNode( "Default" ) ); // TODO
  milxLayerEl.appendChild( milxLayerNameEl );

  QDomElement milxLayerTypeEl = xml.createElement( "LayerType" );
  milxLayerTypeEl.appendChild( xml.createTextNode( "Normal" ) );
  milxLayerEl.appendChild( milxLayerTypeEl );

  QDomElement graphicListEl = xml.createElement( "GraphicList" );
  milxLayerEl.appendChild( graphicListEl );

  foreach ( const QgsVBSMilixAnnotationItem* item, manager->getItems() )
  {
    item->writeMilx( xml, graphicListEl );
  }

  QDomElement crsEl = xml.createElement( "CoordSystemType" );
  crsEl.appendChild( xml.createTextNode( "WGS84" ) );
  milxLayerEl.appendChild( crsEl );

  QDomElement symbolSizeEl = xml.createElement( "SymbolSize" );
  symbolSizeEl.appendChild( xml.createTextNode( QString::number( VBSMilixClient::SymbolSize ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = xml.createElement( "DisplayBW" );
  bwEl.appendChild( xml.createTextNode( "0" ) ); // TODO
  milxLayerEl.appendChild( bwEl );

  dev->write( xml.toString().toUtf8() );
  delete zip;
  delete dev;
  messageBar->pushMessage( tr( "Export Completed" ), "", QgsMessageBar::INFO, 5 );
  return true;
}

bool QgsVBSMilixIO::load( QgsVBSMilixManager* manager , QgsMessageBar *messageBar )
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QStringList filters = QStringList() << tr( "MilX Layer (*.milxly)" ) << tr( "Compressed MilX Layer (*.milxlyz)" );
  QString filename = QFileDialog::getOpenFileName( 0, tr( "Select Milx Layer File" ), lastProjectDir, filters.join( ";;" ) );
  if ( filename.isEmpty() )
  {
    return false;
  }

}
