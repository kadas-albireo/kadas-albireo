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
#include "qgsmessagebar.h"
#include "qgsvbsmilixio.h"
#include "qgsvbsmilixannotationitem.h"
#include "qgsvbsmilixmanager.h"
#include <QDomDocument>
#include <QFileDialog>
#include <QMessageBox>
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

  QDomDocument doc;
  doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
  QDomElement milxDocumentEl = doc.createElement( "MilXDocument_Layer" );
  milxDocumentEl.setAttribute( "xmlns", "http://gs-soft.com/MilX/V3.1" );
  milxDocumentEl.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  doc.appendChild( milxDocumentEl );

  QDomElement milxVersionEl = doc.createElement( "MssLibraryVersionTag" );
  milxVersionEl.appendChild( doc.createTextNode( VBSMilixClient::libraryVersionTag() ) );
  milxDocumentEl.appendChild( milxVersionEl );

  QDomElement milxLayerEl = doc.createElement( "MilXLayer" );
  milxDocumentEl.appendChild( milxLayerEl );

  QDomElement milxLayerNameEl = doc.createElement( "Name" );
  milxLayerNameEl.appendChild( doc.createTextNode( "Default" ) ); // TODO
  milxLayerEl.appendChild( milxLayerNameEl );

  QDomElement milxLayerTypeEl = doc.createElement( "LayerType" );
  milxLayerTypeEl.appendChild( doc.createTextNode( "Normal" ) );
  milxLayerEl.appendChild( milxLayerTypeEl );

  QDomElement graphicListEl = doc.createElement( "GraphicList" );
  milxLayerEl.appendChild( graphicListEl );

  foreach ( const QgsVBSMilixAnnotationItem* item, manager->getItems() )
  {
    item->writeMilx( doc, graphicListEl );
  }

  QDomElement crsEl = doc.createElement( "CoordSystemType" );
  crsEl.appendChild( doc.createTextNode( "WGS84" ) );
  milxLayerEl.appendChild( crsEl );

  QDomElement symbolSizeEl = doc.createElement( "SymbolSize" );
  symbolSizeEl.appendChild( doc.createTextNode( QString::number( VBSMilixClient::SymbolSize ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = doc.createElement( "DisplayBW" );
  bwEl.appendChild( doc.createTextNode( "0" ) ); // TODO
  milxLayerEl.appendChild( bwEl );

  dev->write( doc.toString().toUtf8() );
  delete dev;
  delete zip;
  messageBar->pushMessage( tr( "Export Completed" ), "", QgsMessageBar::INFO, 5 );
  return true;
}

bool QgsVBSMilixIO::load( QgsVBSMilixManager* manager , QgsMapCanvas* canvas, QgsMessageBar *messageBar )
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
  QDomDocument doc;
  doc.setContent( dev );
  dev->deleteLater();


  QDomElement milxDocumentEl = doc.firstChildElement( "MilXDocument_Layer" );
  QDomElement milxVersionEl = milxDocumentEl.firstChildElement( "MssLibraryVersionTag" );
  QString fileMssVer = milxVersionEl.text();

  if ( fileMssVer > VBSMilixClient::libraryVersionTag() )
  {
    messageBar->pushMessage( tr( "Import Failed" ), tr( "The file was created by a newer MSS library version." ), QgsMessageBar::CRITICAL, 5 );
    return false;
  }

  QDomNodeList milxLayerEls = milxDocumentEl.elementsByTagName( "MilXLayer" );
  for ( int iLayer = 0, nLayers = milxLayerEls.count(); iLayer < nLayers; ++iLayer )
  {
    QDomElement milxLayerEl = milxLayerEls.at( iLayer ).toElement();
//    QString layerName = milxLayerEl.firstChildElement( "Name" ).text(); // TODO
//    QString layerType = milxLayerEl.firstChildElement( "LayerType" ).text(); // TODO
    int symbolSize = milxLayerEl.firstChildElement( "SymbolSize" ).text().toInt();
    QString crs = milxLayerEl.firstChildElement( "CoordSystemType" ).text();
    QString utmZone = milxLayerEl.firstChildElement( "CoordSystemUtmZone" ).text();
    const QgsCoordinateTransform* crst = 0;
    if ( crs == "SwissLv03" )
    {
      crst = QgsCoordinateTransformCache::instance()->transform( "EPSG:21781", canvas->mapSettings().destinationCrs().authid() );
    }
    else if ( crs == "WGS84" )
    {
      crst = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", canvas->mapSettings().destinationCrs().authid() );
    }
    else if ( crs == "UTM" )
    {
      QgsCoordinateReferenceSystem utmCrs;
      QString zoneLetter = utmZone.right( 1 ).toUpper();
      QString zoneNumber = utmZone.left( utmZone.length() - 1 );
      QString projZone = zoneNumber + ( zoneLetter == "S" ? " +south" : "" );
      utmCrs.createFromProj4( QString( "+proj=utm +zone=%1 +datum=WGS84 +units=m +no_defs" ).arg( projZone ) );
      crst = QgsCoordinateTransformCache::instance()->transform( utmCrs.authid(), canvas->mapSettings().destinationCrs().authid() );
    }

    QDomNodeList graphicEls = milxLayerEl.firstChildElement( "GraphicList" ).elementsByTagName( "MilXGraphic" );

    // Dry run to validate
    QStringList validationErrors;
    QStringList adjustedSymbolXmls;
    for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
    {
      QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();
      QString mssStringXml = graphicEl.firstChildElement( "MssStringXML" ).text();
      QString adjustedSymbolXml;
      bool valid = false;
      QString messages;
      VBSMilixClient::validateSymbolXml( mssStringXml, fileMssVer, adjustedSymbolXml, valid, messages );
      adjustedSymbolXmls.append( adjustedSymbolXml );
      if ( !valid )
      {
        validationErrors.append( QString( "%1:\n%2\n" ).arg( mssStringXml ).arg( messages ) );
      }
    }
    if ( !validationErrors.isEmpty() )
    {
      QMessageBox::critical( 0, tr( "Import Failed" ), tr( "The following validation errors occured:\n%1" ).arg( validationErrors.join( "\n" ) ) );
      return false;
    }
    for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
    {
      QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();

      QgsVBSMilixAnnotationItem* item = new QgsVBSMilixAnnotationItem( canvas );
      item->readMilx( graphicEl, adjustedSymbolXmls[iGraphic], crst, symbolSize );
      manager->addItem( item );
    }
  }

  messageBar->pushMessage( tr( "Import Completed" ), "", QgsMessageBar::INFO, 5 );
  return true;
}
