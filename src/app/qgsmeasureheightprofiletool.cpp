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
#include "qgsmaptooldrawshape.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgsmeasureheightprofiledialog.h"
#include "qgsmapcanvas.h"
#include "qgsrubberband.h"
#include "qgsgeometryutils.h"
#include "qgsvectorlayer.h"

#include <QMouseEvent>

QgsMeasureHeightProfileTool::QgsMeasureHeightProfileTool( QgsMapCanvas *canvas )
    : QgsMapTool( canvas ), mPicking( false )
{
  setCursor( Qt::ArrowCursor );

  mDrawTool = new QgsMapToolDrawPolyLine( canvas, false );
  mDrawTool->setShowInputWidget( true );
  mDrawTool->setShowNodes( true );

  QSettings settings;
  int red = settings.value( "/qgis/default_measure_color_red", 255 ).toInt();
  int green = settings.value( "/qgis/default_measure_color_green", 0 ).toInt();
  int blue = settings.value( "/qgis/default_measure_color_blue", 0 ).toInt();

  mPosMarker = new QgsRubberBand( canvas, QGis::Point );
  mPosMarker->setIcon( QgsRubberBand::ICON_CIRCLE );
  mPosMarker->setIconSize( 10 );
  mPosMarker->setFillColor( Qt::white );
  mPosMarker->setBorderColor( QColor( red, green, blue ) );
  mPosMarker->setWidth( 2 );

  mDialog = new QgsMeasureHeightProfileDialog( this, 0, Qt::WindowStaysOnTopHint );
  connect( mDrawTool, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
  connect( mDrawTool, SIGNAL( cleared() ), this, SLOT( drawCleared() ) );
}

QgsMeasureHeightProfileTool::~QgsMeasureHeightProfileTool()
{
  delete mDrawTool;
  delete mPosMarker;
}

void QgsMeasureHeightProfileTool::activate()
{
  mPicking = false;
  mDialog->show();
  mDrawTool->activate();
  QgsMapTool::activate();
}

void QgsMeasureHeightProfileTool::deactivate()
{
  mDrawTool->deactivate();
  mDialog->close();
  mDialog->setPoints( QList<QgsPoint>(), mCanvas->mapSettings().destinationCrs() );
  QgsMapTool::deactivate();
}

void QgsMeasureHeightProfileTool::setGeometry( QgsGeometry* geometry, QgsVectorLayer *layer )
{
  mDrawTool->reset();
  mDrawTool->addGeometry( geometry->geometry(), layer->crs() );
  drawFinished();
}

void QgsMeasureHeightProfileTool::pickLine()
{
  mDrawTool->reset();
  mPicking = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void QgsMeasureHeightProfileTool::canvasPressEvent( QMouseEvent *e )
{
  if ( !mPicking )
  {
    mDrawTool->canvasPressEvent( e );
  }
}

void QgsMeasureHeightProfileTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( !mPicking )
  {
    if ( mDrawTool->isFinished() && mDrawTool->getPartCount() > 0 )
    {
      QgsPoint p = toMapCoordinates( e->pos() );
      QList<QgsPoint> points;
      mDrawTool->getPart( 0, points );
      double minDist = std::numeric_limits<double>::max();
      int minIdx;
      QgsPoint minPos;
      for ( int i = 0, nPoints = points.size(); i < nPoints - 1; ++i )
      {
        const QgsPoint& p1 = points[i];
        const QgsPoint& p2 = points[i + 1];
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
        mPosMarker->movePoint( 0, minPos );
        mDialog->setMarkerPos( minIdx, minPos );
      }
    }
    mDrawTool->canvasMoveEvent( e );
  }
}

void QgsMeasureHeightProfileTool::canvasReleaseEvent( QMouseEvent *e )
{
  if ( !mPicking )
  {
    mDrawTool->canvasReleaseEvent( e );
  }
  else
  {
    QgsFeaturePicker::PickResult pickResult = QgsFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), QGis::Line );
    if ( pickResult.feature.isValid() )
    {
      setGeometry( pickResult.feature.geometry(), static_cast<QgsVectorLayer*>( pickResult.layer ) );
    }
    mPicking = false;
    setCursor( Qt::ArrowCursor );
  }
}

void QgsMeasureHeightProfileTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( mPicking && e->key() == Qt::Key_Escape )
  {
    mPicking = false;
    setCursor( Qt::ArrowCursor );
  }
  else
  {
    mDrawTool->keyReleaseEvent( e );
  }
}


void QgsMeasureHeightProfileTool::drawCleared()
{
  mPosMarker->reset( QGis::Point );
  mDialog->clear();
}

void QgsMeasureHeightProfileTool::drawFinished()
{
  QList<QgsPoint> points;
  mDrawTool->getPart( 0, points );
  if ( points.size() > 0 )
  {
    mDialog->setPoints( points, mCanvas->mapSettings().destinationCrs() );
    mPosMarker->addPoint( points[0] );
  }
}
