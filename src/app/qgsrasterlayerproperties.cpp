/***************************************************************************
  qgsrasterlayerproperties.cpp  -  description
  -------------------
      begin                : 1/1/2004
      copyright            : (C) 2004 Tim Sutton
      email                : tim@linfiniti.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <limits>
#include <typeinfo>

#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgsbilinearrasterresampler.h"
#include "qgscontexthelp.h"
#include "qgscontrastenhancement.h"
#include "qgscoordinatetransform.h"
#include "qgscubicrasterresampler.h"
#include "qgsgenericprojectionselector.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprenderer.h"
#include "qgsmaptoolemitpoint.h"
#include "qgsmaptopixel.h"
#include "qgsmultibandcolorrendererwidget.h"
#include "qgspalettedrendererwidget.h"
#include "qgsproject.h"
#include "qgsrasterbandstats.h"
#include "qgsrasterhistogramwidget.h"
#include "qgsrasterlayer.h"
#include "qgsrasterlayerproperties.h"
#include "qgsrasterpyramid.h"
#include "qgsrasterrenderer.h"
#include "qgsrasterrendererregistry.h"
#include "qgsrastertransparency.h"
#include "qgssinglebandgrayrendererwidget.h"
#include "qgssinglebandpseudocolorrendererwidget.h"

#include <QTableWidgetItem>
#include <QHeaderView>

#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPolygonF>
#include <QColorDialog>
#include <QList>
#include <QSettings>
#include <QMouseEvent>
#include <QVector>

QgsRasterLayerProperties::QgsRasterLayerProperties( QgsMapLayer* lyr, QgsMapCanvas* theCanvas, QWidget *parent, Qt::WFlags fl )
    : QDialog( parent, fl ),
    // Constant that signals property not used.
    TRSTRING_NOT_SET( tr( "Not Set" ) ),
    mRasterLayer( qobject_cast<QgsRasterLayer *>( lyr ) ), mRendererWidget( 0 )
{
  mGrayMinimumMaximumEstimated = true;
  mRGBMinimumMaximumEstimated = true;

  setupUi( this );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( this, SIGNAL( accepted() ), this, SLOT( apply() ) );
  connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked() ), this, SLOT( apply() ) );

  connect( sliderTransparency, SIGNAL( valueChanged( int ) ), this, SLOT( sliderTransparency_valueChanged( int ) ) );

  // enable or disable Build Pyramids button depending on selection in pyramid list
  connect( lbxPyramidResolutions, SIGNAL( itemSelectionChanged() ), this, SLOT( toggleBuildPyramidsButton() ) );

  // set up the scale based layer visibility stuff....
  chkUseScaleDependentRendering->setChecked( lyr->hasScaleBasedVisibility() );
  bool projectScales = QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" );
  if ( projectScales )
  {
    QStringList scalesList = QgsProject::instance()->readListEntry( "Scales", "/ScalesList" );
    cbMinimumScale->updateScales( scalesList );
    cbMaximumScale->updateScales( scalesList );
  }
  cbMinimumScale->setScale( 1.0 / lyr->minimumScale() );
  cbMaximumScale->setScale( 1.0 / lyr->maximumScale() );


  leNoDataValue->setValidator( new QDoubleValidator( -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 1000, this ) );

  // build GUI components
  QIcon myPyramidPixmap( QgsApplication::getThemeIcon( "/mIconPyramid.png" ) );
  QIcon myNoPyramidPixmap( QgsApplication::getThemeIcon( "/mIconNoPyramid.png" ) );

  pbnAddValuesManually->setIcon( QgsApplication::getThemeIcon( "/mActionNewAttribute.png" ) );
  pbnAddValuesFromDisplay->setIcon( QgsApplication::getThemeIcon( "/mActionContextHelp.png" ) );
  pbnRemoveSelectedRow->setIcon( QgsApplication::getThemeIcon( "/mActionDeleteAttribute.png" ) );
  pbnDefaultValues->setIcon( QgsApplication::getThemeIcon( "/mActionRemove.png" ) );
  pbnImportTransparentPixelValues->setIcon( QgsApplication::getThemeIcon( "/mActionFileOpen.png" ) );
  pbnExportTransparentPixelValues->setIcon( QgsApplication::getThemeIcon( "/mActionFileSave.png" ) );

  mMapCanvas = theCanvas;
  mPixelSelectorTool = 0;
  if ( mMapCanvas )
  {
    mPixelSelectorTool = new QgsMapToolEmitPoint( theCanvas );
    connect( mPixelSelectorTool, SIGNAL( canvasClicked( const QgsPoint&, Qt::MouseButton ) ), this, SLOT( pixelSelected( const QgsPoint& ) ) );
  }
  else
  {
    pbnAddValuesFromDisplay->setEnabled( false );
  }

  if ( !mRasterLayer )
  {
    return;
  }

  QgsRasterDataProvider* provider = mRasterLayer->dataProvider();

  // Only do pyramids if dealing directly with GDAL.
  if ( provider->capabilities() & QgsRasterDataProvider::BuildPyramids )
  {
    // initialize resampling methods
    cboResamplingMethod->clear();
    foreach ( QString method, QgsRasterDataProvider::pyramidResamplingMethods( mRasterLayer->providerType() ) )
      cboResamplingMethod->addItem( method );

    // build pyramid list
    QList< QgsRasterPyramid > myPyramidList = provider->buildPyramidList();
    QList< QgsRasterPyramid >::iterator myRasterPyramidIterator;

    for ( myRasterPyramidIterator = myPyramidList.begin();
          myRasterPyramidIterator != myPyramidList.end();
          ++myRasterPyramidIterator )
    {
      if ( myRasterPyramidIterator->exists )
      {
        lbxPyramidResolutions->addItem( new QListWidgetItem( myPyramidPixmap,
                                        QString::number( myRasterPyramidIterator->xDim ) + QString( " x " ) +
                                        QString::number( myRasterPyramidIterator->yDim ) ) );
      }
      else
      {
        lbxPyramidResolutions->addItem( new QListWidgetItem( myNoPyramidPixmap,
                                        QString::number( myRasterPyramidIterator->xDim ) + QString( " x " ) +
                                        QString::number( myRasterPyramidIterator->yDim ) ) );
      }
    }
  }
  else
  {
    // disable Pyramids tab completely
    tabPagePyramids->setEnabled( false );
  }

  if ( !( provider->capabilities() & QgsRasterDataProvider::Histogram ) )
  {
    // disable Histogram tab completely
    tabPageHistogram->setEnabled( false );
  }

  QgsDebugMsg( "Setting crs to " + mRasterLayer->crs().toWkt() );
  QgsDebugMsg( "Setting crs to " + mRasterLayer->crs().authid() + " - " + mRasterLayer->crs().description() );
  leSpatialRefSys->setText( mRasterLayer->crs().authid() + " - " + mRasterLayer->crs().description() );
  leSpatialRefSys->setCursorPosition( 0 );

  // Set text for pyramid info box
  QString pyramidFormat( "<h2>%1</h2><p>%2 %3 %4</p><b><font color='red'><p>%5</p><p>%6</p>" );
  QString pyramidHeader    = tr( "Description" );
  QString pyramidSentence1 = tr( "Large resolution raster layers can slow navigation in QGIS." );
  QString pyramidSentence2 = tr( "By creating lower resolution copies of the data (pyramids) performance can be considerably improved as QGIS selects the most suitable resolution to use depending on the level of zoom." );
  QString pyramidSentence3 = tr( "You must have write access in the directory where the original data is stored to build pyramids." );
  QString pyramidSentence4 = tr( "Please note that building internal pyramids may alter the original data file and once created they cannot be removed!" );
  QString pyramidSentence5 = tr( "Please note that building internal pyramids could corrupt your image - always make a backup of your data first!" );

  tePyramidDescription->setHtml( pyramidFormat.arg( pyramidHeader ).arg( pyramidSentence1 )
                                 .arg( pyramidSentence2 ).arg( pyramidSentence3 )
                                 .arg( pyramidSentence4 ).arg( pyramidSentence5 ) );

  QSettings settings;
  restoreGeometry( settings.value( "/Windows/RasterLayerProperties/geometry" ).toByteArray() );

  setWindowTitle( tr( "Layer Properties - %1" ).arg( lyr->name() ) );

  tableTransparency->horizontalHeader()->setResizeMode( 0, QHeaderView::Stretch );
  tableTransparency->horizontalHeader()->setResizeMode( 1, QHeaderView::Stretch );

  //resampling
  const QgsRasterRenderer* renderer = mRasterLayer->renderer();
  mZoomedInResamplingComboBox->insertItem( 0, tr( "Nearest neighbour" ) );
  mZoomedInResamplingComboBox->insertItem( 1, tr( "Bilinear" ) );
  mZoomedInResamplingComboBox->insertItem( 2, tr( "Cubic" ) );
  mZoomedOutResamplingComboBox->insertItem( 0, tr( "Nearest neighbour" ) );
  mZoomedOutResamplingComboBox->insertItem( 1, tr( "Average" ) );

  const QgsRasterResampleFilter* resampleFilter = mRasterLayer->resampleFilter();
  //set combo boxes to current resampling types
  if ( resampleFilter )
  {
    //invert color map
    if ( renderer->invertColor() )
    {
      mInvertColorMapCheckBox->setCheckState( Qt::Checked );
    }

    const QgsRasterResampler* zoomedInResampler = resampleFilter->zoomedInResampler();
    if ( zoomedInResampler )
    {
      if ( zoomedInResampler->type() == "bilinear" )
      {
        mZoomedInResamplingComboBox->setCurrentIndex( 1 );
      }
      else if ( zoomedInResampler->type() == "cubic" )
      {
        mZoomedInResamplingComboBox->setCurrentIndex( 2 );
      }
    }
    else
    {
      mZoomedInResamplingComboBox->setCurrentIndex( 0 );
    }

    const QgsRasterResampler* zoomedOutResampler = resampleFilter->zoomedOutResampler();
    if ( zoomedOutResampler )
    {
      if ( zoomedOutResampler->type() == "bilinear" ) //bilinear resampler does averaging when zooming out
      {
        mZoomedOutResamplingComboBox->setCurrentIndex( 1 );
      }
    }
    else
    {
      mZoomedOutResamplingComboBox->setCurrentIndex( 0 );
    }
    mMaximumOversamplingSpinBox->setValue( resampleFilter->maxOversampling() );
  }

  //transparency band
  if ( provider )
  {
    cboxTransparencyBand->addItem( tr( "None" ), -1 );
    int nBands = provider->bandCount();
    QString bandName;
    for ( int i = 1; i <= nBands; ++i ) //band numbering seem to start at 1
    {
      bandName = provider->generateBandName( i );

      QString colorInterp = provider->colorInterpretationName( i );
      if ( colorInterp != "Undefined" )
      {
        bandName.append( QString( " (%1)" ).arg( colorInterp ) );
      }
      cboxTransparencyBand->addItem( bandName, i );
    }

// Alpha band is set in sync()
#if 0
    if ( renderer )
    {
      QgsDebugMsg( QString( "alphaBand = %1" ).arg( renderer->alphaBand() ) );
      if ( renderer->alphaBand() > 0 )
      {
        cboxTransparencyBand->setCurrentIndex( cboxTransparencyBand->findData( renderer->alphaBand() ) );
      }
    }
#endif
  }

  // create histogram widget
  mHistogramWidget = NULL;
  if ( tabPageHistogram->isEnabled() )
  {
    mHistogramWidget = new QgsRasterHistogramWidget( mRasterLayer, tabPageHistogram );
    mHistogramStackedWidget->addWidget( mHistogramWidget );
  }

  //insert renderer widgets into registry
  QgsRasterRendererRegistry::instance()->insertWidgetFunction( "paletted", QgsPalettedRendererWidget::create );
  QgsRasterRendererRegistry::instance()->insertWidgetFunction( "multibandcolor", QgsMultiBandColorRendererWidget::create );
  QgsRasterRendererRegistry::instance()->insertWidgetFunction( "singlebandpseudocolor", QgsSingleBandPseudoColorRendererWidget::create );
  QgsRasterRendererRegistry::instance()->insertWidgetFunction( "singlebandgray", QgsSingleBandGrayRendererWidget::create );

  //fill available renderers into combo box
  QgsRasterRendererRegistryEntry entry;
  foreach ( QString name, QgsRasterRendererRegistry::instance()->renderersList() )
  {
    if ( QgsRasterRendererRegistry::instance()->rendererData( name, entry ) )
    {
      if (( mRasterLayer->rasterType() != QgsRasterLayer::ColorLayer && entry.name != "singlebandcolordata" ) ||
          ( mRasterLayer->rasterType() == QgsRasterLayer::ColorLayer && entry.name == "singlebandcolordata" ) )
      {
        mRenderTypeComboBox->addItem( entry.visibleName, entry.name );
      }
    }
  }

  if ( renderer )
  {
    QString rendererType = renderer->type();
    int widgetIndex = mRenderTypeComboBox->findData( rendererType );
    if ( widgetIndex != -1 )
    {
      mRenderTypeComboBox->setCurrentIndex( widgetIndex );
    }

    //prevent change between singleband color renderer and the other renderers
    // No more necessary, combo entries according to layer type
#if 0
    if ( rendererType == "singlebandcolordata" )
    {
      mRenderTypeComboBox->setEnabled( false );
    }
    else
    {
      mRenderTypeComboBox->removeItem( mRenderTypeComboBox->findData( "singlebandcolordata" ) );
    }
#endif
  }
  on_mRenderTypeComboBox_currentIndexChanged( mRenderTypeComboBox->currentIndex() );

  updatePipeList();

  // update based on lyr's current state
  sync();

  // set current tab after everything has been initialized
  tabBar->setCurrentIndex( settings.value( "/Windows/RasterLayerProperties/row" ).toInt() );

} // QgsRasterLayerProperties ctor


QgsRasterLayerProperties::~QgsRasterLayerProperties()
{
  QSettings settings;
  settings.setValue( "/Windows/RasterLayerProperties/geometry", saveGeometry() );
  settings.setValue( "/Windows/RasterLayerProperties/row", tabBar->currentIndex() );
  if ( mPixelSelectorTool )
  {
    delete mPixelSelectorTool;
  }
}

void QgsRasterLayerProperties::setupTransparencyTable( int nBands )
{
  QgsDebugMsg( "Entered" );
  tableTransparency->clear();
  tableTransparency->setColumnCount( 0 );
  tableTransparency->setRowCount( 0 );
  mTransparencyToEdited.clear();

  if ( nBands == 3 )
  {
    tableTransparency->setColumnCount( 4 );
    tableTransparency->setHorizontalHeaderItem( 0, new QTableWidgetItem( tr( "Red" ) ) );
    tableTransparency->setHorizontalHeaderItem( 1, new QTableWidgetItem( tr( "Green" ) ) );
    tableTransparency->setHorizontalHeaderItem( 2, new QTableWidgetItem( tr( "Blue" ) ) );
    tableTransparency->setHorizontalHeaderItem( 3, new QTableWidgetItem( tr( "Percent Transparent" ) ) );
  }
  else //1 band
  {
    tableTransparency->setColumnCount( 3 );
// Is it important to distinguish the header? It becomes difficult with range.
#if 0
    if ( QgsRasterLayer::PalettedColor != mRasterLayer->drawingStyle() &&
         QgsRasterLayer::PalettedSingleBandGray != mRasterLayer->drawingStyle() &&
         QgsRasterLayer::PalettedSingleBandPseudoColor != mRasterLayer->drawingStyle() &&
         QgsRasterLayer::PalettedMultiBandColor != mRasterLayer->drawingStyle() )
    {
      tableTransparency->setHorizontalHeaderItem( 0, new QTableWidgetItem( tr( "Gray" ) ) );
    }
    else
    {
      tableTransparency->setHorizontalHeaderItem( 0, new QTableWidgetItem( tr( "Indexed Value" ) ) );
    }
#endif
    tableTransparency->setHorizontalHeaderItem( 0, new QTableWidgetItem( tr( "From" ) ) );
    tableTransparency->setHorizontalHeaderItem( 1, new QTableWidgetItem( tr( "To" ) ) );
    tableTransparency->setHorizontalHeaderItem( 2, new QTableWidgetItem( tr( "Percent Transparent" ) ) );
  }
}

void QgsRasterLayerProperties::populateTransparencyTable( QgsRasterRenderer* renderer )
{
  QgsDebugMsg( "entering." );
  if ( !mRasterLayer )
  {
    return;
  }

  if ( !renderer )
  {
    return;
  }

  int nBands = renderer->usesBands().size();
  setupTransparencyTable( nBands );

  const QgsRasterTransparency* rasterTransparency = renderer->rasterTransparency();
  if ( !rasterTransparency )
  {
    return;
  }

  if ( nBands == 1 )
  {
    QList<QgsRasterTransparency::TransparentSingleValuePixel> pixelList = rasterTransparency->transparentSingleValuePixelList();
    for ( int i = 0; i < pixelList.size(); ++i )
    {
      tableTransparency->insertRow( i );
      setTransparencyCell( i, 0, pixelList[i].min );
      setTransparencyCell( i, 1, pixelList[i].max );
      setTransparencyCell( i, 2, pixelList[i].percentTransparent );
      // break synchronization only if values differ
      if ( pixelList[i].min != pixelList[i].max )
      {
        setTransparencyToEdited( i );
      }
    }
  }
  else if ( nBands == 3 )
  {
    QList<QgsRasterTransparency::TransparentThreeValuePixel> pixelList = rasterTransparency->transparentThreeValuePixelList();
    for ( int i = 0; i < pixelList.size(); ++i )
    {
      tableTransparency->insertRow( i );
      setTransparencyCell( i, 0, pixelList[i].red );
      setTransparencyCell( i, 1, pixelList[i].green );
      setTransparencyCell( i, 2, pixelList[i].blue );
      setTransparencyCell( i, 3, pixelList[i].percentTransparent );
    }
  }

  tableTransparency->resizeColumnsToContents();
  tableTransparency->resizeRowsToContents();
}

void QgsRasterLayerProperties::setRendererWidget( const QString& rendererName )
{
  QgsDebugMsg( "rendererName = " + rendererName );
  QgsRasterRendererWidget* oldWidget = mRendererWidget;

  QgsRasterRendererRegistryEntry rendererEntry;
  if ( QgsRasterRendererRegistry::instance()->rendererData( rendererName, rendererEntry ) )
  {
    if ( rendererEntry.widgetCreateFunction ) //single band color data renderer e.g. has no widget
    {
      QgsDebugMsg( "renderer has widgetCreateFunction" );
      // Current canvas extent (used to calc min/max) in layer CRS
      QgsRectangle myExtent = mMapCanvas->mapRenderer()->outputExtentToLayerExtent( mRasterLayer, mMapCanvas->extent() );
      mRendererWidget = ( *rendererEntry.widgetCreateFunction )( mRasterLayer, myExtent );
      mRendererStackedWidget->addWidget( mRendererWidget );
      if ( oldWidget )
      {
        //compare used bands in new and old renderer and reset transparency dialog if different
        QgsRasterRenderer* oldRenderer = oldWidget->renderer();
        QgsRasterRenderer* newRenderer = mRendererWidget->renderer();
        QList<int> oldBands = oldRenderer->usesBands();
        QList<int> newBands = newRenderer->usesBands();
        if ( oldBands != newBands )
        {
          populateTransparencyTable( newRenderer );
        }
        delete oldRenderer;
        delete newRenderer;
      }
    }
  }
  delete oldWidget;

  if ( mHistogramWidget )
  {
    mHistogramWidget->setRendererWidget( rendererName, mRendererWidget );
  }
}

/**
  @note moved from ctor

  Previously this dialog was created anew with each right-click pop-up menu
  invokation.  Changed so that the dialog always exists after first
  invocation, and is just re-synchronized with its layer's state when
  re-shown.

*/
void QgsRasterLayerProperties::sync()
{
  QSettings myQSettings;

  if ( mRasterLayer->dataProvider()->dataType( 1 ) == QgsRasterDataProvider::ARGB32
       || mRasterLayer->dataProvider()->dataType( 1 ) == QgsRasterDataProvider::ARGB32_Premultiplied )
  {
    gboxNoDataValue->setEnabled( false );
    gboxCustomTransparency->setEnabled( false );
    tabBar->setCurrentWidget( tabPageMetadata );
  }

  if ( !( mRasterLayer->dataProvider()->capabilities() & QgsRasterDataProvider::BuildPyramids ) )
  {
    if ( tabPagePyramids != NULL )
    {
      delete tabPagePyramids;
      tabPagePyramids = NULL;
    }
  }

  if ( !( mRasterLayer->dataProvider()->capabilities() & QgsRasterDataProvider::Histogram ) )
  {
    if ( tabPageHistogram != NULL )
    {
      delete tabPageHistogram;
      tabPageHistogram = NULL;
      delete mHistogramWidget;
      mHistogramWidget = NULL;
    }
  }

  QgsDebugMsg( "populate transparency tab" );

  /*
   * Transparent Pixel Tab
   */

  //set the transparency slider
  QgsRasterRenderer* renderer = mRasterLayer->renderer();
  if ( renderer )
  {
    sliderTransparency->setValue(( 1.0 - renderer->opacity() ) * 255 );
    //update the transparency percentage label
    sliderTransparency_valueChanged(( 1.0 - renderer->opacity() ) * 255 );

    int myIndex = renderer->alphaBand();
    if ( -1 != myIndex )
    {
      cboxTransparencyBand->setCurrentIndex( myIndex );
    }
    else
    {
      cboxTransparencyBand->setCurrentIndex( cboxTransparencyBand->findText( TRSTRING_NOT_SET ) );
    }
  }

  //add current NoDataValue to NoDataValue line edit
  // TODO: should be per band
  // TODO: no data ranges
  if ( mRasterLayer->dataProvider()->srcHasNoDataValue( 1 ) )
  {
    lblSrcNoDataValue->setText( QgsRasterInterface::printValue( mRasterLayer->dataProvider()->srcNoDataValue( 1 ) ) );
  }
  else
  {
    lblSrcNoDataValue->setText( tr( "not defined" ) );
  }

  mSrcNoDataValueCheckBox->setChecked( mRasterLayer->dataProvider()->useSrcNoDataValue( 1 ) );

  bool enableSrcNoData = mRasterLayer->dataProvider()->srcHasNoDataValue( 1 ) && !qIsNaN( mRasterLayer->dataProvider()->srcNoDataValue( 1 ) );

  mSrcNoDataValueCheckBox->setEnabled( enableSrcNoData );
  lblSrcNoDataValue->setEnabled( enableSrcNoData );

  QList<QgsRasterInterface::Range> noDataRangeList = mRasterLayer->dataProvider()->userNoDataValue( 1 );
  QgsDebugMsg( QString( "noDataRangeList.size = %1" ).arg( noDataRangeList.size() ) );
  if ( noDataRangeList.size() > 0 )
  {
    leNoDataValue->insert( QgsRasterInterface::printValue( noDataRangeList.value( 0 ).min ) );
  }
  else
  {
    leNoDataValue->insert( "" );
  }

  populateTransparencyTable( mRasterLayer->renderer() );

  QgsDebugMsg( "populate colormap tab" );
  /*
   * Transparent Pixel Tab
   */

  QgsDebugMsg( "populate general tab" );
  /*
   * General Tab
   */

  //these properties (layer name and label) are provided by the qgsmaplayer superclass
  leLayerSource->setText( mRasterLayer->source() );
  leDisplayName->setText( mRasterLayer->name() );

  //display the raster dimensions and no data value
  if ( mRasterLayer->dataProvider()->capabilities() & QgsRasterDataProvider::Size )
  {
    lblColumns->setText( tr( "Columns: %1" ).arg( mRasterLayer->width() ) );
    lblRows->setText( tr( "Rows: %1" ).arg( mRasterLayer->height() ) );
  }
  else
  {
    // TODO: Account for fixedWidth and fixedHeight WMS layers
    lblColumns->setText( tr( "Columns: " ) + tr( "n/a" ) );
    lblRows->setText( tr( "Rows: " ) + tr( "n/a" ) );
  }

  if ( mRasterLayer->dataProvider()->dataType( 1 ) == QgsRasterDataProvider::ARGB32
       || mRasterLayer->dataProvider()->dataType( 1 ) == QgsRasterDataProvider::ARGB32_Premultiplied )
  {
    lblNoData->setText( tr( "No-Data Value: " ) + tr( "n/a" ) );
  }
  else
  {
    // TODO: all bands
    if ( mRasterLayer->dataProvider()->srcHasNoDataValue( 1 ) )
    {
      lblNoData->setText( tr( "No-Data Value: %1" ).arg( mRasterLayer->dataProvider()->noDataValue( 1 ) ) );
    }
    else
    {
      lblNoData->setText( tr( "No-Data Value: " ) + tr( "n/a" ) );
    }
  }

  //get the thumbnail for the layer
  QPixmap myQPixmap = QPixmap( pixmapThumbnail->width(), pixmapThumbnail->height() );
  mRasterLayer->thumbnailAsPixmap( &myQPixmap );
  pixmapThumbnail->setPixmap( myQPixmap );

  //update the legend pixmap on this dialog
  //pixmapLegend->setPixmap( mRasterLayer->legendAsPixmap() );
  pixmapLegend->setScaledContents( true );
  pixmapLegend->repaint();

  //set the palette pixmap
  //pixmapPalette->setPixmap( mRasterLayer->paletteAsPixmap( mRasterLayer->bandNumber( mRasterLayer->grayBandName() ) ) );
  pixmapPalette->setScaledContents( true );
  pixmapPalette->repaint();

  QgsDebugMsg( "populate metadata tab" );
  /*
   * Metadata Tab
   */
  //populate the metadata tab's text browser widget with gdal metadata info
  QString myStyle = QgsApplication::reportStyleSheet();
  txtbMetadata->document()->setDefaultStyleSheet( myStyle );
  txtbMetadata->setHtml( mRasterLayer->metadata() );

  mLayerTitleLineEdit->setText( mRasterLayer->title() );
  mLayerAbstractTextEdit->setPlainText( mRasterLayer->abstract() );

} // QgsRasterLayerProperties::sync()

/*
 *
 * PUBLIC AND PRIVATE SLOTS
 *
 */
void QgsRasterLayerProperties::apply()
{
  QgsDebugMsg( "apply processing symbology tab" );
  /*
   * Symbology Tab
   */

  //set whether the layer histogram should be inverted
  //mRasterLayer->setInvertHistogram( cboxInvertColorMap->isChecked() );

  QgsDebugMsg( "processing transparency tab" );
  /*
   * Transparent Pixel Tab
   */

  //set NoDataValue
  QList<QgsRasterInterface::Range> myNoDataRangeList;
  if ( "" != leNoDataValue->text() )
  {
    bool myDoubleOk = false;
    double myNoDataValue = leNoDataValue->text().toDouble( &myDoubleOk );
    if ( myDoubleOk )
    {
      //mRasterLayer->setNoDataValue( myNoDataValue );
      QgsRasterInterface::Range myNoDataRange;
      myNoDataRange.min = myNoDataValue;
      myNoDataRange.max = myNoDataValue;

      myNoDataRangeList << myNoDataRange;
    }
  }
  for ( int bandNo = 1; bandNo <= mRasterLayer->dataProvider()->bandCount(); bandNo++ )
  {
    mRasterLayer->dataProvider()->setUserNoDataValue( bandNo, myNoDataRangeList );
    mRasterLayer->dataProvider()->setUseSrcNoDataValue( bandNo, mSrcNoDataValueCheckBox->isChecked() );
  }

  //set renderer from widget
  QgsRasterRendererWidget* rendererWidget = dynamic_cast<QgsRasterRendererWidget*>( mRendererStackedWidget->currentWidget() );
  if ( rendererWidget )
  {
    mRasterLayer->setRenderer( rendererWidget->renderer() );
  }

  //transparency settings
  QgsRasterRenderer* rasterRenderer = mRasterLayer->renderer();
  if ( rasterRenderer )
  {
    rasterRenderer->setAlphaBand( cboxTransparencyBand->itemData( cboxTransparencyBand->currentIndex() ).toInt() );

    //Walk through each row in table and test value. If not valid set to 0.0 and continue building transparency list
    QgsRasterTransparency* rasterTransparency = new QgsRasterTransparency();
    if ( tableTransparency->columnCount() == 4 )
    {
      QgsRasterTransparency::TransparentThreeValuePixel myTransparentPixel;
      QList<QgsRasterTransparency::TransparentThreeValuePixel> myTransparentThreeValuePixelList;
      for ( int myListRunner = 0; myListRunner < tableTransparency->rowCount(); myListRunner++ )
      {
        myTransparentPixel.red = transparencyCellValue( myListRunner, 0 );
        myTransparentPixel.green = transparencyCellValue( myListRunner, 1 );
        myTransparentPixel.blue = transparencyCellValue( myListRunner, 2 );
        myTransparentPixel.percentTransparent = transparencyCellValue( myListRunner, 3 );
        myTransparentThreeValuePixelList.append( myTransparentPixel );
      }
      rasterTransparency->setTransparentThreeValuePixelList( myTransparentThreeValuePixelList );
    }
    else if ( tableTransparency->columnCount() == 3 )
    {
      QgsRasterTransparency::TransparentSingleValuePixel myTransparentPixel;
      QList<QgsRasterTransparency::TransparentSingleValuePixel> myTransparentSingleValuePixelList;
      for ( int myListRunner = 0; myListRunner < tableTransparency->rowCount(); myListRunner++ )
      {
        myTransparentPixel.min = transparencyCellValue( myListRunner, 0 );
        myTransparentPixel.max = transparencyCellValue( myListRunner, 1 );
        myTransparentPixel.percentTransparent = transparencyCellValue( myListRunner, 2 );

        myTransparentSingleValuePixelList.append( myTransparentPixel );
      }
      rasterTransparency->setTransparentSingleValuePixelList( myTransparentSingleValuePixelList );
    }

    rasterRenderer->setRasterTransparency( rasterTransparency );

    //set global transparency
    rasterRenderer->setOpacity(( 255 - sliderTransparency->value() ) / 255.0 );

    //invert color map
    rasterRenderer->setInvertColor( mInvertColorMapCheckBox->isChecked() );
  }

  QgsDebugMsg( "processing general tab" );
  /*
   * General Tab
   */
  mRasterLayer->setLayerName( leDisplayName->text() );

  // set up the scale based layer visibility stuff....
  mRasterLayer->toggleScaleBasedVisibility( chkUseScaleDependentRendering->isChecked() );
  mRasterLayer->setMinimumScale( 1.0 / cbMinimumScale->scale() );
  mRasterLayer->setMaximumScale( 1.0 / cbMaximumScale->scale() );

  //update the legend pixmap
  pixmapLegend->setPixmap( mRasterLayer->legendAsPixmap() );
  pixmapLegend->setScaledContents( true );
  pixmapLegend->repaint();

  QgsRasterResampleFilter* resampleFilter = mRasterLayer->resampleFilter();

  QgsRasterResampler* zoomedInResampler = 0;
  QString zoomedInResamplingMethod = mZoomedInResamplingComboBox->currentText();
  if ( zoomedInResamplingMethod == tr( "Bilinear" ) )
  {
    zoomedInResampler = new QgsBilinearRasterResampler();
  }
  else if ( zoomedInResamplingMethod == tr( "Cubic" ) )
  {
    zoomedInResampler = new QgsCubicRasterResampler();
  }

  if ( resampleFilter )
  {
    resampleFilter->setZoomedInResampler( zoomedInResampler );
  }

  //raster resampling
  QgsRasterResampler* zoomedOutResampler = 0;
  QString zoomedOutResamplingMethod = mZoomedOutResamplingComboBox->currentText();
  if ( zoomedOutResamplingMethod == tr( "Average" ) )
  {
    zoomedOutResampler = new QgsBilinearRasterResampler();
  }

  if ( resampleFilter )
  {
    resampleFilter->setZoomedOutResampler( zoomedOutResampler );
  }

  if ( resampleFilter )
  {
    resampleFilter->setMaxOversampling( mMaximumOversamplingSpinBox->value() );
  }


  //get the thumbnail for the layer
  QPixmap myQPixmap = QPixmap( pixmapThumbnail->width(), pixmapThumbnail->height() );
  mRasterLayer->thumbnailAsPixmap( &myQPixmap );
  pixmapThumbnail->setPixmap( myQPixmap );

  mRasterLayer->setTitle( mLayerTitleLineEdit->text() );
  mRasterLayer->setAbstract( mLayerAbstractTextEdit->toPlainText() );

  // update symbology
  emit refreshLegend( mRasterLayer->id(), false );

  //no need to delete the old one, maplayer will do it if needed
  mRasterLayer->setCacheImage( 0 );

  //make sure the layer is redrawn
  mRasterLayer->triggerRepaint();

  // notify the project we've made a change
  QgsProject::instance()->dirty( true );

  updatePipeList();
}//apply

void QgsRasterLayerProperties::on_buttonBuildPyramids_clicked()
{
  QgsRasterDataProvider* provider = mRasterLayer->dataProvider();

  connect( provider, SIGNAL( progressUpdate( int ) ), mPyramidProgress, SLOT( setValue( int ) ) );
  //
  // Go through the list marking any files that are selected in the listview
  // as true so that we can generate pyramids for them.
  //
  QList< QgsRasterPyramid> myPyramidList = provider->buildPyramidList();
  for ( int myCounterInt = 0; myCounterInt < lbxPyramidResolutions->count(); myCounterInt++ )
  {
    QListWidgetItem *myItem = lbxPyramidResolutions->item( myCounterInt );
    //mark to be pyramided
    myPyramidList[myCounterInt].build = myItem->isSelected() || myPyramidList[myCounterInt].exists;
  }
  //
  // Ask raster layer to build the pyramids
  //

  // let the user know we're going to possibly be taking a while
  QApplication::setOverrideCursor( Qt::WaitCursor );
  QString res = provider->buildPyramids(
                  myPyramidList,
                  cboResamplingMethod->currentText(),
                  ( QgsRasterDataProvider::RasterPyramidsFormat ) cbxPyramidsFormat->currentIndex() );
  QApplication::restoreOverrideCursor();
  mPyramidProgress->setValue( 0 );
  buttonBuildPyramids->setEnabled( false );
  disconnect( provider, SIGNAL( progressUpdate( int ) ), mPyramidProgress, SLOT( setValue( int ) ) );
  if ( !res.isNull() )
  {
    if ( res == "ERROR_WRITE_ACCESS" )
    {
      QMessageBox::warning( this, tr( "Write access denied" ),
                            tr( "Write access denied. Adjust the file permissions and try again." ) );
    }
    else if ( res == "ERROR_WRITE_FORMAT" )
    {
      QMessageBox::warning( this, tr( "Building pyramids failed." ),
                            tr( "The file was not writable. Some formats do not "
                                "support pyramid overviews. Consult the GDAL documentation if in doubt." ) );
    }
    else if ( res == "FAILED_NOT_SUPPORTED" )
    {
      QMessageBox::warning( this, tr( "Building pyramids failed." ),
                            tr( "Building pyramid overviews is not supported on this type of raster." ) );
    }
    else if ( res == "ERROR_JPEG_COMPRESSION" )
    {
      QMessageBox::warning( this, tr( "Building pyramids failed." ),
                            tr( "Building internal pyramid overviews is not supported on raster layers with JPEG compression and your current libtiff library." ) );
    }
    else if ( res == "ERROR_VIRTUAL" )
    {
      QMessageBox::warning( this, tr( "Building pyramids failed." ),
                            tr( "Building pyramid overviews is not supported on this type of raster." ) );
    }

  }

  //
  // repopulate the pyramids list
  //
  lbxPyramidResolutions->clear();
  // Need to rebuild list as some or all pyramids may have failed to build
  myPyramidList = provider->buildPyramidList();
  QIcon myPyramidPixmap( QgsApplication::getThemeIcon( "/mIconPyramid.png" ) );
  QIcon myNoPyramidPixmap( QgsApplication::getThemeIcon( "/mIconNoPyramid.png" ) );

  QList< QgsRasterPyramid >::iterator myRasterPyramidIterator;
  for ( myRasterPyramidIterator = myPyramidList.begin();
        myRasterPyramidIterator != myPyramidList.end();
        ++myRasterPyramidIterator )
  {
    if ( myRasterPyramidIterator->exists )
    {
      lbxPyramidResolutions->addItem( new QListWidgetItem( myPyramidPixmap,
                                      QString::number( myRasterPyramidIterator->xDim ) + QString( " x " ) +
                                      QString::number( myRasterPyramidIterator->yDim ) ) );
    }
    else
    {
      lbxPyramidResolutions->addItem( new QListWidgetItem( myNoPyramidPixmap,
                                      QString::number( myRasterPyramidIterator->xDim ) + QString( " x " ) +
                                      QString::number( myRasterPyramidIterator->yDim ) ) );
    }
  }
  //update the legend pixmap
  pixmapLegend->setPixmap( mRasterLayer->legendAsPixmap() );
  pixmapLegend->setScaledContents( true );
  pixmapLegend->repaint();
  //populate the metadata tab's text browser widget with gdal metadata info
  QString myStyle = QgsApplication::reportStyleSheet();
  txtbMetadata->setHtml( mRasterLayer->metadata() );
  txtbMetadata->document()->setDefaultStyleSheet( myStyle );
}

void QgsRasterLayerProperties::on_mRenderTypeComboBox_currentIndexChanged( int index )
{
  if ( index < 0 )
  {
    return;
  }

  QString rendererName = mRenderTypeComboBox->itemData( index ).toString();
  setRendererWidget( rendererName );
}

void QgsRasterLayerProperties::on_pbnAddValuesFromDisplay_clicked()
{
  if ( mMapCanvas && mPixelSelectorTool )
  {
    mMapCanvas->setMapTool( mPixelSelectorTool );
    //Need to work around the modality of the dialog but can not just hide() it.
    setModal( false );
    lower();
  }
}

void QgsRasterLayerProperties::on_pbnAddValuesManually_clicked()
{
  QgsRasterRenderer* renderer = mRendererWidget->renderer();
  if ( !renderer )
  {
    return;
  }

  tableTransparency->insertRow( tableTransparency->rowCount() );

  int n = renderer->usesBands().size();
  if ( n == 1 ) n++;

  for ( int i = 0; i < n; i++ )
  {
    setTransparencyCell( tableTransparency->rowCount() - 1, i, std::numeric_limits<double>::quiet_NaN() );
  }

  setTransparencyCell( tableTransparency->rowCount() - 1, n, 100 );

  tableTransparency->resizeColumnsToContents();
  tableTransparency->resizeRowsToContents();
}

void QgsRasterLayerProperties::on_pbnChangeSpatialRefSys_clicked()
{

  QgsGenericProjectionSelector * mySelector = new QgsGenericProjectionSelector( this );
  mySelector->setSelectedCrsId( mRasterLayer->crs().srsid() );
  if ( mySelector->exec() )
  {
    QgsCoordinateReferenceSystem srs( mySelector->selectedCrsId(), QgsCoordinateReferenceSystem::InternalCrsId );
    mRasterLayer->setCrs( srs );
  }
  else
  {
    QApplication::restoreOverrideCursor();
  }
  delete mySelector;

  leSpatialRefSys->setText( mRasterLayer->crs().authid() + " - " + mRasterLayer->crs().description() );
  leSpatialRefSys->setCursorPosition( 0 );
}

void QgsRasterLayerProperties::on_pbnDefaultValues_clicked()
{
  QgsDebugMsg( "Entered" );
  if ( !mRendererWidget )
  {
    return;
  }

  QgsRasterRenderer* r = mRendererWidget->renderer();
  if ( !r )
  {
    return;
  }

  int nBands = r->usesBands().size();
  delete r; // really delete?

  setupTransparencyTable( nBands );

// I don't think that noDataValue should be added to transparency list
#if 0
  if ( nBands == 3 )
  {
    if ( mRasterLayer->isNoDataValueValid() )
    {
      tableTransparency->insertRow( tableTransparency->rowCount() );
      setTransparencyCell( 0, 0, mRasterLayer->noDataValue() );
      setTransparencyCell( 0, 1, mRasterLayer->noDataValue() );
      setTransparencyCell( 0, 2, mRasterLayer->noDataValue() );
      setTransparencyCell( 0, 1, 100 );
    }
  }
  else //1 band
  {
    if ( mRasterLayer->isNoDataValueValid() )
    {
      tableTransparency->insertRow( tableTransparency->rowCount() );
      setTransparencyCell( 0, 0, mRasterLayer->noDataValue() );
      setTransparencyCell( 0, 1, mRasterLayer->noDataValue() );
      setTransparencyCell( 0, 1, 100 );
    }
  }
#endif

  tableTransparency->resizeColumnsToContents(); // works only with values
  tableTransparency->resizeRowsToContents();
}

void QgsRasterLayerProperties::setTransparencyCell( int row, int column, double value )
{
  QgsDebugMsg( QString( "value = %1" ).arg( value, 0, 'g', 17 ) );
  QgsRasterDataProvider* provider = mRasterLayer->dataProvider();
  if ( !provider ) return;

  QgsRasterRenderer* renderer = mRendererWidget->renderer();
  if ( !renderer ) return;
  int nBands = renderer->usesBands().size();

  QLineEdit *lineEdit = new QLineEdit();
  lineEdit->setFrame( false ); // frame looks bad in table
  // Without margins row selection is not displayed (important for delete row)
  lineEdit->setContentsMargins( 1, 1, 1, 1 );

  if ( column == tableTransparency->columnCount() - 1 )
  {
    // transparency
    // Who needs transparency as floating point?
    lineEdit->setValidator( new QIntValidator( 0 ) );
    lineEdit->setText( QString::number( static_cast<int>( value ) ) );
  }
  else
  {
    // value
    QString valueString;
    switch ( provider->srcDataType( 1 ) )
    {
      case QgsRasterInterface::Float32:
      case QgsRasterInterface::Float64:
        lineEdit->setValidator( new QDoubleValidator( 0 ) );
        if ( !qIsNaN( value ) )
        {
          valueString = QgsRasterInterface::printValue( value );
        }
        break;
      default:
        lineEdit->setValidator( new QIntValidator( 0 ) );
        if ( !qIsNaN( value ) )
        {
          valueString = QString::number( static_cast<int>( value ) );
        }
        break;
    }
    lineEdit->setText( valueString );
  }
  tableTransparency->setCellWidget( row, column, lineEdit );
  adjustTransparencyCellWidth( row, column );

  if ( nBands == 1 && ( column == 0 || column == 1 ) )
  {
    connect( lineEdit, SIGNAL( textEdited( const QString & ) ), this, SLOT( transparencyCellTextEdited( const QString & ) ) );
  }
  tableTransparency->resizeColumnsToContents();
}

void QgsRasterLayerProperties::setTransparencyCellValue( int row, int column, double value )
{
  QLineEdit *lineEdit = dynamic_cast<QLineEdit *>( tableTransparency->cellWidget( row, column ) );
  if ( !lineEdit ) return;
  lineEdit->setText( QgsRasterInterface::printValue( value ) );
  lineEdit->adjustSize();
  adjustTransparencyCellWidth( row, column );
  tableTransparency->resizeColumnsToContents();
}

double QgsRasterLayerProperties::transparencyCellValue( int row, int column )
{
  QLineEdit *lineEdit = dynamic_cast<QLineEdit *>( tableTransparency->cellWidget( row, column ) );
  if ( !lineEdit || lineEdit->text().isEmpty() )
  {
    std::numeric_limits<double>::quiet_NaN();
  }
  return lineEdit->text().toDouble();
}

void QgsRasterLayerProperties::adjustTransparencyCellWidth( int row, int column )
{
  QLineEdit *lineEdit = dynamic_cast<QLineEdit *>( tableTransparency->cellWidget( row, column ) );
  if ( !lineEdit ) return;

  int width = qMax( lineEdit->fontMetrics().width( lineEdit->text() ) + 10, 100 );
  width = qMax( width, tableTransparency->columnWidth( column ) );

  lineEdit->setFixedWidth( width );
}

void QgsRasterLayerProperties::on_pbnExportTransparentPixelValues_clicked()
{
  QSettings myQSettings;
  QString myLastDir = myQSettings.value( "lastRasterFileFilterDir", "" ).toString();
  QString myFileName = QFileDialog::getSaveFileName( this, tr( "Save file" ), myLastDir, tr( "Textfile" ) + " (*.txt)" );
  if ( !myFileName.isEmpty() )
  {
    if ( !myFileName.endsWith( ".txt", Qt::CaseInsensitive ) )
    {
      myFileName = myFileName + ".txt";
    }

    QFile myOutputFile( myFileName );
    if ( myOutputFile.open( QFile::WriteOnly ) )
    {
      QTextStream myOutputStream( &myOutputFile );
      myOutputStream << "# " << tr( "QGIS Generated Transparent Pixel Value Export File" ) << "\n";
      if ( /*rbtnThreeBand->isChecked() &&*/ QgsRasterLayer::MultiBandColor == mRasterLayer->drawingStyle() )
      {
        myOutputStream << "#\n#\n# " << tr( "Red" ) << "\t" << tr( "Green" ) << "\t" << tr( "Blue" ) << "\t" << tr( "Percent Transparent" );
        for ( int myTableRunner = 0; myTableRunner < tableTransparency->rowCount(); myTableRunner++ )
        {
          myOutputStream << "\n" << QString::number( transparencyCellValue( myTableRunner, 0 ) ) << "\t"
          << QString::number( transparencyCellValue( myTableRunner, 1 ) ) << "\t"
          << QString::number( transparencyCellValue( myTableRunner, 2 ) ) << "\t"
          << QString::number( transparencyCellValue( myTableRunner, 3 ) );
        }
      }
      else
      {
        if ( QgsRasterLayer::PalettedColor != mRasterLayer->drawingStyle() &&
             QgsRasterLayer::PalettedSingleBandGray != mRasterLayer->drawingStyle() &&
             QgsRasterLayer::PalettedSingleBandPseudoColor != mRasterLayer->drawingStyle() &&
             QgsRasterLayer::PalettedMultiBandColor != mRasterLayer->drawingStyle() )
        {
          myOutputStream << "#\n#\n# " << tr( "Gray" ) << "\t" << tr( "Percent Transparent" );
        }
        else
        {
          myOutputStream << "#\n#\n# " << tr( "Indexed Value" ) << "\t" << tr( "Percent Transparent" );
        }

        for ( int myTableRunner = 0; myTableRunner < tableTransparency->rowCount(); myTableRunner++ )
        {
          myOutputStream << "\n" << QString::number( transparencyCellValue( myTableRunner, 0 ) ) << "\t"
          << QString::number( transparencyCellValue( myTableRunner, 1 ) ) << "\t"
          << QString::number( transparencyCellValue( myTableRunner, 2 ) );
        }
      }
    }
    else
    {
      QMessageBox::warning( this, tr( "Write access denied" ), tr( "Write access denied. Adjust the file permissions and try again.\n\n" ) );
    }
  }
}

void QgsRasterLayerProperties::transparencyCellTextEdited( const QString & text )
{
  QgsDebugMsg( QString( "text = %1" ).arg( text ) );
  QgsRasterRenderer* renderer = mRendererWidget->renderer();
  if ( !renderer )
  {
    return;
  }
  int nBands = renderer->usesBands().size();
  if ( nBands == 1 )
  {
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>( sender() );
    if ( !lineEdit ) return;
    int row = -1;
    int column = -1;
    for ( int r = 0 ; r < tableTransparency->rowCount(); r++ )
    {
      for ( int c = 0 ; c < tableTransparency->columnCount(); c++ )
      {
        if ( tableTransparency->cellWidget( r, c ) == sender() )
        {
          row = r;
          column = c;
          break;
        }
      }
      if ( row != -1 ) break;
    }
    QgsDebugMsg( QString( "row = %1 column =%2" ).arg( row ).arg( column ) );

    if ( column == 0 )
    {
      QLineEdit *toLineEdit = dynamic_cast<QLineEdit *>( tableTransparency->cellWidget( row, 1 ) );
      if ( !toLineEdit ) return;
      bool toChanged = mTransparencyToEdited.value( row );
      QgsDebugMsg( QString( "toChanged = %1" ).arg( toChanged ) );
      if ( !toChanged )
      {
        toLineEdit->setText( lineEdit->text() );
      }
    }
    else if ( column == 1 )
    {
      setTransparencyToEdited( row );
    }
  }
}

void QgsRasterLayerProperties::setTransparencyToEdited( int row )
{
  if ( row >= mTransparencyToEdited.size() )
  {
    mTransparencyToEdited.resize( row + 1 );
  }
  mTransparencyToEdited[row] = true;
}

void QgsRasterLayerProperties::on_tabBar_currentChanged( int theTab )
{
  if ( mHistogramWidget == 0 ) return;

  if ( theTab == 5 )
  {
    mHistogramWidget->setActive( true );
  }
  else
  {
    mHistogramWidget->setActive( false );
  }
}

void QgsRasterLayerProperties::on_pbnImportTransparentPixelValues_clicked()
{
  int myLineCounter = 0;
  bool myImportError = false;
  QString myBadLines;
  QSettings myQSettings;
  QString myLastDir = myQSettings.value( "lastRasterFileFilterDir", "" ).toString();
  QString myFileName = QFileDialog::getOpenFileName( this, tr( "Open file" ), myLastDir, tr( "Textfile" ) + " (*.txt)" );
  QFile myInputFile( myFileName );
  if ( myInputFile.open( QFile::ReadOnly ) )
  {
    QTextStream myInputStream( &myInputFile );
    QString myInputLine;
    if ( /*rbtnThreeBand->isChecked() &&*/ QgsRasterLayer::MultiBandColor == mRasterLayer->drawingStyle() )
    {
      for ( int myTableRunner = tableTransparency->rowCount() - 1; myTableRunner >= 0; myTableRunner-- )
      {
        tableTransparency->removeRow( myTableRunner );
      }

      while ( !myInputStream.atEnd() )
      {
        myLineCounter++;
        myInputLine = myInputStream.readLine();
        if ( !myInputLine.isEmpty() )
        {
          if ( !myInputLine.simplified().startsWith( "#" ) )
          {
            QStringList myTokens = myInputLine.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
            if ( myTokens.count() != 4 )
            {
              myImportError = true;
              myBadLines = myBadLines + QString::number( myLineCounter ) + ":\t[" + myInputLine + "]\n";
            }
            else
            {
              tableTransparency->insertRow( tableTransparency->rowCount() );
              for ( int col = 0; col < 4; col++ )
              {
                setTransparencyCell( tableTransparency->rowCount() - 1, col, myTokens[col].toDouble() );
              }
            }
          }
        }
      }
    }
    else
    {
      for ( int myTableRunner = tableTransparency->rowCount() - 1; myTableRunner >= 0; myTableRunner-- )
      {
        tableTransparency->removeRow( myTableRunner );
      }

      while ( !myInputStream.atEnd() )
      {
        myLineCounter++;
        myInputLine = myInputStream.readLine();
        if ( !myInputLine.isEmpty() )
        {
          if ( !myInputLine.simplified().startsWith( "#" ) )
          {
            QStringList myTokens = myInputLine.split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
            if ( myTokens.count() != 3 && myTokens.count() != 2 ) // 2 for QGIS < 1.9 compatibility
            {
              myImportError = true;
              myBadLines = myBadLines + QString::number( myLineCounter ) + ":\t[" + myInputLine + "]\n";
            }
            else
            {
              if ( myTokens.count() == 2 )
              {
                myTokens.insert( 1, myTokens[0] ); // add 'to' value, QGIS < 1.9 compatibility
              }
              tableTransparency->insertRow( tableTransparency->rowCount() );
              for ( int col = 0; col < 3; col++ )
              {
                setTransparencyCell( tableTransparency->rowCount() - 1, col, myTokens[col].toDouble() );
              }
            }
          }
        }
      }
    }

    if ( myImportError )
    {
      QMessageBox::warning( this, tr( "Import Error" ), tr( "The following lines contained errors\n\n%1" ).arg( myBadLines ) );
    }
  }
  else if ( !myFileName.isEmpty() )
  {
    QMessageBox::warning( this, tr( "Read access denied" ), tr( "Read access denied. Adjust the file permissions and try again.\n\n" ) );
  }
  tableTransparency->resizeColumnsToContents();
  tableTransparency->resizeRowsToContents();
}

void QgsRasterLayerProperties::on_pbnRemoveSelectedRow_clicked()
{
  if ( 0 < tableTransparency->rowCount() )
  {
    tableTransparency->removeRow( tableTransparency->currentRow() );
  }
}

void QgsRasterLayerProperties::pixelSelected( const QgsPoint& canvasPoint )
{
  QgsRasterRenderer* renderer = mRendererWidget->renderer();
  if ( !renderer )
  {
    return;
  }

  raise();
  setModal( true );
  activateWindow();

  //Get the pixel values and add a new entry to the transparency table
  if ( mMapCanvas && mPixelSelectorTool )
  {
    mMapCanvas->unsetMapTool( mPixelSelectorTool );
    QMap< int, void *> myPixelMap = mRasterLayer->dataProvider()->identify( mMapCanvas->mapRenderer()->mapToLayerCoordinates( mRasterLayer, canvasPoint ) );

    QList<int> bands = renderer->usesBands();

    QgsRasterDataProvider * provider = mRasterLayer->dataProvider();
    QList<double> values;
    for ( int i = 0; i < bands.size(); ++i )
    {
      int bandNo = bands.value( i );
      if ( myPixelMap.count( bandNo ) == 1 )
      {
        void * data = myPixelMap.value( bandNo );
        double value = provider->readValue( data, provider->dataType( bandNo ), 0 );
        QgsDebugMsg( QString( "value = %1" ).arg( value, 0, 'g', 17 ) );

        if ( provider->isNoDataValue( bandNo, value ) )
        {
          return; // Don't add nodata, transparent anyway
        }
        values.append( value );
      }
    }
    if ( bands.size() == 1 )
    {
      // Set 'to'
      values.insert( 1, values.value( 0 ) );
    }
    tableTransparency->insertRow( tableTransparency->rowCount() );
    for ( int i = 0; i < values.size(); i++ )
    {
      setTransparencyCell( tableTransparency->rowCount() - 1, i, values.value( i ) );
    }
    setTransparencyCell( tableTransparency->rowCount() - 1, tableTransparency->columnCount() - 1, 100 );
  }
  delete renderer;

  tableTransparency->resizeColumnsToContents();
  tableTransparency->resizeRowsToContents();
}

void QgsRasterLayerProperties::sliderTransparency_valueChanged( int theValue )
{
  //set the transparency percentage label to a suitable value
  int myInt = static_cast < int >(( theValue / 255.0 ) * 100 );  //255.0 to prevent integer division
  lblTransparencyPercent->setText( QString::number( myInt ) + "%" );
}//sliderTransparency_valueChanged


QLinearGradient QgsRasterLayerProperties::redGradient()
{
  //define a gradient
  ///@TODO change this to actual polygon dims
  QLinearGradient myGradient = QLinearGradient( mGradientWidth, 0, mGradientWidth, mGradientHeight );
  myGradient.setColorAt( 0.0, QColor( 242, 14, 25, 190 ) );
  myGradient.setColorAt( 0.5, QColor( 175, 29, 37, 190 ) );
  myGradient.setColorAt( 1.0, QColor( 114, 17, 22, 190 ) );
  return myGradient;
}
QLinearGradient QgsRasterLayerProperties::greenGradient()
{
  //define a gradient
  ///@TODO change this to actual polygon dims
  QLinearGradient myGradient = QLinearGradient( mGradientWidth, 0, mGradientWidth, mGradientHeight );
  myGradient.setColorAt( 0.0, QColor( 48, 168, 5, 190 ) );
  myGradient.setColorAt( 0.8, QColor( 36, 122, 4, 190 ) );
  myGradient.setColorAt( 1.0, QColor( 21, 71, 2, 190 ) );
  return myGradient;
}
QLinearGradient QgsRasterLayerProperties::blueGradient()
{
  //define a gradient
  ///@TODO change this to actual polygon dims
  QLinearGradient myGradient = QLinearGradient( mGradientWidth, 0, mGradientWidth, mGradientHeight );
  myGradient.setColorAt( 0.0, QColor( 30, 0, 106, 190 ) );
  myGradient.setColorAt( 0.2, QColor( 30, 72, 128, 190 ) );
  myGradient.setColorAt( 1.0, QColor( 30, 223, 196, 190 ) );
  return myGradient;
}
QLinearGradient QgsRasterLayerProperties::grayGradient()
{
  //define a gradient
  ///@TODO change this to actual polygon dims
  QLinearGradient myGradient = QLinearGradient( mGradientWidth, 0, mGradientWidth, mGradientHeight );
  myGradient.setColorAt( 0.0, QColor( 5, 5, 5, 190 ) );
  myGradient.setColorAt( 0.8, QColor( 122, 122, 122, 190 ) );
  myGradient.setColorAt( 1.0, QColor( 220, 220, 220, 190 ) );
  return myGradient;
}
QLinearGradient QgsRasterLayerProperties::highlightGradient()
{
  //define another gradient for the highlight
  ///@TODO change this to actual polygon dims
  QLinearGradient myGradient = QLinearGradient( mGradientWidth, 0, mGradientWidth, mGradientHeight );
  myGradient.setColorAt( 1.0, QColor( 255, 255, 255, 50 ) );
  myGradient.setColorAt( 0.5, QColor( 255, 255, 255, 100 ) );
  myGradient.setColorAt( 0.0, QColor( 255, 255, 255, 150 ) );
  return myGradient;
}



//
//
// Next four methods for saving and restoring qml style state
//
//
void QgsRasterLayerProperties::on_pbnLoadDefaultStyle_clicked()
{
  bool defaultLoadedFlag = false;
  QString myMessage = mRasterLayer->loadDefaultStyle( defaultLoadedFlag );
  //reset if the default style was loaded ok only
  if ( defaultLoadedFlag )
  {
    QgsRasterRenderer* renderer = mRasterLayer->renderer();
    if ( renderer )
    {
      setRendererWidget( renderer->type() );
    }
    mRasterLayer->triggerRepaint();
  }
  else
  {
    //otherwise let the user know what went wrong
    QMessageBox::information( this,
                              tr( "Default Style" ),
                              myMessage
                            );
  }
}

void QgsRasterLayerProperties::on_pbnSaveDefaultStyle_clicked()
{
  // a flag passed by reference
  bool defaultSavedFlag = false;
  // after calling this the above flag will be set true for success
  // or false if the save operation failed
  QString myMessage = mRasterLayer->saveDefaultStyle( defaultSavedFlag );
  if ( !defaultSavedFlag )
  {
    //let the user know what went wrong
    QMessageBox::information( this,
                              tr( "Default Style" ),
                              myMessage
                            );
  }
}


void QgsRasterLayerProperties::on_pbnLoadStyle_clicked()
{
  QSettings settings;
  QString lastUsedDir = settings.value( "style/lastStyleDir", "." ).toString();

  QString fileName = QFileDialog::getOpenFileName(
                       this,
                       tr( "Load layer properties from style file" ),
                       lastUsedDir,
                       tr( "QGIS Layer Style File" ) + " (*.qml)" );
  if ( fileName.isEmpty() )
    return;

  // ensure the user never omits the extension from the file name
  if ( !fileName.endsWith( ".qml", Qt::CaseInsensitive ) )
    fileName += ".qml";

  bool defaultLoadedFlag = false;
  QString message = mRasterLayer->loadNamedStyle( fileName, defaultLoadedFlag );
  if ( defaultLoadedFlag )
  {
    settings.setValue( "style/lastStyleDir", QFileInfo( fileName ).absolutePath() );
    QgsRasterRenderer* renderer = mRasterLayer->renderer();
    if ( renderer )
    {
      setRendererWidget( renderer->type() );
    }
    mRasterLayer->triggerRepaint();
  }
  else
  {
    QMessageBox::information( this, tr( "Saved Style" ), message );
  }
}


void QgsRasterLayerProperties::on_pbnSaveStyleAs_clicked()
{
  QSettings settings;
  QString lastUsedDir = settings.value( "style/lastStyleDir", "." ).toString();

  QString outputFileName = QFileDialog::getSaveFileName(
                             this,
                             tr( "Save layer properties as style file" ),
                             lastUsedDir,
                             tr( "QGIS Layer Style File" ) + " (*.qml)" );
  if ( outputFileName.isEmpty() )
    return;

  // ensure the user never omits the extension from the file name
  if ( !outputFileName.endsWith( ".qml", Qt::CaseInsensitive ) )
    outputFileName += ".qml";

  bool defaultLoadedFlag = false;
  QString message = mRasterLayer->saveNamedStyle( outputFileName, defaultLoadedFlag );
  if ( defaultLoadedFlag )
  {
    sync();
  }
  else
  {
    QMessageBox::information( this, tr( "Saved Style" ), message );
  }

  settings.setValue( "style/lastStyleDir", QFileInfo( outputFileName ).absolutePath() );
}

#if 0
void QgsRasterLayerProperties::on_btnResetNull_clicked( )
{
  //If reset NoDataValue is checked do this first, will ignore what ever is in the LineEdit
  mRasterLayer->resetNoDataValue();
  if ( mRasterLayer->isNoDataValueValid() )
  {
    leNoDataValue->setText( QString::number( mRasterLayer->noDataValue(), 'g' ) );
  }
  else
  {
    leNoDataValue->clear();
  }
}
#endif

void QgsRasterLayerProperties::toggleBuildPyramidsButton()
{
  if ( lbxPyramidResolutions->selectedItems().empty() )
  {
    buttonBuildPyramids->setEnabled( false );
  }
  else
  {
    buttonBuildPyramids->setEnabled( true );
  }
}

void QgsRasterLayerProperties::updatePipeList()
{
  QgsDebugMsg( "Entered" );

#ifndef QGISDEBUG
  tabBar->removeTab( tabBar->indexOf( tabPagePipe ) );
#else
  mPipeTreeWidget->clear();

  mPipeTreeWidget->header()->setResizeMode( QHeaderView::ResizeToContents );

  if ( mPipeTreeWidget->columnCount() <= 1 )
  {
    QStringList labels;
    labels << tr( "Filter" ) << tr( "Bands" ) << tr( "Time" );
    mPipeTreeWidget->setHeaderLabels( labels );
    connect( mPipeTreeWidget, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( pipeItemClicked( QTreeWidgetItem *, int ) ) );
  }

  QgsRasterPipe *pipe = mRasterLayer->pipe();
  for ( int i = 0; i < pipe->size(); i++ )
  {
    QgsRasterInterface * interface = pipe->at( i );
    QStringList texts;
    QString name;
    // Unfortunately at this moment not all interfaces inherits from QObject
    QObject *o = dynamic_cast<QObject*>( interface );
    if ( o )
    {
      //name = o->objectName(); // gives empty with provider
      name = o->metaObject()->className();
    }
    else
    {
      name = QString( typeid( *interface ).name() ).replace( QRegExp( ".*Qgs" ), "Qgs" );
    }

    texts <<  name << QString( "%1" ).arg( interface->bandCount() );
    texts << QString( "%1 ms" ).arg( interface->time() );
    QTreeWidgetItem *item = new QTreeWidgetItem( texts );

#if 0
    // Switching on/off would be possible but problematic - drawer is not pipe
    // memer so we don't know required output format
    // Checkboxes are very useful however for QgsRasterPipe debugging.
    bool on = interface->on();
    item->setCheckState( 0, on ? Qt::Checked : Qt::Unchecked );

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
#endif
    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    item->setFlags( flags );

    mPipeTreeWidget->addTopLevelItem( item );
  }
  updatePipeItems();
#endif
}

void QgsRasterLayerProperties::pipeItemClicked( QTreeWidgetItem * item, int column )
{
  Q_UNUSED( column );
  QgsDebugMsg( "Entered" );
  int idx = mPipeTreeWidget->indexOfTopLevelItem( item );

  // This should not fail because we have enabled only checkboxes of items
  // which may be changed
  mRasterLayer->pipe()->setOn( idx, item->checkState( 0 ) );

  updatePipeItems();
}

void QgsRasterLayerProperties::updatePipeItems()
{
  QgsDebugMsg( "Entered" );

  QgsRasterPipe *pipe = mRasterLayer->pipe();

  for ( int i = 0; i < pipe->size(); i++ )
  {
    if ( i >= mPipeTreeWidget->topLevelItemCount() ) break;
    QTreeWidgetItem *item = mPipeTreeWidget->topLevelItem( i );
    if ( !item ) continue;
    // Checkboxes disabled for now, see above
#if 0
    QgsRasterInterface * iface = pipe->at( i );
    bool on = iface->on();
    Qt::ItemFlags flags = item->flags();
    if ( pipe->canSetOn( i, !on ) )
    {
      flags |= Qt::ItemIsUserCheckable;
    }
    else
    {
      flags |= ( Qt::ItemFlags )~Qt::ItemIsUserCheckable;
    }
    item->setFlags( flags );
#endif
  }
}
