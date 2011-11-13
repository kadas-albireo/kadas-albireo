/***************************************************************************
                              qgshttprequesthandler.cpp
                              -------------------------
  begin                : June 29, 2007
  copyright            : (C) 2007 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgshttprequesthandler.h"
#include "qgsftptransaction.h"
#include "qgshttptransaction.h"
#include "qgslogger.h"
#include "qgsmapserviceexception.h"
#include <QBuffer>
#include <QByteArray>
#include <QDomDocument>
#include <QFile>
#include <QImage>
#include <QTextStream>
#include <QStringList>
#include <QUrl>
#include <fcgi_stdio.h>

QgsHttpRequestHandler::QgsHttpRequestHandler(): QgsRequestHandler()
{

}

QgsHttpRequestHandler::~QgsHttpRequestHandler()
{

}

void QgsHttpRequestHandler::sendHttpResponse( QByteArray* ba, const QString& format ) const
{
  if ( !ba )
  {
    return;
  }

  if ( ba->size() < 1 )
  {
    return;
  }

  printf( "Content-Type: " );
  printf( format.toLocal8Bit() );
  printf( "\n" );
  printf( "Content-Length: %d\n", ba->size() );
  printf( "\n" );
  fwrite( ba->data(), ba->size(), 1, FCGI_stdout );
}

QString QgsHttpRequestHandler::formatToMimeType( const QString& format ) const
{
  if ( format.compare( "png", Qt::CaseInsensitive ) == 0 )
  {
    return "image/png";
  }
  else if ( format.compare( "jpg", Qt::CaseInsensitive ) == 0 )
  {
    return "image/jpeg";
  }
  else if ( format.compare( "svg", Qt::CaseInsensitive ) == 0 )
  {
    return "image/svg+xml";
  }
  else if ( format.compare( "pdf", Qt::CaseInsensitive ) == 0 )
  {
    return "application/pdf";
  }
  return format;
}

void QgsHttpRequestHandler::sendGetMapResponse( const QString& service, QImage* img ) const
{
  Q_UNUSED( service );
  if ( img )
  {
    if ( mFormat != "PNG" && mFormat != "JPG" )
    {
      sendServiceException( QgsMapServiceException( "InvalidFormat", "Output format '" + mFormat + "' is not supported in the GetMap request" ) );
      return;
    }

    //store the image in a QByteArray and send it directly
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    img->save( &buffer, mFormat.toLocal8Bit().data(), -1 );

    sendHttpResponse( &ba, formatToMimeType( mFormat ) );
  }
}

void QgsHttpRequestHandler::sendGetCapabilitiesResponse( const QDomDocument& doc ) const
{
  QByteArray ba = doc.toByteArray();
  sendHttpResponse( &ba, "text/xml" );
}

void QgsHttpRequestHandler::sendGetStyleResponse( const QDomDocument& doc ) const
{
  QByteArray ba = doc.toByteArray();
  sendHttpResponse( &ba, "text/xml" );
}

void QgsHttpRequestHandler::sendGetFeatureInfoResponse( const QDomDocument& infoDoc, const QString& infoFormat ) const
{
  QByteArray ba;
  QgsDebugMsg( "Info format is:" + infoFormat );

  if ( infoFormat == "text/xml" )
  {
    ba = infoDoc.toByteArray();
  }
  else if ( infoFormat == "text/plain" || infoFormat == "text/html" )
  {
    //create string
    QString featureInfoString;

    if ( infoFormat == "text/plain" )
    {
      featureInfoString.append( "GetFeatureInfo results\n" );
      featureInfoString.append( "\n" );
    }
    else if ( infoFormat == "text/html" )
    {
      featureInfoString.append( "<HEAD>\n" );
      featureInfoString.append( "<TITLE> GetFeatureInfo results </TITLE>\n" );
      featureInfoString.append( "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\n" );
      featureInfoString.append( "</HEAD>\n" );
      featureInfoString.append( "<BODY>\n" );
    }

    QDomNodeList layerList = infoDoc.elementsByTagName( "Layer" );

    //layer loop
    for ( int i = 0; i < layerList.size(); ++i )
    {
      QDomElement layerElem = layerList.at( i ).toElement();
      if ( infoFormat == "text/plain" )
      {
        featureInfoString.append( "Layer '" + layerElem.attribute( "name" ) + "'\n" );
      }
      else if ( infoFormat == "text/html" )
      {
        featureInfoString.append( "<TABLE border=1 width=100%>\n" );
        featureInfoString.append( "<TR><TH width=25%>Layer</TH><TD>" + layerElem.attribute( "name" ) + "</TD></TR>\n" );
        featureInfoString.append( "</BR>" );
      }

      //feature loop (for vector layers)
      QDomNodeList featureNodeList = layerElem.elementsByTagName( "Feature" );
      QDomElement currentFeatureElement;

      if ( featureNodeList.size() < 1 ) //raster layer?
      {
        QDomNodeList attributeNodeList = layerElem.elementsByTagName( "Attribute" );
        for ( int j = 0; j < attributeNodeList.size(); ++j )
        {
          QDomElement attributeElement = attributeNodeList.at( j ).toElement();
          if ( infoFormat == "text/plain" )
          {
            featureInfoString.append( attributeElement.attribute( "name" ) + " = '" +
                                      attributeElement.attribute( "value" ) + "'\n" );
          }
          else if ( infoFormat == "text/html" )
          {
            featureInfoString.append( "<TR><TH>" + attributeElement.attribute( "name" ) + "</TH><TD>" +
                                      attributeElement.attribute( "value" ) + "</TD></TR>\n" );
          }
        }
      }
      else //vector layer
      {
        for ( int j = 0; j < featureNodeList.size(); ++j )
        {
          QDomElement featureElement = featureNodeList.at( j ).toElement();
          if ( infoFormat == "text/plain" )
          {
            featureInfoString.append( "Feature " + featureElement.attribute( "id" ) + "\n" );
          }
          else if ( infoFormat == "text/html" )
          {
            featureInfoString.append( "<TABLE border=1 width=100%>\n" );
            featureInfoString.append( "<TR><TH>Feature</TH><TD>" + featureElement.attribute( "id" ) + "</TD></TR>\n" );
          }
          //attribute loop
          QDomNodeList attributeNodeList = featureElement.elementsByTagName( "Attribute" );
          for ( int k = 0; k < attributeNodeList.size(); ++k )
          {
            QDomElement attributeElement = attributeNodeList.at( k ).toElement();
            if ( infoFormat == "text/plain" )
            {
              featureInfoString.append( attributeElement.attribute( "name" ) + " = '" +
                                        attributeElement.attribute( "value" ) + "'\n" );
            }
            else if ( infoFormat == "text/html" )
            {
              featureInfoString.append( "<TR><TH>" + attributeElement.attribute( "name" ) + "</TH><TD>" + attributeElement.attribute( "value" ) + "</TD></TR>\n" );
            }
          }

          if ( infoFormat == "text/html" )
          {
            featureInfoString.append( "</TABLE>\n</BR>\n" );
          }
        }
      }
      if ( infoFormat == "text/plain" )
      {
        featureInfoString.append( "\n" );
      }
      else if ( infoFormat == "text/html" )
      {
        featureInfoString.append( "</TABLE>\n<BR></BR>\n" );

      }
    }
    if ( infoFormat == "text/html" )
    {
      featureInfoString.append( "</BODY>\n" );
    }
    ba = featureInfoString.toUtf8();
  }
  else //unsupported format, send exception
  {
    sendServiceException( QgsMapServiceException( "InvalidFormat", "Feature info format '" + mFormat + "' is not supported. Possibilities are 'text/plain', 'text/html' or 'text/xml'." ) );
    return;
  }

  sendHttpResponse( &ba, infoFormat );
}

void QgsHttpRequestHandler::sendServiceException( const QgsMapServiceException& ex ) const
{
  //create Exception DOM document
  QDomDocument exceptionDoc;
  QDomElement serviceExceptionReportElem = exceptionDoc.createElement( "ServiceExceptionReport" );
  serviceExceptionReportElem.setAttribute( "version", "1.3.0" );
  serviceExceptionReportElem.setAttribute( "xmlns", "http://www.opengis.net/ogc" );
  exceptionDoc.appendChild( serviceExceptionReportElem );
  QDomElement serviceExceptionElem = exceptionDoc.createElement( "ServiceException" );
  serviceExceptionElem.setAttribute( "code", ex.code() );
  QDomText messageText = exceptionDoc.createTextNode( ex.message() );
  serviceExceptionElem.appendChild( messageText );
  serviceExceptionReportElem.appendChild( serviceExceptionElem );

  QByteArray ba = exceptionDoc.toByteArray();
  sendHttpResponse( &ba, "text/xml" );
}

void QgsHttpRequestHandler::sendGetPrintResponse( QByteArray* ba ) const
{
  sendHttpResponse( ba, formatToMimeType( mFormat ) );
}

void QgsHttpRequestHandler::requestStringToParameterMap( const QString& request, std::map<QString, QString>& parameters )
{
  parameters.clear();

  //parameters are separated by &
  QStringList elements = request.split( "&" );

  QString element, key, value;

  //insert key and value into the map
  for ( QStringList::const_iterator it = elements.begin(); it != elements.end(); ++it )
  {
    element = *it;
    int sepidx = element.indexOf( "=", 0, Qt::CaseSensitive );
    if ( sepidx == -1 )
    {
      continue;
    }

    key = element.left( sepidx );
    value = element.mid( sepidx + 1 );
    value.replace( "+", " " );
    value = QUrl::fromPercentEncoding( value.toLocal8Bit() ); //replace encoded special caracters and utf-8 encodings


    if ( key.compare( "SLD_BODY", Qt::CaseInsensitive ) == 0 )
    {
      key = "SLD";
    }
    else if ( key.compare( "SLD", Qt::CaseInsensitive ) == 0 )
    {
      QByteArray fileContents;
      if ( value.startsWith( "http", Qt::CaseInsensitive ) )
      {
        QgsHttpTransaction http( value );
        if ( !http.getSynchronously( fileContents ) )
        {
          continue;
        }
      }
      else if ( value.startsWith( "ftp", Qt::CaseInsensitive ) )
      {
        QgsFtpTransaction ftp;
        if ( !ftp.get( value, fileContents ) )
        {
          continue;
        }
        value = QUrl::fromPercentEncoding( fileContents );
      }
      else
      {
        continue; //only http and ftp supported at the moment
      }
      value = QUrl::fromPercentEncoding( fileContents );

    }
    parameters.insert( std::make_pair( key.toUpper(), value ) );
    QgsDebugMsg( "inserting pair " + key.toUpper() + " // " + value + " into the parameter map" );
  }

  //feature info format?
  std::map<QString, QString>::const_iterator info_format_it = parameters.find( "INFO_FORMAT" );
  if ( info_format_it != parameters.end() )
  {
    mFormat = info_format_it->second;
  }
  else //capabilities format or GetMap format
  {
    std::map<QString, QString>::const_iterator formatIt = parameters.find( "FORMAT" );
    if ( formatIt != parameters.end() )
    {
      QString formatString = formatIt->second;

      QgsDebugMsg( QString( "formatString is: %1" ).arg( formatString ) );

      //remove the image/ in front of the format
      if ( formatString.compare( "image/png", Qt::CaseInsensitive ) == 0 || formatString.compare( "png", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "PNG";
      }
      else if ( formatString.compare( "image/jpeg", Qt::CaseInsensitive ) == 0 || formatString.compare( "image/jpg", Qt::CaseInsensitive ) == 0
                || formatString.compare( "jpg", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "JPG";
      }
      else if ( formatString.compare( "svg", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "SVG";
      }
      else if ( formatString.compare( "pdf", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "PDF";
      }

      mFormat = formatString;
    }
  }
}

QString QgsHttpRequestHandler::readPostBody() const
{
  char* lengthString = NULL;
  int length = 0;
  char* input = NULL;
  QString inputString;
  QString lengthQString;

  lengthString = getenv( "CONTENT_LENGTH" );
  if ( lengthString != NULL )
  {
    bool conversionSuccess = false;
    lengthQString = QString( lengthString );
    length = lengthQString.toInt( &conversionSuccess );
    QgsDebugMsg( "length is: " + lengthQString );
    if ( conversionSuccess )
    {
      input = ( char* )malloc( length + 1 );
      memset( input, 0, length + 1 );
      for ( int i = 0; i < length; ++i )
      {
        input[i] = getchar();
      }
      //fgets(input, length+1, stdin);
      if ( input != NULL )
      {
        inputString = QString::fromLocal8Bit( input );
      }
      else
      {
        QgsDebugMsg( "input is NULL " );
      }
      free( input );
    }
    else
    {
      QgsDebugMsg( "could not convert CONTENT_LENGTH to int" );
    }
  }
  return inputString;
}
