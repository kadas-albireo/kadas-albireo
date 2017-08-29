/***************************************************************************
                              qgssvgannotationitem.cpp
                              ------------------------
  begin                : November, 2012
  copyright            : (C) 2012 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssvgannotationitem.h"
#include "qgssvgannotationdialog.h"
#include "qgsproject.h"
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <qmath.h>
#include <svg2svgt/processorengine.h>
#include <svg2svgt/ruleengine.h>
#include <svg2svgt/logger.h>

REGISTER_QGS_ANNOTATION_ITEM( QgsSvgAnnotationItem )

QgsSvgAnnotationItem::QgsSvgAnnotationItem( QgsMapCanvas* canvas ): QgsAnnotationItem( canvas )
{

}

QgsSvgAnnotationItem::QgsSvgAnnotationItem( QgsMapCanvas* canvas, QgsSvgAnnotationItem* source )
    : QgsAnnotationItem( canvas, source )
{
  setFilePath( source->mFilePath );
}

void QgsSvgAnnotationItem::writeXML( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( documentElem.isNull() )
  {
    return;
  }

  QDomElement svgAnnotationElem = doc.createElement( "SVGAnnotationItem" );
  svgAnnotationElem.setAttribute( "file", QgsProject::instance()->writePath( mFilePath ) );
  _writeXML( doc, svgAnnotationElem );
  documentElem.appendChild( svgAnnotationElem );
}

void QgsSvgAnnotationItem::readXML( const QDomDocument& doc, const QDomElement& itemElem )
{
  QString filePath = QgsProject::instance()->readPath( itemElem.attribute( "file" ) );
  setFilePath( filePath );
  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }
}

void QgsSvgAnnotationItem::paint( QPainter* painter )
{
  if ( !painter )
  {
    return;
  }

  drawFrame( painter );
  if ( mMapPositionFixed )
  {
    drawMarkerSymbol( painter );
  }

  //keep width/height ratio of svg
  QRect viewBox = mSvgRenderer.viewBox();
  if ( viewBox.isValid() )
  {
    double widthRatio = mFrameSize.width() / viewBox.width();
    double heightRatio = mFrameSize.height() / viewBox.height();
    double renderWidth = 0;
    double renderHeight = 0;
    if ( widthRatio <= heightRatio )
    {
      renderWidth = mFrameSize.width();
      renderHeight = viewBox.height() * mFrameSize.width() / viewBox.width();
    }
    else
    {
      renderHeight = mFrameSize.height();
      renderWidth = viewBox.width() * mFrameSize.height() / viewBox.height();
    }

    double alpha = mAngle / 180. * M_PI;
    double rw = renderHeight * qAbs( qSin( alpha ) ) + renderWidth * qAbs( qCos( alpha ) );
    double rh = renderHeight * qAbs( qCos( alpha ) ) + renderWidth * qAbs( qSin( alpha ) );
    double scale = qMin( renderWidth / rw, renderHeight / rh );
    painter->save();
    painter->scale( scale, scale );
    painter->rotate( mAngle );
    mSvgRenderer.render( painter, QRectF( mOffsetFromReferencePoint.x(), mOffsetFromReferencePoint.y(),
                                          renderWidth, renderHeight ) );
    painter->restore();
  }
  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}

void QgsSvgAnnotationItem::_showItemEditor()
{
  QgsSvgAnnotationDialog( this ).exec();
}

void QgsSvgAnnotationItem::setFilePath( const QString& filepath )
{
  mFilePath = filepath;
  svg2svgt::Logger logger;
  svg2svgt::RuleEngine ruleEngine( logger );
  ruleEngine.setDefaultRules();
  svg2svgt::ProcessorEngine processor( ruleEngine, logger );

  QFile file( filepath );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    mSvgRenderer.load( processor.process( file.readAll() ) );
  }

  QRect viewBox = mSvgRenderer.viewBox();
  if ( !mIsClone )
  {
    setFrameSize( QSizeF( viewBox.width(), viewBox.height() ) );
  }
}

QImage QgsSvgAnnotationItem::billboardImage() const
{
  QImage image( mFrameSize.toSize(), QImage::Format_ARGB32 );
  image.fill( Qt::transparent );
  QPainter painter( &image );
  mSvgRenderer.render( &painter );
  return image;
}
