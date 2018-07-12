/***************************************************************************
      qgsribbonapp.cpp - QGIS Ribbon Interface
      ----------------------------------------
    begin                : Wed Dec 16 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsribbonapp.h"
#include "qgsannotationlayer.h"
#include "qgsattributetabledialog.h"
#include "qgsclipboard.h"
#include "qgscomposer.h"
#include "qgscoordinatedisplayer.h"
#include "qgsdecorationgrid.h"
#include "qgsgeoimageannotationitem.h"
#include "qgsgpsconnection.h"
#include "qgsgpsrouteeditor.h"
#include "qgslayertreemodel.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgslegendinterface.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptoolannotation.h"
#include "qgsmessagebaritem.h"
#include "qgsmultimapmanager.h"
#include "qgsprojecttemplateselectiondialog.h"
#include "qgsredlining.h"
#include "qgsribbonlayertreeviewmenuprovider.h"
#include "qgsproject.h"
#include "qgssvgannotationitem.h"
#include "qgssnappingutils.h"
#include "qgsundowidget.h"

#include <QDrag>
#include <QFileDialog>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QShortcut>

QgsRibbonApp::QgsRibbonApp( QSplashScreen *splash, bool restorePlugins, QWidget* parent, Qt::WindowFlags fl )
    : QgisApp( splash, parent, fl )
{
  mWindowStateSuffix = "_ribbon"; // See QgisApp::saveWindowState, QgisApp::restoreWindowState

#pragma message( "warning: TODO" )
  mProjectMenu = new QMenu( this );
  mEditMenu = new QMenu( this );
  mViewMenu = new QMenu( this );
  mLayerMenu = new QMenu( this );
  mNewLayerMenu = new QMenu( this );
  mAddLayerMenu = new QMenu( this );
  mSettingsMenu = new QMenu( this );
  mPluginMenu = new QMenu( this );
  mDatabaseMenu = new QMenu( this );
  mRasterMenu = new QMenu( this );
  mVectorMenu = new QMenu( this );
  mWebMenu = new QMenu( this );
  mWindowMenu = new QMenu( this );
  mHelpMenu = new QMenu( this );
  mPrintComposersMenu = new QMenu( this );
  mRecentProjectsMenu = new QMenu( this );
  mProjectTemplatesMenu = new QMenu( this );
  mPanelMenu = new QMenu( this );
  mFeatureActionMenu = new QMenu( this );

  QgsRibbonWindowBase::setupUi( this );
  QWidget* topWidget = new QWidget();
  QgsRibbonTopWidget::setupUi( topWidget );
  QWidget* statusWidget = new QWidget();
  QgsRibbonStatusWidget::setupUi( statusWidget );
  setMenuWidget( topWidget );
  statusBar()->addPermanentWidget( statusWidget, 1 );
  mLayersWidget->setVisible( false );
  mLayersWidget->setCursor( Qt::ArrowCursor );
  mLayersWidget->resize( qMax( 10, qMin( 800, QSettings().value( "/UI/ribbonLayersWidgetWidth", 200 ).toInt() ) ), mLayersWidget->height() );
  mLayerTreeViewButton->setCursor( Qt::ArrowCursor );
  mGeodataBox->setCollapsed( false );
  mLayersBox->setCollapsed( false );
  mZoomInOutFrame->setCursor( Qt::ArrowCursor );
  mGeodataBox->setStyleSheet( "QgsCollapsibleGroupBox { font-size: 16px; }" );
  mLayersBox->setStyleSheet( "QgsCollapsibleGroupBox { font-size: 16px; }" );

  // The MilX plugin enables the tab, if the plugin is enabled
  mRibbonWidget->setTabEnabled( mRibbonWidget->indexOf( mMssTab ), false );
  // The Globe plugin enables the action, if the plugin is enabled
  mAction3D->setEnabled( false );

  mLanguageCombo->addItem( tr( "System language" ), "" );
  mLanguageCombo->addItem( "English", "en" );
  mLanguageCombo->addItem( "Deutsch", "de" );
  mLanguageCombo->addItem( QString( "Fran%1ais" ).arg( QChar( 0x00E7 ) ), "fr" );
  mLanguageCombo->addItem( "Italiano", "it" );
  bool localeOverridden = QSettings().value( "/locale/overrideFlag", false ).toBool();
  QString userLocale = QSettings().value( "/locale/userLocale" ).toString();
  if ( !localeOverridden )
  {
    mLanguageCombo->setCurrentIndex( 0 );
  }
  else
  {
    int idx = mLanguageCombo->findData( userLocale.left( 2 ).toLower() );
    if ( idx >= 0 )
    {
      mLanguageCombo->setCurrentIndex( idx );
    }
  }
  connect( mLanguageCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( onLanguageChanged( int ) ) );

  mSpinBoxDecimalPlaces->setValue( QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt() );
  connect( mSpinBoxDecimalPlaces, SIGNAL( valueChanged( int ) ), this, SLOT( onDecimalPlacesChanged( int ) ) );

  mSnappingCheckbox->setChecked( QSettings().value( "/Qgis/snapping/enabled", false ).toBool() );
  connect( mSnappingCheckbox, SIGNAL( toggled( bool ) ), this, SLOT( onSnappingChanged( bool ) ) );

  mInfoBar = new QgsMessageBar( mMapCanvas );
  mInfoBar->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

  mLoadingLabel->adjustSize();
  mLoadingLabel->hide();
  mLoadingTimer.setSingleShot( true );
  mLoadingTimer.setInterval( 500 );

  QMenu* openLayerMenu = new QMenu( this );
  openLayerMenu->addAction( tr( "Add vector layer" ), this, SLOT( addVectorLayer() ) );
  openLayerMenu->addAction( tr( "Add raster layer" ), this, SLOT( addRasterLayer() ) );
  mOpenLayerButton->setMenu( openLayerMenu );

  mMapCanvas->installEventFilter( this );
  mLayersWidgetResizeHandle->installEventFilter( this );

  mCoordinateDisplayer = new QgsCoordinateDisplayer( mDisplayCRSButton, mCoordinateLineEdit, mHeightLineEdit, mHeightUnitCombo, mMapCanvas, this );
  mCRSSelectionButton->setMapCanvas( mMapCanvas );

  connect( mScaleComboBox, SIGNAL( scaleChanged() ), this, SLOT( userScale() ) );

  mNumericInputCheckbox->setChecked( QSettings().value( "/Qgis/showNumericInput", false ).toBool() );
  connect( mNumericInputCheckbox, SIGNAL( toggled( bool ) ), this, SLOT( onNumericInputCheckboxToggled( bool ) ) );

  initLayerTreeView();

  // Base class init
  init( restorePlugins );
  mMapCanvas->snappingUtils()->setReadDefaultConfigFromProject( false );
  int snappingRadius = QSettings().value( "/Qgis/snapping/radius", 10 ).toInt();
  mMapCanvas->snappingUtils()->setDefaultSettings( QgsPointLocator::Vertex, snappingRadius, QgsTolerance::Pixels );
  mMapCanvas->snappingUtils()->setSnapToMapMode( QgsSnappingUtils::SnapAllLayers );

  // Redlining
  QgsRedlining::RedliningUi redliningUi;
  redliningUi.buttonNewObject = mToolButtonRedliningNewObject;
  redliningUi.colorButtonFillColor = mToolButtonRedliningFillColor;
  redliningUi.colorButtonOutlineColor = mToolButtonRedliningBorderColor;
  redliningUi.comboFillStyle = mComboBoxRedliningFillStyle;
  redliningUi.comboOutlineStyle = mComboBoxRedliningBorderStyle;
  redliningUi.spinBoxSize = mSpinBoxRedliningSize;
  mRedlining = new QgsRedlining( this, redliningUi );

  // Route editor
  mGpsRouteEditor = new QgsGPSRouteEditor( this, mActionDrawWaypoint, mActionDrawRoute );

  initGPSDisplay();
  connect( mGpsToolButton, SIGNAL( toggled( bool ) ), this, SLOT( enableGPS( bool ) ) );

  configureButtons();

  mSearchWidget->init( mMapCanvas );

  mCatalogBrowser->reload();
  connect( mRefreshCatalogButton, SIGNAL( clicked( bool ) ), mCatalogBrowser, SLOT( reload() ) );

  restoreFavoriteButton( mFavoriteButton1 );
  restoreFavoriteButton( mFavoriteButton2 );
  restoreFavoriteButton( mFavoriteButton3 );
  restoreFavoriteButton( mFavoriteButton4 );
  connect( mFavoriteButton1, SIGNAL( contextMenuRequested( QPoint ) ), this, SLOT( showFavoriteContextMenu( QPoint ) ) );
  connect( mFavoriteButton2, SIGNAL( contextMenuRequested( QPoint ) ), this, SLOT( showFavoriteContextMenu( QPoint ) ) );
  connect( mFavoriteButton3, SIGNAL( contextMenuRequested( QPoint ) ), this, SLOT( showFavoriteContextMenu( QPoint ) ) );
  connect( mFavoriteButton4, SIGNAL( contextMenuRequested( QPoint ) ), this, SLOT( showFavoriteContextMenu( QPoint ) ) );

  connect( mMapCanvas, SIGNAL( layersChanged( QStringList ) ), this, SLOT( checkOnTheFlyProjection( QStringList ) ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( checkOnTheFlyProjection() ) );
  connect( mMapCanvas, SIGNAL( scaleChanged( double ) ), this, SLOT( showScale( double ) ) );
  connect( mMapCanvas, SIGNAL( mapToolSet( QgsMapTool* ) ), this, SLOT( switchToTabForTool( QgsMapTool* ) ) );
  connect( mMapCanvas, SIGNAL( renderStarting() ), &mLoadingTimer, SLOT( start() ) );
  connect( &mLoadingTimer, SIGNAL( timeout() ), mLoadingLabel, SLOT( show() ) );
  connect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), &mLoadingTimer, SLOT( stop() ) );
  connect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), mLoadingLabel, SLOT( hide() ) );
  connect( mRibbonWidget, SIGNAL( currentChanged( int ) ), this, SLOT( pan() ) ); // Change to pan tool when changing active ribbon tab
  connect( mZoomInButton, SIGNAL( clicked( bool ) ), mapCanvas(), SLOT( zoomIn() ) );
  connect( mZoomOutButton, SIGNAL( clicked( bool ) ), mapCanvas(), SLOT( zoomOut() ) );
  connect( mHomeButton, SIGNAL( clicked( bool ) ), this, SLOT( zoomFull() ) );
  connect( clipboard(), SIGNAL( dataChanged() ), this, SLOT( checkCanPaste() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWasAdded( QgsMapLayer* ) ), this, SLOT( checkLayerProjection( QgsMapLayer* ) ) );
}

QgsRibbonApp::QgsRibbonApp()
    : QgisApp()
{
  mWindowStateSuffix = "_ribbon"; // See QgisApp::saveWindowState, QgisApp::restoreWindowState

  QgsRibbonWindowBase::setupUi( this );
  QWidget* topWidget = new QWidget();
  QgsRibbonTopWidget::setupUi( topWidget );
  QWidget* statusWidget = new QWidget();
  QgsRibbonStatusWidget::setupUi( statusWidget );
  setMenuWidget( topWidget );
  statusBar()->addWidget( statusWidget, 1 );

  mInfoBar = new QgsMessageBar( centralWidget() );
  mUndoWidget = new QgsUndoWidget( NULL, mMapCanvas );
  mMapCanvas->freeze();
  initGPSDisplay();
}

QgsRibbonApp::~QgsRibbonApp()
{
  destroy();
}

bool QgsRibbonApp::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mMapCanvas && ev->type() == QEvent::Resize )
  {
    updateWidgetPositions();
  }
  else if ( obj == mLayersWidgetResizeHandle && ev->type() == QEvent::MouseButtonPress )
  {
    QMouseEvent* e = static_cast<QMouseEvent*>( ev );
    if ( e->button() == Qt::LeftButton )
    {
      mResizePressPos = e->pos();
    }
  }
  else if ( obj == mLayersWidgetResizeHandle && ev->type() == QEvent::MouseMove )
  {
    QMouseEvent* e = static_cast<QMouseEvent*>( ev );
    if ( e->buttons() == Qt::LeftButton )
    {
      QPoint delta = e->pos() - mResizePressPos;
      mLayersWidget->resize( qMax( 10, qMin( 800, mLayersWidget->width() + delta.x() ) ), mLayersWidget->height() );
      QSettings().setValue( "/UI/ribbonLayersWidgetWidth", mLayersWidget->width() );
      mLayerTreeViewButton->move( mLayersWidget->width(), mLayerTreeViewButton->y() );
    }
  }
  return false;
}

void QgsRibbonApp::updateWidgetPositions()
{
  // Make sure +/- buttons have constant distance to upper right corner of map canvas
  int distanceToRightBorder = 9;
  int distanceToTop = 20;
  mZoomInOutFrame->move( mMapCanvas->width() - distanceToRightBorder - mZoomInOutFrame->width(), distanceToTop );

  mHomeButton->move( mMapCanvas->width() - distanceToRightBorder - mHomeButton->height(), distanceToTop + 90 );

  // Resize mLayersWidget and mLayerTreeViewButton
  int distanceToTopBottom = 40;
  int layerTreeHeight = mMapCanvas->height() - 2 * distanceToTopBottom;
  mLayerTreeViewButton->setGeometry( mLayerTreeViewButton->pos().x(), distanceToTopBottom, mLayerTreeViewButton->width(), layerTreeHeight );
  mLayersWidget->setGeometry( mLayersWidget->pos().x(), distanceToTopBottom, mLayersWidget->width(), layerTreeHeight );

  // Resize info bar
  double barwidth = 0.5 * mMapCanvas->width();
  double x = 0.5 * mMapCanvas->width() - 0.5 * barwidth;
  double y = mMapCanvas->y();
  mInfoBar->move( x, y );
  mInfoBar->setFixedWidth( barwidth );

  // Move loading label
  mLoadingLabel->move( mMapCanvas->width() - 5 - mLoadingLabel->width(), mMapCanvas->height() - 5 - mLoadingLabel->height() );
}

void QgsRibbonApp::initLayerTreeView()
{
  QgsLayerTreeModel* model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
#ifdef ENABLE_MODELTEST
  new ModelTest( model, this );
#endif
  model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
  model->setFlag( QgsLayerTreeModel::AllowNodeRename );
  model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
  model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
  model->setAutoCollapseLegendNodes( 1 );
  model->setLayerTooltipMode( QgsLayerTreeModel::TOOLTIP_NAME );

  mLayerTreeView->setModel( model );
  mLayerTreeView->setMenuProvider( new QgsRibbonLayerTreeViewMenuProvider( mLayerTreeView, this ) );

  //setup connections
  connect( mLayerTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );
  connect( mLayerTreeView, SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( activeLayerChanged( QgsMapLayer* ) ) );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( readProject( const QDomDocument& ) ) );

  bool otfTransformAutoEnable = QSettings().value( "/Projections/otfTransformAutoEnable", true ).toBool();
  mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );
}

void QgsRibbonApp::mousePressEvent( QMouseEvent* event )
{
  if ( event->buttons() == Qt::LeftButton )
  {
    QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
    if ( button && !button->objectName().startsWith( "mFavoriteButton" ) )
    {
      mDragStartPos = event->pos();
    }
  }
  QgisApp::mousePressEvent( event );
}

void QgsRibbonApp::mouseMoveEvent( QMouseEvent* event )
{
  if ( event->buttons() == Qt::LeftButton && !mDragStartPos.isNull() && ( mDragStartPos - event->pos() ).manhattanLength() >= QApplication::startDragDistance() )
  {
    QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
    if ( button && !button->objectName().startsWith( "mFavoriteButton" ) )
    {
      QMimeData* mimeData = new QMimeData();
      mimeData->setData( "application/qgis-ribbon-button", button->defaultAction()->objectName().toLocal8Bit() );
      QDrag* drag = new QDrag( this );
      drag->setMimeData( mimeData );
      drag->setPixmap( button->icon().pixmap( 32, 32 ) );
      drag->setHotSpot( QPoint( 16, 16 ) );
      drag->exec( Qt::CopyAction );
      mDragStartPos = QPoint();
    }
  }
  QgisApp::mouseMoveEvent( event );
}

void QgsRibbonApp::dragEnterEvent( QDragEnterEvent* event )
{
  QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
  if ( event->mimeData()->hasFormat( "application/qgis-ribbon-button" ) && button && button->objectName().startsWith( "mFavoriteButton" ) )
  {
    event->acceptProposedAction();
  }
  else
  {
    QgisApp::dragEnterEvent( event );
  }
}

void QgsRibbonApp::dropEvent( QDropEvent* event )
{
  if ( event->mimeData()->hasFormat( "application/qgis-ribbon-button" ) )
  {
    QString actionName = QString::fromLocal8Bit( event->mimeData()->data( "application/qgis-ribbon-button" ).data() );
    QAction* action = findChild<QAction*>( actionName );
    QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
    if ( action && button && button->objectName().startsWith( "mFavoriteButton" ) )
    {
      button->setEnabled( true );
      setActionToButton( action, button );
      QSettings().setValue( "/UI/FavoriteAction/" + button->objectName(), actionName );
    }
  }
  else
  {
    QgisApp::dropEvent( event );
  }
}

void QgsRibbonApp::restoreFavoriteButton( QToolButton* button )
{
  QString actionName = QSettings().value( "/UI/FavoriteAction/" + button->objectName() ).toString();
  if ( actionName.isEmpty() )
  {
    return;
  }

  QAction* action = findChild<QAction*>( actionName );
  if ( action )
  {
    setActionToButton( action, button );
  }
}

void QgsRibbonApp::configureButtons()
{
  //My maps tab

  connect( mActionNew, SIGNAL( triggered() ), this, SLOT( showProjectSelectionWidget() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_N ), this ), SIGNAL( activated() ), mActionNew, SLOT( trigger() ) );
  setActionToButton( mActionNew, mNewButton );

  connect( mActionOpen, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_O ), this ), SIGNAL( activated() ), mActionOpen, SLOT( trigger() ) );
  setActionToButton( mActionOpen, mOpenButton );

  connect( mActionSave, SIGNAL( triggered() ), this, SLOT( saveProject() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_S ), this ), SIGNAL( activated() ), mActionSave, SLOT( trigger() ) );
  setActionToButton( mActionSave, mSaveButton );

  connect( mActionSaveAs, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_S ), this ), SIGNAL( activated() ), mActionSaveAs, SLOT( trigger() ) );
  setActionToButton( mActionSaveAs, mSaveAsButton );

  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_P ), this ), SIGNAL( activated() ), mActionPrint, SLOT( trigger() ) );
  setActionToButton( mActionPrint, mPrintButton );

  connect( mActionCopy, SIGNAL( triggered() ), this, SLOT( saveMapToClipboard() ) );
  setActionToButton( mActionCopy, mCopyButton );

  connect( mActionSaveMapExtent, SIGNAL( triggered() ), this, SLOT( saveMapAsImage() ) );
  setActionToButton( mActionSaveMapExtent, mSaveMapExtentButton );

  connect( mActionExportKML, SIGNAL( triggered() ), this, SLOT( kmlExport() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_E ), this ), SIGNAL( activated() ), mActionExportKML, SLOT( trigger() ) );
  setActionToButton( mActionExportKML, mExportKMLButton );

  connect( mActionImportKML, SIGNAL( triggered() ), this, SLOT( kmlImport() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_I ), this ), SIGNAL( activated() ), mActionImportKML, SLOT( trigger() ) );
  setActionToButton( mActionImportKML, mImportKMLButton );

  //view tab
  connect( mActionZoomLast, SIGNAL( triggered() ), this, SLOT( zoomToPrevious() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_PageUp ), this ), SIGNAL( activated() ), mActionZoomLast, SLOT( trigger() ) );
  setActionToButton( mActionZoomLast, mZoomLastButton );
  connect( mActionZoomNext, SIGNAL( triggered() ), this, SLOT( zoomToNext() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_PageDown ), this ), SIGNAL( activated() ), mActionZoomNext, SLOT( trigger() ) );
  setActionToButton( mActionZoomNext, mZoomNextButton );
  connect( mActionNewMapWindow, SIGNAL( triggered() ), mMultiMapManager, SLOT( addMapWidget() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_V, Qt::CTRL + Qt::Key_W ), this ), SIGNAL( activated() ), mActionNewMapWindow, SLOT( trigger() ) );
  setActionToButton( mActionNewMapWindow, mNewMapWindowButton );
  setActionToButton( mAction3D, m3DButton );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_V, Qt::CTRL + Qt::Key_3 ), this ), SIGNAL( activated() ), mAction3D, SLOT( trigger() ) );
  setActionToButton( mActionGrid, mGridButton );
  connect( mActionGrid, SIGNAL( triggered() ), mDecorationGrid, SLOT( run() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_V, Qt::CTRL + Qt::Key_G ), this ), SIGNAL( activated() ), mActionGrid, SLOT( trigger() ) );

  connect( mActionGuideGrid, SIGNAL( triggered( bool ) ), this, SLOT( toggleGuideGridTool( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_V, Qt::CTRL + Qt::Key_R ), this ), SIGNAL( activated() ), mActionGuideGrid, SLOT( trigger() ) );
  setActionToButton( mActionGuideGrid, mGuideGridButton, mMapTools.mGuideGridTool );

  //draw tab
  connect( mActionPin, SIGNAL( triggered( bool ) ), this, SLOT( addPinAnnotation( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_M ), this ), SIGNAL( activated() ), mActionPin, SLOT( trigger() ) );
  setActionToButton( mActionPin, mPinButton, mMapTools.mPinAnnotation );

  connect( mActionAddImage, SIGNAL( triggered( bool ) ), this, SLOT( addImage() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_I ), this ), SIGNAL( activated() ), mActionAddImage, SLOT( trigger() ) );
  setActionToButton( mActionAddImage, mAddImageButton );

  connect( mActionPaste, SIGNAL( triggered( bool ) ), this, SLOT( paste() ) );
  setActionToButton( mActionPaste, mPasteButton );
  mActionPaste->setEnabled( canPaste() );

  connect( mActionDeleteItems, SIGNAL( triggered( bool ) ), this, SLOT( deleteItems( bool ) ) );
  setActionToButton( mActionDeleteItems, mDeleteItemsButton, mMapTools.mDeleteItems );

  //analysis tab
  connect( mActionDistance, SIGNAL( triggered( bool ) ), this, SLOT( measure( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_D ), this ), SIGNAL( activated() ), mActionDistance, SLOT( trigger() ) );
  setActionToButton( mActionDistance, mDistanceButton, mMapTools.mMeasureDist );

  connect( mActionArea, SIGNAL( triggered( bool ) ), this, SLOT( measureArea( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_A ), this ), SIGNAL( activated() ), mActionArea, SLOT( trigger() ) );
  setActionToButton( mActionArea, mAreaButton, mMapTools.mMeasureArea );

  connect( mActionCircle, SIGNAL( triggered( bool ) ), this, SLOT( measureCircle( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_C ), this ), SIGNAL( activated() ), mActionCircle, SLOT( trigger() ) );
  setActionToButton( mActionCircle, mMeasureCircleButton, mMapTools.mMeasureCircle );

  connect( mActionAzimuth, SIGNAL( triggered( bool ) ), this, SLOT( measureAzimuth( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_B ), this ), SIGNAL( activated() ), mActionAzimuth, SLOT( trigger() ) );
  setActionToButton( mActionAzimuth, mAzimuthButton, mMapTools.mMeasureAzimuth );

  connect( mActionProfile, SIGNAL( triggered( bool ) ), this, SLOT( measureHeightProfile( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_P ), this ), SIGNAL( activated() ), mActionProfile, SLOT( trigger() ) );
  setActionToButton( mActionProfile, mProfileButton, mMapTools.mMeasureHeightProfile );

  connect( mActionSlope, SIGNAL( triggered( bool ) ), this, SLOT( slope( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_S ), this ), SIGNAL( activated() ), mActionSlope, SLOT( trigger() ) );
  setActionToButton( mActionSlope, mSlopeButton, mMapTools.mSlope );

  connect( mActionHillshade, SIGNAL( triggered( bool ) ), this, SLOT( hillshade( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_H ), this ), SIGNAL( activated() ), mActionHillshade, SLOT( trigger() ) );
  setActionToButton( mActionHillshade, mHillshadeButton, mMapTools.mHillshade );

  connect( mActionViewshed, SIGNAL( triggered( bool ) ), this, SLOT( viewshed( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_V ), this ), SIGNAL( activated() ), mActionViewshed, SLOT( trigger() ) );
  setActionToButton( mActionViewshed, mViewshedButton, mMapTools.mViewshed );

  //gps tab
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_W ), this ), SIGNAL( activated() ), mActionDrawWaypoint, SLOT( trigger() ) );
  setActionToButton( mActionDrawWaypoint, mDrawWaypointButton );

  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_R ), this ), SIGNAL( activated() ), mActionDrawRoute, SLOT( trigger() ) );
  setActionToButton( mActionDrawRoute, mDrawRouteButton );

  connect( mActionEnableGPS, SIGNAL( triggered( bool ) ), this, SLOT( enableGPS( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_T ), this ), SIGNAL( activated() ), mActionEnableGPS, SLOT( trigger() ) );
  setActionToButton( mActionEnableGPS, mEnableGPSButton );

  connect( mActionMoveWithGPS, SIGNAL( triggered( bool ) ), this, SLOT( moveWithGPS( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_M ), this ), SIGNAL( activated() ), mActionMoveWithGPS, SLOT( trigger() ) );
  setActionToButton( mActionMoveWithGPS, mMoveWithGPSButton );

  connect( mActionImportGPX, SIGNAL( triggered() ), mGpsRouteEditor, SLOT( importGpx() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_I ), this ), SIGNAL( activated() ), mActionImportGPX, SLOT( trigger() ) );
  setActionToButton( mActionImportGPX, mGpxImportButton );

  connect( mActionExportGPX, SIGNAL( triggered() ), mGpsRouteEditor, SLOT( exportGpx() ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_E ), this ), SIGNAL( activated() ), mActionExportGPX, SLOT( trigger() ) );
  setActionToButton( mActionExportGPX, mGpxExportButton );

  //mss tab
  setActionToButton( mActionMilx, mMilxButton );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_S ), this ), SIGNAL( activated() ), mActionMilx, SLOT( trigger() ) );
  setActionToButton( mActionSaveMilx, mSaveMilxButton );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_E ), this ), SIGNAL( activated() ), mActionSaveMilx, SLOT( trigger() ) );
  setActionToButton( mActionLoadMilx, mLoadMilxButton );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_I ), this ), SIGNAL( activated() ), mActionLoadMilx, SLOT( trigger() ) );
  setActionToButton( mActionImportOVL, mOvlButton );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_O ), this ), SIGNAL( activated() ), mActionImportOVL, SLOT( trigger() ) );

  //help tab
  setActionToButton( mActionHelp, mHelpButton );
  setActionToButton( mActionAbout, mAboutButton );
}

void QgsRibbonApp::setActionToButton( QAction* action, QToolButton* button, QgsMapTool* tool )
{
  button->setDefaultAction( action );
  button->setIconSize( QSize( 32, 32 ) );
  if ( tool )
  {
    tool->setAction( action );
    button->setCheckable( true );
  }
}

QWidget* QgsRibbonApp::addRibbonTab( const QString& name )
{
  QWidget* widget = new QWidget( mRibbonWidget );
  widget->setLayout( new QHBoxLayout() );
  widget->layout()->setContentsMargins( 0, 0, 0, 0 );
  widget->layout()->setSpacing( 6 );
  widget->layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );
  mRibbonWidget->addTab( widget, name );
  return widget;
}

void QgsRibbonApp::addActionToTab( QAction* action, QWidget* tabWidget, QgsMapTool* associatedMapTool )
{
  QgsRibbonButton* button = new QgsRibbonButton();
  button->setObjectName( QUuid::createUuid().toString() );
  button->setText( action->text() );
  button->setMinimumSize( QSize( 0, 80 ) );
  button->setMaximumSize( QSize( 80, 80 ) );
  button->setIconSize( QSize( 32, 32 ) );

  QSizePolicy sizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
  sizePolicy.setHorizontalStretch( 0 );
  sizePolicy.setVerticalStretch( 0 );
  sizePolicy.setHeightForWidth( button->sizePolicy().hasHeightForWidth() );
  button->setSizePolicy( sizePolicy );

  QHBoxLayout* layout = dynamic_cast<QHBoxLayout*>( tabWidget->layout() );
  layout->insertWidget( layout->count() - 1, button );
  setActionToButton( action, button, associatedMapTool );
}

void QgsRibbonApp::attributeTable()
{
  QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( layer )
  {
    QgsAttributeTableDialog *dialog = new QgsAttributeTableDialog( layer, true );
    dialog->show();
    // the dialog will be deleted by itself on close
  }
}

void QgsRibbonApp::on_mLayerTreeViewButton_clicked()
{
  bool visible = mLayersWidget->isVisible();
  mLayersWidget->setVisible( !visible );

  if ( !visible )
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/images/ribbon/layerbaum_unfolded.png" ) );
    mLayerTreeViewButton->move( mLayersWidget->size().width(), mLayerTreeViewButton->y() );
  }
  else
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/images/ribbon/layerbaum_folded.png" ) );
    mLayerTreeViewButton->move( 0, mLayerTreeViewButton->y() );
  }
}

void QgsRibbonApp::checkOnTheFlyProjection( const QStringList& prevLayers )
{
  mInfoBar->popWidget( mReprojMsgItem.data() );
  QString destAuthId = mMapCanvas->mapSettings().destinationCrs().authid();
  QStringList reprojLayers;
  // Look at legend interface instead of maplayerregistry, to only check layers
  // the user can actually see
  foreach ( QgsMapLayer* layer, mMapCanvas->layers() )
  {
    if ( layer->type() != QgsMapLayer::RedliningLayer && layer->type() != QgsMapLayer::PluginLayer && !layer->crs().authid().startsWith( "USER:" ) && layer->crs().authid() != destAuthId && !prevLayers.contains( layer->id() ) )
    {
      reprojLayers.append( layer->name() );
    }
  }
  if ( !reprojLayers.isEmpty() )
  {
    mReprojMsgItem = new QgsMessageBarItem( tr( "On the fly projection enabled" ), tr( "The following layers are being reprojected to the selected CRS: %1. Performance may suffer." ).arg( reprojLayers.join( ", " ) ), QgsMessageBar::INFO, 10, this );
    mInfoBar->pushItem( mReprojMsgItem.data() );
  }
}

void QgsRibbonApp::addImage()
{
  pan(); // Ensure pan tool is active

  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QSet<QString> formats;
  foreach ( const QByteArray& format, QImageReader::supportedImageFormats() )
  {
    formats.insert( QString( "*.%1" ).arg( QString( format ).toLower() ) );
  }
  formats.insert( "*.svg" ); // Ensure svg is present

  QString filter = QString( "Images (%1)" ).arg( QStringList( formats.toList() ).join( " " ) );
  QString filename = QFileDialog::getOpenFileName( this, tr( "Select Image" ), lastDir, filter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  QString errMsg;
  if ( filename.endsWith( ".svg", Qt::CaseInsensitive ) )
  {
    QgsSvgAnnotationItem* item = new QgsSvgAnnotationItem( mapCanvas() );
    item->setFilePath( filename );
    item->setMapPosition( mapCanvas()->extent().center(), mapCanvas()->mapSettings().destinationCrs() );
    QgsAnnotationLayer::getLayer( mapCanvas(), "svgSymbols", tr( "SVG graphics" ) )->addItem( item );
    mapCanvas()->setMapTool( new QgsMapToolEditAnnotation( mapCanvas(), item ) );
  }
  else if ( !QgsGeoImageAnnotationItem::create( mapCanvas(), filename, false, &errMsg ) )
  {
    mInfoBar->pushCritical( tr( "Could not add image" ), errMsg );
  }
}

void QgsRibbonApp::userScale()
{
  mMapCanvas->zoomScale( 1.0 / mScaleComboBox->scale() );
}

void QgsRibbonApp::showScale( double scale )
{
  mScaleComboBox->setScale( 1.0 / scale );
}

void QgsRibbonApp::switchToTabForTool( QgsMapTool *tool )
{
  if ( tool && tool->action() )
  {
    foreach ( QWidget* widget, tool->action()->associatedWidgets() )
    {
      if ( dynamic_cast<QgsRibbonButton*>( widget ) )
      {
        for ( int i = 0, n = mRibbonWidget->count(); i < n; ++i )
        {
          if ( mRibbonWidget->widget( i )->findChild<QWidget*>( widget->objectName() ) )
          {
            mRibbonWidget->blockSignals( true );
            mRibbonWidget->setCurrentIndex( i );
            mRibbonWidget->blockSignals( false );
            break;
          }
        }
      }
    }
    // If action is not associated to a ribbon button, try with redlining and gpx route editor
    if ( tool->action()->parent() == mRedlining )
    {
      mRibbonWidget->blockSignals( true );
      mRibbonWidget->setCurrentWidget( mDrawTab );
      mRibbonWidget->blockSignals( false );
    }
    else if ( tool->action()->parent() == mGpsRouteEditor )
    {
      mRibbonWidget->blockSignals( true );
      mRibbonWidget->setCurrentWidget( mGpsTab );
      mRibbonWidget->blockSignals( false );
    }
  }
  // Nothing found, do nothing
}

void QgsRibbonApp::showProjectSelectionWidget()
{
  QgsProjectTemplateSelectionDialog( this ).exec();
}

void QgsRibbonApp::onLanguageChanged( int idx )
{
  QString locale = mLanguageCombo->itemData( idx ).toString();
  if ( locale.isEmpty() )
  {
    QSettings().setValue( "/locale/overrideFlag", false );
  }
  else
  {
    QSettings().setValue( "/locale/overrideFlag", true );
    QSettings().setValue( "/locale/userLocale", locale );
  }
  QMessageBox::information( this, tr( "Language Changed" ), tr( "The language will be changed at the next program launch." ) );
}

void QgsRibbonApp::onDecimalPlacesChanged( int places )
{
  QSettings().setValue( "/Qgis/measure/decimalplaces", places );
}

void QgsRibbonApp::onSnappingChanged( bool enabled )
{
  QSettings().setValue( "/Qgis/snapping/enabled", enabled );
}

void QgsRibbonApp::onNumericInputCheckboxToggled( bool checked )
{
  QSettings().setValue( "/Qgis/showNumericInput", checked );
}

void QgsRibbonApp::enableGPS( bool enabled )
{
  mGpsToolButton->blockSignals( true );
  mActionEnableGPS->blockSignals( true );
  mGpsToolButton->setChecked( enabled );
  mActionEnableGPS->setChecked( enabled );
  mGpsToolButton->blockSignals( false );
  mActionEnableGPS->blockSignals( false );
  if ( enabled )
  {
    setGPSIcon( Qt::blue );
    messageBar()->pushMessage( tr( "Connecting to GPS device..." ), QString(), QgsMessageBar::INFO, 5 );
    mCanvasGPSDisplay.connectGPS();
  }
  else
  {
    setGPSIcon( Qt::black );
    messageBar()->pushMessage( tr( "GPS connection closed" ), QString(), QgsMessageBar::INFO, 5 );
    mCanvasGPSDisplay.disconnectGPS();
  }
}

void QgsRibbonApp::gpsDetected()
{
  messageBar()->pushMessage( tr( "GPS device successfully connected" ), QgsMessageBar::INFO, messageTimeout() );
  setGPSIcon( Qt::white );
}

void QgsRibbonApp::gpsDisconnected()
{
  enableGPS( false );
}

void QgsRibbonApp::gpsConnectionFailed()
{
  messageBar()->pushMessage( tr( "Connection to GPS device failed" ), QgsMessageBar::CRITICAL, messageTimeout() );
  mGpsToolButton->blockSignals( true );
  mActionEnableGPS->blockSignals( true );
  mGpsToolButton->setChecked( false );
  mActionEnableGPS->setChecked( false );
  mGpsToolButton->blockSignals( false );
  mActionEnableGPS->blockSignals( false );
  setGPSIcon( Qt::black );
}

void QgsRibbonApp::gpsFixChanged( QgsMapCanvasGPSDisplay::FixStatus fixStatus )
{
  switch ( fixStatus )
  {
    case QgsMapCanvasGPSDisplay::NoData:
      setGPSIcon( Qt::white ); break;
    case QgsMapCanvasGPSDisplay::NoFix:
      setGPSIcon( Qt::red ); break;
    case QgsMapCanvasGPSDisplay::Fix2D:
      setGPSIcon( Qt::yellow ); break;
    case QgsMapCanvasGPSDisplay::Fix3D:
      setGPSIcon( Qt::green ); break;
  }
}

void QgsRibbonApp::moveWithGPS( bool enabled )
{
  mCanvasGPSDisplay.setRecenterMap( enabled ? QgsMapCanvasGPSDisplay::WhenNeeded : QgsMapCanvasGPSDisplay::Never );
}

void QgsRibbonApp::initGPSDisplay()
{
  mCanvasGPSDisplay.setMapCanvas( mMapCanvas );
  connect( &mCanvasGPSDisplay, SIGNAL( gpsConnected() ), this, SLOT( gpsDetected() ) );
  connect( &mCanvasGPSDisplay, SIGNAL( gpsDisconnected() ), this, SLOT( gpsDisconnected() ) );
  connect( &mCanvasGPSDisplay, SIGNAL( gpsConnectionFailed() ), this, SLOT( gpsConnectionFailed() ) );
  connect( &mCanvasGPSDisplay, SIGNAL( gpsFixStatusChanged( QgsMapCanvasGPSDisplay::FixStatus ) ), this, SLOT( gpsFixChanged( QgsMapCanvasGPSDisplay::FixStatus ) ) );
  setGPSIcon( Qt::black );
}

void QgsRibbonApp::setGPSIcon( const QColor &color )
{
  QPixmap pixmap( mGpsToolButton->size() );
  pixmap.fill( color );
  mGpsToolButton->setIcon( QIcon( pixmap ) );
}

void QgsRibbonApp::showFavoriteContextMenu( const QPoint& pos )
{
  QgsRibbonButton* button = qobject_cast<QgsRibbonButton*>( QObject::sender() );
  QMenu menu;
  QAction* removeAction = menu.addAction( tr( "Remove" ) );
  if ( menu.exec( button->mapToGlobal( pos ) ) == removeAction )
  {
    QSettings().setValue( "/UI/FavoriteAction/" + button->objectName(), "" );
    button->setText( tr( "Favorite" ) );
    button->setIcon( QIcon( ":/images/ribbon/favorit.png" ) );
    button->setDefaultAction( 0 );
    button->setIconSize( QSize( 16, 16 ) );
    button->setEnabled( false );
  }
}

void QgsRibbonApp::saveProject()
{
  if ( fileSave() )
  {
    messageBar()->pushMessage( tr( "Project saved" ), "", QgsMessageBar::INFO, 3 );
  }
}

void QgsRibbonApp::checkCanPaste()
{
  mActionPaste->setEnabled( canPaste() );
}

void QgsRibbonApp::checkLayerProjection( QgsMapLayer* layer )
{
  if ( layer->crs().authid().startsWith( "USER:" ) )
  {
    QPushButton* btn = new QPushButton( tr( "Manually set projection" ) );
    btn->setFlat( true );
    QgsMessageBarItem* item = new QgsMessageBarItem(
      tr( "Unknown layer projection" ),
      tr( "The projection of the layer %1 could not be recognized, its and features might be misplaced." ).arg( layer->name() ),
      btn,
      QgsMessageBar::WARNING,
      10 );
    connect( btn, &QPushButton::clicked, [=]
    {
      messageBar()->popWidget( item );
      showLayerProperties( layer );
    } );
    messageBar()->pushItem( item );
  }
}
