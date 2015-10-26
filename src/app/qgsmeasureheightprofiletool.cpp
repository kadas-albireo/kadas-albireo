/***************************************************************************
    qgsmeasureheightprofiletool.cpp  -  map tool for measuring height profiles
    ---------------------
    begin                : October 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmeasureheightprofiletool.h"
#include "qgsmeasureheightprofiledialog.h"
#include "qgsmapcanvas.h"
#include "qgscursors.h"
#include "qgsrubberband.h"
#include "qgsgeometryutils.h"

#include <QMouseEvent>

QgsMeasureHeightProfileTool::QgsMeasureHeightProfileTool( QgsMapCanvas *canvas )
    : QgsMapTool( canvas )
{
  setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );

  mRubberBand = new QgsRubberBand( canvas, QGis::Line );
  mRubberBandPoints = new QgsRubberBand( canvas, QGis::Point );

  mDialog = new QgsMeasureHeightProfileDialog( this, 0, Qt::WindowStaysOnTopHint );
  restart();
}

QgsMeasureHeightProfileTool::~QgsMeasureHeightProfileTool()
{
  delete mRubberBand;
  delete mRubberBandPoints;
}

void QgsMeasureHeightProfileTool::restart()
{
  mMoving = false;
  mRubberBand->reset( QGis::Line );
  mRubberBandPoints->reset( QGis::Point );

  int red = QSettings().value( "/qgis/default_measure_color_red", 222 ).toInt();
  int green = QSettings().value( "/qgis/default_measure_color_green", 155 ).toInt();
  int blue = QSettings().value( "/qgis/default_measure_color_blue", 67 ).toInt();
  mRubberBand->setColor( QColor( red, green, blue, 100 ) );
  mRubberBand->setWidth( 3 );
  mRubberBandPoints->setIcon( QgsRubberBand::ICON_CIRCLE );
  mRubberBandPoints->setIconSize( 10 );
  mRubberBandPoints->setColor( QColor( red, green, blue, 150 ) );
}

void QgsMeasureHeightProfileTool::activate()
{
  mDialog->show();
  QgsMapTool::activate();
}

void QgsMeasureHeightProfileTool::deactivate()
{
  mDialog->hide();
  restart();
  QgsMapTool::deactivate();
}

void QgsMeasureHeightProfileTool::canvasMoveEvent( QMouseEvent * e )
{
  QgsPoint p = toMapCoordinates( e->pos() );
  if ( mMoving )
  {
    mRubberBand->movePoint( 1, p );
    mRubberBandPoints->movePoint( 1, p );
  }
  else if ( mRubberBandPoints->partSize( 0 ) == 3 )
  {
    const QgsPoint& p1 = *mRubberBandPoints->getPoint( 0, 0 );
    const QgsPoint& p2 = *mRubberBandPoints->getPoint( 0, 1 );
    QgsPointV2 pProj = QgsGeometryUtils::projPointOnSegment( QgsPointV2( p ), QgsPointV2( p1 ), QgsPointV2( p2 ) );
    mRubberBandPoints->movePoint( 2, QgsPoint( pProj.x(), pProj.y() ) );
    mDialog->setMarkerPos( QgsPoint( pProj.x(), pProj.y() ) );
  }
}

void QgsMeasureHeightProfileTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( !mMoving )
  {
    restart();
    QgsPoint p = toMapCoordinates( e->pos() );
    mRubberBand->addPoint( p );
    mRubberBandPoints->addPoint( p );
    mMoving = true;
  }
  else
  {
    mDialog->setPoints(
      *mRubberBandPoints->getPoint( 0, 0 ),
      *mRubberBandPoints->getPoint( 0, 1 ),
      mCanvas->mapSettings().destinationCrs()
    );
    mMoving = false;
    mRubberBandPoints->addPoint( *mRubberBandPoints->getPoint( 0, 0 ) );
  }
}
