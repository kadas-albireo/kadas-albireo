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

#include "qgsfeaturepicker.h"
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
  mDialog->clear();
  mMoving = false;
  mPicking = false;
  mRubberBand->reset( QGis::Line );
  mRubberBandPoints->reset( QGis::Point );

  QSettings settings;
  int red = settings.value( "/qgis/default_measure_color_red", 222 ).toInt();
  int green = settings.value( "/qgis/default_measure_color_green", 155 ).toInt();
  int blue = settings.value( "/qgis/default_measure_color_blue", 67 ).toInt();
  mRubberBand->setColor( QColor( red, green, blue ) );
  mRubberBand->setWidth( 3 );
  mRubberBandPoints->setIcon( QgsRubberBand::ICON_CIRCLE );
  mRubberBandPoints->setIconSize( 10 );
  mRubberBandPoints->setFillColor( Qt::white );
  mRubberBandPoints->setBorderColor( QColor( red, green, blue ) );
  mRubberBandPoints->setWidth( 2 );
}

void QgsMeasureHeightProfileTool::activate()
{
  mDialog->show();
  QgsMapTool::activate();
}

void QgsMeasureHeightProfileTool::deactivate()
{
  restart();
  mDialog->close();
  QgsMapTool::deactivate();
}

void QgsMeasureHeightProfileTool::setGeometry( QgsGeometry* geometry, QgsVectorLayer *layer )
{
  mRubberBand->addGeometry( geometry, layer );
  mRubberBandPoints->addGeometry( geometry, layer );
  mDialog->setPoints(
    mRubberBandPoints->getPoints().front(),
    mCanvas->mapSettings().destinationCrs()
  );
  mRubberBandPoints->addPoint( *mRubberBandPoints->getPoint( 0, 0 ) );
}

void QgsMeasureHeightProfileTool::pickLine()
{
  restart();
  mPicking = true;
  setCursor( QCursor( Qt::ArrowCursor ) );
}

void QgsMeasureHeightProfileTool::canvasMoveEvent( QMouseEvent * e )
{
  QgsPoint p = toMapCoordinates( e->pos() );
  int nPoints = mRubberBand->partSize( 0 );
  if ( mMoving )
  {
    mRubberBand->movePoint( nPoints - 1 , p );
    mRubberBandPoints->movePoint( nPoints - 1, p );
  }
  else if ( mRubberBandPoints->partSize( 0 ) > 2 )
  {
    double minDist = std::numeric_limits<double>::max();
    int minIdx;
    QgsPoint minPos;
    for ( int i = 0; i < nPoints - 1; ++i )
    {
      const QgsPoint& p1 = *mRubberBandPoints->getPoint( 0, i );
      const QgsPoint& p2 = *mRubberBandPoints->getPoint( 0, i + 1 );
      QgsPointV2 pProjV2 = QgsGeometryUtils::projPointOnSegment( QgsPointV2( p ), QgsPointV2( p1 ), QgsPointV2( p2 ) );
      QgsPoint pProj( pProjV2.x(), pProjV2.y() );
      double dist = pProj.sqrDist( p );
      if ( dist < minDist )
      {
        minDist = dist;
        minPos = pProj;
        minIdx = i;
      }
    }
    if ( qSqrt( minDist ) / mCanvas->mapSettings().mapUnitsPerPixel() < 30. )
    {
      mRubberBandPoints->movePoint( nPoints, minPos );
      mDialog->setMarkerPos( minIdx, minPos );
    }
  }
}

void QgsMeasureHeightProfileTool::canvasReleaseEvent( QMouseEvent * e )
{
  QgsPoint p = toMapCoordinates( e->pos() );
  if ( mPicking )
  {
    QPair<QgsFeature, QgsVectorLayer*> pickResult = QgsFeaturePicker::pick( mCanvas, p, QGis::Line );
    if ( pickResult.first.isValid() && pickResult.first.geometry()->geometry()->vertexCount() > 1 )
    {
      setGeometry( pickResult.first.geometry(), pickResult.second );
    }
    mPicking = false;
    setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );
  }
  else if ( !mMoving )
  {
    restart();
    mRubberBand->addPoint( p );
    mRubberBandPoints->addPoint( p );
    mMoving = true;
  }
  else
  {
    if ( e->button() == Qt::LeftButton )
    {
      mRubberBand->addPoint( p );
      mRubberBandPoints->addPoint( p );
    }
    else if ( e->button() == Qt::RightButton )
    {
      if ( mRubberBandPoints->getPoints().front().size() > 1 )
      {
        mDialog->setPoints(
          mRubberBandPoints->getPoints().front(),
          mCanvas->mapSettings().destinationCrs()
        );
        mRubberBandPoints->addPoint( *mRubberBandPoints->getPoint( 0, 0 ) );
      }
      mMoving = false;
    }
  }
}
