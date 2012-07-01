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

QgsSingleBandColorDataRenderer::QgsSingleBandColorDataRenderer( QgsRasterFace* input, int band ):
    QgsRasterRenderer( input, "singlebandcolordata" ), mBand( band )
{

}

QgsSingleBandColorDataRenderer::~QgsSingleBandColorDataRenderer()
{
}

QgsRasterRenderer* QgsSingleBandColorDataRenderer::create( const QDomElement& elem, QgsRasterFace* input )
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

  double oversamplingX, oversamplingY;
  //startRasterRead( mBand, viewPort, theQgsMapToPixel, oversamplingX, oversamplingY );

  //number of cols/rows in output pixels
  int nCols = 0;
  int nRows = 0;
  //number of raster cols/rows with oversampling
  int nRasterCols = 0;
  int nRasterRows = 0;
  //shift to top left point for the raster part
  int topLeftCol = 0;
  int topLeftRow = 0;
  int currentRasterPos;

  //bool hasTransparency = usesTransparency( viewPort->mSrcCRS, viewPort->mDestCRS );
  bool hasTransparency = false;

  QgsRasterFace::DataType rasterType = ( QgsRasterFace::DataType )mInput->dataType( mBand );


  void* rasterData = mInput->readBlock( bandNo, extent, width, height );

  currentRasterPos = 0;
  QImage img( width, height, QImage::Format_ARGB32 );
  uchar* scanLine = 0;
  for ( int i = 0; i < height; ++i )
  {
    scanLine = img.scanLine( i );
    if ( !hasTransparency )
    {
      memcpy( scanLine, &((( uint* )rasterData )[currentRasterPos] ), nCols * 4 );
      currentRasterPos += nRasterCols;
    }
    else
    {
      for ( int j = 0; j < width; ++j )
      {
        QRgb pixelColor;
        double alpha = 255.0;
        for ( int j = 0; j < nRasterCols; ++j )
        {
          QRgb c((( uint* )( rasterData ) )[currentRasterPos] );
          alpha = qAlpha( c );
          pixelColor = qRgba( qRed( c ), qGreen( c ), qBlue( c ), mOpacity * alpha );
          memcpy( &( scanLine[j*4] ), &pixelColor, 4 );
          ++currentRasterPos;
        }
      }
    }
  }
  VSIFree( rasterData );

  void * data = VSIMalloc( img.byteCount() );
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
