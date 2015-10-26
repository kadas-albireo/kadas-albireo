/***************************************************************************
                                qgsmeasureheightprofiledialog.cpp
                               ------------------
        begin                : October 2015
        copyright            : (C) 2015 Sandro Mani
        email                : manisandro@gmail.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscoordinatetransform.h"
#include "qgsmeasureheightprofiledialog.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgslogger.h"
#include <gdal.h>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>


QgsMeasureHeightProfileDialog::QgsMeasureHeightProfileDialog( QgsMeasureHeightProfileTool *tool, QWidget *parent, Qt::WindowFlags f )
    : QDialog( parent, f ), mTool( tool )
{
  setWindowTitle( tr( "Height profile" ) );
  QGridLayout* gridLayout = new QGridLayout( this );

  mPlot = new QwtPlot( this );
  mPlot->setCanvasBackground( Qt::white );
  mPlot->enableAxis( QwtPlot::yLeft );
  mPlot->enableAxis( QwtPlot::xBottom, false );
  mPlot->setAxisTitle( QwtPlot::yLeft, tr( "Height [m]" ) );
  gridLayout->addWidget( mPlot, 0, 0, 1, 2 );

  QwtPlotGrid* grid = new QwtPlotGrid();
  grid->setMajorPen( Qt::gray );
  grid->attach( mPlot );

  mPlotCurve = new QwtPlotCurve( tr( "Height profile" ) );
  mPlotCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
  mPlotCurve->setPen( Qt::blue );
  mPlotCurve->attach( mPlot );
  mPlotCurve->setData( new QwtPointSeriesData() );

  mPlotMarker = new QwtPlotMarker();
  mPlotMarker->setSymbol( new QwtSymbol( QwtSymbol::Ellipse, QBrush( Qt::blue ), QPen( Qt::blue ), QSize( 5, 5 ) ) );
  mPlotMarker->attach( mPlot );

  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  gridLayout->addWidget( bbox, 1, 0, 1, 2 );
  connect( bbox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( this, SIGNAL( finished( int ) ), this, SLOT( finish() ) );

  restoreGeometry( QSettings().value( "/Windows/MeasureHeightProfile/geometry" ).toByteArray() );
}

void QgsMeasureHeightProfileDialog::setPoints( const QgsPoint &p1, const QgsPoint &p2, const QgsCoordinateReferenceSystem &crs )
{
  mPoints.first = p1;
  mPoints.second = p2;
  mPointsCrs = crs;
  replot();
}

void QgsMeasureHeightProfileDialog::setMarkerPos( const QgsPoint &p )
{
  double l = qSqrt( mPoints.second.sqrDist( mPoints.first ) );
  double d = qSqrt( p.sqrDist( mPoints.first ) );
  double val = d / l * 100;
  mPlotMarker->setValue( val, mPlotCurve->data()->sample( val ).y() );
  mPlot->replot();
}

void QgsMeasureHeightProfileDialog::finish()
{
  QSettings().setValue( "/Windows/MeasureHeightProfile/geometry", saveGeometry() );
  mTool->restart();
  mTool->deactivate();
}

void QgsMeasureHeightProfileDialog::replot()
{
  QString rasterFile = QSettings().value( "/vbsfunctionality/heightmap" ).toString();

  if ( rasterFile.isEmpty() )
  {
    QgsDebugMsg( QString( "No raster specified" ) );
    return;
  }
  GDALDatasetH raster = GDALOpen( rasterFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( !raster )
  {
    QgsDebugMsg( QString( "Failed to open raster file: %1" ).arg( rasterFile ) );
    return;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    QgsDebugMsg( "Failed to get raster geotransform" );
    GDALClose( raster );
    return;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs( proj );
  if ( !rasterCrs.isValid() )
  {
    QgsDebugMsg( "Failed to get raster CRS" );
    GDALClose( raster );
    return;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !raster )
  {
    QgsDebugMsg( "Failed to open raster band 0" );
    GDALClose( raster );
    return;
  }

  // Take 100 points between chosen
  QVector<QPointF> samples;
  double d = qSqrt( mPoints.second.sqrDist( mPoints.first ) );
  QgsVector dir = QgsVector( mPoints.second - mPoints.first ).normal();
  for ( int i = 0; i < 100; ++i )
  {
    QgsPoint p = mPoints.first + dir * ( d * ( i / 100. ) );
    // Transform geo position to raster CRS
    QgsPoint pRaster = QgsCoordinateTransform( mPointsCrs, rasterCrs ).transform( p );


    // Transform raster geo position to pixel coordinates
    double col = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
    double row = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );

    double pixValues[4] = {};
    if ( CE_None != GDALRasterIO( band, GF_Read,
                                  qFloor( row ), qFloor( col ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
    {
      QgsDebugMsg( "Failed to read pixel values" );
      samples.append( QPointF( i, 0 ) );
    }
    else
    {
      // Interpolate values
      double lambdaR = row - qFloor( row );
      double lambdaC = col - qFloor( col );

      double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                     + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
      samples.append( QPointF( i, value ) );
    }
  }

  GDALClose( raster );

  static_cast<QwtPointSeriesData*>( mPlotCurve->data() )->setSamples( samples );
  mPlotMarker->setValue( 0, 0 );
  mPlot->replot();
}
