/***************************************************************************
 *  qgsvbsmaptoolmilix.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Oct 01, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbsmaptoolmilix.h"
#include "qgsvbsmilixannotationitem.h"
#include "qgsvbsmilixlayer.h"
#include "qgsmapcanvas.h"
#include <QMouseEvent>

QgsVBSMapToolMilix::QgsVBSMapToolMilix( QgsMapCanvas* canvas, QgsVBSMilixLayer* layer, const QString& symbolXml, const QString &symbolMilitaryName, int nMinPoints, bool hasVariablePoints, const QPixmap& preview )
    : QgsMapTool( canvas ), mSymbolXml( symbolXml ), mSymbolMilitaryName( symbolMilitaryName ), mMinNPoints( nMinPoints ), mNPressedPoints( 0 ), mHasVariablePoints( hasVariablePoints ), mPreview( preview ), mItem( 0 ), mLayer( layer )
{
  setCursor( QCursor( preview, -0.5 * preview.width(), -0.5 * preview.height() ) );
}

QgsVBSMapToolMilix::~QgsVBSMapToolMilix()
{
  // If an item is still set, it means that positioning was not finished when the tool is disabled -> delete it
  delete mItem;
}

void QgsVBSMapToolMilix::canvasPressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( mItem == 0 )
    {
      // Deselect any previously selected items
      QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
      if ( selectedItem )
      {
        selectedItem->setSelected( false );
      }

      mItem = new QgsVBSMilixAnnotationItem( mCanvas );
      mItem->setMapPosition( toMapCoordinates( e->pos() ) );
      mItem->setSymbolXml( mSymbolXml, mSymbolMilitaryName, mMinNPoints > 1 );
      mItem->setSelected( true );
      setCursor( Qt::CrossCursor );
      mNPressedPoints = 1;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mMinNPoints && mHasVariablePoints )
      {
        mItem->appendPoint( e->pos() );
      }
    }
    else if ( mNPressedPoints < mMinNPoints || mHasVariablePoints )
    {
      ++mNPressedPoints;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mMinNPoints && mHasVariablePoints )
      {
        mItem->appendPoint( e->pos() );
      }
    }

    if ( mNPressedPoints >= mMinNPoints && !mHasVariablePoints )
    {
      // Max points reached, stop
      mItem->finalize();
      mLayer->addItem( mItem->createMilixItem() );
      delete mItem;
      mCanvas->refresh();
      mItem = 0;
      mNPressedPoints = 0;
      mCanvas->unsetMapTool( this );
    }
  }
  else if ( e->button() == Qt::RightButton && mItem != 0 )
  {
    if ( mNPressedPoints + 1 >= mMinNPoints )
    {
      // Done with N point symbol, stop
      mItem->finalize();
      mLayer->addItem( mItem->createMilixItem() );
      delete mItem;
      mCanvas->refresh();
      mItem = 0;
      mCanvas->unsetMapTool( this );
    }
    else if ( mNPressedPoints + 1 < mMinNPoints )
    {
      // premature stop
      delete mItem;
      mItem = 0;
      mCanvas->unsetMapTool( this );
    }
  }
}

void QgsVBSMapToolMilix::canvasMoveEvent( QMouseEvent * e )
{
  if ( mItem != 0 && e->buttons() == Qt::NoButton )
  {
    mItem->movePoint( mItem->absolutePointIdx( mNPressedPoints ), e->pos() );
  }
}
