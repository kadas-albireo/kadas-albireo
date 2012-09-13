/***************************************************************************
                         qgssinglebandcolordatarenderer.cpp
                         ----------------------------------
    begin                : January 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssinglebandcolordatarenderer.h"
#include "qgsrasterviewport.h"
#include <QDomDocument>
#include <QDomElement>
#include <QImage>

QgsSingleBandColorDataRenderer::QgsSingleBandColorDataRenderer( QgsRasterInterface* input, int band ):
    QgsRasterRenderer( input, "singlebandcolordata" ), mBand( band )
{

}

QgsSingleBandColorDataRenderer::~QgsSingleBandColorDataRenderer()
{
}

QgsRasterInterface * QgsSingleBandColorDataRenderer::clone() const
{
  QgsSingleBandColorDataRenderer * renderer = new QgsSingleBandColorDataRenderer( 0, mBand );
  return renderer;
}

QgsRasterRenderer* QgsSingleBandColorDataRenderer::create( const QDomElement& elem, QgsRasterInterface* input )
{
  if ( elem.isNull() )
  {
    return 0;
  }

  int band = elem.attribute( "band", "-1" ).toInt();
  QgsRasterRenderer* r = new QgsSingleBandColorDataRenderer( input, band );
  r->readXML( elem );
  return r;
}

void * QgsSingleBandColorDataRenderer::readBlock( int bandNo, QgsRectangle  const & extent, int width, int height )
{
  if ( !mInput )
  {
    return 0;
  }

  int currentRasterPos;

  bool hasTransparency = usesTransparency();

  void* rasterData = mInput->block( bandNo, extent, width, height );
  if ( ! rasterData )
  {
    QgsDebugMsg("No raster data!" );
    return 0;
  }

  currentRasterPos = 0;
  QImage img( width, height, QImage::Format_ARGB32 );
  if ( img.isNull() )
  {
    QgsDebugMsg( "Could not create QImage" );
    VSIFree( rasterData );
    return 0;
  }

  uchar* scanLine = 0;
  for ( int i = 0; i < height; ++i )
  {
    scanLine = img.scanLine( i );
    if ( !hasTransparency )
    {
      memcpy( scanLine, &((( uint* )rasterData )[currentRasterPos] ), width * 4 );
      currentRasterPos += width;
    }
    else
    {
      QRgb pixelColor;
      double alpha = 255.0;
      for ( int j = 0; j < width; ++j )
      {
        QRgb c((( uint* )( rasterData ) )[currentRasterPos] );
        alpha = qAlpha( c );
        pixelColor = qRgba( mOpacity * qRed( c ), mOpacity * qGreen( c ), mOpacity * qBlue( c ), mOpacity * alpha );
        memcpy( &( scanLine[j*4] ), &pixelColor, 4 );
        ++currentRasterPos;
      }
    }
  }
  VSIFree( rasterData );

  void * data = VSIMalloc( img.byteCount() );
  if ( ! data )
  {
    QgsDebugMsg( QString( "Couldn't allocate output data memory of % bytes" ).arg( img.byteCount() ) );
    return 0;
  }
  return memcpy( data, img.bits(), img.byteCount() );
}

void QgsSingleBandColorDataRenderer::writeXML( QDomDocument& doc, QDomElement& parentElem ) const
{
  if ( parentElem.isNull() )
  {
    return;
  }

  QDomElement rasterRendererElem = doc.createElement( "rasterrenderer" );
  _writeXML( doc, rasterRendererElem );
  rasterRendererElem.setAttribute( "band", mBand );
  parentElem.appendChild( rasterRendererElem );
}

QList<int> QgsSingleBandColorDataRenderer::usesBands() const
{
  QList<int> bandList;
  if ( mBand != -1 )
  {
    bandList << mBand;
  }
  return bandList;
}

bool QgsSingleBandColorDataRenderer::setInput( QgsRasterInterface* input )
{
  // Renderer can only work with numerical values in at least 1 band
  if ( !input ) return false;

  if ( !mOn )
  {
    // In off mode we can connect to anything
    mInput = input;
    return true;
  }

  if ( input->dataType( 1 ) == QgsRasterInterface::ARGB32 ||
       input->dataType( 1 ) == QgsRasterInterface::ARGB32_Premultiplied )
  {
    mInput = input;
    return true;
  }
  return false;
}
