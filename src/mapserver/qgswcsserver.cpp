/***************************************************************************
                              qgswcsserver.cpp
                              -------------------
  begin                : December 9, 2013
  copyright            : (C) 2013 by René-Luc D'Hont
  email                : rldhont at 3liz dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgswcsserver.h"
#include "qgsconfigparser.h"
#include "qgscrscache.h"
#include "qgsrasterlayer.h"
#include "qgsrasterpipe.h"
#include "qgsrasterprojector.h"
#include "qgsrasterfilewriter.h"
#include "qgslogger.h"
#include "qgsmapserviceexception.h"

#include <QUrl>

#ifndef Q_WS_WIN
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

static const QString WCS_NAMESPACE = "http://www.opengis.net/wcs";
static const QString GML_NAMESPACE = "http://www.opengis.net/gml";
static const QString OGC_NAMESPACE = "http://www.opengis.net/ogc";

QgsWCSServer::QgsWCSServer( QMap<QString, QString> parameters )
    : mParameterMap( parameters )
    , mConfigParser( 0 )
{
}

QgsWCSServer::~QgsWCSServer()
{
}

QgsWCSServer::QgsWCSServer()
{
}

QDomDocument QgsWCSServer::getCapabilities()
{
  QgsDebugMsg( "Entering." );
  QDomDocument doc;
  //wcs:WCS_Capabilities element
  QDomElement wcsCapabilitiesElement = doc.createElement( "WCS_Capabilities"/*wcs:WCS_Capabilities*/ );
  wcsCapabilitiesElement.setAttribute( "xmlns", WCS_NAMESPACE );
  wcsCapabilitiesElement.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  wcsCapabilitiesElement.setAttribute( "xsi:schemaLocation", WCS_NAMESPACE + " http://schemas.opengis.net/wcs/1.0.0/wcsCapabilities.xsd" );
  wcsCapabilitiesElement.setAttribute( "xmlns:gml", GML_NAMESPACE );
  wcsCapabilitiesElement.setAttribute( "xmlns:xlink", "http://www.w3.org/1999/xlink" );
  wcsCapabilitiesElement.setAttribute( "version", "1.0.0" );
  wcsCapabilitiesElement.setAttribute( "updateSequence", "0" );
  doc.appendChild( wcsCapabilitiesElement );

  if ( mConfigParser )
  {
    mConfigParser->serviceCapabilities( wcsCapabilitiesElement, doc );
  }

  //INSERT Service

  //wcs:Capability element
  QDomElement capabilityElement = doc.createElement( "Capability"/*wcs:Capability*/ );
  wcsCapabilitiesElement.appendChild( capabilityElement );

  //wcs:Request element
  QDomElement requestElement = doc.createElement( "Request"/*wcs:Request*/ );
  capabilityElement.appendChild( requestElement );

  //wcs:GetCapabilities
  QDomElement getCapabilitiesElement = doc.createElement( "GetCapabilities"/*wcs:GetCapabilities*/ );
  requestElement.appendChild( getCapabilitiesElement );

  QDomElement dcpTypeElement = doc.createElement( "DCPType"/*wcs:DCPType*/ );
  getCapabilitiesElement.appendChild( dcpTypeElement );
  QDomElement httpElement = doc.createElement( "HTTP"/*wcs:HTTP*/ );
  dcpTypeElement.appendChild( httpElement );

  //Prepare url
  QString hrefString = mConfigParser->wcsServiceUrl();
  if ( hrefString.isEmpty() )
  {
    hrefString = mConfigParser->serviceUrl();
  }
  if ( hrefString.isEmpty() )
  {
    hrefString = serviceUrl();
  }

  QDomElement getElement = doc.createElement( "Get"/*wcs:Get*/ );
  httpElement.appendChild( getElement );
  QDomElement onlineResourceElement = doc.createElement( "OnlineResource"/*wcs:OnlineResource*/ );
  onlineResourceElement.setAttribute( "xlink:type", "simple" );
  onlineResourceElement.setAttribute( "xlink:href", hrefString );
  getElement.appendChild( onlineResourceElement );

  QDomElement getCapabilitiesDhcTypePostElement = dcpTypeElement.cloneNode().toElement();//this is the same as for 'GetCapabilities'
  getCapabilitiesDhcTypePostElement.firstChild().firstChild().toElement().setTagName( "Post" );
  getCapabilitiesElement.appendChild( getCapabilitiesDhcTypePostElement );

  QDomElement describeCoverageElement = getCapabilitiesElement.cloneNode().toElement();//this is the same as 'GetCapabilities'
  describeCoverageElement.setTagName( "DescribeCoverage" );
  requestElement.appendChild( describeCoverageElement );

  QDomElement getCoverageElement = getCapabilitiesElement.cloneNode().toElement();//this is the same as 'GetCapabilities'
  getCoverageElement.setTagName( "GetCoverage" );
  requestElement.appendChild( getCoverageElement );

  /*
   * Adding layer list in ContentMetadata
   */
  QDomElement contentMetadataElement = doc.createElement( "ContentMetadata"/*wcs:ContentMetadata*/ );
  wcsCapabilitiesElement.appendChild( contentMetadataElement );
  /*
   * Adding layer liste in contentMetadataElement
   */
  if ( mConfigParser )
  {
    mConfigParser->wcsContentMetadata( contentMetadataElement, doc );
  }

  return doc;
}

QDomDocument QgsWCSServer::describeCoverage()
{
  QgsDebugMsg( "Entering." );
  QDomDocument doc;
  //wcs:WCS_Capabilities element
  QDomElement coveDescElement = doc.createElement( "CoverageDescription"/*wcs:CoverageDescription*/ );
  coveDescElement.setAttribute( "xmlns", WCS_NAMESPACE );
  coveDescElement.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  coveDescElement.setAttribute( "xsi:schemaLocation", WCS_NAMESPACE + " http://schemas.opengis.net/wcs/1.0.0/describeCoverage.xsd" );
  coveDescElement.setAttribute( "xmlns:gml", GML_NAMESPACE );
  coveDescElement.setAttribute( "xmlns:xlink", "http://www.w3.org/1999/xlink" );
  coveDescElement.setAttribute( "version", "1.0.0" );
  coveDescElement.setAttribute( "updateSequence", "0" );
  doc.appendChild( coveDescElement );

  //defining coverage name
  QString coveName = "";
  //read COVERAGE
  QMap<QString, QString>::const_iterator cove_name_it = mParameterMap.find( "COVERAGE" );
  if ( cove_name_it != mParameterMap.end() )
  {
    coveName = cove_name_it.value();
  }
  if ( coveName == "" )
  {
    QMap<QString, QString>::const_iterator cove_name_it = mParameterMap.find( "IDENTIFIER" );
    if ( cove_name_it != mParameterMap.end() )
    {
      coveName = cove_name_it.value();
    }
  }
  mConfigParser->describeCoverage( coveName, coveDescElement, doc );
  return doc;
}

QByteArray* QgsWCSServer::getCoverage()
{
  QStringList wcsLayersId = mConfigParser->wcsLayers();

  QList<QgsMapLayer*> layerList;

  QStringList mErrors = QStringList();

  //defining coverage name
  QString coveName = "";
  //read COVERAGE
  QMap<QString, QString>::const_iterator cove_name_it = mParameterMap.find( "COVERAGE" );
  if ( cove_name_it != mParameterMap.end() )
  {
    coveName = cove_name_it.value();
  }
  if ( coveName == "" )
  {
    QMap<QString, QString>::const_iterator cove_name_it = mParameterMap.find( "IDENTIFIER" );
    if ( cove_name_it != mParameterMap.end() )
    {
      coveName = cove_name_it.value();
    }
  }

  if ( coveName == "" )
  {
    mErrors << QString( "COVERAGE is mandatory" );
  }

  layerList = mConfigParser->mapLayerFromCoverage( coveName );
  if ( layerList.size() < 1 )
  {
    mErrors << QString( "The layer for the COVERAGE '%1' is not found" ).arg( coveName );
  }

  bool conversionSuccess;
  // BBOX
  bool bboxOk = false;
  double minx = 0.0, miny = 0.0, maxx = 0.0, maxy = 0.0;
  // WIDTh and HEIGHT
  int width = 0, height = 0;
  // CRS
  QString crs = "";

  // read BBOX
  QMap<QString, QString>::const_iterator bbIt = mParameterMap.find( "BBOX" );
  if ( bbIt == mParameterMap.end() )
  {
    minx = 0; miny = 0; maxx = 0; maxy = 0;
  }
  else
  {
    bboxOk = true;
    QString bbString = bbIt.value();
    minx = bbString.section( ",", 0, 0 ).toDouble( &conversionSuccess );
    if ( !conversionSuccess ) {bboxOk = false;}
    miny = bbString.section( ",", 1, 1 ).toDouble( &conversionSuccess );
    if ( !conversionSuccess ) {bboxOk = false;}
    maxx = bbString.section( ",", 2, 2 ).toDouble( &conversionSuccess );
    if ( !conversionSuccess ) {bboxOk = false;}
    maxy = bbString.section( ",", 3, 3 ).toDouble( &conversionSuccess );
    if ( !conversionSuccess ) {bboxOk = false;}
  }
  if ( !bboxOk )
  {
    mErrors << QString( "The BBOX is mandatory and has to be xx.xxx,yy.yyy,xx.xxx,yy.yyy" );
  }

  // read WIDTH
  width = mParameterMap.value( "WIDTH", "0" ).toInt( &conversionSuccess );
  if ( !conversionSuccess )
    width = 0;
  // read HEIGHT
  height = mParameterMap.value( "HEIGHT", "0" ).toInt( &conversionSuccess );
  if ( !conversionSuccess )
  {
    height = 0;
  }

  if ( width < 0 || height < 0 )
  {
    mErrors << QString( "The WIDTH and HEIGHT are mandatory and have to be integer" );
  }

  crs = mParameterMap.value( "CRS", "" );
  if ( crs == "" )
  {
    mErrors << QString( "The CRS is mandatory" );
  }
  QgsCoordinateReferenceSystem outputCRS = QgsCRSCache::instance()->crsByAuthId( crs );
  if ( !outputCRS.isValid() )
  {
    mErrors << QString( "Could not create output CRS" );
  }

  if ( mErrors.count() != 0 )
  {
    throw QgsMapServiceException( "RequestNotWellFormed", mErrors.join( ". " ) );
  }

  QgsRectangle rect( minx, miny, maxx, maxy );

  QgsMapLayer* layer = layerList.at( 0 );
  QgsRasterLayer* rLayer = dynamic_cast<QgsRasterLayer*>( layer );
  if ( rLayer && wcsLayersId.contains( rLayer->id() ) )
  {
    QTemporaryFile tempFile;
    tempFile.open();
    QgsRasterFileWriter fileWriter( tempFile.fileName() );

    // clone pipe/provider
    QgsRasterPipe* pipe = new QgsRasterPipe();
    if ( !pipe->set( rLayer->dataProvider()->clone() ) )
    {
      mErrors << QString( "Cannot set pipe provider" );
      throw QgsMapServiceException( "RequestNotWellFormed", mErrors.join( ". " ) );
    }

    // add projector if necessary
    if ( outputCRS != rLayer->crs() )
    {
      QgsRasterProjector * projector = new QgsRasterProjector;
      projector->setCRS( rLayer->crs(), outputCRS );
      if ( !pipe->insert( 2, projector ) )
      {
        mErrors << QString( "Cannot set pipe projector" );
        throw QgsMapServiceException( "RequestNotWellFormed", mErrors.join( ". " ) );
      }
    }

    QgsRasterFileWriter::WriterError err = fileWriter.writeRaster( pipe, width, height, rect, outputCRS );
    if ( err != QgsRasterFileWriter::NoError )
    {
      mErrors << QString( "Cannot write raster error code: %1" ).arg( err );
      throw QgsMapServiceException( "RequestNotWellFormed", mErrors.join( ". " ) );
    }
    delete pipe;
    QByteArray* ba = 0;
    ba = new QByteArray();
    *ba = tempFile.readAll();

    return ba;
  }
  return 0;
}

QString QgsWCSServer::serviceUrl() const
{
  QUrl mapUrl( getenv( "REQUEST_URI" ) );
  mapUrl.setHost( getenv( "SERVER_NAME" ) );

  //Add non-default ports to url
  QString portString = getenv( "SERVER_PORT" );
  if ( !portString.isEmpty() )
  {
    bool portOk;
    int portNumber = portString.toInt( &portOk );
    if ( portOk )
    {
      if ( portNumber != 80 )
      {
        mapUrl.setPort( portNumber );
      }
    }
  }

  if ( QString( getenv( "HTTPS" ) ).compare( "on", Qt::CaseInsensitive ) == 0 )
  {
    mapUrl.setScheme( "https" );
  }
  else
  {
    mapUrl.setScheme( "http" );
  }

  QList<QPair<QString, QString> > queryItems = mapUrl.queryItems();
  QList<QPair<QString, QString> >::const_iterator queryIt = queryItems.constBegin();
  for ( ; queryIt != queryItems.constEnd(); ++queryIt )
  {
    if ( queryIt->first.compare( "REQUEST", Qt::CaseInsensitive ) == 0 )
    {
      mapUrl.removeQueryItem( queryIt->first );
    }
    else if ( queryIt->first.compare( "VERSION", Qt::CaseInsensitive ) == 0 )
    {
      mapUrl.removeQueryItem( queryIt->first );
    }
    else if ( queryIt->first.compare( "SERVICE", Qt::CaseInsensitive ) == 0 )
    {
      mapUrl.removeQueryItem( queryIt->first );
    }
    else if ( queryIt->first.compare( "_DC", Qt::CaseInsensitive ) == 0 )
    {
      mapUrl.removeQueryItem( queryIt->first );
    }
  }
  return mapUrl.toString();
}
