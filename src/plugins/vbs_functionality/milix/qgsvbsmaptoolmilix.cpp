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
#include "qgsvbsmilixmanager.h"
#include "qgsmapcanvas.h"
#include <QMouseEvent>

QgsVBSMapToolMilix::QgsVBSMapToolMilix( QgsMapCanvas* canvas, QgsVBSMilixManager* manager, const QString& symbolXml, int nPoints, const QPixmap& preview )
    : QgsMapTool( canvas ), mSymbolXml( symbolXml ), mNPoints( nPoints ), mNPressedPoints( 0 ), mPreview( preview ), mItem( 0 ), mManager( manager )
{
  setCursor( QCursor( preview, 0, 0 ) );
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
      mItem->setSymbolXml( mSymbolXml );
      mItem->setMapPosition( toMapCoordinates( e->pos() ) );
      mItem->setSelected( true );
      mNPressedPoints = 0;
      setCursor( Qt::CrossCursor );
    }
    if ( mNPressedPoints < mNPoints || mNPoints == -1 )
    {
      ++mNPressedPoints;
      mItem->addPoint( toMapCoordinates( e->pos() ) );
    }

    if ( mNPressedPoints >= mNPoints && mNPoints != -1 )
    {
      // Max points reached, stop
      mManager->addItem( mItem );
      mItem = 0;
      setCursor( QCursor( mPreview, 0, 0 ) );
    }
  }
  else if ( e->button() == Qt::RightButton && mItem != 0 )
  {
    if ( mNPoints == -1 )
    {
      // Done with N point symbol, stop
      mManager->addItem( mItem );
      mItem = 0;
      setCursor( QCursor( mPreview, 0, 0 ) );
    }
    else if ( mNPressedPoints < mNPoints )
    {
      // premature stop
      delete mItem;
      mItem = 0;
      setCursor( QCursor( mPreview, 0, 0 ) );
    }
  }
}

void QgsVBSMapToolMilix::canvasMoveEvent( QMouseEvent * e )
{
  if ( mItem != 0 && e->buttons() == Qt::NoButton )
  {
    mItem->modifyPoint( mNPressedPoints - 1, toMapCoordinates( e->pos() ) );
  }
}
