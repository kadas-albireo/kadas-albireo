/***************************************************************************
                              qgsgeoimageannotationitem.cpp
                              ------------------------
  begin                : August, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
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
#include "qgsgeoimageannotationitem.h"
#include "qgsproject.h"
#include "qgsmapcanvas.h"
#include <QDomDocument>
#include <QDomElement>
#include <QImageReader>
#include <QDesktopServices>
#include <exiv2/exiv2.hpp>


QgsGeoImageAnnotationItem* QgsGeoImageAnnotationItem::create( QgsMapCanvas *canvas, const QString &filePath, QString* errMsg )
{
  QgsPoint wgs84Pos;
  if ( !readGeoPos( filePath, wgs84Pos, errMsg ) )
  {
    return 0;
  }
  QgsGeoImageAnnotationItem* item = new QgsGeoImageAnnotationItem( canvas );
  item->setFilePath( filePath );
  item->setMapPosition( wgs84Pos, QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
  return item;
}

bool QgsGeoImageAnnotationItem::readGeoPos( const QString &filePath, QgsPoint &wgs84Pos, QString* errMsg )
{
  // Read EXIF position
  Exiv2::Image::AutoPtr image;
  try
  {
    image = Exiv2::ImageFactory::open( filePath.toLocal8Bit().data() );
  }
  catch ( const Exiv2::Error& )
  {
    if ( errMsg ) *errMsg = tr( "Could not read image" );
    return false;
  }

  if ( image.get() == 0 )
  {
    if ( errMsg ) *errMsg = tr( "Could not read image" );
    return false;
  }

  image->readMetadata();
  Exiv2::ExifData &exifData = image->exifData();
  if ( exifData.empty() )
  {
    if ( errMsg ) *errMsg = tr( "Failed to read EXIF tags" );
    return false;
  }

  Exiv2::ExifData::iterator itLatRef = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLatitudeRef" ) );
  Exiv2::ExifData::iterator itLatVal = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLatitude" ) );
  Exiv2::ExifData::iterator itLonRef = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLongitudeRef" ) );
  Exiv2::ExifData::iterator itLonVal = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLongitude" ) );

  if ( itLatRef == exifData.end() || itLatVal == exifData.end() ||
       itLonRef == exifData.end() || itLonVal == exifData.end() )
  {
    if ( errMsg ) *errMsg = tr( "Failed to read position EXIF tags" );
    return false;
  }
  QString latRef = QString::fromStdString( itLatRef->value().toString() );
  QStringList latVals = QString::fromStdString( itLatVal->value().toString() ).split( QRegExp( "\\s+" ) );
//  QString lonRef = QString::fromStdString(itLonRef->value().toString());
  QStringList lonVals = QString::fromStdString( itLonVal->value().toString() ).split( QRegExp( "\\s+" ) );

  double lat = 0, lon = 0;
  double div = 1;
  foreach ( const QString& rational, latVals )
  {
    QStringList pair = rational.split( "/" );
    if ( pair.size() != 2 )
      break;
    lat += ( pair[0].toDouble() / pair[1].toDouble() ) / div;
    div *= 60;
  }

  div = 1;
  foreach ( const QString& rational, lonVals )
  {
    QStringList pair = rational.split( "/" );
    if ( pair.size() != 2 )
      break;
    lon += ( pair[0].toDouble() / pair[1].toDouble() ) / div;
    div *= 60;
  }

  if ( latRef.compare( "S", Qt::CaseInsensitive ) == 0 )
  {
    lat *= -1;
  }
  wgs84Pos = QgsPoint( lon, lat );
  return true;
}


QgsGeoImageAnnotationItem::QgsGeoImageAnnotationItem( QgsMapCanvas *canvas )
    : QgsAnnotationItem( canvas )
{
  setItemFlags( ItemIsNotResizeable | ItemHasNoMarker );
}

void QgsGeoImageAnnotationItem::setFilePath( const QString& filePath )
{
  mFilePath = filePath;

  QImageReader reader( mFilePath );

  QSize imageSize = reader.size();
  // Scale such that largest dimension is max 64px
  if ( imageSize.width() > imageSize.height() )
  {
    imageSize.setHeight(( 64. * imageSize.height() ) / imageSize.width() );
    imageSize.setWidth( 64 );
  }
  else
  {
    imageSize.setWidth(( 64. * imageSize.width() ) / imageSize.height() );
    imageSize.setHeight( 64 );
  }

  reader.setBackgroundColor( Qt::white );
  reader.setScaledSize( imageSize );
  mImage = reader.read().convertToFormat( QImage::Format_RGB32 );
  setFrameSize( QSize( imageSize.width() + 4, imageSize.height() + 4 ) );
}

void QgsGeoImageAnnotationItem::writeXML( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( documentElem.isNull() )
  {
    return;
  }

  QDomElement geoImageAnnotationElem = doc.createElement( "GeoImageAnnotationItem" );
  geoImageAnnotationElem.setAttribute( "file", QgsProject::instance()->writePath( mFilePath ) );
  _writeXML( doc, geoImageAnnotationElem );
  documentElem.appendChild( geoImageAnnotationElem );
}

void QgsGeoImageAnnotationItem::readXML( const QDomDocument& doc, const QDomElement& itemElem )
{
  QString filePath = QgsProject::instance()->readPath( itemElem.attribute( "file" ) );
  QgsPoint wgs84Pos;
  if ( !readGeoPos( filePath, wgs84Pos ) )
  {
    // Suicide
    delete this;
    return;
  }

  setFilePath( filePath );
  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }

  setMapPosition( wgs84Pos, QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
}

void QgsGeoImageAnnotationItem::paint( QPainter* painter )
{
  if ( !painter )
  {
    return;
  }

  drawFrame( painter );

  painter->drawImage( mOffsetFromReferencePoint.x() + 2, mOffsetFromReferencePoint.y() + 2, mImage );

  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}

void QgsGeoImageAnnotationItem::_showItemEditor()
{
  QDesktopServices::openUrl( QUrl::fromLocalFile( mFilePath ) );
}
