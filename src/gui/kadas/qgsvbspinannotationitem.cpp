/***************************************************************************
                              qgsvbspinannotationitem.cpp
                              ------------------------
  begin                : August, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbspinannotationitem.h"
#include "qgscoordinatetransform.h"
#include "qgsvbscoordinatedisplayer.h"
#include "qgsmapcanvas.h"
#include <QApplication>
#include <QClipboard>
#include <QGraphicsSceneContextMenuEvent>
#include <QImageReader>
#include <qmath.h>
#include <QMenu>
#include <QSettings>

QgsVBSPinAnnotationItem::QgsVBSPinAnnotationItem( QgsMapCanvas* canvas , QgsVBSCoordinateDisplayer *coordinateDisplayer )
    : QgsSvgAnnotationItem( canvas ), mCoordinateDisplayer( coordinateDisplayer )
{
  setItemFlags( QgsAnnotationItem::ItemIsNotResizeable |
                QgsAnnotationItem::ItemHasNoFrame |
                QgsAnnotationItem::ItemHasNoMarker |
                QgsAnnotationItem::ItemIsNotEditable );
  QSize imageSize = QImageReader( ":/vbsfunctionality/icons/pin_red.svg" ).size();
  setFilePath( ":/vbsfunctionality/icons/pin_red.svg" );
  setFrameSize( imageSize );
  setOffsetFromReferencePoint( QPointF( -imageSize.width() / 2., -imageSize.height() ) );
  connect( mCoordinateDisplayer, SIGNAL( displayFormatChanged() ), this, SLOT( updateToolTip() ) );
}

void QgsVBSPinAnnotationItem::updateToolTip()
{
  QString posStr = mCoordinateDisplayer->getDisplayString( mGeoPos, mGeoPosCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mGeoPos.toString() ).arg( mGeoPosCrs.authid() );
  }
  QString toolTipText = tr( "Position: %1\nHeight: %3" )
                        .arg( posStr )
                        .arg( mCoordinateDisplayer->getHeightAtPos( mGeoPos, mGeoPosCrs, QGis::Meters ) );
  setToolTip( toolTipText );
}

void QgsVBSPinAnnotationItem::setMapPosition( const QgsPoint& pos )
{
  QgsSvgAnnotationItem::setMapPosition( pos );
  updateToolTip();
}

void QgsVBSPinAnnotationItem::showContextMenu( const QPoint& screenPos )
{
  QMenu menu;
  menu.addAction( tr( "Copy position" ), this, SLOT( copyPosition() ) );
  menu.addAction( tr( "Remove" ), this, SLOT( deleteLater() ) );
  menu.exec( screenPos );
}

void QgsVBSPinAnnotationItem::copyPosition()
{
  QApplication::clipboard()->setText( toolTip() );
}
