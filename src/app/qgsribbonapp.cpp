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
#include "qgslayertreemodel.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgsmultimapmanager.h"
#include "qgsredlining.h"
#include "qgsribbonlayertreeviewmenuprovider.h"
#include "qgsproject.h"
#include "qgsundowidget.h"

#include <QMenu>
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
  mLayerTreeView->setVisible( false );
  mLayerTreeView->setCursor( Qt::ArrowCursor );
  mLayerTreeViewButton->setCursor( Qt::ArrowCursor );
  mZoomInOutFrame->setCursor( Qt::ArrowCursor );

  mInfoBar = new QgsMessageBar( mMapCanvas );
  mInfoBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );

  initLayerTreeView();

  // Base class init
  init( restorePlugins );

  mCoordinateDisplayer = new QgsCoordinateDisplayer( mCRSComboBox, mCoordinateLineEdit, mMapCanvas, this );
  connect( mCoordinateDisplayer, SIGNAL( coordinateDisplayFormatChanged( QgsCoordinateUtils::TargetFormat&, QString& ) ), this, SIGNAL( coordinateDisplayFormatChanged( QgsCoordinateUtils::TargetFormat&, QString& ) ) );
  mCRSSelectionButton->setMapCanvas( mMapCanvas );

  connect( mScaleComboBox, SIGNAL( scaleChanged() ), this, SLOT( userScale() ) );

  configureButtons();

  mSearchWidget->init( mMapCanvas );

  restoreFavoriteButton( mFavoriteButton1 );
  restoreFavoriteButton( mFavoriteButton2 );
  restoreFavoriteButton( mFavoriteButton3 );
  restoreFavoriteButton( mFavoriteButton4 );

  //Redlining
  QgsRedlining::RedliningUi redliningUi;
  redliningUi.buttonNewObject = mToolButtonRedliningNewObject;
  redliningUi.colorButtonFillColor = mToolButtonRedliningFillColor;
  redliningUi.colorButtonOutlineColor = mToolButtonRedliningBorderColor;
  redliningUi.comboFillStyle = mComboBoxRedliningFillStyle;
  redliningUi.comboOutlineStyle = mComboBoxRedliningBorderStyle;
  redliningUi.spinBoxSize = mSpinBoxRedliningSize;
  mRedlining = new QgsRedlining( this, redliningUi );
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

void QgsRibbonApp::getCoordinateDisplayFormat( QgsCoordinateUtils::TargetFormat& format, QString& epsg )
{
  mCoordinateDisplayer->getCoordinateDisplayFormat( format, epsg );
}

void QgsRibbonApp::resizeEvent( QResizeEvent* event )
{
  QWidget::resizeEvent( event );
  updateWidgetPositions();
}

void QgsRibbonApp::updateWidgetPositions()
{
  QRect mapCanvasGeometry = mMapCanvas->geometry();

  //make sure +/- buttons have constant distance to upper right corner of map canvas
  QWidget* zoomLayoutWidget = dynamic_cast<QWidget*>( mZoomInOutLayout->parent() );
  if ( zoomLayoutWidget )
  {
    QRect zoomLayoutGeometry = zoomLayoutWidget->geometry();
    int distanceToRightBorder = 9;
    int distanceToTop = 20;
    zoomLayoutWidget->move( mapCanvasGeometry.width() - distanceToRightBorder - zoomLayoutGeometry.width(), distanceToTop );
  }

  //resize mLayerTreeView and mLayerTreeViewButton
  int distanceToTopBottom = 40;
  int layerTreeHeight = mapCanvasGeometry.height() - 2 * distanceToTopBottom;
  mLayerTreeViewButton->setGeometry( mLayerTreeViewButton->pos().x(), distanceToTopBottom, mLayerTreeViewButton->geometry().width(), layerTreeHeight );
  mLayerTreeView->setGeometry( mLayerTreeView->pos().x(), distanceToTopBottom, mLayerTreeView->geometry().width(), layerTreeHeight );
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

  mLayerTreeView->setModel( model );
  mLayerTreeView->setMenuProvider( new QgsRibbonLayerTreeViewMenuProvider( mLayerTreeView, this ) );

  //setup connections
  connect( mLayerTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( readProject( const QDomDocument& ) ) );

  bool otfTransformAutoEnable = QSettings().value( "/Projections/otfTransformAutoEnable", true ).toBool();
  mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );
}

void QgsRibbonApp::mousePressEvent( QMouseEvent* event )
{
  if ( event->button() == Qt::LeftButton )
  {
    mDragStartPos = event->pos();
    QWidget* dragStart = childAt( event->pos() );
    if ( dragStart )
    {
      mDragStartActionName = dragStart->property( "actionName" ).toString();
    }
  }
  QWidget::mousePressEvent( event );
}

void QgsRibbonApp::mouseMoveEvent( QMouseEvent* event )
{
  if ( event->buttons() & Qt::LeftButton )
  {
    QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
    if ( button )
    {
      int distance = ( event->pos() - mDragStartPos ).manhattanLength();
      if ( distance >= QApplication::startDragDistance() )
      {
        QIcon dragIcon = button->icon();
        performDrag( &dragIcon );
      }
    }
  }
  QWidget::mouseMoveEvent( event );
}

void QgsRibbonApp::dropEvent( QDropEvent* event )
{
  if ( !event || mDragStartActionName.isEmpty() )
  {
    return;
  }

  //get button under mouse
  QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
  if ( !button )
  {
    return;
  }


  QAction* action = findChild<QAction*>( mDragStartActionName );
  if ( !action )
  {
    return;
  }

  setActionToButton( action, button );

  //save in settings for next restart
  QSettings s;
  s.setValue( "/UI/FavoriteAction/" + button->objectName(), mDragStartActionName );
  mDragStartActionName.clear();
}

void QgsRibbonApp::dragEnterEvent( QDragEnterEvent* event )
{
  QgsRibbonButton* button = dynamic_cast<QgsRibbonButton*>( childAt( event->pos() ) );
  if ( button && button->acceptDrops() )
  {
    event->acceptProposedAction();
  }
}

void QgsRibbonApp::performDrag( const QIcon* icon )
{
  QMimeData *mimeData = new QMimeData();

  QDrag *drag = new QDrag( this );
  drag->setMimeData( mimeData );
  if ( icon )
  {
    drag->setPixmap( icon->pixmap( 32, 32 ) );
  }
  drag->exec( Qt::CopyAction );
}

void QgsRibbonApp::restoreFavoriteButton( QToolButton* button )
{
  if ( !button )
  {
    return;
  }

  QSettings s;
  QString actionName = s.value( "/UI/FavoriteAction/" + button->objectName() ).toString();
  if ( actionName.isEmpty() )
  {
    return;
  }

  QAction* action = findChild<QAction*>( actionName );
  if ( !action )
  {
    return;
  }

  setActionToButton( action, button );
}

void QgsRibbonApp::configureButtons()
{
  //My maps tab

  //mActionNew
  connect( mActionNew, SIGNAL( triggered() ), this, SLOT( fileNew() ) );
  setActionToButton( mActionNew, mNewButton );
  //mActionOpen
  connect( mActionOpen, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
  setActionToButton( mActionOpen, mOpenButton );
  //mActionSave
  connect( mActionSave, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
  setActionToButton( mActionSave, mSaveButton );
  //mActionCopy
  setActionToButton( mActionCopy, mCopyButton );
  //mActionCopyToClipboard
  //setActionToButton( mActionCopyToClipboard, mCopyToClipboardButton );
  //mActionPrint
  setActionToButton( mActionPrint, mPrintButton );
  //mActionSaveMapExtent
  connect( mActionSaveMapExtent, SIGNAL( triggered() ), this, SLOT( saveMapAsImage() ) );
  setActionToButton( mActionSaveMapExtent, mSaveMapExtentButton );
  //mActionExportKML
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

  //draw tab

  connect( mActionPin, SIGNAL( triggered( bool ) ), this, SLOT( addPinAnnotation( bool ) ) );
  setActionToButton( mActionPin, mPinButton, mMapTools.mPinAnnotation );

  //Analysis tab

  connect( mActionDistance, SIGNAL( triggered( bool ) ), this, SLOT( measure( bool ) ) );
  setActionToButton( mActionDistance, mDistanceButton, mMapTools.mMeasureDist );

  connect( mActionArea, SIGNAL( triggered( bool ) ), this, SLOT( measureArea( bool ) ) );
  setActionToButton( mActionArea, mAreaButton, mMapTools.mMeasureArea );

  connect( mActionCircle, SIGNAL( triggered( bool ) ), this, SLOT( measureCircle( bool ) ) );
  setActionToButton( mActionCircle, mMeasureCircleButton, mMapTools.mMeasureCircle );

  connect( mActionProfile, SIGNAL( triggered( bool ) ), this, SLOT( measureHeightProfile( bool ) ) );
  setActionToButton( mActionProfile, mProfileButton, mMapTools.mMeasureHeightProfile );

  connect( mActionAzimuth, SIGNAL( triggered( bool ) ), this, SLOT( measureAngle( bool ) ) );
  setActionToButton( mActionAzimuth, mAzimuthButton, mMapTools.mMeasureAngle );

  connect( mActionSlope, SIGNAL( triggered( bool ) ), this, SLOT( slope( bool ) ) );
  setActionToButton( mActionSlope, mSlopeButton, mMapTools.mSlope );

  connect( mActionHillshade, SIGNAL( triggered( bool ) ), this, SLOT( hillshade( bool ) ) );
  setActionToButton( mActionHillshade, mHillshadeButton, mMapTools.mHillshade );

  connect( mActionViewshed, SIGNAL( triggered( bool ) ), this, SLOT( viewshed( bool ) ) );
  setActionToButton( mActionViewshed, mViewshedButton, mMapTools.mViewshed );

  //mActionWPS
  setActionToButton( mActionWPS, mWPSButton );
}

void QgsRibbonApp::setActionToButton( QAction* action, QToolButton* button, QgsMapTool* tool )
{
  button->setDefaultAction( action );
  button->setProperty( "actionName", action->objectName() );
  if ( tool )
  {
    tool->setAction( action );
    button->setCheckable( true );
  }
}

void QgsRibbonApp::on_mLayerTreeViewButton_clicked()
{
  if ( !mLayerTreeView )
  {
    return;
  }

  bool visible = mLayerTreeView->isVisible();
  mLayerTreeView->setVisible( !visible );

  if ( !visible )
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/images/ribbon/layerbaum_unfolded.png" ) );
    mLayerTreeViewButton->move( mLayerTreeView->size().width() - 5, mLayerTreeViewButton->y() );
  }
  else
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/images/ribbon/layerbaum_folded.png" ) );
    mLayerTreeViewButton->move( -2, mLayerTreeViewButton->y() );
  }
}

void QgsRibbonApp::on_mZoomInButton_clicked()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomIn();
  }
}

void QgsRibbonApp::on_mZoomOutButton_clicked()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomOut();
  }
}
