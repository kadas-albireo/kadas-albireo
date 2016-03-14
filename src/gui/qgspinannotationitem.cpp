/***************************************************************************
                              qgspinannotationitem.cpp
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

#include "qgspinannotationitem.h"
#include "qgsproject.h"
#include "qgsbillboardregistry.h"
#include "qgscrscache.h"
#include <QApplication>
#include <QClipboard>
#include <QImageReader>
#include <QMenu>

QgsPinAnnotationItem::QgsPinAnnotationItem( QgsMapCanvas* canvas , QgsCoordinateUtils::TargetFormat targetFormat , const QString &targetEPSG )
    : QgsSvgAnnotationItem( canvas ), mTargetFormat( targetFormat ), mTargetEPSG( targetEPSG )
{
  setItemFlags( QgsAnnotationItem::ItemIsNotResizeable |
                QgsAnnotationItem::ItemHasNoFrame |
                QgsAnnotationItem::ItemHasNoMarker |
                QgsAnnotationItem::ItemIsNotEditable );
  QSize imageSize = QImageReader( ":/images/themes/default/pin_red.svg" ).size();
  setFilePath( ":/images/themes/default/pin_red.svg" );
  setFrameSize( imageSize );
  setOffsetFromReferencePoint( QPointF( -imageSize.width() / 2., -imageSize.height() ) );
}

QgsPinAnnotationItem::~QgsPinAnnotationItem()
{
  if ( !mIsClone )
  {
    QgsBillBoardRegistry::instance()->removeItem( this );
  }
}

QgsPinAnnotationItem::QgsPinAnnotationItem( QgsMapCanvas* canvas, QgsPinAnnotationItem* source )
    : QgsSvgAnnotationItem( canvas, source )
{
  mTargetFormat = source->mTargetFormat;
  mTargetEPSG = source->mTargetEPSG;
}

void QgsPinAnnotationItem::changeCoordinateFormatter( QgsCoordinateUtils::TargetFormat targetFormat, const QString &targetEPSG )
{
  mTargetFormat = targetFormat;
  mTargetEPSG = targetEPSG;
  updateToolTip();
}

void QgsPinAnnotationItem::updateToolTip()
{
  QString posStr = QgsCoordinateUtils::getDisplayString( mGeoPos, mGeoPosCrs, mTargetFormat, mTargetEPSG );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mGeoPos.toString() ).arg( mGeoPosCrs.authid() );
  }
  QString toolTipText = tr( "Position: %1\nHeight: %3" )
                        .arg( posStr )
                        .arg( QgsCoordinateUtils::getHeightAtPos( mGeoPos, mGeoPosCrs, QGis::Meters ) );
  setToolTip( toolTipText );
}

void QgsPinAnnotationItem::setMapPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem &crs )
{
  QgsSvgAnnotationItem::setMapPosition( pos, crs );
  updateToolTip();
  if ( !mIsClone )
  {
    QgsPoint worldPos = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" )->transform( mGeoPos );
    QgsBillBoardRegistry::instance()->addItem( this, getImage(), worldPos );
  }
}

void QgsPinAnnotationItem::showContextMenu( const QPoint& screenPos )
{
  QMenu menu;
  menu.addAction( tr( "Copy position" ), this, SLOT( copyPosition() ) );
  menu.addAction( tr( "Remove" ), this, SLOT( deleteLater() ) );
  menu.exec( screenPos );
}

void QgsPinAnnotationItem::writeXML( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( !documentElem.isNull() )
  {
    QDomElement pinAnnotationElem = doc.createElement( "PinAnnotationItem" );
    pinAnnotationElem.setAttribute( "file", QgsProject::instance()->writePath( mFilePath ) );
    _writeXML( doc, pinAnnotationElem );
    documentElem.appendChild( pinAnnotationElem );
  }
}

void QgsPinAnnotationItem::copyPosition()
{
  QApplication::clipboard()->setText( toolTip() );
}
