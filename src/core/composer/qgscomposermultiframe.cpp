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
  mComposition->addMultiFrame( this );
}

QgsComposerMultiFrame::QgsComposerMultiFrame(): mComposition( 0 ), mResizeMode( UseExistingFrames )
{
}

QgsComposerMultiFrame::~QgsComposerMultiFrame()
{
}

void QgsComposerMultiFrame::setResizeMode( ResizeMode mode )
{
  if ( mode != mResizeMode )
  {
    mResizeMode = mode;
    recalculateFrameSizes();
  }
}

void QgsComposerMultiFrame::recalculateFrameSizes()
{
  if ( mFrameItems.size() < 1 )
  {
    return;
  }

  QSizeF size = totalSize();
  double totalHeight = size.height();

  if ( totalHeight < 1 )
  {
    return;
  }

  double currentY = 0;
  double currentHeight = 0;
  QgsComposerFrame* currentItem = 0;

  for ( int i = 0; i < mFrameItems.size(); ++i )
  {
    if ( currentY >= totalHeight )
    {
      if ( mResizeMode == ExtendToNextPage ) //remove unneeded frames in extent mode
      {
        for ( int j = mFrameItems.size(); j > i; --j )
        {
          removeFrame( j - 1 );
        }
      }
      return;
    }

    currentItem = mFrameItems.value( i );
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

      //add new pages if necessary
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
    }
  }
}

void QgsComposerMultiFrame::handleFrameRemoval( QgsComposerItem* item )
{
  QgsComposerFrame* frame = dynamic_cast<QgsComposerFrame*>( item );
  if ( !frame )
  {
    return;
  }
  int index = mFrameItems.indexOf( frame );
  if ( index == -1 )
  {
    return;
  }
  mFrameItems.removeAt( index );

  if ( mFrameItems.size() < 1 )
  {
    if ( mComposition )
    {
      //schedule this composer multi frame for deletion
      mComposition->removeMultiFrame( this );
    }
  }
  else
  {
    recalculateFrameSizes();
  }
}

void QgsComposerMultiFrame::removeFrame( int i )
{
  QgsComposerFrame* frameItem = mFrameItems[i];
  if ( mComposition )
  {
    mComposition->removeComposerItem( frameItem );
  }
}

void QgsComposerMultiFrame::update()
{
  QList<QgsComposerFrame*>::iterator frameIt = mFrameItems.begin();
  for ( ; frameIt != mFrameItems.end(); ++frameIt )
  {
    ( *frameIt )->update();
  }
}
