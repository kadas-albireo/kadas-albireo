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
  QgsDebugMsg( "Checking byte array is ok to send..." );
  if ( !ba )
  {
    return;
  }

  if ( ba->size() < 1 )
  {
    return;
  }

  QgsDebugMsg( "Byte array looks good, returning response..." );
  QgsDebugMsg( QString( "Content size: %1" ).arg( ba->size() ) );
  QgsDebugMsg( QString( "Content format: %1" ).arg( format ) );
  printf( "Content-Type: " );
  printf( format.toLocal8Bit() );
  printf( "\n" );
  printf( "Content-Length: %d\n", ba->size() );
  printf( "\n" );
  int result = fwrite( ba->data(), ba->size(), 1, FCGI_stdout );
#ifdef QGISDEBUG
  QgsDebugMsg( QString( "Sent %1 bytes" ).arg( result ) );
#else
  Q_UNUSED( result );
#endif
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
  QgsDebugMsg( "Sending getmap response..." );
  if ( img )
  {
    bool png16Bit = ( mFormatString.compare( "image/png; mode=16bit", Qt::CaseInsensitive ) == 0 );
    bool png8Bit = ( mFormatString.compare( "image/png; mode=8bit", Qt::CaseInsensitive ) == 0 );
    bool png1Bit = ( mFormatString.compare( "image/png; mode=1bit", Qt::CaseInsensitive ) == 0 );
    bool isBase64 = mFormatString.endsWith( ";base64", Qt::CaseInsensitive );
    if ( mFormat != "PNG" && mFormat != "JPG" && !png16Bit && !png8Bit && !png1Bit )
    {
      QgsDebugMsg( "service exception - incorrect image format requested..." );
      sendServiceException( QgsMapServiceException( "InvalidFormat", "Output format '" + mFormatString + "' is not supported in the GetMap request" ) );
      return;
    }

    //store the image in a QByteArray and send it directly
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );

    if ( png8Bit )
    {
      QVector<QRgb> colorTable;
      medianCut( colorTable, 256, *img );
      QImage palettedImg = img->convertToFormat( QImage::Format_Indexed8, colorTable, Qt::ColorOnly | Qt::ThresholdDither |
                           Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection );
      palettedImg.save( &buffer, "PNG", -1 );
    }
    else if ( png16Bit )
    {
      QImage palettedImg = img->convertToFormat( QImage::Format_ARGB4444_Premultiplied );
      palettedImg.save( &buffer, "PNG", -1 );
    }
    else if ( png1Bit )
    {
      QImage palettedImg = img->convertToFormat( QImage::Format_Mono, Qt::MonoOnly | Qt::ThresholdDither |
                           Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection );
      palettedImg.save( &buffer, "PNG", -1 );
    }
    else
    {
      img->save( &buffer, mFormat.toLocal8Bit().data(), -1 );
    }

    if ( isBase64 )
    {
      ba = ba.toBase64();
    }

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
  if ( mFormatString.endsWith( ";base64", Qt::CaseInsensitive ) )
  {
    *ba = ba->toBase64();
  }
  sendHttpResponse( ba, formatToMimeType( mFormat ) );
}

bool QgsHttpRequestHandler::startGetFeatureResponse( QByteArray* ba, const QString& infoFormat ) const
{
  if ( !ba )
  {
    return false;
  }

  if ( ba->size() < 1 )
  {
    return false;
  }

  QString format;
  if ( infoFormat == "GeoJSON" )
    format = "text/plain";
  else
    format = "text/xml";

  printf( "Content-Type: " );
  printf( format.toLocal8Bit() );
  printf( "\n" );
  printf( "\n" );
  fwrite( ba->data(), ba->size(), 1, FCGI_stdout );
  return true;
}

void QgsHttpRequestHandler::sendGetFeatureResponse( QByteArray* ba ) const
{
  if ( !ba )
  {
    return;
  }

  if ( ba->size() < 1 )
  {
    return;
  }
  fwrite( ba->data(), ba->size(), 1, FCGI_stdout );
}

void QgsHttpRequestHandler::endGetFeatureResponse( QByteArray* ba ) const
{
  if ( !ba )
  {
    return;
  }

  fwrite( ba->data(), ba->size(), 1, FCGI_stdout );
}

void QgsHttpRequestHandler::requestStringToParameterMap( const QString& request, QMap<QString, QString>& parameters )
{
  parameters.clear();


  //insert key and value into the map (parameters are separated by &
  foreach ( QString element, request.split( "&" ) )
  {
    int sepidx = element.indexOf( "=", 0, Qt::CaseSensitive );
    if ( sepidx == -1 )
    {
      continue;
    }

    QString key = element.left( sepidx );
    QString value = element.mid( sepidx + 1 );
    value.replace( "+", " " );
    value = QUrl::fromPercentEncoding( value.toLocal8Bit() ); //replace encoded special caracters and utf-8 encodings
    key = QUrl::fromPercentEncoding( key.toLocal8Bit() ); //replace encoded special caracters and utf-8 encodings

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
    parameters.insert( key.toUpper(), value );
    QgsDebugMsg( "inserting pair " + key.toUpper() + " // " + value + " into the parameter map" );
  }

  //feature info format?
  QString infoFormat = parameters.value( "INFO_FORMAT" );
  if ( !infoFormat.isEmpty() )
  {
    mFormat = infoFormat;
  }
  else //capabilities format or GetMap format
  {
    mFormatString = parameters.value( "FORMAT" );
    QString formatString = mFormatString;
    if ( !formatString.isEmpty() )
    {
      QgsDebugMsg( QString( "formatString is: %1" ).arg( formatString ) );

      //remove the image/ in front of the format
      if ( formatString.contains( "image/png", Qt::CaseInsensitive ) || formatString.compare( "png", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "PNG";
      }
      else if ( formatString.contains( "image/jpeg", Qt::CaseInsensitive ) || formatString.contains( "image/jpg", Qt::CaseInsensitive )
                || formatString.compare( "jpg", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "JPG";
      }
      else if ( formatString.compare( "svg", Qt::CaseInsensitive ) == 0 )
      {
        formatString = "SVG";
      }
      else if ( formatString.contains( "pdf", Qt::CaseInsensitive ) )
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

void QgsHttpRequestHandler::medianCut( QVector<QRgb>& colorTable, int nColors, const QImage& inputImage )
{
  QHash<QRgb, int> inputColors;
  imageColors( inputColors, inputImage );

  if ( inputColors.size() <= nColors ) //all the colors in the image can be mapped to one palette color
  {
    colorTable.resize( inputColors.size() );
    int index = 0;
    QHash<QRgb, int>::const_iterator inputColorIt = inputColors.constBegin();
    for ( ; inputColorIt != inputColors.constEnd(); ++inputColorIt )
    {
      colorTable[index] = inputColorIt.key();
      ++index;
    }
    return;
  }

  //create first box
  QgsColorBox firstBox; //QList< QPair<QRgb, int> >
  int firstBoxPixelSum = 0;
  QHash<QRgb, int>::const_iterator inputColorIt = inputColors.constBegin();
  for ( ; inputColorIt != inputColors.constEnd(); ++inputColorIt )
  {
    firstBox.push_back( qMakePair( inputColorIt.key(), inputColorIt.value() ) );
    firstBoxPixelSum += inputColorIt.value();
  }

  QgsColorBoxMap colorBoxMap; //QMultiMap< int, ColorBox >
  colorBoxMap.insert( firstBoxPixelSum, firstBox );
  QMap<int, QgsColorBox>::iterator colorBoxMapIt = colorBoxMap.end();

  //split boxes until number of boxes == nColors or all the boxes have color count 1
  bool allColorsMapped = false;
  while ( colorBoxMap.size() < nColors )
  {
    //start at the end of colorBoxMap and pick the first entry with number of colors < 1
    colorBoxMapIt = colorBoxMap.end();
    while ( true )
    {
      --colorBoxMapIt;
      if ( colorBoxMapIt.value().size() > 1 )
      {
        splitColorBox( colorBoxMapIt.value(), colorBoxMap, colorBoxMapIt );
        break;
      }
      if ( colorBoxMapIt == colorBoxMap.begin() )
      {
        allColorsMapped = true;
        break;
      }
    }

    if ( allColorsMapped )
    {
      break;
    }
    else
    {
      continue;
    }
  }

  //get representative colors for the boxes
  int index = 0;
  colorTable.resize( colorBoxMap.size() );
  QgsColorBoxMap::const_iterator colorBoxIt = colorBoxMap.constBegin();
  for ( ; colorBoxIt != colorBoxMap.constEnd(); ++colorBoxIt )
  {
    colorTable[index] = boxColor( colorBoxIt.value(), colorBoxIt.key() );
    ++index;
  }
}

void QgsHttpRequestHandler::imageColors( QHash<QRgb, int>& colors, const QImage& image )
{
  colors.clear();
  int width = image.width();
  int height = image.height();

  const QRgb* currentScanLine = 0;
  QHash<QRgb, int>::iterator colorIt;
  for ( int i = 0; i < height; ++i )
  {
    currentScanLine = ( const QRgb* )( image.scanLine( i ) );
    for ( int j = 0; j < width; ++j )
    {
      colorIt = colors.find( currentScanLine[j] );
      if ( colorIt == colors.end() )
      {
        colors.insert( currentScanLine[j], 1 );
      }
      else
      {
        colorIt.value()++;
      }
    }
  }
}

void QgsHttpRequestHandler::splitColorBox( QgsColorBox& colorBox, QgsColorBoxMap& colorBoxMap,
    QMap<int, QgsColorBox>::iterator colorBoxMapIt )
{

  if ( colorBox.size() < 2 )
  {
    return; //need at least two colors for a split
  }

  //a,r,g,b ranges
  int redRange = 0;
  int greenRange = 0;
  int blueRange = 0;
  int alphaRange = 0;

  if ( !minMaxRange( colorBox, redRange, greenRange, blueRange, alphaRange ) )
  {
    return;
  }

  //sort color box for a/r/g/b
  if ( redRange >= greenRange && redRange >= blueRange && redRange >= alphaRange )
  {
    qSort( colorBox.begin(), colorBox.end(), redCompare );
  }
  else if ( greenRange >= redRange && greenRange >= blueRange && greenRange >= alphaRange )
  {
    qSort( colorBox.begin(), colorBox.end(), greenCompare );
  }
  else if ( blueRange >= redRange && blueRange >= greenRange && blueRange >= alphaRange )
  {
    qSort( colorBox.begin(), colorBox.end(), blueCompare );
  }
  else
  {
    qSort( colorBox.begin(), colorBox.end(), alphaCompare );
  }

  //get median
  double halfSum = colorBoxMapIt.key() / 2.0;
  int currentSum = 0;
  int currentListIndex = 0;

  QgsColorBox::iterator colorBoxIt = colorBox.begin();
  for ( ; colorBoxIt != colorBox.end(); ++colorBoxIt )
  {
    currentSum += colorBoxIt->second;
    if ( currentSum >= halfSum )
    {
      break;
    }
    ++currentListIndex;
  }

  if ( currentListIndex > ( colorBox.size() - 2 ) ) //if the median is contained in the last color, split one item before that
  {
    --currentListIndex;
    currentSum -= colorBoxIt->second;
  }
  else
  {
    ++colorBoxIt; //the iterator needs to point behind the last item to remove
  }

  //do split: replace old color box, insert new one
  QgsColorBox newColorBox1 = colorBox.mid( 0, currentListIndex + 1 );
  colorBoxMap.insert( currentSum, newColorBox1 );

  colorBox.erase( colorBox.begin(), colorBoxIt );
  QgsColorBox newColorBox2 = colorBox;
  colorBoxMap.erase( colorBoxMapIt );
  colorBoxMap.insert( halfSum * 2.0 - currentSum, newColorBox2 );
}

bool QgsHttpRequestHandler::minMaxRange( const QgsColorBox& colorBox, int& redRange, int& greenRange, int& blueRange, int& alphaRange )
{
  if ( colorBox.size() < 1 )
  {
    return false;
  }

  int rMin = INT_MAX;
  int gMin = INT_MAX;
  int bMin = INT_MAX;
  int aMin = INT_MAX;
  int rMax = INT_MIN;
  int gMax = INT_MIN;
  int bMax = INT_MIN;
  int aMax = INT_MIN;

  int currentRed = 0; int currentGreen = 0; int currentBlue = 0; int currentAlpha = 0;

  QgsColorBox::const_iterator colorBoxIt = colorBox.constBegin();
  for ( ; colorBoxIt != colorBox.constEnd(); ++colorBoxIt )
  {
    currentRed = qRed( colorBoxIt->first );
    if ( currentRed > rMax )
    {
      rMax = currentRed;
    }
    if ( currentRed < rMin )
    {
      rMin = currentRed;
    }

    currentGreen = qGreen( colorBoxIt->first );
    if ( currentGreen > gMax )
    {
      gMax = currentGreen;
    }
    if ( currentGreen < gMin )
    {
      gMin = currentGreen;
    }

    currentBlue = qBlue( colorBoxIt->first );
    if ( currentBlue > bMax )
    {
      bMax = currentBlue;
    }
    if ( currentBlue < bMin )
    {
      bMin = currentBlue;
    }

    currentAlpha = qAlpha( colorBoxIt->first );
    if ( currentAlpha > aMax )
    {
      aMax = currentAlpha;
    }
    if ( currentAlpha < aMin )
    {
      aMin = currentAlpha;
    }
  }

  redRange = rMax - rMin;
  greenRange = gMax - gMin;
  blueRange = bMax - bMin;
  alphaRange = aMax - aMin;
  return true;
}

bool QgsHttpRequestHandler::redCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 )
{
  return qRed( c1.first ) < qRed( c2.first );
}

bool QgsHttpRequestHandler::greenCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 )
{
  return qGreen( c1.first ) < qGreen( c2.first );
}

bool QgsHttpRequestHandler::blueCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 )
{
  return qBlue( c1.first ) < qBlue( c2.first );
}

bool QgsHttpRequestHandler::alphaCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 )
{
  return qAlpha( c1.first ) < qAlpha( c2.first );
}

QRgb QgsHttpRequestHandler::boxColor( const QgsColorBox& box, int boxPixels )
{
  double avRed = 0;
  double avGreen = 0;
  double avBlue = 0;
  double avAlpha = 0;
  QRgb currentColor;
  int currentPixel;

  double weight;

  QgsColorBox::const_iterator colorBoxIt = box.constBegin();
  for ( ; colorBoxIt != box.constEnd(); ++colorBoxIt )
  {
    currentColor = colorBoxIt->first;
    currentPixel = colorBoxIt->second;
    weight = ( double )currentPixel / boxPixels;
    avRed += ( qRed( currentColor ) * weight );
    avGreen += ( qGreen( currentColor ) * weight );
    avBlue += ( qBlue( currentColor ) * weight );
    avAlpha += ( qAlpha( currentColor ) * weight );
  }

  return qRgba( avRed, avGreen, avBlue, avAlpha );
}
