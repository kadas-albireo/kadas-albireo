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
#include "qgsmessagebaritem.h"
#include "qgsmultimapmanager.h"
#include "qgsprojecttemplateselectiondialog.h"
#include "qgsredlining.h"
#include "qgsribbonlayertreeviewmenuprovider.h"
#include "qgsproject.h"
#include "qgsundowidget.h"

#include <QFileDialog>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>

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
  mLayerTreeViewButton->setCursor( Qt::ArrowCursor );
  mGeodataBox->setCollapsed( false );
  mLayersBox->setCollapsed( false );
  mLayersSpacer->setVisible( false );
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

  mCoordinateDisplayer = new QgsCoordinateDisplayer( mDisplayCRSButton, mCoordinateLineEdit, mHeightUnitCombo, mMapCanvas, this );
  mCRSSelectionButton->setMapCanvas( mMapCanvas );

  connect( mScaleComboBox, SIGNAL( scaleChanged() ), this, SLOT( userScale() ) );
  connect( mLayersBox, SIGNAL( collapsedStateChanged( bool ) ), this, SLOT( toggleLayersSpacer() ) );
  connect( mGeodataBox, SIGNAL( collapsedStateChanged( bool ) ), this, SLOT( toggleLayersSpacer() ) );

  mNumericInputCheckbox->setChecked( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  connect( mNumericInputCheckbox, SIGNAL( toggled( bool ) ), this, SLOT( onNumericInputCheckboxToggled( bool ) ) );

  initLayerTreeView();

  // Base class init
  init( restorePlugins );

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

  connect( mMapCanvas, SIGNAL( layersChanged() ), this, SLOT( checkOnTheFlyProjection() ) );
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

void QgsRibbonApp::setTheme( QString themeName )
{
  //change themes of all composers
  QSet<QgsComposer*>::iterator composerIt = mPrintComposers.begin();
  for ( ; composerIt != mPrintComposers.end(); ++composerIt )
  {
    ( *composerIt )->setupTheme();
  }

  emit currentThemeChanged( themeName );
}

bool QgsRibbonApp::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mMapCanvas && ev->type() == QEvent::Resize )
  {
    updateWidgetPositions();
  }
  return false;
}

void QgsRibbonApp::updateWidgetPositions()
{
  QRect mapCanvasGeometry = mMapCanvas->geometry();

  // Make sure +/- buttons have constant distance to upper right corner of map canvas
  QRect zoomLayoutGeometry = mZoomInOutFrame->geometry();
  int distanceToRightBorder = 9;
  int distanceToTop = 20;
  mZoomInOutFrame->move( mapCanvasGeometry.width() - distanceToRightBorder - zoomLayoutGeometry.width(), distanceToTop );

  // Resize mLayersWidget and mLayerTreeViewButton
  int distanceToTopBottom = 40;
  int layerTreeHeight = mapCanvasGeometry.height() - 2 * distanceToTopBottom;
  mLayerTreeViewButton->setGeometry( mLayerTreeViewButton->pos().x(), distanceToTopBottom, mLayerTreeViewButton->geometry().width(), layerTreeHeight );
  mLayersWidget->setGeometry( mLayersWidget->pos().x(), distanceToTopBottom, mLayersWidget->geometry().width(), layerTreeHeight );

  // Resize info bar
  double barwidth = 0.5 * mMapCanvas->geometry().width();
  double x = 0.5 * mMapCanvas->geometry().width() - 0.5 * barwidth;
  double y = mMapCanvas->geometry().y();
  mInfoBar->move( x, y );
  mInfoBar->setFixedWidth( barwidth );

  // Move loading label
  mLoadingLabel->move( mMapCanvas->geometry().width() - 5 - mLoadingLabel->width(), mMapCanvas->geometry().height() - 5 - mLoadingLabel->height() );
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
  model->setAutoCollapseLegendNodes( 10 );
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
  if ( event->mimeData()->hasFormat( "application/qgis-ribbon-button" ) )
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
    if ( action && button )
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
  setActionToButton( mActionNew, mNewButton );

  connect( mActionOpen, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
  setActionToButton( mActionOpen, mOpenButton );

  connect( mActionSave, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
  setActionToButton( mActionSave, mSaveButton );

  connect( mActionSaveAs, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );
  setActionToButton( mActionSaveAs, mSaveAsButton );

  setActionToButton( mActionPrint, mPrintButton );

  connect( mActionCopy, SIGNAL( triggered() ), this, SLOT( saveMapToClipboard() ) );
  setActionToButton( mActionCopy, mCopyButton );

  setActionToButton( mActionPrint, mPrintButton );

  connect( mActionSaveMapExtent, SIGNAL( triggered() ), this, SLOT( saveMapAsImage() ) );
  setActionToButton( mActionSaveMapExtent, mSaveMapExtentButton );

  connect( mActionExportKML, SIGNAL( triggered() ), this, SLOT( kmlExport() ) );
  setActionToButton( mActionExportKML, mExportKMLButton );

  //mActionImportOVL
  //setActionToButton( mActionImportOVL, mImportOVLButton );

  //view tab
  connect( mActionZoomLast, SIGNAL( triggered() ), this, SLOT( zoomToPrevious() ) );
  setActionToButton( mActionZoomLast, mZoomLastButton );
  connect( mActionZoomNext, SIGNAL( triggered() ), this, SLOT( zoomToNext() ) );
  setActionToButton( mActionZoomNext, mZoomNextButton );
  connect( mActionNewMapWindow, SIGNAL( triggered() ), mMultiMapManager, SLOT( addMapWidget() ) );
  setActionToButton( mActionNewMapWindow, mNewMapWindowButton );
  setActionToButton( mAction3D, m3DButton );
  setActionToButton( mActionGrid, mGridButton );
  connect( mActionGrid, SIGNAL( triggered() ), mDecorationGrid, SLOT( run() ) );

  //draw tab
  connect( mActionPin, SIGNAL( triggered( bool ) ), this, SLOT( addPinAnnotation( bool ) ) );
  setActionToButton( mActionPin, mPinButton, mMapTools.mPinAnnotation );

  connect( mActionAddCameraPicture, SIGNAL( triggered( bool ) ), this, SLOT( addCameraPicture() ) );
  setActionToButton( mActionAddCameraPicture, mAddPictureButton );

  connect( mActionDeleteItems, SIGNAL( triggered( bool ) ), this, SLOT( deleteItems( bool ) ) );
  setActionToButton( mActionDeleteItems, mDeleteItemsButton, mMapTools.mDeleteItems );

  //analysis tab
  connect( mActionDistance, SIGNAL( triggered( bool ) ), this, SLOT( measure( bool ) ) );
  setActionToButton( mActionDistance, mDistanceButton, mMapTools.mMeasureDist );

  connect( mActionArea, SIGNAL( triggered( bool ) ), this, SLOT( measureArea( bool ) ) );
  setActionToButton( mActionArea, mAreaButton, mMapTools.mMeasureArea );

  connect( mActionCircle, SIGNAL( triggered( bool ) ), this, SLOT( measureCircle( bool ) ) );
  setActionToButton( mActionCircle, mMeasureCircleButton, mMapTools.mMeasureCircle );

  connect( mActionProfile, SIGNAL( triggered( bool ) ), this, SLOT( measureHeightProfile( bool ) ) );
  setActionToButton( mActionProfile, mProfileButton, mMapTools.mMeasureHeightProfile );

  connect( mActionAzimuth, SIGNAL( triggered( bool ) ), this, SLOT( measureAzimuth( bool ) ) );
  setActionToButton( mActionAzimuth, mAzimuthButton, mMapTools.mMeasureAzimuth );

  connect( mActionSlope, SIGNAL( triggered( bool ) ), this, SLOT( slope( bool ) ) );
  setActionToButton( mActionSlope, mSlopeButton, mMapTools.mSlope );

  connect( mActionHillshade, SIGNAL( triggered( bool ) ), this, SLOT( hillshade( bool ) ) );
  setActionToButton( mActionHillshade, mHillshadeButton, mMapTools.mHillshade );

  connect( mActionViewshed, SIGNAL( triggered( bool ) ), this, SLOT( viewshed( bool ) ) );
  setActionToButton( mActionViewshed, mViewshedButton, mMapTools.mViewshed );

  //gps tab
  setActionToButton( mActionDrawWaypoint, mDrawWaypointButton );
  setActionToButton( mActionDrawRoute, mDrawRouteButton );

  connect( mActionEnableGPS, SIGNAL( triggered( bool ) ), this, SLOT( enableGPS( bool ) ) );
  setActionToButton( mActionEnableGPS, mEnableGPSButton );

  connect( mActionMoveWithGPS, SIGNAL( triggered( bool ) ), this, SLOT( moveWithGPS( bool ) ) );
  setActionToButton( mActionMoveWithGPS, mMoveWithGPSButton );

  connect( mActionImportGPX, SIGNAL( triggered() ), mGpsRouteEditor, SLOT( importGpx() ) );
  setActionToButton( mActionImportGPX, mGpxImportButton );

  connect( mActionExportGPX, SIGNAL( triggered() ), mGpsRouteEditor, SLOT( exportGpx() ) );
  setActionToButton( mActionExportGPX, mGpxExportButton );

  //mss tab
  setActionToButton( mActionMilx, mMilxButton );
  setActionToButton( mActionSaveMilx, mSaveMilxButton );
  setActionToButton( mActionLoadMilx, mLoadMilxButton );
  setActionToButton( mActionImportOVL, mOvlButton );

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

void QgsRibbonApp::checkOnTheFlyProjection( )
{
  mInfoBar->popWidget( mReprojMsgItem.data() );
  QString destAuthId = mMapCanvas->mapSettings().destinationCrs().authid();
  QStringList reprojLayers;
  // Look at legend interface instead of maplayerregistry, to only check layers
  // the user can actually see
  foreach ( QgsMapLayer* layer, mMapCanvas->layers() )
  {
    if ( layer->type() != QgsMapLayer::RedliningLayer && layer->type() != QgsMapLayer::PluginLayer && layer->crs().authid() != destAuthId )
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

void QgsRibbonApp::addCameraPicture()
{
  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QSet<QString> formats;
  foreach ( const QByteArray& format, QImageReader::supportedImageFormats() )
  {
    formats.insert( QString( "*.%1" ).arg( QString( format ).toLower() ) );
  }
  QString filter = QString( "Camera pictures (%1)" ).arg( QStringList( formats.toList() ).join( " " ) );
  QString filename = QFileDialog::getOpenFileName( this, tr( "Select Camera Picture" ), lastDir, filter );
  if ( !filename.isEmpty() )
  {
    QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
    QString errMsg;
    if ( !QgsGeoImageAnnotationItem::create( mapCanvas(), filename, &errMsg ) )
    {
      mInfoBar->pushCritical( tr( "Could not add picture" ), errMsg );
    }
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

void QgsRibbonApp::toggleLayersSpacer()
{
  mLayersSpacer->setVisible( mLayersBox->isCollapsed() || mGeodataBox->isCollapsed() );
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

void QgsRibbonApp::onNumericInputCheckboxToggled( bool checked )
{
  QSettings().setValue( "/qgis/showNumericInput", checked );
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
