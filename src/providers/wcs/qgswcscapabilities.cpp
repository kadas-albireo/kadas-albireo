/***************************************************************************
  qgswcscapabilities.cpp  -  WCS capabilities
                             -------------------
    begin                : 17 Mar, 2005
    copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au

    wcs                  : 4/2012 Radim Blazek, based on qgswmsprovider.cpp

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <typeinfo>

#define WCS_THRESHOLD 200  // time to wait for an answer without emitting dataChanged() 
#include "qgslogger.h"
#include "qgswcscapabilities.h"
#include "qgsowsconnection.h"

#include <cmath>

#include "qgscoordinatetransform.h"
#include "qgsdatasourceuri.h"
#include "qgsrasterlayer.h"
#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsnetworkaccessmanager.h"
#include <qgsmessageoutput.h>
#include <qgsmessagelog.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>

#if QT_VERSION >= 0x40500
#include <QNetworkDiskCache>
#endif

#include <QUrl>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPixmap>
#include <QSet>
#include <QSettings>
#include <QEventLoop>
#include <QCoreApplication>
#include <QTime>

#ifdef _MSC_VER
#include <float.h>
#define isfinite(x) _finite(x)
#endif

#ifdef QGISDEBUG
#include <QFile>
#include <QDir>
#endif

/*
QgsWcsCapabilities::QgsWcsCapabilities( QString const &theUri )
{
  mUri.setEncodedUri( theUri );

  QgsDebugMsg( "theUri = " + theUri );
}
*/

QgsWcsCapabilities::QgsWcsCapabilities( QgsDataSourceURI const &theUri ):
    mUri( theUri ),
    mCoverageCount( 0 )
{
  QgsDebugMsg( "uri = " + mUri.encodedUri() );

  retrieveServerCapabilities();
}

QgsWcsCapabilities::QgsWcsCapabilities( ):
    mCoverageCount( 0 )
{
}

QgsWcsCapabilities::~QgsWcsCapabilities()
{
  QgsDebugMsg( "deconstructing." );
}

// TODO: return if successful
void QgsWcsCapabilities::setUri( QgsDataSourceURI const &theUri )
{
  mUri = theUri;

  clear();
  retrieveServerCapabilities( );
}

QString QgsWcsCapabilities::prepareUri( QString uri )
{
  if ( !uri.contains( "?" ) )
  {
    uri.append( "?" );
  }
  else if ( uri.right( 1 ) != "?" && uri.right( 1 ) != "&" )
  {
    uri.append( "&" );
  }

  return uri;
}

QgsWcsCapabilitiesProperty QgsWcsCapabilities::capabilities()
{
  return mCapabilities;
}

bool QgsWcsCapabilities::supportedCoverages( QVector<QgsWcsCoverageSummary> &coverageSummary )
{
  QgsDebugMsg( "Entering." );

  // Allow the provider to collect the capabilities first.
  if ( !retrieveServerCapabilities() )
  {
    return false;
  }

  coverageSummary = mCoveragesSupported;

  QgsDebugMsg( "Exiting." );

  return true;
}

QString QgsWcsCapabilities::getCoverageUrl() const
{
  QString url = mCapabilities.operationsMetadata.getCoverage.dcp.http.get.xlinkHref;
  if ( url.isEmpty() )
  {
    url = mUri.param( "url" );
  }
  return url;
}

bool QgsWcsCapabilities::sendRequest( QString const & url )
{
  QgsDebugMsg( "url = " + url );
  mError = "";
  QNetworkRequest request( url );
  setAuthorization( request );
  request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork );
  request.setAttribute( QNetworkRequest::CacheSaveControlAttribute, true );

  QgsDebugMsg( QString( "getcapabilities: %1" ).arg( url ) );
  mCapabilitiesReply = QgsNetworkAccessManager::instance()->get( request );

  connect( mCapabilitiesReply, SIGNAL( finished() ), this, SLOT( capabilitiesReplyFinished() ) );
  connect( mCapabilitiesReply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( capabilitiesReplyProgress( qint64, qint64 ) ) );

  while ( mCapabilitiesReply )
  {
    QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
  }

  if ( mCapabilitiesResponse.isEmpty() )
  {
    if ( mError.isEmpty() )
    {
      mErrorFormat = "text/plain";
      mError = tr( "empty capabilities document" );
    }
    return false;
  }

  if ( mCapabilitiesResponse.startsWith( "<html>" ) ||
       mCapabilitiesResponse.startsWith( "<HTML>" ) )
  {
    mErrorFormat = "text/html";
    mError = mCapabilitiesResponse;
    return false;
  }
  return true;
}

void QgsWcsCapabilities::clear()
{
  mCoverageCount = 0;
  mCoveragesSupported.clear();
  QgsWcsCapabilitiesProperty c;
  mCapabilities = c;
}

bool QgsWcsCapabilities::retrieveServerCapabilities( )
{
  clear();
  QStringList versions;

  // 1.0.0 - VERSION
  // 1.1.0 - AcceptedVersions (not supported by UMN Mapserver 6.0.3 - defaults to 1.1.1
  versions << "AcceptVersions=1.1.0,1.0.0" << "VERSION=1.0.0";

  foreach( QString v, versions )
  {
    if ( retrieveServerCapabilities( v ) )
    {
      return true;
    }
  }

  return false;
}

bool QgsWcsCapabilities::retrieveServerCapabilities( QString preferredVersion )
{
  clear();

  QString url = prepareUri( mUri.param( "url" ) ) + "SERVICE=WCS&REQUEST=GetCapabilities";

  if ( !preferredVersion.isEmpty() )
  {
    url += "&" + preferredVersion;
  }

  if ( ! sendRequest( url ) ) { return false; }

  QgsDebugMsg( "Converting to Dom." );

  bool domOK;
  domOK = parseCapabilitiesDom( mCapabilitiesResponse, mCapabilities );

  if ( !domOK )
  {
    // We had an Dom exception -
    // mErrorTitle and mError are pre-filled by parseCapabilitiesDom

    mError += tr( "\nTried URL: %1" ).arg( url );

    QgsDebugMsg( "!domOK: " + mError );

    return false;
  }

  return true;
}

bool QgsWcsCapabilities::describeCoverage( QString const &identifier, bool forceRefresh )
{
  QgsDebugMsg( " identifier = " + identifier );

  QgsWcsCoverageSummary *coverage = coverageSummary( identifier );
  if ( !coverage )
  {
    QgsDebugMsg( "coverage not found" );
    return false;
  }

  if ( coverage->described && ! forceRefresh ) return true;

  QString url = prepareUri( mUri.param( "url" ) ) + "SERVICE=WCS&REQUEST=DescribeCoverage&COVERAGE=" + coverage->identifier;
  url += "&VERSION=" + mVersion;

  if ( ! sendRequest( url ) ) { return false; }

  QgsDebugMsg( "Converting to Dom." );

  bool domOK = false;
  if ( mVersion.startsWith( "1.0" ) )
  {
    domOK = parseDescribeCoverageDom10( mCapabilitiesResponse, coverage );
  }
  else if ( mVersion.startsWith( "1.1" ) )
  {
    domOK = parseDescribeCoverageDom11( mCapabilitiesResponse, coverage );
  }

  if ( !domOK )
  {
    // We had an Dom exception -
    // mErrorTitle and mError are pre-filled by parseCapabilitiesDom

    mError += tr( "\nTried URL: %1" ).arg( url );

    QgsDebugMsg( "!domOK: " + mError );

    return false;
  }

  QgsDebugMsg( "supportedFormat = " + coverage->supportedFormat.join( "," ) );

  return true;
}

void QgsWcsCapabilities::capabilitiesReplyFinished()
{
  if ( mCapabilitiesReply->error() == QNetworkReply::NoError )
  {
    QVariant redirect = mCapabilitiesReply->attribute( QNetworkRequest::RedirectionTargetAttribute );
    if ( !redirect.isNull() )
    {
      emit statusChanged( tr( "Capabilities request redirected." ) );

      QNetworkRequest request( redirect.toUrl() );
      setAuthorization( request );
      request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork );
      request.setAttribute( QNetworkRequest::CacheSaveControlAttribute, true );

      mCapabilitiesReply->deleteLater();
      QgsDebugMsg( QString( "redirected getcapabilities: %1" ).arg( redirect.toString() ) );
      mCapabilitiesReply = QgsNetworkAccessManager::instance()->get( request );

      connect( mCapabilitiesReply, SIGNAL( finished() ), this, SLOT( capabilitiesReplyFinished() ) );
      connect( mCapabilitiesReply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( capabilitiesReplyProgress( qint64, qint64 ) ) );
      return;
    }

    mCapabilitiesResponse = mCapabilitiesReply->readAll();

    if ( mCapabilitiesResponse.isEmpty() )
    {
      mErrorFormat = "text/plain";
      mError = tr( "empty of capabilities: %1" ).arg( mCapabilitiesReply->errorString() );
    }
  }
  else
  {
    mErrorFormat = "text/plain";
    mError = tr( "Download of capabilities failed: %1" ).arg( mCapabilitiesReply->errorString() );
    QgsMessageLog::logMessage( mError, tr( "WCS" ) );
    mCapabilitiesResponse.clear();
  }

  mCapabilitiesReply->deleteLater();
  mCapabilitiesReply = 0;
}

void QgsWcsCapabilities::capabilitiesReplyProgress( qint64 bytesReceived, qint64 bytesTotal )
{
  QString msg = tr( "%1 of %2 bytes of capabilities downloaded." ).arg( bytesReceived ).arg( bytesTotal < 0 ? QString( "unknown number of" ) : QString::number( bytesTotal ) );
  QgsDebugMsg( msg );
  emit statusChanged( msg );
}

QString QgsWcsCapabilities::stripNS( const QString & name )
{
  return name.contains( ":" ) ? name.section( ':', 1 ) : name;
}

bool QgsWcsCapabilities::parseCapabilitiesDom( QByteArray const &xml, QgsWcsCapabilitiesProperty &capabilities )
{
  QgsDebugMsg( "Entered." );
#ifdef QGISDEBUG
  QFile file( QDir::tempPath() + "/qgis-wcs-capabilities.xml" );
  if ( file.open( QIODevice::WriteOnly ) )
  {
    file.write( xml );
    file.close();
  }
#endif

  if ( ! convertToDom( xml ) ) return false;

  QDomElement docElem = mCapabilitiesDom.documentElement();

  // Assert that the DTD is what we expected (i.e. a WCS Capabilities document)
  QgsDebugMsg( "testing tagName " + docElem.tagName() );

  QString tagName = stripNS( docElem.tagName() );
  if (
    // We don't support 1.0, but try WCS_Capabilities tag to get version
    tagName != "WCS_Capabilities" && // 1.0
    tagName != "Capabilities"  // 1.1, tags seen: Capabilities, wcs:Capabilities
  )
  {
    mErrorTitle = tr( "Dom Exception" );
    mErrorFormat = "text/plain";
    mError = tr( "Could not get WCS capabilities in the expected format (DTD): no %1 found.\nThis might be due to an incorrect WCS Server URL.\nTag:%3\nResponse was:\n%4" )
             .arg( "Capabilities" )
             .arg( docElem.tagName() )
             .arg( QString( xml ) );

    QgsLogger::debug( "Dom Exception: " + mError );

    return false;
  }

  capabilities.version = docElem.attribute( "version" );
  mVersion = capabilities.version;

  if ( !mVersion.startsWith( "1.0" ) && !mVersion.startsWith( "1.1" ) )
  {
    mErrorTitle = tr( "Version not supported" );
    mErrorFormat = "text/plain";
    mError = tr( "WCS server version %1 is not supported by Quantum GIS (supported versions: 1.0.0, 1.1.0, 1.1.2)" )
             .arg( mVersion );

    QgsLogger::debug( "WCS version: " + mError );

    return false;
  }

  // Start walking through XML.
  QDomNode n = docElem.firstChild();

  while ( !n.isNull() )
  {
    QDomElement e = n.toElement();
    if ( !e.isNull() )
    {
      // Version 1.0
      QString tagName = stripNS( e.tagName() );
      QgsDebugMsg( tagName );
      if ( tagName == "Service" )
      {
        parseService( e, capabilities.serviceIdentification );
      }
      else if ( tagName == "Capability" )
      {
        parseCapability( e, capabilities.operationsMetadata );
      }
      else if ( tagName == "ContentMetadata" )
      {
        parseContentMetadata( e, capabilities.contents );
      }
      // Version 1.1
      else if ( tagName == "ServiceIdentification" )
      {
        parseServiceIdentification( e, capabilities.serviceIdentification );
      }
      else if ( tagName == "OperationsMetadata" )
      {
        parseOperationsMetadata( e, capabilities.operationsMetadata );
      }
      else if ( tagName == "Contents" )
      {
        parseCoverageSummary( e, capabilities.contents );
      }
    }
    n = n.nextSibling();
  }

  return true;
}

QDomElement QgsWcsCapabilities::firstChild( const QDomElement &element, const QString & name )
{
  QDomNode n1 = element.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );
      if ( tagName == name )
      {
        QgsDebugMsg( name + " found" );
        return el;
      }
    }
    n1 = n1.nextSibling();
  }
  QgsDebugMsg( name + " not found" );
  return QDomElement();
}

QString QgsWcsCapabilities::firstChildText( const QDomElement &element, const QString & name )
{
  QDomElement el = firstChild( element, name );
  if ( !el.isNull() )
  {
    QgsDebugMsg( name + " : " + el.text() );
    return el.text();
  }
  return QString();
}

QList<QDomElement> QgsWcsCapabilities::domElements( const QDomElement &element, const QString & path )
{
  QList<QDomElement> list;

  QStringList names = path.split( "." );
  if ( names.size() == 0 ) return list;
  QString name = names.value( 0 );
  names.removeFirst();

  QDomNode n1 = element.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );
      if ( tagName == name )
      {
        QgsDebugMsg( name + " found" );
        if ( names.size() == 0 )
        {
          list.append( el );
        }
        else
        {
          list.append( domElements( el,  names.join( "." ) ) );
        }
      }
    }
    n1 = n1.nextSibling();
  }

  return list;
}

QDomElement QgsWcsCapabilities::domElement( const QDomElement &element, const QString & path )
{
  QStringList names = path.split( "." );
  if ( names.size() == 0 ) return QDomElement();

  QDomElement el = firstChild( element, names.value( 0 ) );
  if ( names.size() == 1 || el.isNull() )
  {
    return el;
  }
  names.removeFirst();
  return domElement( el, names.join( "." ) );
}

QString QgsWcsCapabilities::domElementText( const QDomElement &element, const QString & path )
{
  QDomElement el = domElement( element, path );
  return el.text();
}

QList<int> QgsWcsCapabilities::parseInts( const QString &text )
{
  QList<int> list;
  foreach( QString s, text.split( " " ) )
  {
    int i;
    bool ok;
    list.append( s.toInt( &ok ) );
    if ( !ok )
    {
      list.clear();
      return list;
    }
  }
  return list;
}

// ------------------------ 1.0 ----------------------------------------------
void QgsWcsCapabilities::parseService( const QDomElement &e, QgsWcsServiceIdentification &serviceIdentification ) // 1.0
{
  serviceIdentification.title = firstChildText( e, "name" );
  serviceIdentification.abstract = firstChildText( e, "description" );
  // 1.0 has also "label"
}

void QgsWcsCapabilities::parseCapability( QDomElement const & e,  QgsWcsOperationsMetadata &operationsMetadata )
{
  QDomElement requestElement = firstChild( e, "Request" );
  QDomElement getCoverageElement = firstChild( requestElement, "GetCoverage" );
  QDomElement dcpTypeElement = firstChild( getCoverageElement, "DCPType" );
  QDomElement httpElement = firstChild( dcpTypeElement, "HTTP" );
  QDomElement getElement = firstChild( httpElement, "Get" );
  QDomElement onlineResourceElement = firstChild( getElement, "OnlineResource" );

  operationsMetadata.getCoverage.dcp.http.get.xlinkHref = onlineResourceElement.attribute( "xlink:href" );

  QgsDebugMsg( "getCoverage.dcp.http.get.xlinkHref = " + operationsMetadata.getCoverage.dcp.http.get.xlinkHref );
}

void QgsWcsCapabilities::parseContentMetadata( QDomElement const & e, QgsWcsCoverageSummary &coverageSummary )
{
  QDomNode n1 = e.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );

      if ( tagName == "CoverageOfferingBrief" )
      {
        QgsWcsCoverageSummary subCoverageSummary;

        subCoverageSummary.described = false;
        subCoverageSummary.width = 0;
        subCoverageSummary.height = 0;
        subCoverageSummary.hasSize = false;

        parseCoverageOfferingBrief( el, subCoverageSummary, &coverageSummary );

        coverageSummary.coverageSummary.push_back( subCoverageSummary );
      }
    }
    n1 = n1.nextSibling();
  }
}

void QgsWcsCapabilities::parseCoverageOfferingBrief( QDomElement const & e, QgsWcsCoverageSummary &coverageSummary, QgsWcsCoverageSummary *parent )
{
  Q_UNUSED( parent );
  QgsDebugMsg( "Entered" );
  coverageSummary.orderId = ++mCoverageCount;

  coverageSummary.identifier = firstChildText( e, "name" );
  coverageSummary.title = firstChildText( e, "label" );
  coverageSummary.abstract = firstChildText( e, "description" );

  QDomElement lonLatEnvelopeElement = firstChild( e, "lonLatEnvelope" );

  QDomNodeList posNodes = lonLatEnvelopeElement.elementsByTagName( "gml:pos" );
  QList<double> lon, lat;
  for ( int i = 0; i < posNodes.size(); i++ )
  {
    QDomNode posNode = posNodes.at( i );
    QString lonLatText = posNode.toElement().text();
    QStringList lonLat = lonLatText.split( QRegExp( " +" ) );
    if ( lonLat.size() != 2 )
    {
      QgsDebugMsg( "Cannot parse lonLatEnvelope: " + lonLatText );
      continue;
    }
    double lo, la;
    bool loOk, laOk;
    lo = lonLat.value( 0 ).toDouble( &loOk );
    la = lonLat.value( 1 ).toDouble( &laOk );
    if ( loOk && laOk )
    {
      lon << lo;
      lat << la;
    }
    else
    {
      QgsDebugMsg( "Cannot parse lonLatEnvelope: " + lonLatText );
    }
  }
  if ( lon.size() == 2 )
  {
    double w, e, s, n;
    w =  qMin( lon[0], lon[1] );
    e =  qMax( lon[0], lon[1] );
    n =  qMax( lat[0], lat[1] );
    s =  qMin( lat[0], lat[1] );

    coverageSummary.wgs84BoundingBox = QgsRectangle( w, s, e, n );
    QgsDebugMsg( "wgs84BoundingBox = " + coverageSummary.wgs84BoundingBox.toString() );
  }

  if ( !coverageSummary.identifier.isEmpty() )
  {
    QgsDebugMsg( "add coverage " + coverageSummary.identifier + " to supported" );
    mCoveragesSupported.push_back( coverageSummary );
  }

  if ( !coverageSummary.coverageSummary.empty() )
  {
    mCoverageParentIdentifiers[ coverageSummary.orderId ] = QStringList() << coverageSummary.identifier << coverageSummary.title << coverageSummary.abstract;
  }
  QgsDebugMsg( QString( "coverage orderId = %1 identifier = %2" ).arg( coverageSummary.orderId ).arg( coverageSummary.identifier ) );
}

bool QgsWcsCapabilities::convertToDom( QByteArray const &xml )
{
  QgsDebugMsg( "Entered." );
  // Convert completed document into a Dom
  QString errorMsg;
  int errorLine;
  int errorColumn;
  bool contentSuccess = mCapabilitiesDom.setContent( xml, false, &errorMsg, &errorLine, &errorColumn );

  if ( !contentSuccess )
  {
    mErrorTitle = tr( "Dom Exception" );
    mErrorFormat = "text/plain";
    mError = tr( "Could not get WCS capabilities: %1 at line %2 column %3\nThis is probably due to an incorrect WCS Server URL.\nResponse was:\n\n%4" )
             .arg( errorMsg )
             .arg( errorLine )
             .arg( errorColumn )
             .arg( QString( xml ) );

    QgsLogger::debug( "Dom Exception: " + mError );

    return false;
  }
  return true;
}

bool QgsWcsCapabilities::parseDescribeCoverageDom10( QByteArray const &xml, QgsWcsCoverageSummary *coverage )
{
  QgsDebugMsg( "coverage->identifier = " + coverage->identifier );
  if ( ! convertToDom( xml ) ) return false;

  QDomElement docElem = mCapabilitiesDom.documentElement();

  QgsDebugMsg( "testing tagName " + docElem.tagName() );

  QString tagName = stripNS( docElem.tagName() );
  if ( tagName != "CoverageDescription" )
  {
    mErrorTitle = tr( "Dom Exception" );
    mErrorFormat = "text/plain";
    mError = tr( "Could not get WCS capabilities in the expected format (DTD): no %1 found.\nThis might be due to an incorrect WCS Server URL.\nTag:%3\nResponse was:\n%4" )
             .arg( "CoverageDescription" )
             .arg( docElem.tagName() )
             .arg( QString( xml ) );

    QgsLogger::debug( "Dom Exception: " + mError );

    return false;
  }

  QDomElement coverageOfferingElement = firstChild( docElem, "CoverageOffering" );

  if ( coverageOfferingElement.isNull() ) return false;
  QDomElement supportedCRSsElement = firstChild( coverageOfferingElement, "supportedCRSs" );

  QDomNode n1 = supportedCRSsElement.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );

      if ( tagName == "requestResponseCRSs" )
      {
        coverage->supportedCrs << el.text();
      }
    }
    n1 = n1.nextSibling();
  }
  QgsDebugMsg( "supportedCrs = " + coverage->supportedCrs.join( "," ) );

  QDomElement supportedFormatsElement = firstChild( coverageOfferingElement, "supportedFormats" );

  n1 = supportedFormatsElement.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );

      if ( tagName == "formats" )
      {
        // may be GTiff, GeoTIFF, TIFF, GIF, ....
        coverage->supportedFormat << el.text();
      }
    }
    n1 = n1.nextSibling();
  }
  QgsDebugMsg( "supportedFormat = " + coverage->supportedFormat.join( "," ) );

  // spatialDomain and Grid/RectifiedGrid are optional according to specificationi.
  // If missing, we cannot get native resolution and size.
  QDomElement gridElement = domElement( coverageOfferingElement, "domainSet.spatialDomain.Grid" );

  if ( gridElement.isNull() )
  {
    gridElement = domElement( coverageOfferingElement, "domainSet.spatialDomain.RectifiedGrid" );
  }

  if ( !gridElement.isNull() )
  {
    QList<int> low = parseInts( domElementText( gridElement, "limits.GridEnvelope.low" ) );
    QList<int> high = parseInts( domElementText( gridElement, "limits.GridEnvelope.high" ) );
    if ( low.size() == 2 && high.size() == 2 )
    {
      coverage->width = high[0] - low[0] + 1;
      coverage->height = high[1] - low[1] + 1;
      coverage->hasSize = true;
    }
    // RectifiedGrid has also gml:origin which we dont need I think (attention however
    // it should contain gml:Point but mapserver 6.0.3 / WCS 1.0.0 is using gml:pos instead)
    // RectifiedGrid also contains 2 gml:offsetVector which could be used to get resolution
    // but it should be sufficient to calc resolution from size
  }
  QgsDebugMsg( QString( "width = %1 height = %2" ).arg( coverage->width ).arg( coverage->height ) );

  coverage->described = true;

  return true;
}
// ------------------------ 1.1 ----------------------------------------------
bool QgsWcsCapabilities::parseDescribeCoverageDom11( QByteArray const &xml, QgsWcsCoverageSummary *coverage )
{
  QgsDebugMsg( "coverage->identifier = " + coverage->identifier );
  if ( ! convertToDom( xml ) ) return false;

  QDomElement docElem = mCapabilitiesDom.documentElement();

  QgsDebugMsg( "testing tagName " + docElem.tagName() );

  QString tagName = stripNS( docElem.tagName() );
  if ( tagName != "CoverageDescriptions" )
  {
    mErrorTitle = tr( "Dom Exception" );
    mErrorFormat = "text/plain";
    mError = tr( "Could not get WCS capabilities in the expected format (DTD): no %1 found.\nThis might be due to an incorrect WCS Server URL.\nTag:%3\nResponse was:\n%4" )
             .arg( "CoverageDescriptions" )
             .arg( docElem.tagName() )
             .arg( QString( xml ) );

    QgsLogger::debug( "Dom Exception: " + mError );

    return false;
  }

  // Get image size, we can get it from BoundingBox with crs=urn:ogc:def:crs:OGC::imageCRS
  // but while at least one BoundingBox is mandatory, it does not have to be urn:ogc:def:crs:OGC::imageCRS
  // TODO: if BoundingBox with crs=urn:ogc:def:crs:OGC::imageCRS is not found,
  // we could calculate image size from GridCRS.GridOffsets (if available)
  QList<QDomElement> boundingBoxElements = domElements( docElem, "CoverageDescription.Domain.SpatialDomain.BoundingBox" );

  QgsDebugMsg( QString( "%1 BoundingBox found" ).arg( boundingBoxElements.size() ) );

  foreach( QDomElement el, boundingBoxElements )
  {
    if ( el.attribute( "crs" ) != "urn:ogc:def:crs:OGC::imageCRS" ) continue;

    QList<int> low = parseInts( domElementText( el, "LowerCorner" ) );
    QList<int> high = parseInts( domElementText( el, "UpperCorner" ) );
    if ( low.size() == 2 && high.size() == 2 )
    {
      coverage->width = high[0] - low[0] + 1;
      coverage->height = high[1] - low[1] + 1;
      coverage->hasSize = true;
    }
    break;
  }
  QgsDebugMsg( QString( "width = %1 height = %2" ).arg( coverage->width ).arg( coverage->height ) );

  coverage->described = true;

  return true;
}
void QgsWcsCapabilities::parseServiceIdentification( const QDomElement &e, QgsWcsServiceIdentification &serviceIdentification ) // 1.1
{
  serviceIdentification.title = firstChildText( e, "Title" );
  serviceIdentification.abstract = firstChildText( e, "Abstract" );
}

void QgsWcsCapabilities::parseHttp( QDomElement const & e, QgsWcsHTTP& http )
{
  http.get.xlinkHref = firstChild( e, "Get" ).attribute( "xlink:href" );
  QgsDebugMsg( "http.get.xlinkHref = " + http.get.xlinkHref );
}

void QgsWcsCapabilities::parseDcp( QDomElement const & e, QgsWcsDCP& dcp )
{
  QDomElement el = firstChild( e, "HTTP" );
  parseHttp( el, dcp.http );
}

void QgsWcsCapabilities::parseOperation( QDomElement const & e, QgsWcsOperation& operation )
{
  QDomElement el = firstChild( e, "DCP" );
  parseDcp( el, operation.dcp );
}

void QgsWcsCapabilities::parseOperationsMetadata( QDomElement const & e,  QgsWcsOperationsMetadata &operationsMetadata )
{
  QDomNode n1 = e.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );

      if ( tagName == "Operation" && el.attribute( "name" ) == "GetCoverage" )
      {
        parseOperation( el, operationsMetadata.getCoverage );
      }
    }
    n1 = n1.nextSibling();
  }
}

void QgsWcsCapabilities::parseCoverageSummary( QDomElement const & e, QgsWcsCoverageSummary &coverageSummary, QgsWcsCoverageSummary *parent )
{
  coverageSummary.orderId = ++mCoverageCount;

  coverageSummary.identifier = firstChildText( e, "Identifier" );
  coverageSummary.title = firstChildText( e, "Title" );
  coverageSummary.abstract = firstChildText( e, "Abstract" );

  QDomNode n1 = e.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );
      QgsDebugMsg( tagName + " : " + el.text() );

      if ( tagName == "SupportedFormat" )
      {
        // image/tiff, ...
        coverageSummary.supportedFormat << el.text();
      }
      else if ( tagName == "SupportedCRS" )
      {
        // TODO: SupportedCRS may be URL referencing a document
        // URN format: urn:ogc:def:objectType:authority:version:code
        // URN example: urn:ogc:def:crs:EPSG::4326
        QStringList urn = el.text().split( ":" );
        if ( urn.size() == 7 )
        {
          coverageSummary.supportedCrs << urn.value( 4 ) + ":" + urn.value( 6 );
        }
      }
      else if ( tagName == "WGS84BoundingBox" )
      {
        QDomElement wBoundLongitudeElem, eBoundLongitudeElem, sBoundLatitudeElem, nBoundLatitudeElem;

        QStringList lower = n1.namedItem( "ows:LowerCorner" ).toElement().text().split( QRegExp( " +" ) );
        QStringList upper = n1.namedItem( "ows:UpperCorner" ).toElement().text().split( QRegExp( " +" ) );

        double w, e, s, n;
        bool wOk, eOk, sOk, nOk;
        w = lower.value( 0 ).toDouble( &wOk );
        s = lower.value( 1 ).toDouble( &sOk );
        e = upper.value( 0 ).toDouble( &eOk );
        n = upper.value( 1 ).toDouble( &nOk );
        if ( wOk && eOk && sOk && nOk )
        {
          coverageSummary.wgs84BoundingBox = QgsRectangle( w, s, e, n );
        }
      }
    }
    n1 = n1.nextSibling();
  }

  // We collected params to be inherited, do children
  n1 = e.firstChild();
  while ( !n1.isNull() )
  {
    QDomElement el = n1.toElement();
    if ( !el.isNull() )
    {
      QString tagName = stripNS( el.tagName() );

      if ( tagName == "CoverageSummary" )
      {
        QgsDebugMsg( "      Nested coverage." );

        QgsWcsCoverageSummary subCoverageSummary;

        subCoverageSummary.described = false;
        subCoverageSummary.width = 0;
        subCoverageSummary.height = 0;
        subCoverageSummary.hasSize = false;

        // Inherit
        subCoverageSummary.supportedCrs = coverageSummary.supportedCrs;
        subCoverageSummary.supportedFormat = coverageSummary.supportedFormat;

        parseCoverageSummary( el, subCoverageSummary, &coverageSummary );

        coverageSummary.coverageSummary.push_back( subCoverageSummary );
      }
    }
    n1 = n1.nextSibling();
  }

  if ( parent && parent->orderId > 1 ) // ignore Contents to put them on top level
  {
    QgsDebugMsg( QString( "coverage orderId = %1 identifier = %2 has parent %3" ).arg( coverageSummary.orderId ).arg( coverageSummary.identifier ).arg( parent->orderId ) );
    mCoverageParents[ coverageSummary.orderId ] = parent->orderId;
  }

  if ( !coverageSummary.identifier.isEmpty() )
  {
    QgsDebugMsg( "add coverage " + coverageSummary.identifier + " to supported" );
    mCoveragesSupported.push_back( coverageSummary );
  }

  if ( !coverageSummary.coverageSummary.empty() )
  {
    mCoverageParentIdentifiers[ coverageSummary.orderId ] = QStringList() << coverageSummary.identifier << coverageSummary.title << coverageSummary.abstract;
  }
  QgsDebugMsg( QString( "coverage orderId = %1 identifier = %2" ).arg( coverageSummary.orderId ).arg( coverageSummary.identifier ) );

}

void QgsWcsCapabilities::coverageParents( QMap<int, int> &parents, QMap<int, QStringList> &parentNames ) const
{
  parents = mCoverageParents;
  parentNames = mCoverageParentIdentifiers;
}

QString QgsWcsCapabilities::lastErrorTitle()
{
  return mErrorTitle;
}

QString QgsWcsCapabilities::lastError()
{
  QgsDebugMsg( "returning '" + mError  + "'." );
  return mError;
}

QString QgsWcsCapabilities::lastErrorFormat()
{
  return mErrorFormat;
}

void QgsWcsCapabilities::setAuthorization( QNetworkRequest &request ) const
{
  QgsDebugMsg( "entered" );
  if ( mUri.hasParam( "username" ) && mUri.hasParam( "password" ) )
  {
    QgsDebugMsg( "setAuthorization " + mUri.param( "username" ) );
    request.setRawHeader( "Authorization", "Basic " + QString( "%1:%2" ).arg( mUri.param( "username" ) ).arg( mUri.param( "password" ) ).toAscii().toBase64() );
  }
}

void QgsWcsCapabilities::showMessageBox( const QString& title, const QString& text )
{
  QgsMessageOutput *message = QgsMessageOutput::createMessageOutput();
  message->setTitle( title );
  message->setMessage( text, QgsMessageOutput::MessageText );
  message->showMessage();
}

QgsWcsCoverageSummary* QgsWcsCapabilities::coverageSummary( QString const & theIdentifier, QgsWcsCoverageSummary* parent )
{
  //QgsDebugMsg( "theIdentifier = " + theIdentifier );
  if ( !parent )
  {
    parent = &( mCapabilities.contents );
  }

  //foreach( const QgsWcsCoverageSummary &c, parent->coverageSummary )
  for ( QVector<QgsWcsCoverageSummary>::iterator c = parent->coverageSummary.begin(); c != parent->coverageSummary.end(); ++c )
  {
    //QgsDebugMsg( "c->identifier = " + c->identifier );
    if ( c->identifier == theIdentifier )
    {
      return c;
    }
    else
    {
      // search sub coverages
      QgsWcsCoverageSummary * sc = coverageSummary( theIdentifier, c );
      if ( sc )
      {
        return sc;
      }
    }
  }
  return 0;
}
