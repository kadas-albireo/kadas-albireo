/***************************************************************************
                              qgscomposerhtml.cpp
    ------------------------------------------------------------
    begin                : July 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposerhtml.h"
#include "qgscomposerframe.h"
#include "qgscomposition.h"
#include <QCoreApplication>
#include <QImage>
#include <QPainter>
#include <QWebFrame>
#include <QWebPage>

QgsComposerHtml::QgsComposerHtml( QgsComposition* c, qreal x, qreal y, qreal width, qreal height ): QgsComposerMultiFrame( c ), mWebPage( 0 ), mLoaded( false ), mHtmlUnitsToMM( 1.0 )
{
  mHtmlUnitsToMM = htmlUnitsToMM();
  mWebPage = new QWebPage();
  QObject::connect( mWebPage, SIGNAL( loadFinished( bool ) ), this, SLOT( frameLoaded( bool ) ) );
  setUrl( QUrl( "http://www.qgis.org" ) );//test
  QgsComposerFrame* frame = new QgsComposerFrame( c, this, x, y, width, height );
  addFrame( frame );
  mComposition->addItem( frame );
  recalculateFrameSizes();
}

QgsComposerHtml::QgsComposerHtml(): QgsComposerMultiFrame( 0 ), mWebPage( 0 ), mLoaded( false ), mHtmlUnitsToMM( 1.0 )
{
}

QgsComposerHtml::~QgsComposerHtml()
{
  delete mWebPage;
}

void QgsComposerHtml::setUrl( const QUrl& url )
{
  if ( !mWebPage )
  {
    return;
  }

  mUrl = url;
  mWebPage->mainFrame()->load( mUrl );
  while ( !mLoaded )
  {
    qApp->processEvents();
  }
  QSize contentsSize = mWebPage->mainFrame()->contentsSize();
  mWebPage->setViewportSize( contentsSize );

  double pixelPerMM = mComposition->printResolution() / 25.4;
  mSize.setWidth( contentsSize.width() / pixelPerMM );
  mSize.setHeight( contentsSize.height() / pixelPerMM );
}

void QgsComposerHtml::frameLoaded( bool ok )
{
  mLoaded = true;
}

QSizeF QgsComposerHtml::totalSize() const
{
  return mSize;
}

void QgsComposerHtml::render( QPainter* p, const QRectF& renderExtent )
{
  if ( !mWebPage )
  {
    return;
  }

  p->save();
  p->scale( 1.0 / mHtmlUnitsToMM, 1.0 / mHtmlUnitsToMM );
  mWebPage->mainFrame()->render( p, QRegion( renderExtent.left(), renderExtent.top(), renderExtent.width() * mHtmlUnitsToMM, renderExtent.height() * mHtmlUnitsToMM ) );
  p->restore();
}

double QgsComposerHtml::htmlUnitsToMM()
{
  if ( !mComposition )
  {
    return 1.0;
  }

  QImage img( 1, 1, QImage::Format_ARGB32_Premultiplied );
  double debug = img.dotsPerMeterX();
  double pixelPerMM = mComposition->printResolution() / 25.4;
  return ( pixelPerMM / ( img.dotsPerMeterX() / 1000.0 ) );
}
