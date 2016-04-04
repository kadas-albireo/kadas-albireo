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

#include "qgisapp.h"
#include "qgscoordinatetransform.h"
#include "qgscoordinateformat.h"
#include "qgsimageannotationitem.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmeasureheightprofiledialog.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgslogger.h"
#include "qgsrubberband.h"
#include "qgsproject.h"
#include <gdal.h>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <qwt_scale_draw.h>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <qmath.h>


class QgsMeasureHeightProfileDialog::ScaleDraw : public QwtScaleDraw
{
  public:
    ScaleDraw( double maxVal, int nSamples ) : mFactor( maxVal / nSamples )
    {
    }
    QwtText label( double value ) const override
    {
      return QLocale::system().toString( value * mFactor );
    }
  private:
    double mFactor;
};


QgsMeasureHeightProfileDialog::QgsMeasureHeightProfileDialog( QgsMeasureHeightProfileTool *tool, QWidget *parent, Qt::WindowFlags f )
    : QDialog( parent, f ), mTool( tool ), mLineOfSightMarker( 0 ), mNSamples( 300 )
{
  setWindowTitle( tr( "Height profile" ) );
  setAttribute( Qt::WA_ShowWithoutActivating );
  QVBoxLayout* vboxLayout = new QVBoxLayout( this );

  QPushButton* pickButton = new QPushButton( QIcon( ":/images/themes/default/mActionSelect.svg" ), tr( "Measure along existing line" ), this );
  connect( pickButton, SIGNAL( clicked( bool ) ), mTool, SLOT( pickLine() ) );
  vboxLayout->addWidget( pickButton );

  QGis::UnitType heightDisplayUnit = QgsCoordinateFormat::instance()->getHeightDisplayUnit();

  mPlot = new QwtPlot( this );
  mPlot->setCanvasBackground( Qt::white );
  mPlot->enableAxis( QwtPlot::yLeft );
  mPlot->enableAxis( QwtPlot::xBottom );
  mPlot->setAxisTitle( QwtPlot::yLeft, heightDisplayUnit == QGis::Feet ? tr( "Height [ft]" ) : tr( "Height [m]" ) );
  mPlot->setAxisTitle( QwtPlot::xBottom, tr( "Distance [m]" ) );
  vboxLayout->addWidget( mPlot );

  QwtPlotGrid* grid = new QwtPlotGrid();
#if QWT_VERSION < 0x060000
  grid->setMajPen( QPen( Qt::gray ) );
#else
  grid->setMajorPen( QPen( Qt::gray ) );
#endif
  grid->attach( mPlot );

  mPlotCurve = new QwtPlotCurve( tr( "Height profile" ) );
  mPlotCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
  QPen curvePen;
  curvePen.setColor( Qt::red );
  curvePen.setJoinStyle( Qt::RoundJoin );
  mPlotCurve->setPen( curvePen );
  mPlotCurve->setBaseline( 0 );
  mPlotCurve->setBrush( QColor( 255, 127, 127 ) );
  mPlotCurve->attach( mPlot );
#if QWT_VERSION >= 0x060000
  mPlotCurve->setData( new QwtPointSeriesData() );
#endif

  mPlotMarker = new QwtPlotMarker();
#if QWT_VERSION < 0x060000
  mPlotMarker->setSymbol( QwtSymbol( QwtSymbol::Ellipse, QBrush( Qt::blue ), QPen( Qt::blue ), QSize( 5, 5 ) ) );
#else
  mPlotMarker->setSymbol( new QwtSymbol( QwtSymbol::Ellipse, QBrush( Qt::blue ), QPen( Qt::blue ), QSize( 5, 5 ) ) );
#endif
  mPlotMarker->attach( mPlot );

  mLineOfSightGroupBoxgroupBox = new QGroupBox( this );
  mLineOfSightGroupBoxgroupBox->setTitle( tr( "Line of sight" ) );
  mLineOfSightGroupBoxgroupBox->setCheckable( true );
  connect( mLineOfSightGroupBoxgroupBox, SIGNAL( clicked( bool ) ), this, SLOT( updateLineOfSight() ) );
  QGridLayout* layoutLOS = new QGridLayout();
  layoutLOS->setContentsMargins( 2, 2, 2, 2 );
  mLineOfSightGroupBoxgroupBox->setLayout( layoutLOS );
  layoutLOS->addWidget( new QLabel( tr( "Observer height:" ) ), 0, 0 );
  mObserverHeightSpinBox = new QDoubleSpinBox();
  mObserverHeightSpinBox->setRange( 0, 8000 );
  mObserverHeightSpinBox->setDecimals( 1 );
  mObserverHeightSpinBox->setSuffix( heightDisplayUnit == QGis::Feet ? " ft" : " m" );
  mObserverHeightSpinBox->setValue( QSettings().value( "/qgis/measure/heightprofile/observerheight", 2.0 ).toDouble() );
  mObserverHeightSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  connect( mObserverHeightSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( updateLineOfSight() ) );
  layoutLOS->addWidget( mObserverHeightSpinBox, 0, 1 );
  layoutLOS->addWidget( new QLabel( tr( "Target height:" ) ), 0, 2 );
  mTargetHeightSpinBox = new QDoubleSpinBox();
  mTargetHeightSpinBox->setRange( 0, 8000 );
  mTargetHeightSpinBox->setDecimals( 1 );
  mTargetHeightSpinBox->setSuffix( heightDisplayUnit == QGis::Feet ? " ft" : " m" );
  mTargetHeightSpinBox->setValue( QSettings().value( "/qgis/measure/heightprofile/targetheight", 2.0 ).toDouble() );
  mTargetHeightSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  connect( mTargetHeightSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( updateLineOfSight() ) );
  layoutLOS->addWidget( mTargetHeightSpinBox, 0, 3 );
  layoutLOS->addWidget( new QLabel( tr( "Heights relative to:" ) ), 0, 4 );
  mHeightModeCombo = new QComboBox();
  mHeightModeCombo->addItem( tr( "Ground" ), static_cast<int>( HeightRelToGround ) );
  mHeightModeCombo->addItem( tr( "Sea level" ), static_cast<int>( HeightRelToSeaLevel ) );
  connect( mHeightModeCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateLineOfSight() ) );
  layoutLOS->addWidget( mHeightModeCombo, 0, 5 );

  vboxLayout->addWidget( mLineOfSightGroupBoxgroupBox );

  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  QPushButton* copyButton = bbox->addButton( tr( "Copy to clipboard" ), QDialogButtonBox::ActionRole );
  QPushButton* addButton = bbox->addButton( tr( "Add to canvas" ), QDialogButtonBox::ActionRole );
  vboxLayout->addWidget( bbox );
  connect( bbox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( copyButton, SIGNAL( clicked( bool ) ), this, SLOT( copyToClipboard() ) );
  connect( addButton, SIGNAL( clicked( bool ) ), this, SLOT( addToCanvas() ) );
  connect( this, SIGNAL( finished( int ) ), this, SLOT( finish() ) );

  connect( QgsCoordinateFormat::instance(), SIGNAL( heightDisplayUnitChanged( QGis::UnitType ) ), this, SLOT( replot() ) );

  restoreGeometry( QSettings().value( "/Windows/MeasureHeightProfile/geometry" ).toByteArray() );
}

void QgsMeasureHeightProfileDialog::setPoints( const QList<QgsPoint>& points, const QgsCoordinateReferenceSystem &crs )
{
  mPoints = points;
  mPointsCrs = crs;
  mTotLength = 0;
  mSegmentLengths.clear();
  for ( int i = 0, n = mPoints.size() - 1; i < n; ++i )
  {
    mSegmentLengths.append( qSqrt( mPoints[i + 1].sqrDist( mPoints[i] ) ) );
    mTotLength += mSegmentLengths.back();
  }
  mLineOfSightGroupBoxgroupBox->setEnabled( points.size() == 2 );
  replot();
}

void QgsMeasureHeightProfileDialog::setMarkerPos( int segment, const QgsPoint &p )
{
  double x = qSqrt( p.sqrDist( mPoints[segment] ) );
  for ( int i = 0; i < segment; ++i )
  {
    x += mSegmentLengths[i];
  }
  int idx = qMin( int( x / mTotLength * mNSamples ), mNSamples - 1 );
#if QWT_VERSION < 0x060000
  mPlotMarker->setValue( mPlotCurve->x( idx ), mPlotCurve->y( idx ) );
#else
  mPlotMarker->setValue( mPlotCurve->data()->sample( idx ) );
#endif
  mPlot->replot();
}

void QgsMeasureHeightProfileDialog::clear()
{
#if QWT_VERSION < 0x060000
  mPlotCurve->setData( QVector<double>(), QVector<double>() );
#else
  static_cast<QwtPointSeriesData*>( mPlotCurve->data() )->setSamples( QVector<QPointF>() );
#endif
  mPlotMarker->setValue( 0, 0 );
  qDeleteAll( mLinesOfSight );
  mLinesOfSight.clear();
  qDeleteAll( mLinesOfSightRB );
  mLinesOfSightRB.clear();
  delete mLineOfSightMarker;
  mLineOfSightMarker = 0;
  mPlot->replot();
  mLineOfSightGroupBoxgroupBox->setEnabled( false );
}

void QgsMeasureHeightProfileDialog::finish()
{
  QSettings().setValue( "/Windows/MeasureHeightProfile/geometry", saveGeometry() );
  mTool->canvas()->unsetMapTool( mTool );
  clear();
}

void QgsMeasureHeightProfileDialog::replot()
{
  QGis::UnitType vertDisplayUnit = QgsCoordinateFormat::instance()->getHeightDisplayUnit();
  mPlot->setAxisTitle( QwtPlot::yLeft, vertDisplayUnit == QGis::Feet ? tr( "Height [ft]" ) : tr( "Height [m]" ) );
  mObserverHeightSpinBox->setSuffix( vertDisplayUnit == QGis::Feet ? " ft" : " m" );
  mTargetHeightSpinBox->setSuffix( vertDisplayUnit == QGis::Feet ? " ft" : " m" );

  if ( mPoints.isEmpty() )
  {
    return;
  }

  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayer::RasterLayer )
  {
    QgisApp::instance()->messageBar()->pushMessage( tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." ), QgsMessageBar::INFO, 10 );
    return;
  }
  QString rasterFile = layer->source();
  GDALDatasetH raster = GDALOpen( rasterFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( !raster )
  {
    QMessageBox::warning( 0, tr( "Error" ), tr( "Failed to open raster file: %1" ).arg( rasterFile ) );
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

  // Get vertical unit
  QGis::UnitType vertUnit = strcmp( GDALGetRasterUnitType( band ), "ft" ) == 0 ? QGis::Feet : QGis::Meters;
  double heightConversion = QGis::fromUnitToUnitFactor( vertUnit, vertDisplayUnit );
  mPlot->setAxisTitle( QwtPlot::yLeft, vertDisplayUnit == QGis::Feet ? tr( "Height [ft]" ) : tr( "Height [m]" ) );
  mObserverHeightSpinBox->setSuffix( vertDisplayUnit == QGis::Feet ? " ft" : " m" );
  mTargetHeightSpinBox->setSuffix( vertDisplayUnit == QGis::Feet ? " ft" : " m" );

#if QWT_VERSION < 0x060000
  QVector<double> xSamples, ySamples;
#else
  QVector<QPointF> samples;
#endif
  double x = 0;
  for ( int i = 0, n = mPoints.size() - 1; i < n; ++i )
  {
    if ( x >= mSegmentLengths[i] )
    {
      continue;
    }
    QgsVector dir = QgsVector( mPoints[i + 1] - mPoints[i] ).normal();
    while ( x < mSegmentLengths[i] )
    {
      QgsPoint p = mPoints[i] + dir * x;
      // Transform geo position to raster CRS
      QgsPoint pRaster = QgsCoordinateTransform( mPointsCrs, rasterCrs ).transform( p );


      // Transform raster geo position to pixel coordinates
      double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
      double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );

      double pixValues[4] = {};
      if ( CE_None != GDALRasterIO( band, GF_Read,
                                    qFloor( col ), qFloor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
      {
        QgsDebugMsg( "Failed to read pixel values" );
#if QWT_VERSION < 0x060000
        xSamples.append( xSamples.size() );
        ySamples.append( 0 );
#else
        samples.append( QPointF( samples.size(), 0 ) );
#endif
      }
      else
      {
        // Interpolate values
        double lambdaR = row - qFloor( row );
        double lambdaC = col - qFloor( col );

        double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                       + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
#if QWT_VERSION < 0x060000
        xSamples.append( xSamples.size() );
        ySamples.append( value * heightConversion );
#else
        samples.append( QPointF( samples.size(), value * heightConversion ) );
#endif
      }
      x += mTotLength / mNSamples;
    }
    x -= mSegmentLengths[i];
  }

#if QWT_VERSION < 0x060000
  mPlotCurve->setData( xSamples, ySamples );
#else
  static_cast<QwtPointSeriesData*>( mPlotCurve->data() )->setSamples( samples );
#endif
  mPlotMarker->setValue( 0, 0 );
  mPlot->setAxisScaleDraw( QwtPlot::xBottom, new ScaleDraw( mTotLength, xSamples.size() ) );
  double step = qPow( 10, qFloor( log10( mTotLength ) ) ) / ( mTotLength ) * xSamples.size();
  while ( xSamples.size() / step < 10 ) step /= 2.;
  mPlot->setAxisScale( QwtPlot::xBottom, 0, xSamples.size(), step );

  GDALClose( raster );

  updateLineOfSight( );
}

void QgsMeasureHeightProfileDialog::updateLineOfSight( )
{
  qDeleteAll( mLinesOfSight );
  mLinesOfSight.clear();
  qDeleteAll( mLinesOfSightRB );
  mLinesOfSightRB.clear();
  delete mLineOfSightMarker;
  mLineOfSightMarker = 0;

  if ( !mLineOfSightGroupBoxgroupBox->isEnabled() || !mLineOfSightGroupBoxgroupBox->isChecked() )
  {
    mPlot->replot();
    return;
  }

  int nSamples = mPlotCurve->dataSize();
#if QWT_VERSION < 0x060000
  QVector<QPointF> samples;
  for ( int i = 0; i < nSamples; ++i )
  {
    samples.append( QPointF( mPlotCurve->x( i ), mPlotCurve->y( i ) ) );
  }
#else
  QVector<QPointF> samples = static_cast<QwtPointSeriesData*>( mPlotCurve->data() )->samples();
#endif

  QVector< QVector<QPointF> > losSampleSet;
  losSampleSet.append( QVector<QPointF>() );
  bool heightRelToGround = static_cast<HeightMode>( mHeightModeCombo->itemData( mHeightModeCombo->currentIndex() ).toInt() ) == HeightRelToGround;

  double obsHeight = mObserverHeightSpinBox->value();
  if ( heightRelToGround )
    obsHeight += samples.front().y();
  QPointF p1( samples.front().x(), obsHeight );

  double meterToDisplayUnit = QGis::fromUnitToUnitFactor( QGis::Meters, QgsCoordinateFormat::instance()->getHeightDisplayUnit() );

  for ( int i = 0; i < nSamples - 1; ++i )
  {
    // Curvature correction
    double distFromObserver = samples[i].x() / mNSamples * mTotLength;
    double earthRadius = 6370000;
    double hCorr = 0.87 * distFromObserver * distFromObserver / ( 2 * earthRadius ) * meterToDisplayUnit;

    double targetHeight = mTargetHeightSpinBox->value();
    if ( heightRelToGround )
    {
      targetHeight += samples[i].y();
    }
    QPointF p2( samples[i].x(), targetHeight - hCorr );
    // X = p1.x() + d * (p2.x() - p1.x())
    // Y = p1.y() + d * (p2.y() - p1.y())
    // => d = (X - p1.x()) / (p2.x() - p1.x())
    // => Y = p1.y() + (X - p1.x()) / (p2.x() - p1.x()) * (p2.y() - p1.y())
    bool visible = true;
    for ( int j = 0; j < i; ++j )
    {
      double Y = p1.y() + ( samples[j].x() - p1.x() ) / ( p2.x() - p1.x() ) * ( p2.y() - p1.y() );
      if ( Y < samples[j].y() )
      {
        visible = false;
        break;
      }
    }
    if (( visible && losSampleSet.size() % 2 == 0 ) || ( !visible && losSampleSet.size() % 2 == 1 ) )
    {
      losSampleSet.append( QVector<QPointF>() );
    }
    losSampleSet.back().append( samples[i] );
  }

  Qt::GlobalColor colors[] = {Qt::green, Qt::red};
  int iColor = 0;
  foreach ( const QVector<QPointF>& losSamples, losSampleSet )
  {
    QwtPlotCurve* curve = new QwtPlotCurve( tr( "Line of sight" ) );
    curve->setRenderHint( QwtPlotItem::RenderAntialiased );
    QPen losPen;
    losPen.setColor( colors[iColor] );
    losPen.setWidth( 5 );
    curve->setPen( losPen );
#if QWT_VERSION < 0x060000
    QVector<double> losXSamples, losYSamples;
    foreach ( const QPointF& sample, losSamples )
    {
      losXSamples.append( sample.x() );
      losYSamples.append( sample.y() );
    }
    curve->setData( losXSamples, losYSamples );
#else
    curve->setData( new QwtPointSeriesData() );
    static_cast<QwtPointSeriesData*>( curve->data() )->setSamples( losSamples );
#endif
    curve->attach( mPlot );
    mLinesOfSight.append( curve );

    QgsRubberBand* rubberBand = new QgsRubberBand( mTool->canvas() );
    double lambda1 = losSamples.front().x() / mNSamples;
    double lambda2 = losSamples.back().x() / mNSamples;
    rubberBand->addPoint( mPoints[0] + ( mPoints[1] - mPoints[0] ) * lambda1 );
    rubberBand->addPoint( mPoints[0] + ( mPoints[1] - mPoints[0] ) * lambda2 );
    rubberBand->setColor( colors[iColor] );
    rubberBand->setWidth( 5 );
    mLinesOfSightRB.append( rubberBand );

    iColor = ( iColor + 1 ) % 2;
  }

  double observerHeight = mObserverHeightSpinBox->value();
  if ( heightRelToGround )
  {
    observerHeight += samples.front().y();
  }
  mLineOfSightMarker = new QwtPlotMarker();
#if QWT_VERSION < 0x060000
  QwtSymbol observerMarkerSymbol( QwtSymbol::LTriangle, QBrush( Qt::white ), QPen( Qt::black ), QSize( 8, 8 ) );
  mLineOfSightMarker->setSymbol( observerMarkerSymbol );
  mLineOfSightMarker->setValue( samples.front().x(), observerHeight );
#else
  QwtSymbol* observerMarkerSymbol = new QwtSymbol( QwtSymbol::Pixmap );
  observerMarkerSymbol->setPixmap( QPixmap( ":/images/themes/default/observer.svg" ) );
  observerMarkerSymbol->setPinPoint( QPointF( 0, 4 ) );
  mLineOfSightMarker->setSymbol( observerMarkerSymbol );
  mLineOfSightMarker->setValue( QPointF( samples.front().x(), observerHeight ) );
#endif
  mLineOfSightMarker->attach( mPlot );

  mPlot->replot();
}

void QgsMeasureHeightProfileDialog::copyToClipboard()
{
  QImage image( mPlot->size(), QImage::Format_ARGB32 );
  mPlotMarker->setVisible( false );
  mPlot->replot();
  mPlot->render( &image );
  mPlotMarker->setVisible( true );
  mPlot->replot();
  QApplication::clipboard()->setImage( image );
}

void QgsMeasureHeightProfileDialog::addToCanvas()
{
  QImage image( mPlot->size(), QImage::Format_ARGB32 );
  mPlotMarker->setVisible( false );
  mPlot->replot();
  mPlot->render( &image );
  mPlotMarker->setVisible( true );
  mPlot->replot();
  QgsImageAnnotationItem* item = new QgsImageAnnotationItem( mTool->canvas() );
  item->setImage( image );
  item->setMapPosition( mTool->canvas()->mapSettings().extent().center() );
}

void QgsMeasureHeightProfileDialog::keyPressEvent( QKeyEvent* ev )
{
  if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return )
  {
    ev->ignore();
  }
  else
  {
    QDialog::keyPressEvent( ev );
  }
}
