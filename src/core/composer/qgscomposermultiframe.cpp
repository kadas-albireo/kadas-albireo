/***************************************************************************
                              qgscomposermultiframe.cpp
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

#include "qgscomposermultiframe.h"
#include "qgscomposerframe.h"

QgsComposerMultiFrame::QgsComposerMultiFrame( QgsComposition* c ): mComposition( c ), mResizeMode( UseExistingFrames )
{
  //debug
  mResizeMode = ExtendToNextPage;
}

QgsComposerMultiFrame::QgsComposerMultiFrame(): mComposition( 0 ), mResizeMode( UseExistingFrames )
{
}

QgsComposerMultiFrame::~QgsComposerMultiFrame()
{
}

void QgsComposerMultiFrame::recalculateFrameSizes()
{
  if ( mFrameItems.size() < 1 )
  {
    return;
  }

  QSizeF size = totalSize();
  double totalHeight = size.height();
  double currentY = 0;
  double currentHeight = 0;
  QgsComposerFrame* currentItem = 0;

  QList<QgsComposerFrame*>::iterator frameIt = mFrameItems.begin();
  for ( ; frameIt != mFrameItems.end(); ++frameIt )
  {
    if ( currentY >= totalHeight )
    {
      return;
    }
    currentItem = *frameIt;
    currentHeight = currentItem->rect().height();
    currentItem->setContentSection( QRectF( 0, currentY, currentItem->rect().width(), currentHeight ) );
    currentItem->update();
    currentY += currentHeight;
  }

  //at end of frames but there is  still content left. Add other pages if ResizeMode ==
  if ( mResizeMode == ExtendToNextPage )
  {
    while ( currentY < totalHeight )
    {
      //find out on which page the lower left point of the last frame is
      int page = currentItem->transform().dy() / ( mComposition->paperHeight() + mComposition->spaceBetweenPages() );
      //double offset = currentItem->transform().dy() - page * ( mComposition->paperHeight() + mComposition->spaceBetweenPages() );

      //e.v. add a new page
      if ( mComposition->numPages() < ( page + 2 ) )
      {
        mComposition->setNumPages( page + 2 );
      }

      //copy last frame
      QgsComposerFrame* newFrame = new QgsComposerFrame( mComposition, this, 0, currentItem->transform().dy() + mComposition->paperHeight() + mComposition->spaceBetweenPages(),
          currentItem->rect().width(), currentItem->rect().height() );
      newFrame->setContentSection( QRectF( 0, currentY, newFrame->rect().width(), newFrame->rect().height() ) );
      currentY += newFrame->rect().height();
      currentItem = newFrame;
      addFrame( newFrame );
      mComposition->addItem( newFrame );
    }
  }

#if 0
  if ( mFrameItems.size() > 0 )
  {
    QSizeF size = totalSize();
    QgsComposerFrame* item = mFrameItems[0];
    item->setContentSection( QRectF( 0, 0, item->rect().width(), item->rect().height() ) );
  }
#endif //0
}

void QgsComposerMultiFrame::addFrame( QgsComposerFrame* frame )
{
  mFrameItems.push_back( frame );
  QObject::connect( frame, SIGNAL( sizeChanged() ), this, SLOT( recalculateFrameSizes() ) );
}
