/***************************************************************************
                         qgsrasterdrawer.cpp
                         ---------------------
    begin                : June 2012
    copyright            : (C) 2012 by Radim Blazek
    email                : radim dot blazek at gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslogger.h"
#include "qgsrasterdataprovider.h"
#include "qgsrasterdrawer.h"
#include "qgsrasteriterator.h"
#include "qgsrasterviewport.h"
#include "qgsrendercontext.h"
#include "qgsmaptopixel.h"
#include <QImage>
#include <QPainter>
#include <QPrinter>

QgsRasterDrawer::QgsRasterDrawer( QgsRasterIterator* iterator ): mIterator( iterator )
{
}

QgsRasterDrawer::~QgsRasterDrawer()
{
}

void QgsRasterDrawer::draw( QPainter* p, QgsRasterViewPort* viewPort, const QgsMapToPixel* theQgsMapToPixel, const QgsRenderContext* ctx )
{
  QgsDebugMsg( "Entered" );
  if ( !p || !mIterator || !viewPort || !theQgsMapToPixel )
  {
    return;
  }

  int width = viewPort->mWidth;
  int height = viewPort->mHeight;
  QgsMapToPixel mapToPixel( *theQgsMapToPixel );
  double scaleFactor = 1.0;

  if ( isWMTSLayer( mIterator->input() ) ) //wmts case, comply with standard pixel size of 0.28mm from specification
  {
    scaleFactor = ( 1.0 / 0.28 * 25.4 ) / p->device()->logicalDpiX();
    width = width * scaleFactor;
    height = height * scaleFactor;
    mapToPixel.setParameters( ctx->extent().width() / width, ctx->extent().xMinimum(), ctx->extent().yMinimum(), height );
  }

  // last pipe filter has only 1 band
  int bandNumber = 1;
  mIterator->startRasterRead( bandNumber, width, height, viewPort->mDrawnExtent );

  //number of cols/rows in output pixels
  int nCols = 0;
  int nRows = 0;
  //shift to top left point for the raster part
  int topLeftCol = 0;
  int topLeftRow = 0;

  // We know that the output data type of last pipe filter is QImage data

  QgsRasterBlock *block;

  // readNextRasterPart calcs and resets  nCols, nRows, topLeftCol, topLeftRow
  while ( mIterator->readNextRasterPart( bandNumber, nCols, nRows,
                                         &block, topLeftCol, topLeftRow ) )
  {
    if ( !block )
    {
      QgsDebugMsg( "Cannot get block" );
      continue;
    }

    QImage img = block->image();

    // Because of bug in Acrobat Reader we must use "white" transparent color instead
    // of "black" for PDF. See #9101.
    QPrinter *printer = dynamic_cast<QPrinter *>( p->device() );
    if ( printer && printer->outputFormat() == QPrinter::PdfFormat )
    {
      QgsDebugMsg( "PdfFormat" );

      img = img.convertToFormat( QImage::Format_ARGB32 );
      QRgb transparentBlack = qRgba( 0, 0, 0, 0 );
      QRgb transparentWhite = qRgba( 255, 255, 255, 0 );
      for ( int x = 0; x < img.width(); x++ )
      {
        for ( int y = 0; y < img.height(); y++ )
        {
          if ( img.pixel( x, y ) == transparentBlack )
          {
            img.setPixel( x, y, transparentWhite );
          }
        }
      }
    }

    drawImage( p, viewPort, img, topLeftCol, topLeftRow, scaleFactor, &mapToPixel );
    QgsDebugMsg( "Block drawn" );

    delete block;
    if ( ctx && ctx->renderingStopped() ) { break; }
  }
}

void QgsRasterDrawer::drawImage( QPainter* p, QgsRasterViewPort* viewPort, const QImage& img, int topLeftCol, int topLeftRow, double scaleFactor, const QgsMapToPixel* theQgsMapToPixel ) const
{
  if ( !p || !viewPort )
  {
    return;
  }

  //top left position in device coords
  QPoint tlPoint = QPoint( viewPort->mTopLeftPoint.x() + topLeftCol, viewPort->mTopLeftPoint.y() + topLeftRow );
  p->save();
  p->setRenderHint( QPainter::Antialiasing, false );

  // Blending problem was reported with PDF output if background color has alpha < 255
  // in #7766, it seems to be a bug in Qt, setting a brush with alpha 255 is a workaround
  // which should not harm anything
  p->setBrush( QBrush( QColor( Qt::white ), Qt::NoBrush ) );

  if ( theQgsMapToPixel )
  {
    int w = theQgsMapToPixel->mapWidth();
    int h = theQgsMapToPixel->mapHeight();

    double rotation = theQgsMapToPixel->mapRotation();
    if ( rotation )
    {
      // both viewPort and image sizes are dependent on scale
      double cx = w / 2.0;
      double cy = h / 2.0;
      p->translate( cx, cy );
      p->rotate( rotation );
      p->translate( -cx, -cy );
    }
  }

  QRect source( 0, 0, img.width(), img.height() );
  QRect target( tlPoint.x(), tlPoint.y(), img.width() / scaleFactor, img.height() / scaleFactor );
  p->drawImage( target, img, source );

#if 0
  // For debugging:
  QRectF br = QRectF( tlPoint, img.size() );
  QPointF c = br.center();
  double rad = std::max( br.width(), br.height() ) / 10;
  p->drawRoundedRect( br, rad, rad );
  p->drawLine( QLineF( br.x(), br.y(), br.x() + br.width(), br.y() + br.height() ) );
  p->drawLine( QLineF( br.x() + br.width(), br.y(), br.x(), br.y() + br.height() ) );

  double nw = br.width() * 0.5; double nh = br.height() * 0.5;
  br = QRectF( c - QPointF( nw / 2, nh / 2 ), QSize( nw, nh ) );
  p->drawRoundedRect( br, rad, rad );

  nw = br.width() * 0.5; nh = br.height() * 0.5;
  br = QRectF( c - QPointF( nw / 2, nh / 2 ), QSize( nw, nh ) );
  p->drawRoundedRect( br, rad, rad );
#endif

  p->restore();
}

bool QgsRasterDrawer::isWMTSLayer( const QgsRasterInterface* iface )
{
  if ( iface && iface->srcInput() )
  {
    const QgsRasterDataProvider* provider  = dynamic_cast<const QgsRasterDataProvider*>( iface->srcInput() );
    QVariant resolutions = provider->property( "resolutions" );
    if ( provider->name() == "wms" && !resolutions.isNull() )
    {
      return true;
    }
  }
  return false;
}

