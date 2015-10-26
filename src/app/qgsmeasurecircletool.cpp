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

#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsrubberband.h"
#include "qgssnappingutils.h"

#include "qgsmeasuredialog.h"
#include "qgsmeasurecircletool.h"
#include "qgscursors.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>


void QgsMeasureCircleTool::updateLabel( int idx )
{
  QList<QgsPoint> radius;
  radius.append( *mRubberBandPoints->getPoint( idx, 0 ) );
  radius.append( *mRubberBandPoints->getPoint( idx, 1 ) );
  mTextLabels[idx]->setHtml( QString( "<div style=\"background: rgba(255, 255, 255, 64); padding: 5px; border-radius: 5px;\">%1<br />(r=%2)</div>" )
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
                            i == 359
                          );
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
  if ( e->button() == Qt::LeftButton && mCurrentPart == -1 )
  {
    ++mCurrentPart;
    addPart( point );
  }
  else if ( mRubberBand->getPoints().size() - 1 == mCurrentPart )
  {
    // Close current part
    ++mCurrentPart;
  }
  else if ( mRubberBand->getPoints().size() == mCurrentPart && e->button() == Qt::LeftButton )
  {
    addPart( point );
  }
}

void QgsMeasureCircleTool::addPart( const QgsPoint &point )
{
  mCenterPos = point;

  mTextLabels.append( new QGraphicsTextItem( "", 0, mCanvas->scene() ) );
  mTextLabels.back()->setDefaultTextColor( Qt::blue );
  QFont font = mTextLabels.back()->font();
  font.setBold( true );
  mTextLabels.back()->setFont( font );
  mTextLabels.back()->setPos( toCanvasCoordinates( point ) );

  double d2r = M_PI / 180.;
  for ( int i = 0; i < 360; ++i )
  {
    mRubberBand->addPoint(
      QgsPoint( mCenterPos.x() + 0.0001 * qCos( i * d2r ), mCenterPos.y() + 0.0001 * qSin( i * d2r ) ),
      i == 359, mCurrentPart );
  }
  mRubberBandPoints->addPoint( point, true, mCurrentPart );

  mDialog->addPart();
  mDialog->updateMeasurements();
}
