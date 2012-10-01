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
#include "qgsaddremovemultiframecommand.h"
#include <QCoreApplication>
#include <QPainter>
#include <QWebFrame>
#include <QWebPage>

QgsComposerHtml::QgsComposerHtml( QgsComposition* c, bool createUndoCommands ): QgsComposerMultiFrame( c, createUndoCommands ),
    mWebPage( 0 ), mLoaded( false ), mHtmlUnitsToMM( 1.0 )
{
  mHtmlUnitsToMM = htmlUnitsToMM();
  mWebPage = new QWebPage();
  QObject::connect( mWebPage, SIGNAL( loadFinished( bool ) ), this, SLOT( frameLoaded( bool ) ) );
  if ( mComposition )
  {
    QObject::connect( mComposition, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( handleFrameRemoval( QgsComposerItem* ) ) );
  }
}

QgsComposerHtml::QgsComposerHtml(): QgsComposerMultiFrame( 0, false ), mWebPage( 0 ), mLoaded( false ), mHtmlUnitsToMM( 1.0 )
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
  mLoaded = false;

  mUrl = url;
  mWebPage->mainFrame()->load( mUrl );
  while ( !mLoaded )
  {
    qApp->processEvents();
  }

  if ( frameCount() < 1)  return;
  //QSize contentsSize = mWebPage->mainFrame()->contentsSize();

  QRectF contentRect = this->mFrameItems.at(0)->boundingRect();
  //there is going to be a little rounding error converting from float to int
  QSize contentsSize = QSize( (int)(contentRect.width() * mHtmlUnitsToMM),
                              (int)(contentRect.height() * mHtmlUnitsToMM));
  mWebPage->setViewportSize( contentsSize );

  //suppress scroll bars always
  mWebPage->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
  mWebPage->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

  mSize.setWidth( contentsSize.width() / mHtmlUnitsToMM );
  mSize.setHeight( contentsSize.height() / mHtmlUnitsToMM );
  recalculateFrameSizes();
  emit changed();
}

void QgsComposerHtml::frameLoaded( bool ok )
{
  Q_UNUSED( ok );
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
  p->translate( 0.0, -renderExtent.top() * mHtmlUnitsToMM );
  mWebPage->mainFrame()->render( p, QRegion( renderExtent.left(), renderExtent.top() * mHtmlUnitsToMM, renderExtent.width() * mHtmlUnitsToMM, renderExtent.height() * mHtmlUnitsToMM ) );
  p->restore();
}

double QgsComposerHtml::htmlUnitsToMM()
{
  if ( !mComposition )
  {
    return 1.0;
  }

  return ( mComposition->printResolution() / 96.0 ); //webkit seems to assume a standard dpi of 96
}

void QgsComposerHtml::addFrame( QgsComposerFrame* frame, bool recalcFrameSizes )
{
  mFrameItems.push_back( frame );
  QObject::connect( frame, SIGNAL( sizeChanged() ), this, SLOT( recalculateFrameSizes() ) );
  if ( mComposition )
  {
    mComposition->addComposerHtmlFrame( this, frame );
  }

  if ( recalcFrameSizes )
  {
    recalculateFrameSizes();
  }
}

bool QgsComposerHtml::writeXML( QDomElement& elem, QDomDocument & doc, bool ignoreFrames ) const
{
  QDomElement htmlElem = doc.createElement( "ComposerHtml" );
  htmlElem.setAttribute( "url", mUrl.toString() );
  bool state = _writeXML( htmlElem, doc, ignoreFrames );
  elem.appendChild( htmlElem );
  return state;
}

bool QgsComposerHtml::readXML( const QDomElement& itemElem, const QDomDocument& doc, bool ignoreFrames )
{
  deleteFrames();
  QString urlString = itemElem.attribute( "url" );
  if ( !urlString.isEmpty() )
  {
    setUrl( QUrl( urlString ) );
  }
  return _readXML( itemElem, doc, ignoreFrames );
}
