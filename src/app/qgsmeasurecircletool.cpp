/***************************************************************************
    qgsmeasurecircletool.cpp  -  map tool for measuring circular areas
    ---------------------
    begin                : October 2015
    copyright            : (C) 2015 Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscursors.h"
#include "qgsfeaturepicker.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmeasuredialog.h"
#include "qgsmeasurecircletool.h"
#include "qgsrubberband.h"
#include "qgssnappingutils.h"
#include "qgsvectorlayer.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>


void QgsMeasureCircleTool::updateLabel( int idx )
{
  QList<QgsPoint> radius;
  radius.append( *mRubberBandPoints->getPoint( idx, 0 ) );
  radius.append( *mRubberBandPoints->getPoint( idx, 1 ) );
  mTextLabels[idx]->setHtml( QString(
                               "<div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">%1</div>"
                               "<div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">(r=%2)</div>" )
                             .arg( mDialog->getPartMeasurement( idx ) )
                             .arg( mDialog->formatValue( mDialog->measureGeometry( radius, false ), false ) ) );
}

void QgsMeasureCircleTool::updateRubberbandGeometry( const QgsPoint& point )
{
  double r = QgsVector( point - mCenterPos ).length();
  double d2r = M_PI / 180.;
  for ( int i = 0; i < 360; ++i )
  {
    mRubberBand->movePoint( i,
                            QgsPoint( mCenterPos.x() + r * qCos( i * d2r ), mCenterPos.y() + r * qSin( i * d2r ) ),
                            mCurrentPart,
                            i == 359 );
  }
}

//////////////////////////

void QgsMeasureCircleTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( mCurrentPart >= 0 && mRubberBand->getPoints().size() > mCurrentPart )
  {
    QgsPoint point = snapPoint( e->pos() );

    updateRubberbandGeometry( point );
    mRubberBandPoints->movePoint( point, mCurrentPart );
    mDialog->updateMeasurements();
    mTextLabels.back()->setPos( toCanvasCoordinates( mRubberBand->partMidpoint( mCurrentPart ) ) );
    updateLabel( mCurrentPart );
  }
}

void QgsMeasureCircleTool::canvasReleaseEvent( QMouseEvent * e )
{
  QgsPoint point = snapPoint( e->pos() );
  if ( mPickFeature )
  {
    QPair<QgsFeature, QgsVectorLayer*> pickResult = QgsFeaturePicker::pick( toMapCoordinates( e->pos() ), QGis::Polygon );
    if ( pickResult.first.isValid() )
    {
      QgsRectangle bbox = pickResult.first.geometry()->boundingBox();
      QgsPoint p1 = toMapCoordinates( pickResult.second, QgsPoint( bbox.xMinimum(), bbox.yMinimum() ) );
      QgsPoint p2 = toMapCoordinates( pickResult.second, QgsPoint( bbox.xMaximum(), bbox.yMaximum() ) );
      QgsPoint center( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) );
      double radius = qSqrt( p1.sqrDist( center ) );
      initCircle( toMapCoordinates( pickResult.second, center ), radius );
      updateLabel( mCurrentPart );
      ++mCurrentPart;
    }
    mPickFeature = false;
    setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );
  }
  else if ( e->button() == Qt::LeftButton && mCurrentPart == -1 )
  {
    ++mCurrentPart;
    initCircle( point );
  }
  else if ( mRubberBand->getPoints().size() - 1 == mCurrentPart )
  {
    // Close current part
    ++mCurrentPart;
  }
  else if ( mRubberBand->getPoints().size() == mCurrentPart && e->button() == Qt::LeftButton )
  {
    initCircle( point );
  }
}

void QgsMeasureCircleTool::initCircle( const QgsPoint &center, double radius )
{
  addPart( toCanvasCoordinates( center ) );
  mCenterPos = center;

  double d2r = M_PI / 180.;
  for ( int i = 0; i < 360; ++i )
  {
    mRubberBand->addPoint(
      QgsPoint( mCenterPos.x() + radius * qCos( i * d2r ), mCenterPos.y() + radius * qSin( i * d2r ) ),
      i == 359, mCurrentPart );
  }
  mRubberBandPoints->addPoint( center, false, mCurrentPart );
  mRubberBandPoints->addPoint( *mRubberBand->getPoint( mCurrentPart, 0 ), true, mCurrentPart );

  mDialog->updateMeasurements();
}
