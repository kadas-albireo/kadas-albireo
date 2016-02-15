/***************************************************************************
                              qgsimageannotationitem.cpp
                              ------------------------
  begin                : November, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : manisandro@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsimageannotationitem.h"
#include "qgsproject.h"
#include <QDataStream>
#include <QDomDocument>
#include <QDomElement>
#include <QPainter>

REGISTER_QGS_ANNOTATION_ITEM( QgsImageAnnotationItem )

QgsImageAnnotationItem::QgsImageAnnotationItem( QgsMapCanvas* canvas ): QgsAnnotationItem( canvas )
{
  setItemFlags( ItemHasNoMarker | ItemHasNoFrame );
}

QgsImageAnnotationItem::QgsImageAnnotationItem( QgsMapCanvas* canvas, QgsImageAnnotationItem* source )
    : QgsAnnotationItem( canvas, source )
{
  mImage = source->mImage;
}

void QgsImageAnnotationItem::writeXML( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( documentElem.isNull() )
  {
    return;
  }
  QByteArray imageData;
  QDataStream ds( &imageData, QIODevice::WriteOnly ); ds << mImage;

  QDomElement imageAnnotationElem = doc.createElement( "ImageAnnotationItem" );
  imageAnnotationElem.setAttribute( "data", QString( imageData.toBase64() ) );
  _writeXML( doc, imageAnnotationElem );
  documentElem.appendChild( imageAnnotationElem );
}

void QgsImageAnnotationItem::readXML( const QDomDocument& doc, const QDomElement& itemElem )
{
  QByteArray imageData = QByteArray::fromBase64( itemElem.attribute( "data" ).toAscii() );
  QDataStream ds( imageData ); ds >> mImage;
  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }
}

void QgsImageAnnotationItem::paint( QPainter* painter )
{
  if ( !painter )
  {
    return;
  }

  QRectF r( mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y(), mFrameSize.width(), mFrameSize.height() );
  QPainter::RenderHints prevHints = painter->renderHints();
  painter->setRenderHints( QPainter::SmoothPixmapTransform | QPainter::Antialiasing );
  painter->drawImage( r, mImage );
  painter->setRenderHints( prevHints );

  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}

void QgsImageAnnotationItem::_showItemEditor()
{
}

void QgsImageAnnotationItem::setImage( const QImage& image )
{
  setFrameSize( image.size() );
  mImage = image;
}
