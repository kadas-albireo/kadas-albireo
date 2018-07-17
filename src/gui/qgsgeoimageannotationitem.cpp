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
#include "qgsannotationlayer.h"
#include "qgsgeoimageannotationitem.h"
#include "qgsmaptoolannotation.h"
#include "qgsproject.h"
#include "qgsmapcanvas.h"
#include <QDesktopServices>
#include <QDomDocument>
#include <QDomElement>
#include <QImageReader>
#include <QMenu>
#include <exiv2/exiv2.hpp>

REGISTER_QGS_ANNOTATION_ITEM( QgsGeoImageAnnotationItem )

QgsGeoImageAnnotationItem* QgsGeoImageAnnotationItem::create( QgsMapCanvas *canvas, const QString &filePath, bool onlyGeoreferenced, QString* errMsg )
{
  QImageReader reader( filePath );
  if ( reader.format().isEmpty() || reader.size().isEmpty() )
  {
    // Image format not supported
    return 0;
  }
  QgsPoint wgs84Pos;
  bool locked = true;
  // Fall back to canvas center position if image has no geotags
  if ( !readGeoPos( filePath, wgs84Pos, errMsg ) )
  {
    if ( onlyGeoreferenced )
    {
      return 0;
    }
    locked = false;
    const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( canvas->mapSettings().destinationCrs().authid(), "EPSG:4326" );
    wgs84Pos = crst->transform( canvas->mapSettings().extent().center() );
  }
  QgsGeoImageAnnotationItem* item = new QgsGeoImageAnnotationItem( canvas );
  if ( locked )
  {
    item->setItemFlags( QgsAnnotationItem::ItemMapPositionLocked );
  }
  item->setFilePath( filePath );
  item->setMapPosition( wgs84Pos, QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
  QgsAnnotationLayer::getLayer( canvas, "geoImage", tr( "Pictures" ) )->addItem( item );
  canvas->setMapTool( new QgsMapToolEditAnnotation( canvas, item ) );
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
  QString lonRef = QString::fromStdString( itLonRef->value().toString() );
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
  if ( lonRef.compare( "W", Qt::CaseInsensitive ) == 0 )
  {
    lon *= -1;
  }
  wgs84Pos = QgsPoint( lon, lat );
  return true;
}


QgsGeoImageAnnotationItem::QgsGeoImageAnnotationItem( QgsMapCanvas *canvas )
    : QgsAnnotationItem( canvas )
{
  setItemFlags( QgsAnnotationItem::ItemKeepsAspectRatio );
}

QgsGeoImageAnnotationItem::~QgsGeoImageAnnotationItem()
{

}

QgsGeoImageAnnotationItem::QgsGeoImageAnnotationItem( QgsMapCanvas* canvas, QgsGeoImageAnnotationItem* source )
    : QgsAnnotationItem( canvas, source )
{
  mFilePath = source->mFilePath;
  mImage = source->mImage;
}

void QgsGeoImageAnnotationItem::setFilePath( const QString& filePath, bool originalSize )
{
  mFilePath = filePath;
  mName = QFileInfo( mFilePath ).baseName();

  QImageReader reader( mFilePath );

  QSize imageSize = reader.size();
  // Scale such that largest dimension is max 64px
  if ( originalSize )
  {
    // pass
  }
  else if ( imageSize.width() > imageSize.height() )
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
  mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );
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
  mFilePath = QgsProject::instance()->writePath( mFilePath, QString::null, true );
  geoImageAnnotationElem.setAttribute( "file", mFilePath );
  QgsPoint worldPos = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" )->transform( mGeoPos );
  geoImageAnnotationElem.setAttribute( "lon", worldPos.x() );
  geoImageAnnotationElem.setAttribute( "lat", worldPos.y() );
  _writeXML( doc, geoImageAnnotationElem );
  documentElem.appendChild( geoImageAnnotationElem );
}

void QgsGeoImageAnnotationItem::readXML( const QDomDocument& doc, const QDomElement& itemElem )
{
  QString filePath = QgsProject::instance()->readPath( itemElem.attribute( "file" ) );
  QString lon = itemElem.attribute( "lon" );
  QString lat = itemElem.attribute( "lat" );
  QgsPoint wgs84Pos( lon.toDouble(), lat.toDouble() );

  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }

  mFilePath = filePath;
  updateImage();
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

void QgsGeoImageAnnotationItem::handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos )
{
  QgsAnnotationItem::handleMoveAction( moveAction, newPos, oldPos );
  if ( moveAction >= ResizeFrameUp && moveAction <= ResizeFrameRightDown )
  {
    updateImage();
  }
}

void QgsGeoImageAnnotationItem::_showItemEditor()
{
  QDesktopServices::openUrl( QUrl::fromLocalFile( mFilePath ) );
}

void QgsGeoImageAnnotationItem::updateImage()
{
  QImageReader reader( mFilePath );
  reader.setBackgroundColor( Qt::white );
  reader.setScaledSize( QSize( mFrameSize.width() - 4, mFrameSize.height() - 4 ) );
  mImage = reader.read().convertToFormat( QImage::Format_RGB32 );
}

void QgsGeoImageAnnotationItem::setPositionLocked( bool locked )
{
  if ( !locked )
  {
    setItemFlags( itemFlags() & ~QgsAnnotationItem::ItemMapPositionLocked );
  }
  else
  {
    setItemFlags( itemFlags() | QgsAnnotationItem::ItemMapPositionLocked );
  }
}

void QgsGeoImageAnnotationItem::setFrameVisible( bool visible )
{
  if ( !visible )
  {
    setItemFlags( itemFlags() | QgsAnnotationItem::ItemHasNoFrame | QgsAnnotationItem::ItemMarkerCentered );
    setFrameSize( frameSize() ); // Recompute offset from reference point
  }
  else
  {
    setItemFlags( itemFlags() & ~QgsAnnotationItem::ItemHasNoFrame & ~QgsAnnotationItem::ItemMarkerCentered );
  }
}

void QgsGeoImageAnnotationItem::showContextMenu( const QPoint &screenPos )
{
  QMenu menu;
  menu.addAction( tr( "Open" ), this, SLOT( _showItemEditor() ) );

  QAction* lockAction = menu.addAction( tr( "Lock position" ), this, SLOT( setPositionLocked( bool ) ) );
  lockAction->setCheckable( true );
  lockAction->setChecked( itemFlags() & QgsAnnotationItem::ItemMapPositionLocked );

  QAction* frameAction = menu.addAction( tr( "Show frame" ), this, SLOT( setFrameVisible( bool ) ) );
  frameAction->setCheckable( true );
  frameAction->setChecked(( itemFlags() & QgsAnnotationItem::ItemHasNoFrame ) == 0 );

  menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteLater() ) );
  menu.exec( screenPos );
}
