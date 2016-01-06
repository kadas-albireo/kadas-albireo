/***************************************************************************
  qgsclassicapp.cpp  -  description
  -------------------

           begin                : Sat Jun 22 2002
           copyright            : (C) 2002 by Gary E.Sherman
           email                : sherman at mrcc.com
           Romans 3:23=>Romans 6:23=>Romans 10:9,10=>Romans 12
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsclassicapp.h"
#include "qgsadvanceddigitizingdockwidget.h"
#include "qgsapplayertreeviewmenuprovider.h"
#include "qgsapplication.h"
#include "qgsattributeaction.h"
#include "qgscolorbuttonv2.h"
#include "qgsclipboard.h"
#include "qgscomposer.h"
#include "qgscustomization.h"
#include "qgscustomlayerorderwidget.h"
#include "qgsdoublespinbox.h"
#include "qgsdecorationcopyright.h"
#include "qgsdecorationnortharrow.h"
#include "qgsdecorationscalebar.h"
#include "qgsdecorationgrid.h"
#include "qgslayertreemodel.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgslayertreeregistrybridge.h"
#include "qgslayertreeutils.h"
#include "qgslayertreeview.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayeractionregistry.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptip.h"
#include "qgsmaptoolpinlabels.h"
#include "qgsmaptoolshowhidelabels.h"
#include "qgsmaptoolrotatelabel.h"
#include "qgsmaptoolrotatepointsymbols.h"
#include "qgsproject.h"
#include "qgsredlining.h"
#include "qgsscalecombobox.h"
#include "qgsundowidget.h"
#include "qgsvectordataprovider.h"
#include "qgsvisibilitypresets.h"
#include "geos_c.h"

#include <QProgressBar>
#include <QToolButton>
#include <QWhatsThis>

QgsClassicApp::QgsClassicApp( QSplashScreen *splash, bool restorePlugins, QWidget *parent, Qt::WindowFlags fl )
    : QgisApp( splash, parent, fl )
    , mScaleLabel( 0 )
    , mScaleEdit( 0 )
    , mScaleEditValidator( 0 )
    , mCoordsLabel( 0 )
    , mCoordsEdit( 0 )
    , mCoordsEditValidator( 0 )
    , mRotationLabel( 0 )
    , mRotationEdit( 0 )
    , mRotationEditValidator( 0 )
    , mProgressBar( 0 )
    , mRenderSuppressionCBox( 0 )
    , mOnTheFlyProjectionStatusLabel( 0 )
    , mOnTheFlyProjectionStatusButton( 0 )
    , mMessageButton( 0 )
    , mFeatureActionMenu( 0 )
    , mPopupMenu( 0 )
    , mDatabaseMenu( 0 )
    , mWebMenu( 0 )
    , mToolPopupOverviews( 0 )
    , mToolPopupDisplay( 0 )
{
  setupUi( this );

  QWidget *centralWidget = this->centralWidget();
  QGridLayout *centralLayout = new QGridLayout( centralWidget );
  centralWidget->setLayout( centralLayout );
  centralLayout->setContentsMargins( 0, 0, 0, 0 );

  mMapCanvas = new QgsMapCanvas( centralWidget );
  centralLayout->addWidget( mMapCanvas, 0, 0, 2, 1 );
  mLayerTreeView = new QgsLayerTreeView( this );

  mPanelMenu = new QMenu( tr( "Panels" ), this );
  mPanelMenu->setObjectName( "mPanelMenu" );
  mToolbarMenu = new QMenu( tr( "Toolbars" ), this );
  mToolbarMenu->setObjectName( "mToolbarMenu" );

  // a bar to warn the user with non-blocking messages
  mInfoBar = new QgsMessageBar( centralWidget );
  mInfoBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
  centralLayout->addWidget( mInfoBar, 0, 0, 1, 1 );

  initLayerTreeView();
  setupMenus();

  // Base class init
  init( restorePlugins );

  createActions();
  createActionGroups();
  createToolBars();
  createStatusBar();
  setToolActions();

  connect( mMapCanvas, SIGNAL( zoomLastStatusChanged( bool ) ), mActionZoomLast, SLOT( setEnabled( bool ) ) );
  connect( mMapCanvas, SIGNAL( zoomNextStatusChanged( bool ) ), mActionZoomNext, SLOT( setEnabled( bool ) ) );
  connect( mMapCanvas, SIGNAL( xyCoordinates( const QgsPoint & ) ), this, SLOT( showMouseCoordinate( const QgsPoint & ) ) );
  connect( mMapCanvas, SIGNAL( scaleChanged( double ) ), this, SLOT( showScale( double ) ) );
  connect( mMapCanvas, SIGNAL( extentsChanged() ), this, SLOT( showExtents() ) );
  connect( mMapCanvas, SIGNAL( rotationChanged( double ) ), this, SLOT( showRotation() ) );
  connect( mMapCanvas, SIGNAL( destinationCrsChanged() ), this, SLOT( updateCRSStatusBar() ) );
  connect( mMapCanvas, SIGNAL( hasCrsTransformEnabledChanged( bool ) ), this, SLOT( updateCRSStatusBar() ) );
  connect( mRenderSuppressionCBox, SIGNAL( toggled( bool ) ), mMapCanvas, SLOT( setRenderFlag( bool ) ) );
  connect( mActionPreviewModeOff, SIGNAL( triggered() ), this, SLOT( disablePreviewMode() ) );
  connect( mActionPreviewModeGrayscale, SIGNAL( triggered() ), this, SLOT( activateGrayscalePreview() ) );
  connect( mActionPreviewModeMono, SIGNAL( triggered() ), this, SLOT( activateMonoPreview() ) );
  connect( mActionPreviewProtanope, SIGNAL( triggered() ), this, SLOT( activateProtanopePreview() ) );
  connect( mActionPreviewDeuteranope, SIGNAL( triggered() ), this, SLOT( activateDeuteranopePreview() ) );
  connect( this, SIGNAL( projectScalesChanged( QStringList ) ), mScaleEdit, SLOT( updateScales( QStringList ) ) );
  connect( mMessageButton, SIGNAL( toggled( bool ) ), mLogDock, SLOT( setVisible( bool ) ) );
  connect( mLogDock, SIGNAL( visibilityChanged( bool ) ), mMessageButton, SLOT( setChecked( bool ) ) );
  connect( QgsMapLayerActionRegistry::instance(), SIGNAL( changed() ), this, SLOT( refreshActionFeatureAction() ) );
  connect( mUndoWidget, SIGNAL( undoStackChanged() ), this, SLOT( updateUndoActions() ) );
  connect( mInternalClipboard, SIGNAL( dataChanged() ), this, SLOT( clipboardDataChanged() ) );

  // Do main window customization - after window state has been restored, before the window is shown
  QgsCustomization::instance()->updateMainWindow( mToolbarMenu );

  //Redlining
  QgsRedlining::RedliningUi redliningUi;
  redliningUi.buttonNewObject = new QToolButton();
  redliningUi.colorButtonFillColor = new QgsColorButtonV2();
  redliningUi.colorButtonOutlineColor = new QgsColorButtonV2();
  redliningUi.comboFillStyle = new QComboBox();
  redliningUi.comboOutlineStyle = new QComboBox();
  redliningUi.spinBoxSize = new QSpinBox();
  mRedlining = new QgsRedlining( this, redliningUi );

  mRedliningToolBar->addWidget( redliningUi.buttonNewObject );
  mRedliningToolBar->addWidget( new QLabel( tr( "Border/Size:" ) ) );
  mRedliningToolBar->addWidget( redliningUi.spinBoxSize );
  mRedliningToolBar->addWidget( new QLabel( tr( "Outline:" ) ) );
  mRedliningToolBar->addWidget( redliningUi.colorButtonOutlineColor );
  mRedliningToolBar->addWidget( redliningUi.comboOutlineStyle );
  mRedliningToolBar->addWidget( new QLabel( tr( "Fill:" ) ) );
  mRedliningToolBar->addWidget( new QgsColorButtonV2() );
  mRedliningToolBar->addWidget( redliningUi.comboFillStyle );
}

QgsClassicApp::QgsClassicApp()
    : QgisApp()
    , mScaleLabel( 0 )
    , mScaleEdit( 0 )
    , mScaleEditValidator( 0 )
    , mCoordsLabel( 0 )
    , mCoordsEdit( 0 )
    , mCoordsEditValidator( 0 )
    , mRotationLabel( 0 )
    , mRotationEdit( 0 )
    , mRotationEditValidator( 0 )
    , mProgressBar( 0 )
    , mRenderSuppressionCBox( 0 )
    , mToggleExtentsViewButton( 0 )
    , mOnTheFlyProjectionStatusLabel( 0 )
    , mOnTheFlyProjectionStatusButton( 0 )
    , mMessageButton( 0 )
    , mFeatureActionMenu( 0 )
    , mPopupMenu( 0 )
    , mDatabaseMenu( 0 )
    , mWebMenu( 0 )
    , mToolPopupOverviews( 0 )
    , mToolPopupDisplay( 0 )
{
  setupUi( this );
  mMapCanvas = new QgsMapCanvas();
  mLayerTreeView = new QgsLayerTreeView( this );
  mInfoBar = new QgsMessageBar( centralWidget() );
  mMapCanvas->freeze();
  mUndoWidget = new QgsUndoWidget( NULL, mMapCanvas );
}

QgsClassicApp::~QgsClassicApp()
{
  destroy();
}

QToolBar *QgsClassicApp::addToolBar( QString name )
{
  QToolBar *toolBar = QMainWindow::addToolBar( name );
  // add to the Toolbar submenu
  mToolbarMenu->addAction( toolBar->toggleViewAction() );
  return toolBar;
}

void QgsClassicApp::addToolBar( QToolBar* toolBar, Qt::ToolBarArea area )
{
  QMainWindow::addToolBar( area, toolBar );
  // add to the Toolbar submenu
  mToolbarMenu->addAction( toolBar->toggleViewAction() );
}

void QgsClassicApp::setTheme( QString theThemeName )
{
  /*****************************************************************
  // Init the toolbar icons by setting the icon for each action.
  // All toolbar/menu items must be a QAction in order for this
  // to work.
  //
  // When new toolbar/menu QAction objects are added to the interface,
  // add an entry below to set the icon
  //
  // PNG names must match those defined for the default theme. The
  // default theme is installed in <prefix>/share/qgis/themes/default.
  //
  // New core themes can be added by creating a subdirectory under src/themes
  // and modifying the appropriate CMakeLists.txt files. User contributed themes
  // will be installed directly into <prefix>/share/qgis/themes/<themedir>.
  //
  // Themes can be selected from the preferences dialog. The dialog parses
  // the themes directory and builds a list of themes (ie subdirectories)
  // for the user to choose from.
  //
  */
  QgsApplication::setThemeName( theThemeName );
  //QgsDebugMsg("Setting theme to \n" + theThemeName);
  mActionNewProject->setIcon( QgsApplication::getThemeIcon( "/mActionFileNew.svg" ) );
  mActionOpenProject->setIcon( QgsApplication::getThemeIcon( "/mActionFileOpen.svg" ) );
  mActionSaveProject->setIcon( QgsApplication::getThemeIcon( "/mActionFileSave.svg" ) );
  mActionSaveProjectAs->setIcon( QgsApplication::getThemeIcon( "/mActionFileSaveAs.svg" ) );
  mActionNewPrintComposer->setIcon( QgsApplication::getThemeIcon( "/mActionNewComposer.svg" ) );
  mActionShowComposerManager->setIcon( QgsApplication::getThemeIcon( "/mActionComposerManager.svg" ) );
  mActionSaveMapAsImage->setIcon( QgsApplication::getThemeIcon( "/mActionSaveMapAsImage.png" ) );
  mActionExit->setIcon( QgsApplication::getThemeIcon( "/mActionFileExit.png" ) );
  mActionAddOgrLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddOgrLayer.svg" ) );
  mActionAddRasterLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddRasterLayer.svg" ) );
#ifdef HAVE_POSTGRESQL
  mActionAddPgLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddPostgisLayer.svg" ) );
#endif
  mActionNewSpatiaLiteLayer->setIcon( QgsApplication::getThemeIcon( "/mActionNewSpatiaLiteLayer.svg" ) );
  mActionAddSpatiaLiteLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddSpatiaLiteLayer.svg" ) );
#ifdef HAVE_MSSQL
  mActionAddMssqlLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddMssqlLayer.svg" ) );
#endif
#ifdef HAVE_ORACLE
  mActionAddOracleLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddOracleLayer.svg" ) );
#endif
  mActionRemoveLayer->setIcon( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ) );
  mActionDuplicateLayer->setIcon( QgsApplication::getThemeIcon( "/mActionDuplicateLayer.svg" ) );
  mActionSetLayerCRS->setIcon( QgsApplication::getThemeIcon( "/mActionSetLayerCRS.png" ) );
  mActionSetProjectCRSFromLayer->setIcon( QgsApplication::getThemeIcon( "/mActionSetProjectCRSFromLayer.png" ) );
  mActionNewVectorLayer->setIcon( QgsApplication::getThemeIcon( "/mActionNewVectorLayer.svg" ) );
  mActionNewMemoryLayer->setIcon( QgsApplication::getThemeIcon( "/mActionCreateMemory.png" ) );
  mActionAddAllToOverview->setIcon( QgsApplication::getThemeIcon( "/mActionAddAllToOverview.svg" ) );
  mActionHideAllLayers->setIcon( QgsApplication::getThemeIcon( "/mActionHideAllLayers.png" ) );
  mActionShowAllLayers->setIcon( QgsApplication::getThemeIcon( "/mActionShowAllLayers.png" ) );
  mActionHideSelectedLayers->setIcon( QgsApplication::getThemeIcon( "/mActionHideSelectedLayers.png" ) );
  mActionShowSelectedLayers->setIcon( QgsApplication::getThemeIcon( "/mActionShowSelectedLayers.png" ) );
  mActionRemoveAllFromOverview->setIcon( QgsApplication::getThemeIcon( "/mActionRemoveAllFromOverview.svg" ) );
  mActionToggleFullScreen->setIcon( QgsApplication::getThemeIcon( "/mActionToggleFullScreen.png" ) );
  mActionProjectProperties->setIcon( QgsApplication::getThemeIcon( "/mActionProjectProperties.png" ) );
  mActionManagePlugins->setIcon( QgsApplication::getThemeIcon( "/mActionShowPluginManager.svg" ) );
  mActionShowPythonDialog->setIcon( QgsApplication::getThemeIcon( "console/iconRunConsole.png" ) );
  mActionCheckQgisVersion->setIcon( QgsApplication::getThemeIcon( "/mActionCheckQgisVersion.png" ) );
  mActionOptions->setIcon( QgsApplication::getThemeIcon( "/mActionOptions.svg" ) );
  mActionConfigureShortcuts->setIcon( QgsApplication::getThemeIcon( "/mActionOptions.svg" ) );
  mActionCustomization->setIcon( QgsApplication::getThemeIcon( "/mActionOptions.svg" ) );
  mActionHelpContents->setIcon( QgsApplication::getThemeIcon( "/mActionHelpContents.svg" ) );
  mActionLocalHistogramStretch->setIcon( QgsApplication::getThemeIcon( "/mActionLocalHistogramStretch.png" ) );
  mActionFullHistogramStretch->setIcon( QgsApplication::getThemeIcon( "/mActionFullHistogramStretch.png" ) );
  mActionIncreaseBrightness->setIcon( QgsApplication::getThemeIcon( "/mActionIncreaseBrightness.svg" ) );
  mActionDecreaseBrightness->setIcon( QgsApplication::getThemeIcon( "/mActionDecreaseBrightness.svg" ) );
  mActionIncreaseContrast->setIcon( QgsApplication::getThemeIcon( "/mActionIncreaseContrast.svg" ) );
  mActionDecreaseContrast->setIcon( QgsApplication::getThemeIcon( "/mActionDecreaseContrast.svg" ) );
  mActionZoomActualSize->setIcon( QgsApplication::getThemeIcon( "/mActionZoomNative.png" ) );
  mActionQgisHomePage->setIcon( QgsApplication::getThemeIcon( "/mActionQgisHomePage.png" ) );
  mActionAbout->setIcon( QgsApplication::getThemeIcon( "/mActionHelpAbout.png" ) );
  mActionSponsors->setIcon( QgsApplication::getThemeIcon( "/mActionHelpSponsors.png" ) );
  mActionDraw->setIcon( QgsApplication::getThemeIcon( "/mActionDraw.svg" ) );
  mActionToggleEditing->setIcon( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ) );
  mActionSaveLayerEdits->setIcon( QgsApplication::getThemeIcon( "/mActionSaveAllEdits.svg" ) );
  mActionAllEdits->setIcon( QgsApplication::getThemeIcon( "/mActionAllEdits.svg" ) );
  mActionSaveEdits->setIcon( QgsApplication::getThemeIcon( "/mActionSaveEdits.svg" ) );
  mActionSaveAllEdits->setIcon( QgsApplication::getThemeIcon( "/mActionSaveAllEdits.svg" ) );
  mActionRollbackEdits->setIcon( QgsApplication::getThemeIcon( "/mActionRollbackEdits.svg" ) );
  mActionRollbackAllEdits->setIcon( QgsApplication::getThemeIcon( "/mActionRollbackAllEdits.svg" ) );
  mActionCancelEdits->setIcon( QgsApplication::getThemeIcon( "/mActionCancelEdits.svg" ) );
  mActionCancelAllEdits->setIcon( QgsApplication::getThemeIcon( "/mActionCancelAllEdits.svg" ) );
  mActionCutFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionEditCut.png" ) );
  mActionCopyFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionEditCopy.png" ) );
  mActionPasteFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionEditPaste.png" ) );
  mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionCapturePoint.png" ) );
  mActionMoveFeature->setIcon( QgsApplication::getThemeIcon( "/mActionMoveFeature.png" ) );
  mActionRotateFeature->setIcon( QgsApplication::getThemeIcon( "/mActionRotateFeature.png" ) );
  mActionReshapeFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionReshape.png" ) );
  mActionSplitFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionSplitFeatures.svg" ) );
  mActionSplitParts->setIcon( QgsApplication::getThemeIcon( "/mActionSplitParts.svg" ) );
  mActionDeleteSelected->setIcon( QgsApplication::getThemeIcon( "/mActionDeleteSelected.svg" ) );
  mActionNodeTool->setIcon( QgsApplication::getThemeIcon( "/mActionNodeTool.png" ) );
  mActionSimplifyFeature->setIcon( QgsApplication::getThemeIcon( "/mActionSimplify.png" ) );
  mActionUndo->setIcon( QgsApplication::getThemeIcon( "/mActionUndo.png" ) );
  mActionRedo->setIcon( QgsApplication::getThemeIcon( "/mActionRedo.png" ) );
  mActionAddRing->setIcon( QgsApplication::getThemeIcon( "/mActionAddRing.png" ) );
  mActionFillRing->setIcon( QgsApplication::getThemeIcon( "/mActionFillRing.svg" ) );
  mActionAddPart->setIcon( QgsApplication::getThemeIcon( "/mActionAddPart.png" ) );
  mActionDeleteRing->setIcon( QgsApplication::getThemeIcon( "/mActionDeleteRing.png" ) );
  mActionDeletePart->setIcon( QgsApplication::getThemeIcon( "/mActionDeletePart.png" ) );
  mActionMergeFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionMergeFeatures.png" ) );
  mActionOffsetCurve->setIcon( QgsApplication::getThemeIcon( "/mActionOffsetCurve.png" ) );
  mActionMergeFeatureAttributes->setIcon( QgsApplication::getThemeIcon( "/mActionMergeFeatureAttributes.png" ) );
  mActionRotatePointSymbols->setIcon( QgsApplication::getThemeIcon( "mActionRotatePointSymbols.png" ) );
  mActionZoomIn->setIcon( QgsApplication::getThemeIcon( "/mActionZoomIn.svg" ) );
  mActionZoomOut->setIcon( QgsApplication::getThemeIcon( "/mActionZoomOut.svg" ) );
  mActionZoomFullExtent->setIcon( QgsApplication::getThemeIcon( "/mActionZoomFullExtent.svg" ) );
  mActionZoomToSelected->setIcon( QgsApplication::getThemeIcon( "/mActionZoomToSelected.svg" ) );
  mActionShowRasterCalculator->setIcon( QgsApplication::getThemeIcon( "/mActionShowRasterCalculator.png" ) );
  mActionPan->setIcon( QgsApplication::getThemeIcon( "/mActionPan.svg" ) );
  mActionPanToSelected->setIcon( QgsApplication::getThemeIcon( "/mActionPanToSelected.svg" ) );
  mActionZoomLast->setIcon( QgsApplication::getThemeIcon( "/mActionZoomLast.svg" ) );
  mActionZoomNext->setIcon( QgsApplication::getThemeIcon( "/mActionZoomNext.svg" ) );
  mActionZoomToLayer->setIcon( QgsApplication::getThemeIcon( "/mActionZoomToLayer.svg" ) );
  mActionZoomActualSize->setIcon( QgsApplication::getThemeIcon( "/mActionZoomActual.svg" ) );
  mActionIdentify->setIcon( QgsApplication::getThemeIcon( "/mActionIdentify.svg" ) );
  mActionFeatureAction->setIcon( QgsApplication::getThemeIcon( "/mAction.svg" ) );
  mActionSelectFeatures->setIcon( QgsApplication::getThemeIcon( "/mActionSelectRectangle.svg" ) );
  mActionSelectPolygon->setIcon( QgsApplication::getThemeIcon( "/mActionSelectPolygon.svg" ) );
  mActionSelectFreehand->setIcon( QgsApplication::getThemeIcon( "/mActionSelectFreehand.svg" ) );
  mActionSelectRadius->setIcon( QgsApplication::getThemeIcon( "/mActionSelectRadius.svg" ) );
  mActionDeselectAll->setIcon( QgsApplication::getThemeIcon( "/mActionDeselectAll.svg" ) );
  mActionSelectByExpression->setIcon( QgsApplication::getThemeIcon( "/mIconExpressionSelect.svg" ) );
  mActionOpenTable->setIcon( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ) );
  mActionOpenFieldCalc->setIcon( QgsApplication::getThemeIcon( "/mActionCalculateField.png" ) );
  mActionMeasure->setIcon( QgsApplication::getThemeIcon( "/mActionMeasure.png" ) );
  mActionMeasureArea->setIcon( QgsApplication::getThemeIcon( "/mActionMeasureArea.png" ) );
  mActionMeasureAngle->setIcon( QgsApplication::getThemeIcon( "/mActionMeasureAngle.png" ) );
  mActionMeasureCircle->setIcon( QgsApplication::getThemeIcon( "/mActionMeasureCircle.png" ) );
  mActionMeasureHeightProfile->setIcon( QgsApplication::getThemeIcon( "/mActionMeasureHeightProfile.png" ) );
  mActionMapTips->setIcon( QgsApplication::getThemeIcon( "/mActionMapTips.png" ) );
  mActionShowBookmarks->setIcon( QgsApplication::getThemeIcon( "/mActionShowBookmarks.png" ) );
  mActionNewBookmark->setIcon( QgsApplication::getThemeIcon( "/mActionNewBookmark.png" ) );
  mActionCustomProjection->setIcon( QgsApplication::getThemeIcon( "/mActionCustomProjection.svg" ) );
  mActionAddWmsLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddWmsLayer.svg" ) );
  mActionAddWcsLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddWcsLayer.svg" ) );
  mActionAddWfsLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddWfsLayer.svg" ) );
  mActionAddAfsLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddAfsLayer.svg" ) );
  mActionAddAmsLayer->setIcon( QgsApplication::getThemeIcon( "/mActionAddAmsLayer.svg" ) );
  mActionAddToOverview->setIcon( QgsApplication::getThemeIcon( "/mActionInOverview.svg" ) );
  mActionFormAnnotation->setIcon( QgsApplication::getThemeIcon( "/mActionFormAnnotation.png" ) );
  mActionHtmlAnnotation->setIcon( QgsApplication::getThemeIcon( "/mActionFormAnnotation.png" ) );
  mActionTextAnnotation->setIcon( QgsApplication::getThemeIcon( "/mActionTextAnnotation.png" ) );
  mActionLabeling->setIcon( QgsApplication::getThemeIcon( "/mActionLabeling.png" ) );
  mActionShowPinnedLabels->setIcon( QgsApplication::getThemeIcon( "/mActionShowPinnedLabels.svg" ) );
  mActionPinLabels->setIcon( QgsApplication::getThemeIcon( "/mActionPinLabels.svg" ) );
  mActionShowHideLabels->setIcon( QgsApplication::getThemeIcon( "/mActionShowHideLabels.svg" ) );
  mActionMoveLabel->setIcon( QgsApplication::getThemeIcon( "/mActionMoveLabel.png" ) );
  mActionRotateLabel->setIcon( QgsApplication::getThemeIcon( "/mActionRotateLabel.svg" ) );
  mActionChangeLabelProperties->setIcon( QgsApplication::getThemeIcon( "/mActionChangeLabelProperties.png" ) );
  mActionDecorationCopyright->setIcon( QgsApplication::getThemeIcon( "/copyright_label.png" ) );
  mActionDecorationNorthArrow->setIcon( QgsApplication::getThemeIcon( "/north_arrow.png" ) );
  mActionDecorationScaleBar->setIcon( QgsApplication::getThemeIcon( "/scale_bar.png" ) );
  mActionDecorationGrid->setIcon( QgsApplication::getThemeIcon( "/transformed.png" ) );

  //change themes of all composers
  QSet<QgsComposer*>::iterator composerIt = mPrintComposers.begin();
  for ( ; composerIt != mPrintComposers.end(); ++composerIt )
  {
    ( *composerIt )->setupTheme();
  }

  emit currentThemeChanged( theThemeName );
}

void QgsClassicApp::getCoordinateDisplayFormat( QgsCoordinateUtils::TargetFormat& format, QString& epsg )
{
  format = QgsCoordinateUtils::EPSG;
  epsg = mapCanvas()->mapSettings().destinationCrs().authid();
}

void QgsClassicApp::createActions()
{
  mActionPluginSeparator1 = 0;  // plugin list separator will be created when the first plugin is loaded
  mActionPluginSeparator2 = 0;  // python separator will be created only if python is found
  mActionRasterSeparator = 0;   // raster plugins list separator will be created when the first plugin is loaded

  // Project Menu Items

  connect( mActionNewProject, SIGNAL( triggered() ), this, SLOT( fileNew() ) );
  connect( mActionNewBlankProject, SIGNAL( triggered() ), this, SLOT( fileNewBlank() ) );
  connect( mActionOpenProject, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
  connect( mActionSaveProject, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
  connect( mActionSaveProjectAs, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );
  connect( mActionSaveMapAsImage, SIGNAL( triggered() ), this, SLOT( saveMapAsImage() ) );
  connect( mActionSaveMapToClipboard, SIGNAL( triggered() ), this, SLOT( saveMapToClipboard( ) ) );
  connect( mActionNewPrintComposer, SIGNAL( triggered() ), this, SLOT( newPrintComposer() ) );
  connect( mActionShowComposerManager, SIGNAL( triggered() ), this, SLOT( showComposerManager() ) );
  connect( mActionExit, SIGNAL( triggered() ), this, SLOT( fileExit() ) );
  connect( mActionDxfExport, SIGNAL( triggered() ), this, SLOT( dxfExport() ) );
  connect( mActionKMLExport, SIGNAL( triggered() ), this, SLOT( kmlExport() ) );

  // Edit Menu Items

  connect( mActionUndo, SIGNAL( triggered() ), mUndoWidget, SLOT( undo() ) );
  connect( mActionRedo, SIGNAL( triggered() ), mUndoWidget, SLOT( redo() ) );
  connect( mActionCutFeatures, SIGNAL( triggered() ), this, SLOT( editCut() ) );
  connect( mActionCopyFeatures, SIGNAL( triggered() ), this, SLOT( editCopy() ) );
  connect( mActionPasteFeatures, SIGNAL( triggered() ), this, SLOT( editPaste() ) );
  connect( mActionPasteAsNewVector, SIGNAL( triggered() ), this, SLOT( pasteAsNewVector() ) );
  connect( mActionPasteAsNewMemoryVector, SIGNAL( triggered() ), this, SLOT( pasteAsNewMemoryVector() ) );
  connect( mActionCopyStyle, SIGNAL( triggered() ), this, SLOT( copyStyle() ) );
  connect( mActionPasteStyle, SIGNAL( triggered() ), this, SLOT( pasteStyle() ) );
  connect( mActionAddFeature, SIGNAL( triggered() ), this, SLOT( addFeature() ) );
  connect( mActionCircularStringCurvePoint, SIGNAL( triggered() ), this, SLOT( circularStringCurvePoint() ) );
  connect( mActionCircularStringRadius, SIGNAL( triggered() ), this, SLOT( circularStringRadius() ) );
  connect( mActionMoveFeature, SIGNAL( triggered() ), this, SLOT( moveFeature() ) );
  connect( mActionRotateFeature, SIGNAL( triggered() ), this, SLOT( rotateFeature() ) );

  connect( mActionReshapeFeatures, SIGNAL( triggered() ), this, SLOT( reshapeFeatures() ) );
  connect( mActionSplitFeatures, SIGNAL( triggered() ), this, SLOT( splitFeatures() ) );
  connect( mActionSplitParts, SIGNAL( triggered() ), this, SLOT( splitParts() ) );
  connect( mActionDeleteSelected, SIGNAL( triggered() ), this, SLOT( deleteSelected() ) );
  connect( mActionAddRing, SIGNAL( triggered() ), this, SLOT( addRing() ) );
  connect( mActionFillRing, SIGNAL( triggered() ), this, SLOT( fillRing() ) );
  connect( mActionAddPart, SIGNAL( triggered() ), this, SLOT( addPart() ) );
  connect( mActionSimplifyFeature, SIGNAL( triggered() ), this, SLOT( simplifyFeature() ) );
  connect( mActionDeleteRing, SIGNAL( triggered() ), this, SLOT( deleteRing() ) );
  connect( mActionDeletePart, SIGNAL( triggered() ), this, SLOT( deletePart() ) );
  connect( mActionMergeFeatures, SIGNAL( triggered() ), this, SLOT( mergeSelectedFeatures() ) );
  connect( mActionMergeFeatureAttributes, SIGNAL( triggered() ), this, SLOT( mergeAttributesOfSelectedFeatures() ) );
  connect( mActionNodeTool, SIGNAL( triggered() ), this, SLOT( nodeTool() ) );
  connect( mActionRotatePointSymbols, SIGNAL( triggered() ), this, SLOT( rotatePointSymbols() ) );
  connect( mActionSnappingOptions, SIGNAL( triggered() ), this, SLOT( snappingOptions() ) );
  connect( mActionOffsetCurve, SIGNAL( triggered() ), this, SLOT( offsetCurve() ) );

  // View Menu Items

  connect( mActionPan, SIGNAL( triggered() ), this, SLOT( pan() ) );
  connect( mActionPanToSelected, SIGNAL( triggered() ), this, SLOT( panToSelected() ) );
  connect( mActionZoomIn, SIGNAL( triggered() ), this, SLOT( zoomIn() ) );
  connect( mActionZoomOut, SIGNAL( triggered() ), this, SLOT( zoomOut() ) );
  connect( mActionSelectFeatures, SIGNAL( triggered() ), this, SLOT( selectFeatures() ) );
  connect( mActionSelectPolygon, SIGNAL( triggered() ), this, SLOT( selectByPolygon() ) );
  connect( mActionSelectFreehand, SIGNAL( triggered() ), this, SLOT( selectByFreehand() ) );
  connect( mActionSelectRadius, SIGNAL( triggered() ), this, SLOT( selectByRadius() ) );
  connect( mActionDeselectAll, SIGNAL( triggered() ), this, SLOT( deselectAll() ) );
  connect( mActionSelectByExpression, SIGNAL( triggered() ), this, SLOT( selectByExpression() ) );
  connect( mActionIdentify, SIGNAL( triggered() ), this, SLOT( identify() ) );
  connect( mActionFeatureAction, SIGNAL( triggered() ), this, SLOT( doFeatureAction() ) );
  connect( mActionMeasure, SIGNAL( triggered( bool ) ), this, SLOT( measure( bool ) ) );
  connect( mActionMeasureArea, SIGNAL( triggered( bool ) ), this, SLOT( measureArea( bool ) ) );
  connect( mActionMeasureCircle, SIGNAL( triggered( bool ) ), this, SLOT( measureCircle( bool ) ) );
  connect( mActionMeasureHeightProfile, SIGNAL( triggered( bool ) ), this, SLOT( measureHeightProfile( bool ) ) );
  connect( mActionMeasureAngle, SIGNAL( triggered( bool ) ), this, SLOT( measureAngle( bool ) ) );
  connect( mActionZoomFullExtent, SIGNAL( triggered() ), this, SLOT( zoomFull() ) );
  connect( mActionZoomToLayer, SIGNAL( triggered() ), this, SLOT( zoomToLayerExtent() ) );
  connect( mActionZoomToSelected, SIGNAL( triggered() ), this, SLOT( zoomToSelected() ) );
  connect( mActionZoomLast, SIGNAL( triggered() ), this, SLOT( zoomToPrevious() ) );
  connect( mActionZoomNext, SIGNAL( triggered() ), this, SLOT( zoomToNext() ) );
  connect( mActionZoomActualSize, SIGNAL( triggered() ), this, SLOT( zoomActualSize() ) );
  connect( mActionMapTips, SIGNAL( triggered() ), this, SLOT( toggleMapTips() ) );
  connect( mActionNewBookmark, SIGNAL( triggered() ), this, SLOT( newBookmark() ) );
  connect( mActionShowBookmarks, SIGNAL( triggered() ), this, SLOT( showBookmarks() ) );
  connect( mActionDraw, SIGNAL( triggered() ), this, SLOT( refreshMapCanvas() ) );
  connect( mActionTextAnnotation, SIGNAL( triggered( bool ) ), this, SLOT( addTextAnnotation( bool ) ) );
  connect( mActionFormAnnotation, SIGNAL( triggered( bool ) ), this, SLOT( addFormAnnotation( bool ) ) );
  connect( mActionHtmlAnnotation, SIGNAL( triggered( bool ) ), this, SLOT( addHtmlAnnotation( bool ) ) );
  connect( mActionSvgAnnotation, SIGNAL( triggered( bool ) ), this, SLOT( addSvgAnnotation( bool ) ) );
  connect( mActionPinAnnotation, SIGNAL( triggered( bool ) ), this, SLOT( addPinAnnotation( bool ) ) );
  connect( mActionLabeling, SIGNAL( triggered() ), this, SLOT( labeling() ) );

  // Layer Menu Items

  connect( mActionNewVectorLayer, SIGNAL( triggered() ), this, SLOT( newVectorLayer() ) );
  connect( mActionNewSpatiaLiteLayer, SIGNAL( triggered() ), this, SLOT( newSpatialiteLayer() ) );
  connect( mActionNewMemoryLayer, SIGNAL( triggered() ), this, SLOT( newMemoryLayer() ) );
  connect( mActionShowRasterCalculator, SIGNAL( triggered() ), this, SLOT( showRasterCalculator() ) );
  connect( mActionEmbedLayers, SIGNAL( triggered() ), this, SLOT( embedLayers() ) );
  connect( mActionAddLayerDefinition, SIGNAL( triggered() ), this, SLOT( addLayerDefinition() ) );
  connect( mActionAddOgrLayer, SIGNAL( triggered() ), this, SLOT( addVectorLayer() ) );
  connect( mActionAddRasterLayer, SIGNAL( triggered() ), this, SLOT( addRasterLayer() ) );
  connect( mActionAddPgLayer, SIGNAL( triggered() ), this, SLOT( addDatabaseLayer() ) );
  connect( mActionAddSpatiaLiteLayer, SIGNAL( triggered() ), this, SLOT( addSpatiaLiteLayer() ) );
  connect( mActionAddMssqlLayer, SIGNAL( triggered() ), this, SLOT( addMssqlLayer() ) );
  connect( mActionAddOracleLayer, SIGNAL( triggered() ), this, SLOT( addOracleLayer() ) );
  connect( mActionAddWmsLayer, SIGNAL( triggered() ), this, SLOT( addWmsLayer() ) );
  connect( mActionAddWcsLayer, SIGNAL( triggered() ), this, SLOT( addWcsLayer() ) );
  connect( mActionAddWfsLayer, SIGNAL( triggered() ), this, SLOT( addWfsLayer() ) );
  connect( mActionAddAfsLayer, SIGNAL( triggered() ), this, SLOT( addAfsLayer() ) );
  connect( mActionAddAmsLayer, SIGNAL( triggered() ), this, SLOT( addAmsLayer() ) );
  connect( mActionAddDelimitedText, SIGNAL( triggered() ), this, SLOT( addDelimitedTextLayer() ) );
  connect( mActionOpenTable, SIGNAL( triggered() ), this, SLOT( attributeTable() ) );
  connect( mActionOpenFieldCalc, SIGNAL( triggered() ), this, SLOT( fieldCalculator() ) );
  connect( mActionToggleEditing, SIGNAL( triggered() ), this, SLOT( toggleEditing() ) );
  connect( mActionSaveLayerEdits, SIGNAL( triggered() ), this, SLOT( saveActiveLayerEdits() ) );
  connect( mActionSaveEdits, SIGNAL( triggered() ), this, SLOT( saveEdits() ) );
  connect( mActionSaveAllEdits, SIGNAL( triggered() ), this, SLOT( saveAllEdits() ) );
  connect( mActionRollbackEdits, SIGNAL( triggered() ), this, SLOT( rollbackEdits() ) );
  connect( mActionRollbackAllEdits, SIGNAL( triggered() ), this, SLOT( rollbackAllEdits() ) );
  connect( mActionCancelEdits, SIGNAL( triggered() ), this, SLOT( cancelEdits() ) );
  connect( mActionCancelAllEdits, SIGNAL( triggered() ), this, SLOT( cancelAllEdits() ) );
  connect( mActionLayerSaveAs, SIGNAL( triggered() ), this, SLOT( saveAsFile() ) );
  connect( mActionSaveLayerDefinition, SIGNAL( triggered() ), this, SLOT( saveAsLayerDefinition() ) );
  connect( mActionRemoveLayer, SIGNAL( triggered() ), this, SLOT( removeLayer() ) );
  connect( mActionDuplicateLayer, SIGNAL( triggered() ), this, SLOT( duplicateLayers() ) );
  connect( mActionSetLayerScaleVisibility, SIGNAL( triggered() ), this, SLOT( setLayerScaleVisibility() ) );
  connect( mActionSetLayerCRS, SIGNAL( triggered() ), this, SLOT( setLayerCRS() ) );
  connect( mActionSetProjectCRSFromLayer, SIGNAL( triggered() ), this, SLOT( setProjectCRSFromLayer() ) );
  connect( mActionLayerProperties, SIGNAL( triggered() ), this, SLOT( layerProperties() ) );
  connect( mActionLayerSubsetString, SIGNAL( triggered() ), this, SLOT( layerSubsetString() ) );
  connect( mActionAddToOverview, SIGNAL( triggered() ), this, SLOT( isInOverview() ) );
  connect( mActionAddAllToOverview, SIGNAL( triggered() ), this, SLOT( addAllToOverview() ) );
  connect( mActionRemoveAllFromOverview, SIGNAL( triggered() ), this, SLOT( removeAllFromOverview() ) );
  connect( mActionShowAllLayers, SIGNAL( triggered() ), this, SLOT( showAllLayers() ) );
  connect( mActionHideAllLayers, SIGNAL( triggered() ), this, SLOT( hideAllLayers() ) );
  connect( mActionShowSelectedLayers, SIGNAL( triggered() ), this, SLOT( showSelectedLayers() ) );
  connect( mActionHideSelectedLayers, SIGNAL( triggered() ), this, SLOT( hideSelectedLayers() ) );

  // Plugin Menu Items

  connect( mActionManagePlugins, SIGNAL( triggered() ), this, SLOT( showPluginManager() ) );
  connect( mActionShowPythonDialog, SIGNAL( triggered() ), this, SLOT( showPythonDialog() ) );

  // Settings Menu Items

  connect( mActionToggleFullScreen, SIGNAL( triggered() ), this, SLOT( toggleFullScreen() ) );
  connect( mActionProjectProperties, SIGNAL( triggered() ), this, SLOT( projectProperties() ) );
  connect( mActionOptions, SIGNAL( triggered() ), this, SLOT( options() ) );
  connect( mActionCustomProjection, SIGNAL( triggered() ), this, SLOT( customProjection() ) );
  connect( mActionConfigureShortcuts, SIGNAL( triggered() ), this, SLOT( configureShortcuts() ) );
  connect( mActionStyleManagerV2, SIGNAL( triggered() ), this, SLOT( showStyleManagerV2() ) );
  connect( mActionCustomization, SIGNAL( triggered() ), this, SLOT( customize() ) );

#ifdef Q_OS_MAC
  // Window Menu Items

  mActionWindowMinimize = new QAction( tr( "Minimize" ), this );
  mActionWindowMinimize->setShortcut( tr( "Ctrl+M", "Minimize Window" ) );
  mActionWindowMinimize->setStatusTip( tr( "Minimizes the active window to the dock" ) );
  connect( mActionWindowMinimize, SIGNAL( triggered() ), this, SLOT( showActiveWindowMinimized() ) );

  mActionWindowZoom = new QAction( tr( "Zoom" ), this );
  mActionWindowZoom->setStatusTip( tr( "Toggles between a predefined size and the window size set by the user" ) );
  connect( mActionWindowZoom, SIGNAL( triggered() ), this, SLOT( toggleActiveWindowMaximized() ) );

  mActionWindowAllToFront = new QAction( tr( "Bring All to Front" ), this );
  mActionWindowAllToFront->setStatusTip( tr( "Bring forward all open windows" ) );
  connect( mActionWindowAllToFront, SIGNAL( triggered() ), this, SLOT( bringAllToFront() ) );

  // list of open windows
  mWindowActions = new QActionGroup( this );
#endif

  // Vector edits menu
  QMenu* menuAllEdits = new QMenu( tr( "Current Edits" ), this );
  menuAllEdits->addAction( mActionSaveEdits );
  menuAllEdits->addAction( mActionRollbackEdits );
  menuAllEdits->addAction( mActionCancelEdits );
  menuAllEdits->addSeparator();
  menuAllEdits->addAction( mActionSaveAllEdits );
  menuAllEdits->addAction( mActionRollbackAllEdits );
  menuAllEdits->addAction( mActionCancelAllEdits );
  mActionAllEdits->setMenu( menuAllEdits );

  // Raster toolbar items
  connect( mActionLocalHistogramStretch, SIGNAL( triggered() ), this, SLOT( localHistogramStretch() ) );
  connect( mActionFullHistogramStretch, SIGNAL( triggered() ), this, SLOT( fullHistogramStretch() ) );
  connect( mActionLocalCumulativeCutStretch, SIGNAL( triggered() ), this, SLOT( localCumulativeCutStretch() ) );
  connect( mActionFullCumulativeCutStretch, SIGNAL( triggered() ), this, SLOT( fullCumulativeCutStretch() ) );
  connect( mActionIncreaseBrightness, SIGNAL( triggered() ), this, SLOT( increaseBrightness() ) );
  connect( mActionDecreaseBrightness, SIGNAL( triggered() ), this, SLOT( decreaseBrightness() ) );
  connect( mActionIncreaseContrast, SIGNAL( triggered() ), this, SLOT( increaseContrast() ) );
  connect( mActionDecreaseContrast, SIGNAL( triggered() ), this, SLOT( decreaseContrast() ) );

  // Vector Menu Items
  connect( mActionOSMDownload, SIGNAL( triggered() ), this, SLOT( osmDownloadDialog() ) );
  connect( mActionOSMImport, SIGNAL( triggered() ), this, SLOT( osmImportDialog() ) );
  connect( mActionOSMExport, SIGNAL( triggered() ), this, SLOT( osmExportDialog() ) );

  // Help Menu Items

#ifdef Q_OS_MAC
  mActionHelpContents->setShortcut( QString( "Ctrl+?" ) );
  mActionQgisHomePage->setShortcut( QString() );
#endif

  mActionHelpContents->setEnabled( QFileInfo( QgsApplication::pkgDataPath() + "/doc/index.html" ).exists() );

  connect( mActionHelpContents, SIGNAL( triggered() ), this, SLOT( helpContents() ) );
  connect( mActionHelpAPI, SIGNAL( triggered() ), this, SLOT( apiDocumentation() ) );
  connect( mActionNeedSupport, SIGNAL( triggered() ), this, SLOT( supportProviders() ) );
  connect( mActionQgisHomePage, SIGNAL( triggered() ), this, SLOT( helpQgisHomePage() ) );
  connect( mActionCheckQgisVersion, SIGNAL( triggered() ), this, SLOT( checkQgisVersion() ) );
  connect( mActionAbout, SIGNAL( triggered() ), this, SLOT( about() ) );
  connect( mActionSponsors, SIGNAL( triggered() ), this, SLOT( sponsors() ) );

  connect( mActionShowPinnedLabels, SIGNAL( toggled( bool ) ), this, SLOT( showPinnedLabels( bool ) ) );
  connect( mActionPinLabels, SIGNAL( triggered() ), this, SLOT( pinLabels() ) );
  connect( mActionShowHideLabels, SIGNAL( triggered() ), this, SLOT( showHideLabels() ) );
  connect( mActionMoveLabel, SIGNAL( triggered() ), this, SLOT( moveLabel() ) );
  connect( mActionRotateLabel, SIGNAL( triggered() ), this, SLOT( rotateLabel() ) );
  connect( mActionChangeLabelProperties, SIGNAL( triggered() ), this, SLOT( changeLabelProperties() ) );

  // Decoration actions
  connect( mActionDecorationCopyright, SIGNAL( triggered() ), mDecorationCopyright, SLOT( run() ) );
  connect( mActionDecorationNorthArrow, SIGNAL( triggered() ), mDecorationNorthArrow, SLOT( run() ) );
  connect( mActionDecorationScaleBar, SIGNAL( triggered() ), mDecorationScaleBar, SLOT( run() ) );
  connect( mActionDecorationGrid, SIGNAL( triggered() ), mDecorationGrid, SLOT( run() ) );

#ifndef HAVE_POSTGRESQL
  delete mActionAddPgLayer;
  mActionAddPgLayer = 0;
#endif

#ifndef HAVE_MSSQL
  delete mActionAddMssqlLayer;
  mActionAddMssqlLayer = 0;
#endif

#ifndef HAVE_ORACLE
  delete mActionAddOracleLayer;
  mActionAddOracleLayer = 0;
#endif

  if ( !mPythonUtils || !mPythonUtils->isEnabled() )
  {
    delete mActionShowPythonDialog;
  }
}

void QgsClassicApp::createActionGroups()
{
  //
  // Map Tool Group
  mMapToolGroup = new QActionGroup( this );
  mMapToolGroup->addAction( mActionPan );
  mMapToolGroup->addAction( mActionZoomIn );
  mMapToolGroup->addAction( mActionZoomOut );
  mMapToolGroup->addAction( mActionIdentify );
  mMapToolGroup->addAction( mActionFeatureAction );
  mMapToolGroup->addAction( mActionSelectFeatures );
  mMapToolGroup->addAction( mActionSelectPolygon );
  mMapToolGroup->addAction( mActionSelectFreehand );
  mMapToolGroup->addAction( mActionSelectRadius );
  mMapToolGroup->addAction( mActionDeselectAll );
  mMapToolGroup->addAction( mActionMeasure );
  mMapToolGroup->addAction( mActionMeasureArea );
  mMapToolGroup->addAction( mActionMeasureCircle );
  mMapToolGroup->addAction( mActionMeasureHeightProfile );
  mMapToolGroup->addAction( mActionMeasureAngle );
  mMapToolGroup->addAction( mActionAddFeature );
  mMapToolGroup->addAction( mActionCircularStringCurvePoint );
  mMapToolGroup->addAction( mActionCircularStringRadius );
  mMapToolGroup->addAction( mActionMoveFeature );
  mMapToolGroup->addAction( mActionRotateFeature );
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
  ((GEOS_VERSION_MAJOR>3) || ((GEOS_VERSION_MAJOR==3) && (GEOS_VERSION_MINOR>=3)))
  mMapToolGroup->addAction( mActionOffsetCurve );
#endif
  mMapToolGroup->addAction( mActionReshapeFeatures );
  mMapToolGroup->addAction( mActionSplitFeatures );
  mMapToolGroup->addAction( mActionSplitParts );
  mMapToolGroup->addAction( mActionDeleteSelected );
  mMapToolGroup->addAction( mActionAddRing );
  mMapToolGroup->addAction( mActionFillRing );
  mMapToolGroup->addAction( mActionAddPart );
  mMapToolGroup->addAction( mActionSimplifyFeature );
  mMapToolGroup->addAction( mActionDeleteRing );
  mMapToolGroup->addAction( mActionDeletePart );
  mMapToolGroup->addAction( mActionMergeFeatures );
  mMapToolGroup->addAction( mActionMergeFeatureAttributes );
  mMapToolGroup->addAction( mActionNodeTool );
  mMapToolGroup->addAction( mActionRotatePointSymbols );
  mMapToolGroup->addAction( mActionPinLabels );
  mMapToolGroup->addAction( mActionShowHideLabels );
  mMapToolGroup->addAction( mActionMoveLabel );
  mMapToolGroup->addAction( mActionRotateLabel );
  mMapToolGroup->addAction( mActionChangeLabelProperties );

  //
  // Preview Modes Group
  QActionGroup* mPreviewGroup = new QActionGroup( this );
  mPreviewGroup->setExclusive( true );
  mActionPreviewModeOff->setActionGroup( mPreviewGroup );
  mActionPreviewModeGrayscale->setActionGroup( mPreviewGroup );
  mActionPreviewModeMono->setActionGroup( mPreviewGroup );
  mActionPreviewProtanope->setActionGroup( mPreviewGroup );
  mActionPreviewDeuteranope->setActionGroup( mPreviewGroup );
}

void QgsClassicApp::setupMenus()
{
  /*
   * The User Interface Guidelines for each platform specify different locations
   * for the following items.
   *
   * Project Properties:
   * Gnome, Mac, Win - File/Project menu above print commands (Win doesn't specify)
   * Kde - Settings menu
   *
   * Custom CRS, Options:
   * Gnome - bottom of Edit menu
   * Mac - Application menu (moved automatically by Qt)
   * Kde, Win - Settings menu (Win should use Tools menu but we don't have one)
   *
   * Panel and Toolbar submenus, Toggle Full Screen:
   * Gnome, Mac, Win - View menu
   * Kde - Settings menu
   *
   * For Mac, About and Exit are also automatically moved by Qt to the Application menu.
   */

  // Get platform for menu layout customization (Gnome, Kde, Mac, Win)
  QDialogButtonBox::ButtonLayout layout =
    QDialogButtonBox::ButtonLayout( style()->styleHint( QStyle::SH_DialogButtonLayout, 0, this ) );

  // Project Menu

  // Connect once for the entire submenu.
  connect( mRecentProjectsMenu, SIGNAL( triggered( QAction * ) ),
           this, SLOT( openProject( QAction * ) ) );
  connect( mProjectFromTemplateMenu, SIGNAL( triggered( QAction * ) ),
           this, SLOT( fileNewFromTemplateAction( QAction * ) ) );

  if ( layout == QDialogButtonBox::GnomeLayout || layout == QDialogButtonBox::MacLayout || layout == QDialogButtonBox::WinLayout )
  {
    QAction* before = mActionNewPrintComposer;
    mSettingsMenu->removeAction( mActionProjectProperties );
    mProjectMenu->insertAction( before, mActionProjectProperties );
    mProjectMenu->insertSeparator( before );
  }

  // View Menu

  if ( layout != QDialogButtonBox::KdeLayout )
  {
    mViewMenu->addSeparator();
    mViewMenu->addMenu( mPanelMenu );
    mViewMenu->addMenu( mToolbarMenu );
    mViewMenu->addAction( mActionToggleFullScreen );
  }
  else
  {
    // on the top of the settings menu
    QAction* before = mActionProjectProperties;
    mSettingsMenu->insertMenu( before, mPanelMenu );
    mSettingsMenu->insertMenu( before, mToolbarMenu );
    mSettingsMenu->insertAction( before, mActionToggleFullScreen );
    mSettingsMenu->insertSeparator( before );
  }


#ifdef Q_OS_MAC
  //disabled for OSX - see #10761
  //also see http://qt-project.org/forums/viewthread/3630 QGraphicsEffects are not well supported on OSX
  mMenuPreviewMode->menuAction()->setVisible( false );
#endif


#ifdef Q_OS_MAC

  // keep plugins from hijacking About and Preferences application menus
  // these duplicate actions will be moved to application menus by Qt
  mProjectMenu->addAction( mActionAbout );
  mProjectMenu->addAction( mActionOptions );

  // Window Menu

  mWindowMenu = new QMenu( tr( "Window" ), this );

  mWindowMenu->addAction( mActionWindowMinimize );
  mWindowMenu->addAction( mActionWindowZoom );
  mWindowMenu->addSeparator();

  mWindowMenu->addAction( mActionWindowAllToFront );
  mWindowMenu->addSeparator();

  // insert before Help menu, as per Mac OS convention
  menuBar()->insertMenu( mHelpMenu->menuAction(), mWindowMenu );
#endif

  // Database Menu
  // don't add it yet, wait for a plugin
  mDatabaseMenu = new QMenu( tr( "&Database" ), menuBar() );
  mDatabaseMenu->setObjectName( "mDatabaseMenu" );
  // Web Menu
  // don't add it yet, wait for a plugin
  mWebMenu = new QMenu( tr( "&Web" ), menuBar() );
  mWebMenu->setObjectName( "mWebMenu" );

  // Help menu
  // add What's this button to it
  QAction* before = mActionHelpAPI;
  QAction* actionWhatsThis = QWhatsThis::createAction( this );
  actionWhatsThis->setIcon( QgsApplication::getThemeIcon( "/mActionWhatsThis.svg" ) );
  mHelpMenu->insertAction( before, actionWhatsThis );
}

void QgsClassicApp::createToolBars()
{
  QSettings settings;
  int size = settings.value( "/IconSize", QGIS_ICON_SIZE ).toInt();
  setIconSize( QSize( size, size ) );
  // QSize myIconSize ( 32,32 ); //large icons
  // Note: we need to set each object name to ensure that
  // qmainwindow::saveState and qmainwindow::restoreState
  // work properly

  QList<QToolBar*> toolbarMenuToolBars;
  toolbarMenuToolBars << mFileToolBar
  << mLayerToolBar
  << mDigitizeToolBar
  << mAdvancedDigitizeToolBar
  << mMapNavToolBar
  << mAttributesToolBar
  << mPluginToolBar
  << mHelpToolBar
  << mRasterToolBar
  << mVectorToolBar
  << mDatabaseToolBar
  << mWebToolBar
  << mLabelToolBar;

  QList<QAction*> toolbarMenuActions;
  // Set action names so that they can be used in customization
  foreach ( QToolBar *toolBar, toolbarMenuToolBars )
  {
    toolBar->toggleViewAction()->setObjectName( "mActionToggle" + toolBar->objectName().mid( 1 ) );
    toolbarMenuActions << toolBar->toggleViewAction();
  }

  // sort actions in toolbar menu
  qSort( toolbarMenuActions.begin(), toolbarMenuActions.end(), cmpByText_ );

  mToolbarMenu->addActions( toolbarMenuActions );

  // select tool button

  QToolButton *bt = new QToolButton( mAttributesToolBar );
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  QList<QAction*> selectActions;
  selectActions << mActionSelectFeatures << mActionSelectPolygon
  << mActionSelectFreehand << mActionSelectRadius;
  bt->addActions( selectActions );

  QAction* defSelectAction = mActionSelectFeatures;
  switch ( settings.value( "/UI/selectTool", 0 ).toInt() )
  {
    case 0: defSelectAction = mActionSelectFeatures; break;
    case 1: defSelectAction = mActionSelectFeatures; break;
    case 2: defSelectAction = mActionSelectRadius; break;
    case 3: defSelectAction = mActionSelectPolygon; break;
    case 4: defSelectAction = mActionSelectFreehand; break;
  }
  bt->setDefaultAction( defSelectAction );
  QAction* selectAction = mAttributesToolBar->insertWidget( mActionDeselectAll, bt );
  selectAction->setObjectName( "ActionSelect" );
  connect( bt, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );

  // feature action tool button

  bt = new QToolButton( mAttributesToolBar );
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  bt->setDefaultAction( mActionFeatureAction );
  mFeatureActionMenu = new QMenu( bt );
  connect( mFeatureActionMenu, SIGNAL( triggered( QAction * ) ), this, SLOT( updateDefaultFeatureAction( QAction * ) ) );
  connect( mFeatureActionMenu, SIGNAL( aboutToShow() ), this, SLOT( refreshFeatureActions() ) );
  bt->setMenu( mFeatureActionMenu );
  QAction* featureActionAction = mAttributesToolBar->insertWidget( selectAction, bt );
  featureActionAction->setObjectName( "ActionFeatureAction" );

  // measure tool button

  bt = new QToolButton( mAttributesToolBar );
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  bt->addAction( mActionMeasure );
  bt->addAction( mActionMeasureArea );
  bt->addAction( mActionMeasureCircle );
  bt->addAction( mActionMeasureHeightProfile );
  bt->addAction( mActionMeasureAngle );

  QAction* defMeasureAction = mActionMeasure;
  switch ( settings.value( "/UI/measureTool", 0 ).toInt() )
  {
    case 0: defMeasureAction = mActionMeasure; break;
    case 1: defMeasureAction = mActionMeasureArea; break;
    case 2: defMeasureAction = mActionMeasureAngle; break;
    case 3: defMeasureAction = mActionMeasureCircle; break;
    case 4: defMeasureAction = mActionMeasureHeightProfile; break;
  }
  bt->setDefaultAction( defMeasureAction );
  QAction* measureAction = mAttributesToolBar->insertWidget( mActionMapTips, bt );
  measureAction->setObjectName( "ActionMeasure" );
  connect( bt, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );

  // annotation tool button

  bt = new QToolButton();
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  bt->addAction( mActionTextAnnotation );
  bt->addAction( mActionFormAnnotation );
  bt->addAction( mActionHtmlAnnotation );
  bt->addAction( mActionSvgAnnotation );
  bt->addAction( mActionPinAnnotation );

  QAction* defAnnotationAction = mActionTextAnnotation;
  switch ( settings.value( "/UI/annotationTool", 0 ).toInt() )
  {
    case 0: defAnnotationAction = mActionTextAnnotation; break;
    case 1: defAnnotationAction = mActionFormAnnotation; break;
    case 2: defAnnotationAction = mActionHtmlAnnotation; break;
    case 3: defAnnotationAction = mActionSvgAnnotation; break;
    case 4: defAnnotationAction = mActionPinAnnotation; break;

  }
  bt->setDefaultAction( defAnnotationAction );
  QAction* annotationAction = mAttributesToolBar->addWidget( bt );
  annotationAction->setObjectName( "ActionAnnotation" );
  connect( bt, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );

  //circular string digitize tool button
  QToolButton* tbAddCircularString = new QToolButton( mDigitizeToolBar );
  tbAddCircularString->setPopupMode( QToolButton::MenuButtonPopup );
  tbAddCircularString->addAction( mActionCircularStringCurvePoint );
  tbAddCircularString->addAction( mActionCircularStringRadius );
  tbAddCircularString->setDefaultAction( mActionCircularStringCurvePoint );
  connect( tbAddCircularString, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );
  mDigitizeToolBar->insertWidget( mActionMoveFeature, tbAddCircularString );

  // vector layer edits tool buttons
  QToolButton* tbAllEdits = qobject_cast<QToolButton *>( mDigitizeToolBar->widgetForAction( mActionAllEdits ) );
  tbAllEdits->setPopupMode( QToolButton::InstantPopup );

  // new layer tool button

  bt = new QToolButton();
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  bt->addAction( mActionNewSpatiaLiteLayer );
  bt->addAction( mActionNewVectorLayer );
  bt->addAction( mActionNewMemoryLayer );

  QAction* defNewLayerAction = mActionNewVectorLayer;
  switch ( settings.value( "/UI/defaultNewLayer", 1 ).toInt() )
  {
    case 0: defNewLayerAction = mActionNewSpatiaLiteLayer; break;
    case 1: defNewLayerAction = mActionNewVectorLayer; break;
    case 2: defNewLayerAction = mActionNewMemoryLayer; break;
  }
  bt->setDefaultAction( defNewLayerAction );
  QAction* newLayerAction = mLayerToolBar->addWidget( bt );

  newLayerAction->setObjectName( "ActionNewLayer" );
  connect( bt, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );

  // map service tool button
  bt = new QToolButton();
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  bt->addAction( mActionAddWmsLayer );
  bt->addAction( mActionAddAmsLayer );
  QAction* defMapServiceAction = mActionAddWmsLayer;
  switch ( settings.value( "/UI/defaultMapService", 0 ).toInt() )
  {
    case 0: defMapServiceAction = mActionAddWmsLayer; break;
    case 1: defMapServiceAction = mActionAddAmsLayer; break;
  };
  bt->setDefaultAction( defMapServiceAction );
  QAction* mapServiceAction = mLayerToolBar->insertWidget( mActionAddWmsLayer, bt );
  mLayerToolBar->removeAction( mActionAddWmsLayer );
  mapServiceAction->setObjectName( "ActionMapService" );
  connect( bt, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );

  // feature service tool button
  bt = new QToolButton();
  bt->setPopupMode( QToolButton::MenuButtonPopup );
  bt->addAction( mActionAddWfsLayer );
  bt->addAction( mActionAddAfsLayer );
  QAction* defFeatureServiceAction = mActionAddWfsLayer;
  switch ( settings.value( "/UI/defaultFeatureService", 0 ).toInt() )
  {
    case 0: defFeatureServiceAction = mActionAddWfsLayer; break;
    case 1: defFeatureServiceAction = mActionAddAfsLayer; break;
  };
  bt->setDefaultAction( defFeatureServiceAction );
  QAction* featureServiceAction = mLayerToolBar->insertWidget( mActionAddWfsLayer, bt );
  mLayerToolBar->removeAction( mActionAddWfsLayer );
  featureServiceAction->setObjectName( "ActionFeatureService" );
  connect( bt, SIGNAL( triggered( QAction * ) ), this, SLOT( toolButtonActionTriggered( QAction * ) ) );

  // Help Toolbar

  QAction* actionWhatsThis = QWhatsThis::createAction( this );
  actionWhatsThis->setIcon( QgsApplication::getThemeIcon( "/mActionWhatsThis.svg" ) );
  mHelpToolBar->addAction( actionWhatsThis );

  // Cad toolbar
  mAdvancedDigitizeToolBar->insertAction( mActionUndo, mAdvancedDigitizingDockWidget->enableAction() );
}

void QgsClassicApp::createStatusBar()
{
  //remove borders from children under Windows
  statusBar()->setStyleSheet( "QStatusBar::item {border: none;}" );

  // Add a panel to the status bar for the scale, coords and progress
  // And also rendering suppression checkbox
  mProgressBar = new QProgressBar( statusBar() );
  mProgressBar->setObjectName( "mProgressBar" );
  mProgressBar->setMaximumWidth( 100 );
  mProgressBar->hide();
  mProgressBar->setWhatsThis( tr( "Progress bar that displays the status "
                                  "of rendering layers and other time-intensive operations" ) );
  statusBar()->addPermanentWidget( mProgressBar, 1 );

  connect( mMapCanvas, SIGNAL( renderStarting() ), this, SLOT( canvasRefreshStarted() ) );
  connect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( canvasRefreshFinished() ) );

  // Bumped the font up one point size since 8 was too
  // small on some platforms. A point size of 9 still provides
  // plenty of display space on 1024x768 resolutions
  QFont myFont( "Arial", 9 );

  statusBar()->setFont( myFont );
  //toggle to switch between mouse pos and extents display in status bar widget
  mToggleExtentsViewButton = new QToolButton( statusBar() );
  mToggleExtentsViewButton->setObjectName( "mToggleExtentsViewButton" );
  mToggleExtentsViewButton->setMaximumWidth( 20 );
  mToggleExtentsViewButton->setMaximumHeight( 20 );
  mToggleExtentsViewButton->setIcon( QgsApplication::getThemeIcon( "tracking.png" ) );
  mToggleExtentsViewButton->setToolTip( tr( "Toggle extents and mouse position display" ) );
  mToggleExtentsViewButton->setCheckable( true );
  connect( mToggleExtentsViewButton, SIGNAL( toggled( bool ) ), this, SLOT( extentsViewToggled( bool ) ) );
  statusBar()->addPermanentWidget( mToggleExtentsViewButton, 0 );

  // add a label to show current scale
  mCoordsLabel = new QLabel( QString(), statusBar() );
  mCoordsLabel->setObjectName( "mCoordsLabel" );
  mCoordsLabel->setFont( myFont );
  mCoordsLabel->setMinimumWidth( 10 );
  mCoordsLabel->setMaximumHeight( 20 );
  mCoordsLabel->setMargin( 3 );
  mCoordsLabel->setAlignment( Qt::AlignCenter );
  mCoordsLabel->setFrameStyle( QFrame::NoFrame );
  mCoordsLabel->setText( tr( "Coordinate:" ) );
  mCoordsLabel->setToolTip( tr( "Current map coordinate" ) );
  statusBar()->addPermanentWidget( mCoordsLabel, 0 );

  //coords status bar widget
  mCoordsEdit = new QLineEdit( QString(), statusBar() );
  mCoordsEdit->setObjectName( "mCoordsEdit" );
  mCoordsEdit->setFont( myFont );
  mCoordsEdit->setMinimumWidth( 10 );
  mCoordsEdit->setMaximumWidth( 300 );
  mCoordsEdit->setMaximumHeight( 20 );
  mCoordsEdit->setContentsMargins( 0, 0, 0, 0 );
  mCoordsEdit->setAlignment( Qt::AlignCenter );
  QRegExp coordValidator( "[+-]?\\d+\\.?\\d*\\s*,\\s*[+-]?\\d+\\.?\\d*" );
  mCoordsEditValidator = new QRegExpValidator( coordValidator, mCoordsEdit );
  mCoordsEdit->setWhatsThis( tr( "Shows the map coordinates at the "
                                 "current cursor position. The display is continuously updated "
                                 "as the mouse is moved. It also allows editing to set the canvas "
                                 "center to a given position. The format is lat,lon or east,north" ) );
  mCoordsEdit->setToolTip( tr( "Current map coordinate (lat,lon or east,north)" ) );
  statusBar()->addPermanentWidget( mCoordsEdit, 0 );
  connect( mCoordsEdit, SIGNAL( returnPressed() ), this, SLOT( userCenter() ) );
  mDizzyTimer = new QTimer( this );
  connect( mDizzyTimer, SIGNAL( timeout() ), this, SLOT( dizzy() ) );

  // add a label to show current scale
  mScaleLabel = new QLabel( QString(), statusBar() );
  mScaleLabel->setObjectName( "mScaleLable" );
  mScaleLabel->setFont( myFont );
  mScaleLabel->setMinimumWidth( 10 );
  mScaleLabel->setMaximumHeight( 20 );
  mScaleLabel->setMargin( 3 );
  mScaleLabel->setAlignment( Qt::AlignCenter );
  mScaleLabel->setFrameStyle( QFrame::NoFrame );
  mScaleLabel->setText( tr( "Scale " ) );
  mScaleLabel->setToolTip( tr( "Current map scale" ) );
  statusBar()->addPermanentWidget( mScaleLabel, 0 );

  mScaleEdit = new QgsScaleComboBox( statusBar() );
  mScaleEdit->setObjectName( "mScaleEdit" );
  mScaleEdit->setFont( myFont );
  // seems setFont() change font only for popup not for line edit,
  // so we need to set font for it separately
  mScaleEdit->lineEdit()->setFont( myFont );
  mScaleEdit->setMinimumWidth( 10 );
  mScaleEdit->setMaximumWidth( 100 );
  mScaleEdit->setMaximumHeight( 20 );
  mScaleEdit->setContentsMargins( 0, 0, 0, 0 );
  mScaleEdit->setWhatsThis( tr( "Displays the current map scale" ) );
  mScaleEdit->setToolTip( tr( "Current map scale (formatted as x:y)" ) );

  statusBar()->addPermanentWidget( mScaleEdit, 0 );
  connect( mScaleEdit, SIGNAL( scaleChanged() ), this, SLOT( userScale() ) );

  if ( QgsMapCanvas::rotationEnabled() )
  {
    // add a widget to show/set current rotation
    mRotationLabel = new QLabel( QString(), statusBar() );
    mRotationLabel->setObjectName( "mRotationLabel" );
    mRotationLabel->setFont( myFont );
    mRotationLabel->setMinimumWidth( 10 );
    mRotationLabel->setMaximumHeight( 20 );
    mRotationLabel->setMargin( 3 );
    mRotationLabel->setAlignment( Qt::AlignCenter );
    mRotationLabel->setFrameStyle( QFrame::NoFrame );
    mRotationLabel->setText( tr( "Rotation:" ) );
    mRotationLabel->setToolTip( tr( "Current clockwise map rotation in degrees" ) );
    statusBar()->addPermanentWidget( mRotationLabel, 0 );

    mRotationEdit = new QgsDoubleSpinBox( statusBar() );
    mRotationEdit->setObjectName( "mRotationEdit" );
    mRotationEdit->setClearValue( 0.0 );
    mRotationEdit->setKeyboardTracking( false );
    mRotationEdit->setMaximumWidth( 120 );
    mRotationEdit->setDecimals( 1 );
    mRotationEdit->setRange( -180.0, 180.0 );
    mRotationEdit->setWrapping( true );
    mRotationEdit->setSingleStep( 5.0 );
    mRotationEdit->setFont( myFont );
    mRotationEdit->setWhatsThis( tr( "Shows the current map clockwise rotation "
                                     "in degrees. It also allows editing to set "
                                     "the rotation" ) );
    mRotationEdit->setToolTip( tr( "Current clockwise map rotation in degrees" ) );
    statusBar()->addPermanentWidget( mRotationEdit, 0 );
    connect( mRotationEdit, SIGNAL( valueChanged( double ) ), this, SLOT( userRotation() ) );

    showRotation();
  }

  // render suppression status bar widget
  mRenderSuppressionCBox = new QCheckBox( tr( "Render" ), statusBar() );
  mRenderSuppressionCBox->setObjectName( "mRenderSuppressionCBox" );
  mRenderSuppressionCBox->setChecked( true );
  mRenderSuppressionCBox->setFont( myFont );
  mRenderSuppressionCBox->setWhatsThis( tr( "When checked, the map layers "
                                        "are rendered in response to map navigation commands and other "
                                        "events. When not checked, no rendering is done. This allows you "
                                        "to add a large number of layers and symbolize them before rendering." ) );
  mRenderSuppressionCBox->setToolTip( tr( "Toggle map rendering" ) );
  statusBar()->addPermanentWidget( mRenderSuppressionCBox, 0 );
  // On the fly projection status bar icon
  // Changed this to a tool button since a QPushButton is
  // sculpted on OS X and the icon is never displayed [gsherman]
  mOnTheFlyProjectionStatusButton = new QToolButton( statusBar() );
  mOnTheFlyProjectionStatusButton->setAutoRaise( true );
  mOnTheFlyProjectionStatusButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  mOnTheFlyProjectionStatusButton->setObjectName( "mOntheFlyProjectionStatusButton" );
  // Maintain uniform widget height in status bar by setting button height same as labels
  // For Qt/Mac 3.3, the default toolbutton height is 30 and labels were expanding to match
  mOnTheFlyProjectionStatusButton->setMaximumHeight( mScaleLabel->height() );
  mOnTheFlyProjectionStatusButton->setIcon( QgsApplication::getThemeIcon( "mIconProjectionEnabled.png" ) );
  mOnTheFlyProjectionStatusButton->setWhatsThis( tr( "This icon shows whether "
      "on the fly coordinate reference system transformation is enabled or not. "
      "Click the icon to bring up "
      "the project properties dialog to alter this behaviour." ) );
  mOnTheFlyProjectionStatusButton->setToolTip( tr( "CRS status - Click "
      "to open coordinate reference system dialog" ) );
  connect( mOnTheFlyProjectionStatusButton, SIGNAL( clicked() ),
           this, SLOT( projectPropertiesProjections() ) );//bring up the project props dialog when clicked
  statusBar()->addPermanentWidget( mOnTheFlyProjectionStatusButton, 0 );
  statusBar()->showMessage( tr( "Ready" ) );

  mMessageButton = new QToolButton( statusBar() );
  mMessageButton->setAutoRaise( true );
  mMessageButton->setIcon( QgsApplication::getThemeIcon( "bubble.svg" ) );
  mMessageButton->setToolTip( tr( "Messages" ) );
  mMessageButton->setWhatsThis( tr( "Messages" ) );
  mMessageButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  mMessageButton->setObjectName( "mMessageLogViewerButton" );
  mMessageButton->setMaximumHeight( mScaleLabel->height() );
  mMessageButton->setCheckable( true );
  statusBar()->addPermanentWidget( mMessageButton, 0 );
}

void QgsClassicApp::initLayerTreeView()
{
  layerTreeView()->setWhatsThis( tr( "Map legend that displays all the layers currently on the map canvas. Click on the check box to turn a layer on or off. Double click on a layer in the legend to customize its appearance and set other properties." ) );

  mLayerTreeDock = new QDockWidget( tr( "Layers" ), this );
  mLayerTreeDock->setObjectName( "Layers" );
  mLayerTreeDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

  QgsLayerTreeModel* model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
#ifdef ENABLE_MODELTEST
  new ModelTest( model, this );
#endif
  model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
  model->setFlag( QgsLayerTreeModel::AllowNodeRename );
  model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
  model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
  model->setAutoCollapseLegendNodes( 10 );

  layerTreeView()->setModel( model );
  layerTreeView()->setMenuProvider( new QgsAppLayerTreeViewMenuProvider( layerTreeView(), mapCanvas() ) );

  setupLayerTreeViewFromSettings();

  connect( layerTreeView(), SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );
  connect( layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( activeLayerChanged( QgsMapLayer* ) ) );
  connect( layerTreeView()->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), this, SLOT( updateNewLayerInsertionPoint() ) );
  connect( layerTreeView()->selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ), this, SLOT( legendLayerSelectionChanged() ) );
  connect( QgsProject::instance()->layerTreeRegistryBridge(), SIGNAL( addedLayersToLayerTree( QList<QgsMapLayer*> ) ),
           this, SLOT( autoSelectAddedLayer( QList<QgsMapLayer*> ) ) );

  // add group tool button
  QToolButton* btnAddGroup = new QToolButton;
  btnAddGroup->setAutoRaise( true );
  btnAddGroup->setIcon( QgsApplication::getThemeIcon( "/mActionFolder.png" ) );
  btnAddGroup->setToolTip( tr( "Add Group" ) );
  connect( btnAddGroup, SIGNAL( clicked() ), layerTreeView()->defaultActions(), SLOT( addGroup() ) );

  // visibility groups tool button
  QToolButton* btnVisibilityPresets = new QToolButton;
  btnVisibilityPresets->setAutoRaise( true );
  btnVisibilityPresets->setToolTip( tr( "Manage Layer Visibility" ) );
  btnVisibilityPresets->setIcon( QgsApplication::getThemeIcon( "/mActionShowAllLayers.png" ) );
  btnVisibilityPresets->setPopupMode( QToolButton::InstantPopup );
  btnVisibilityPresets->setMenu( QgsVisibilityPresets::instance()->menu() );

  // filter legend tool button
  mBtnFilterLegend = new QToolButton;
  mBtnFilterLegend->setAutoRaise( true );
  mBtnFilterLegend->setCheckable( true );
  mBtnFilterLegend->setToolTip( tr( "Filter Legend By Map Content" ) );
  mBtnFilterLegend->setIcon( QgsApplication::getThemeIcon( "/mActionFilter.png" ) );
  connect( mBtnFilterLegend, SIGNAL( clicked() ), this, SLOT( toggleFilterLegendByMap() ) );

  // expand / collapse tool buttons
  QToolButton* btnExpandAll = new QToolButton;
  btnExpandAll->setAutoRaise( true );
  btnExpandAll->setIcon( QgsApplication::getThemeIcon( "/mActionExpandTree.png" ) );
  btnExpandAll->setToolTip( tr( "Expand All" ) );
  connect( btnExpandAll, SIGNAL( clicked() ), mLayerTreeView, SLOT( expandAll() ) );
  QToolButton* btnCollapseAll = new QToolButton;
  btnCollapseAll->setAutoRaise( true );
  btnCollapseAll->setIcon( QgsApplication::getThemeIcon( "/mActionCollapseTree.png" ) );
  btnCollapseAll->setToolTip( tr( "Collapse All" ) );
  connect( btnCollapseAll, SIGNAL( clicked() ), mLayerTreeView, SLOT( collapseAll() ) );

  QToolButton* btnRemoveItem = new QToolButton;
  btnRemoveItem->setDefaultAction( mActionRemoveLayer );
  btnRemoveItem->setAutoRaise( true );

  QHBoxLayout* toolbarLayout = new QHBoxLayout;
  toolbarLayout->setContentsMargins( QMargins( 5, 0, 5, 0 ) );
  toolbarLayout->addWidget( btnAddGroup );
  toolbarLayout->addWidget( btnVisibilityPresets );
  toolbarLayout->addWidget( mBtnFilterLegend );
  toolbarLayout->addWidget( btnExpandAll );
  toolbarLayout->addWidget( btnCollapseAll );
  toolbarLayout->addWidget( btnRemoveItem );
  toolbarLayout->addStretch();

  QWidget* layerTreeToolbar = new QWidget();
  layerTreeToolbar->setLayout( toolbarLayout );
  layerTreeToolbar->setObjectName( "layerTreeToolbar" );

  QVBoxLayout* vboxLayout = new QVBoxLayout;
  vboxLayout->setMargin( 0 );
  vboxLayout->addWidget( layerTreeToolbar );
  vboxLayout->addWidget( mLayerTreeView );

  QWidget* w = new QWidget;
  w->setLayout( vboxLayout );
  mLayerTreeDock->setWidget( w );
  addDockWidget( Qt::LeftDockWidgetArea, mLayerTreeDock );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mapCanvas(), this );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), mLayerTreeCanvasBridge, SLOT( readProject( QDomDocument ) ) );

  bool otfTransformAutoEnable = QSettings().value( "/Projections/otfTransformAutoEnable", true ).toBool();
  mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );

  mMapLayerOrder = new QgsCustomLayerOrderWidget( mLayerTreeCanvasBridge, this );
  mMapLayerOrder->setObjectName( "theMapLayerOrder" );

  mMapLayerOrder->setWhatsThis( tr( "Map layer list that displays all layers in drawing order." ) );
  mLayerOrderDock = new QDockWidget( tr( "Layer order" ), this );
  mLayerOrderDock->setObjectName( "LayerOrder" );
  mLayerOrderDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

  mLayerOrderDock->setWidget( mMapLayerOrder );
  addDockWidget( Qt::LeftDockWidgetArea, mLayerOrderDock );
  mLayerOrderDock->hide();

  legendLayerSelectionChanged();
}

void QgsClassicApp::setToolActions()
{
  mMapTools.mZoomIn->setAction( mActionZoomIn );
  mMapTools.mZoomOut->setAction( mActionZoomOut );
  mMapTools.mPan->setAction( mActionPan );
  mMapTools.mIdentify->setAction( mActionIdentify );
  mMapTools.mFeatureAction->setAction( mActionFeatureAction );
  mMapTools.mMeasureDist->setAction( mActionMeasure );
  mMapTools.mMeasureArea->setAction( mActionMeasureArea );
  mMapTools.mMeasureCircle->setAction( mActionMeasureCircle );
  mMapTools.mMeasureHeightProfile->setAction( mActionMeasureHeightProfile );
  mMapTools.mMeasureAngle->setAction( mActionMeasureAngle );
  mMapTools.mTextAnnotation->setAction( mActionTextAnnotation );
  mMapTools.mFormAnnotation->setAction( mActionFormAnnotation );
  mMapTools.mHtmlAnnotation->setAction( mActionHtmlAnnotation );
  mMapTools.mSvgAnnotation->setAction( mActionSvgAnnotation );
  mMapTools.mPinAnnotation->setAction( mActionPinAnnotation );
  mMapTools.mAddFeature->setAction( mActionAddFeature );
  mMapTools.mCircularStringCurvePoint->setAction( mActionCircularStringCurvePoint );
  mMapTools.mCircularStringRadius->setAction( mActionCircularStringRadius );
  mMapTools.mMoveFeature->setAction( mActionMoveFeature );
  mMapTools.mRotateFeature->setAction( mActionRotateFeature );
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
((GEOS_VERSION_MAJOR>3) || ((GEOS_VERSION_MAJOR==3) && (GEOS_VERSION_MINOR>=3)))
  mMapTools.mOffsetCurve->setAction( mActionOffsetCurve );
#else
  mAdvancedDigitizeToolBar->removeAction( mActionOffsetCurve );
  mEditMenu->removeAction( mActionOffsetCurve );
#endif //GEOS_VERSION
  mMapTools.mReshapeFeatures->setAction( mActionReshapeFeatures );
  mMapTools.mSplitFeatures->setAction( mActionSplitFeatures );
  mMapTools.mSplitParts->setAction( mActionSplitParts );
  mMapTools.mSelectFeatures->setAction( mActionSelectFeatures );
  mMapTools.mSelectPolygon->setAction( mActionSelectPolygon );
  mMapTools.mSelectFreehand->setAction( mActionSelectFreehand );
  mMapTools.mSelectRadius->setAction( mActionSelectRadius );
  mMapTools.mAddRing->setAction( mActionAddRing );
  mMapTools.mFillRing->setAction( mActionFillRing );
  mMapTools.mAddPart->setAction( mActionAddPart );
  mMapTools.mSimplifyFeature->setAction( mActionSimplifyFeature );
  mMapTools.mDeleteRing->setAction( mActionDeleteRing );
  mMapTools.mDeletePart->setAction( mActionDeletePart );
  mMapTools.mNodeTool->setAction( mActionNodeTool );
  mMapTools.mRotatePointSymbolsTool->setAction( mActionRotatePointSymbols );
  mMapTools.mPinLabels->setAction( mActionPinLabels );
  mMapTools.mShowHideLabels->setAction( mActionShowHideLabels );
  mMapTools.mMoveLabel->setAction( mActionMoveLabel );
  mMapTools.mRotateLabel->setAction( mActionRotateLabel );
  mMapTools.mChangeLabelProperties->setAction( mActionChangeLabelProperties );
}

QMenu* QgsClassicApp::getPluginMenu( QString menuName )
{
  /* Plugin menu items are below the plugin separator (which may not exist yet
   * if no plugins are loaded) and above the python separator. If python is not
   * present, there is no python separator and the plugin list is at the bottom
   * of the menu.
   */

#ifdef Q_OS_MAC
  // Mac doesn't have '&' keyboard shortcuts.
  menuName.remove( QChar( '&' ) );
#endif
  QAction *before = mActionPluginSeparator2;  // python separator or end of list
  if ( !mActionPluginSeparator1 )
  {
    // First plugin - create plugin list separator
    mActionPluginSeparator1 = mPluginMenu->insertSeparator( before );
  }
  else
  {
    QString dst = menuName;
    dst.remove( QChar( '&' ) );

    // Plugins exist - search between plugin separator and python separator or end of list
    QList<QAction*> actions = mPluginMenu->actions();
    int end = mActionPluginSeparator2 ? actions.indexOf( mActionPluginSeparator2 ) : actions.count();
    for ( int i = actions.indexOf( mActionPluginSeparator1 ) + 1; i < end; i++ )
    {
      QString src = actions.at( i )->text();
      src.remove( QChar( '&' ) );

      int comp = dst.localeAwareCompare( src );
      if ( comp < 0 )
      {
        // Add item before this one
        before = actions.at( i );
        break;
      }
      else if ( comp == 0 )
      {
        // Plugin menu item already exists
        return actions.at( i )->menu();
      }
    }
  }
  // It doesn't exist, so create
  QMenu *menu = new QMenu( menuName, this );
  menu->setObjectName( normalizedMenuName( menuName ) );
  // Where to put it? - we worked that out above...
  mPluginMenu->insertMenu( before, menu );

  return menu;
}

void QgsClassicApp::addPluginToMenu( QString name, QAction* action )
{
  QMenu* menu = getPluginMenu( name );
  menu->addAction( action );
}

void QgsClassicApp::removePluginMenu( QString name, QAction* action )
{
  QMenu* menu = getPluginMenu( name );
  menu->removeAction( action );
  if ( menu->actions().count() == 0 )
  {
    mPluginMenu->removeAction( menu->menuAction() );
  }
  // Remove separator above plugins in Plugin menu if no plugins remain
  QList<QAction*> actions = mPluginMenu->actions();
  int end = mActionPluginSeparator2 ? actions.indexOf( mActionPluginSeparator2 ) : actions.count();
  if ( actions.indexOf( mActionPluginSeparator1 ) + 1 == end )
  {
    mPluginMenu->removeAction( mActionPluginSeparator1 );
    mActionPluginSeparator1 = NULL;
  }
}

QMenu* QgsClassicApp::getDatabaseMenu( QString menuName )
{
#ifdef Q_OS_MAC
  // Mac doesn't have '&' keyboard shortcuts.
  menuName.remove( QChar( '&' ) );
#endif
  QString dst = menuName;
  dst.remove( QChar( '&' ) );

  QAction *before = NULL;
  QList<QAction*> actions = mDatabaseMenu->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    QString src = actions.at( i )->text();
    src.remove( QChar( '&' ) );

    int comp = dst.localeAwareCompare( src );
    if ( comp < 0 )
    {
      // Add item before this one
      before = actions.at( i );
      break;
    }
    else if ( comp == 0 )
    {
      // Plugin menu item already exists
      return actions.at( i )->menu();
    }
  }
  // It doesn't exist, so create
  QMenu *menu = new QMenu( menuName, this );
  menu->setObjectName( normalizedMenuName( menuName ) );
  if ( before )
    mDatabaseMenu->insertMenu( before, menu );
  else
    mDatabaseMenu->addMenu( menu );

  return menu;
}

QMenu* QgsClassicApp::getRasterMenu( QString menuName )
{
#ifdef Q_OS_MAC
  // Mac doesn't have '&' keyboard shortcuts.
  menuName.remove( QChar( '&' ) );
#endif

  QAction *before = NULL;
  if ( !mActionRasterSeparator )
  {
    // First plugin - create plugin list separator
    mActionRasterSeparator = mRasterMenu->insertSeparator( before );
  }
  else
  {
    QString dst = menuName;
    dst.remove( QChar( '&' ) );
    // Plugins exist - search between plugin separator and python separator or end of list
    QList<QAction*> actions = mRasterMenu->actions();
    for ( int i = actions.indexOf( mActionRasterSeparator ) + 1; i < actions.count(); i++ )
    {
      QString src = actions.at( i )->text();
      src.remove( QChar( '&' ) );

      int comp = dst.localeAwareCompare( src );
      if ( comp < 0 )
      {
        // Add item before this one
        before = actions.at( i );
        break;
      }
      else if ( comp == 0 )
      {
        // Plugin menu item already exists
        return actions.at( i )->menu();
      }
    }
  }

  // It doesn't exist, so create
  QMenu *menu = new QMenu( menuName, this );
  menu->setObjectName( normalizedMenuName( menuName ) );
  if ( before )
    mRasterMenu->insertMenu( before, menu );
  else
    mRasterMenu->addMenu( menu );

  return menu;
}

QMenu* QgsClassicApp::getVectorMenu( QString menuName )
{
#ifdef Q_OS_MAC
  // Mac doesn't have '&' keyboard shortcuts.
  menuName.remove( QChar( '&' ) );
#endif
  QString dst = menuName;
  dst.remove( QChar( '&' ) );

  QAction *before = NULL;
  QList<QAction*> actions = mVectorMenu->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    QString src = actions.at( i )->text();
    src.remove( QChar( '&' ) );

    int comp = dst.localeAwareCompare( src );
    if ( comp < 0 )
    {
      // Add item before this one
      before = actions.at( i );
      break;
    }
    else if ( comp == 0 )
    {
      // Plugin menu item already exists
      return actions.at( i )->menu();
    }
  }
  // It doesn't exist, so create
  QMenu *menu = new QMenu( menuName, this );
  menu->setObjectName( normalizedMenuName( menuName ) );
  if ( before )
    mVectorMenu->insertMenu( before, menu );
  else
    mVectorMenu->addMenu( menu );

  return menu;
}

QMenu* QgsClassicApp::getWebMenu( QString menuName )
{
#ifdef Q_OS_MAC
  // Mac doesn't have '&' keyboard shortcuts.
  menuName.remove( QChar( '&' ) );
#endif
  QString dst = menuName;
  dst.remove( QChar( '&' ) );

  QAction *before = NULL;
  QList<QAction*> actions = mWebMenu->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    QString src = actions.at( i )->text();
    src.remove( QChar( '&' ) );

    int comp = dst.localeAwareCompare( src );
    if ( comp < 0 )
    {
      // Add item before this one
      before = actions.at( i );
      break;
    }
    else if ( comp == 0 )
    {
      // Plugin menu item already exists
      return actions.at( i )->menu();
    }
  }
  // It doesn't exist, so create
  QMenu *menu = new QMenu( menuName, this );
  menu->setObjectName( normalizedMenuName( menuName ) );
  if ( before )
    mWebMenu->insertMenu( before, menu );
  else
    mWebMenu->addMenu( menu );

  return menu;
}

void QgsClassicApp::insertAddLayerAction( QAction *action )
{
  mAddLayerMenu->insertAction( mActionAddLayerSeparator, action );
}

void QgsClassicApp::removeAddLayerAction( QAction *action )
{
  mAddLayerMenu->removeAction( action );
}

void QgsClassicApp::addPluginToDatabaseMenu( QString name, QAction* action )
{
  QMenu* menu = getDatabaseMenu( name );
  menu->addAction( action );

  // add the Database menu to the menuBar if not added yet
  if ( mDatabaseMenu->actions().count() != 1 )
    return;

  QAction* before = NULL;
  QList<QAction*> actions = menuBar()->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    if ( actions.at( i )->menu() == mDatabaseMenu )
      return;

    // goes before Web menu, if present
    if ( actions.at( i )->menu() == mWebMenu )
    {
      before = actions.at( i );
      break;
    }
  }
  for ( int i = 0; i < actions.count(); i++ )
  {
    // defaults to after Raster menu, which is already in qgisapp.ui
    if ( actions.at( i )->menu() == mRasterMenu )
    {
      if ( !before )
      {
        before = actions.at( i += 1 );
        break;
      }
    }
  }
  if ( before )
    menuBar()->insertMenu( before, mDatabaseMenu );
  else
    // fallback insert
    menuBar()->insertMenu( firstRightStandardMenu()->menuAction(), mDatabaseMenu );
}

void QgsClassicApp::addPluginToRasterMenu( QString name, QAction* action )
{
  QMenu* menu = getRasterMenu( name );
  menu->addAction( action );
}

void QgsClassicApp::addPluginToVectorMenu( QString name, QAction* action )
{
  QMenu* menu = getVectorMenu( name );
  menu->addAction( action );
}

void QgsClassicApp::addPluginToWebMenu( QString name, QAction* action )
{
  QMenu* menu = getWebMenu( name );
  menu->addAction( action );

  // add the Vector menu to the menuBar if not added yet
  if ( mWebMenu->actions().count() != 1 )
    return;

  QAction* before = NULL;
  QList<QAction*> actions = menuBar()->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    // goes after Database menu, if present
    if ( actions.at( i )->menu() == mDatabaseMenu )
    {
      before = actions.at( i += 1 );
      // don't break here
    }

    if ( actions.at( i )->menu() == mWebMenu )
      return;
  }
  for ( int i = 0; i < actions.count(); i++ )
  {
    // defaults to after Raster menu, which is already in qgisapp.ui
    if ( actions.at( i )->menu() == mRasterMenu )
    {
      if ( !before )
      {
        before = actions.at( i += 1 );
        break;
      }
    }
  }

  if ( before )
    menuBar()->insertMenu( before, mWebMenu );
  else
    // fallback insert
    menuBar()->insertMenu( firstRightStandardMenu()->menuAction(), mWebMenu );
}

void QgsClassicApp::removePluginDatabaseMenu( QString name, QAction* action )
{
  QMenu* menu = getDatabaseMenu( name );
  menu->removeAction( action );
  if ( menu->actions().count() == 0 )
  {
    mDatabaseMenu->removeAction( menu->menuAction() );
  }

  // remove the Database menu from the menuBar if there are no more actions
  if ( mDatabaseMenu->actions().count() > 0 )
    return;

  QList<QAction*> actions = menuBar()->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    if ( actions.at( i )->menu() == mDatabaseMenu )
    {
      menuBar()->removeAction( actions.at( i ) );
      return;
    }
  }
}

void QgsClassicApp::removePluginRasterMenu( QString name, QAction* action )
{
  QMenu* menu = getRasterMenu( name );
  menu->removeAction( action );
  if ( menu->actions().count() == 0 )
  {
    mRasterMenu->removeAction( menu->menuAction() );
  }

  // Remove separator above plugins in Raster menu if no plugins remain
  QList<QAction*> actions = mRasterMenu->actions();
  if ( actions.indexOf( mActionRasterSeparator ) + 1 == actions.count() )
  {
    mRasterMenu->removeAction( mActionRasterSeparator );
    mActionRasterSeparator = NULL;
  }
}

void QgsClassicApp::removePluginVectorMenu( QString name, QAction* action )
{
  QMenu* menu = getVectorMenu( name );
  menu->removeAction( action );
  if ( menu->actions().count() == 0 )
  {
    mVectorMenu->removeAction( menu->menuAction() );
  }

  // remove the Vector menu from the menuBar if there are no more actions
  if ( mVectorMenu->actions().count() > 0 )
    return;

  QList<QAction*> actions = menuBar()->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    if ( actions.at( i )->menu() == mVectorMenu )
    {
      menuBar()->removeAction( actions.at( i ) );
      return;
    }
  }
}

void QgsClassicApp::removePluginWebMenu( QString name, QAction* action )
{
  QMenu* menu = getWebMenu( name );
  menu->removeAction( action );
  if ( menu->actions().count() == 0 )
  {
    mWebMenu->removeAction( menu->menuAction() );
  }

  // remove the Web menu from the menuBar if there are no more actions
  if ( mWebMenu->actions().count() > 0 )
    return;

  QList<QAction*> actions = menuBar()->actions();
  for ( int i = 0; i < actions.count(); i++ )
  {
    if ( actions.at( i )->menu() == mWebMenu )
    {
      menuBar()->removeAction( actions.at( i ) );
      return;
    }
  }
}

int QgsClassicApp::addPluginToolBarIcon( QAction * qAction )
{
  mPluginToolBar->addAction( qAction );
  return 0;
}

QAction*QgsClassicApp::addPluginToolBarWidget( QWidget* widget )
{
  return mPluginToolBar->addWidget( widget );
}

void QgsClassicApp::removePluginToolBarIcon( QAction *qAction )
{
  mPluginToolBar->removeAction( qAction );
}

int QgsClassicApp::addRasterToolBarIcon( QAction * qAction )
{
  mRasterToolBar->addAction( qAction );
  return 0;
}

QAction*QgsClassicApp::addRasterToolBarWidget( QWidget* widget )
{
  return mRasterToolBar->addWidget( widget );
}

void QgsClassicApp::removeRasterToolBarIcon( QAction *qAction )
{
  mRasterToolBar->removeAction( qAction );
}

int QgsClassicApp::addVectorToolBarIcon( QAction * qAction )
{
  mVectorToolBar->addAction( qAction );
  return 0;
}

QAction*QgsClassicApp::addVectorToolBarWidget( QWidget* widget )
{
  return mVectorToolBar->addWidget( widget );
}

void QgsClassicApp::removeVectorToolBarIcon( QAction *qAction )
{
  mVectorToolBar->removeAction( qAction );
}

int QgsClassicApp::addDatabaseToolBarIcon( QAction * qAction )
{
  mDatabaseToolBar->addAction( qAction );
  return 0;
}

QAction*QgsClassicApp::addDatabaseToolBarWidget( QWidget* widget )
{
  return mDatabaseToolBar->addWidget( widget );
}

void QgsClassicApp::removeDatabaseToolBarIcon( QAction *qAction )
{
  mDatabaseToolBar->removeAction( qAction );
}

int QgsClassicApp::addWebToolBarIcon( QAction * qAction )
{
  mWebToolBar->addAction( qAction );
  return 0;
}

QAction*QgsClassicApp::addWebToolBarWidget( QWidget* widget )
{
  return mWebToolBar->addWidget( widget );
}

void QgsClassicApp::removeWebToolBarIcon( QAction *qAction )
{
  mWebToolBar->removeAction( qAction );
}

void QgsClassicApp::toolButtonActionTriggered( QAction *action )
{
  QToolButton *bt = qobject_cast<QToolButton *>( sender() );
  if ( !bt )
    return;

  QSettings settings;
  if ( action == mActionSelectFeatures )
    settings.setValue( "/UI/selectTool", 1 );
  else if ( action == mActionSelectRadius )
    settings.setValue( "/UI/selectTool", 2 );
  else if ( action == mActionSelectPolygon )
    settings.setValue( "/UI/selectTool", 3 );
  else if ( action == mActionSelectFreehand )
    settings.setValue( "/UI/selectTool", 4 );
  else if ( action == mActionMeasure )
    settings.setValue( "/UI/measureTool", 0 );
  else if ( action == mActionMeasureArea )
    settings.setValue( "/UI/measureTool", 1 );
  else if ( action == mActionMeasureAngle )
    settings.setValue( "/UI/measureTool", 2 );
  else if ( action == mActionMeasureCircle )
    settings.setValue( "/UI/measureCircle", 3 );
  else if ( action == mActionMeasureHeightProfile )
    settings.setValue( "/UI/measureHeightProfile", 4 );
  else if ( action == mActionTextAnnotation )
    settings.setValue( "/UI/annotationTool", 0 );
  else if ( action == mActionFormAnnotation )
    settings.setValue( "/UI/annotationTool", 1 );
  else if ( action == mActionHtmlAnnotation )
    settings.setValue( "/UI/annotationTool", 2 );
  else if ( action == mActionSvgAnnotation )
    settings.setValue( "UI/annotationTool", 3 );
  else if ( action == mActionPinAnnotation )
    settings.setValue( "/UI/annotationTool", 4 );
  else if ( action == mActionNewSpatiaLiteLayer )
    settings.setValue( "/UI/defaultNewLayer", 0 );
  else if ( action == mActionNewVectorLayer )
    settings.setValue( "/UI/defaultNewLayer", 1 );
  else if ( action == mActionNewMemoryLayer )
    settings.setValue( "/UI/defaultNewLayer", 2 );
  else if ( action == mActionAddWfsLayer )
    settings.setValue( "/UI/defaultFeatureService", 0 );
  else if ( action == mActionAddAfsLayer )
    settings.setValue( "/UI/defaultFeatureService", 1 );
  else if ( action == mActionAddWmsLayer )
    settings.setValue( "/UI/defaultMapService", 0 );
  else if ( action == mActionAddAmsLayer )
    settings.setValue( "/UI/defaultMapService", 1 );
  bt->setDefaultAction( action );
}

void QgsClassicApp::extentsViewToggled( bool theFlag )
{
  if ( theFlag )
  {
    //extents view mode!
    mToggleExtentsViewButton->setIcon( QgsApplication::getThemeIcon( "extents.png" ) );
    mCoordsEdit->setToolTip( tr( "Map coordinates for the current view extents" ) );
    mCoordsEdit->setReadOnly( true );
    showExtents();
  }
  else
  {
    //mouse cursor pos view mode!
    mToggleExtentsViewButton->setIcon( QgsApplication::getThemeIcon( "tracking.png" ) );
    mCoordsEdit->setToolTip( tr( "Map coordinates at mouse cursor position" ) );
    mCoordsEdit->setReadOnly( false );
    mCoordsLabel->setText( tr( "Coordinate:" ) );
  }
}

void QgsClassicApp::showMouseCoordinate( const QgsPoint & p )
{
  if ( mMapTipsVisible )
  {
    // store the point, we need it for when the maptips timer fires
    mLastMapPosition = p;

    // we use this slot to control the timer for maptips since it is fired each time
    // the mouse moves.
    if ( mMapCanvas->underMouse() )
    {
      // Clear the maptip (this is done conditionally)
      mpMaptip->clear( mapCanvas() );
      // don't start the timer if the mouse is not over the map canvas
      mpMapTipsTimer->start();
      //QgsDebugMsg("Started maptips timer");
    }
  }
  if ( mToggleExtentsViewButton->isChecked() )
  {
    //we are in show extents mode so no need to do anything
    return;
  }
  else
  {
    if ( mMapCanvas->mapUnits() == QGis::Degrees )
    {
      if ( !mMapCanvas->mapSettings().destinationCrs().isValid() )
        return;

      QgsPoint geo = p;
      if ( !mMapCanvas->mapSettings().destinationCrs().geographicFlag() )
      {
        QgsCoordinateTransform ct( mMapCanvas->mapSettings().destinationCrs(), QgsCoordinateReferenceSystem( GEOSRID ) );
        geo = ct.transform( p );
      }
      QString format = QgsProject::instance()->readEntry( "PositionPrecision", "/DegreeFormat", "D" );

      if ( format == "DM" )
        mCoordsEdit->setText( geo.toDegreesMinutes( mMousePrecisionDecimalPlaces ) );
      else if ( format == "DMS" )
        mCoordsEdit->setText( geo.toDegreesMinutesSeconds( mMousePrecisionDecimalPlaces ) );
      else
        mCoordsEdit->setText( geo.toString( mMousePrecisionDecimalPlaces ) );
    }
    else
    {
      mCoordsEdit->setText( p.toString( mMousePrecisionDecimalPlaces ) );
    }

    if ( mCoordsEdit->width() > mCoordsEdit->minimumWidth() )
    {
      mCoordsEdit->setMinimumWidth( mCoordsEdit->width() );
    }
  }
}


void QgsClassicApp::showScale( double theScale )
{
  mScaleEdit->setScale( 1.0 / theScale );

  // Not sure if the lines below do anything meaningful /Homann
  if ( mScaleEdit->width() > mScaleEdit->minimumWidth() )
  {
    mScaleEdit->setMinimumWidth( mScaleEdit->width() );
  }
}

void QgsClassicApp::userScale()
{
  mMapCanvas->zoomScale( 1.0 / mScaleEdit->scale() );
}

void QgsClassicApp::userCenter()
{
  if ( mCoordsEdit->text() == "dizzy" )
  {
    // sometimes you may feel a bit dizzy...
    if ( mDizzyTimer->isActive() )
    {
      mDizzyTimer->stop();
      mMapCanvas->setSceneRect( mMapCanvas->viewport()->rect() );
      mMapCanvas->setTransform( QTransform() );
    }
    else
      mDizzyTimer->start( 100 );
  }
  else if ( mCoordsEdit->text() == "retro" )
  {
    mMapCanvas->setProperty( "retro", !mMapCanvas->property( "retro" ).toBool() );
    refreshMapCanvas();
  }

  QStringList parts = mCoordsEdit->text().split( ',' );
  if ( parts.size() != 2 )
    return;

  bool xOk;
  double x = parts.at( 0 ).toDouble( &xOk );
  if ( !xOk )
    return;

  bool yOk;
  double y = parts.at( 1 ).toDouble( &yOk );
  if ( !yOk )
    return;

  mMapCanvas->setCenter( QgsPoint( x, y ) );
  mMapCanvas->refresh();
}

void QgsClassicApp::userRotation()
{
  double degrees = mRotationEdit->value();
  mMapCanvas->setRotation( degrees );
  mMapCanvas->refresh();
}

void QgsClassicApp::showExtents()
{
  // allow symbols in the legend update their preview if they use map units
  layerTreeView()->layerTreeModel()->setLegendMapViewData( mapCanvas()->mapUnitsPerPixel(), mapCanvas()->mapSettings().outputDpi(), mapCanvas()->scale() );

  if ( !mToggleExtentsViewButton->isChecked() )
  {
    return;
  }

  // update the statusbar with the current extents.
  QgsRectangle myExtents = mapCanvas()->extent();
  mCoordsLabel->setText( tr( "Extents:" ) );
  mCoordsEdit->setText( myExtents.toString( true ) );
  //ensure the label is big enough
  if ( mCoordsEdit->width() > mCoordsEdit->minimumWidth() )
  {
    mCoordsEdit->setMinimumWidth( mCoordsEdit->width() );
  }
}

void QgsClassicApp::showRotation()
{
  // update the statusbar with the current rotation.
  double myrotation = mapCanvas()->rotation();
  mRotationEdit->setValue( myrotation );
}

// slot to update the progress bar in the status bar
void QgsClassicApp::showProgress( int theProgress, int theTotalSteps )
{
  if ( theProgress == theTotalSteps )
  {
    mProgressBar->reset();
    mProgressBar->hide();
  }
  else
  {
    //only call show if not already hidden to reduce flicker
    if ( !mProgressBar->isVisible() )
    {
      mProgressBar->show();
    }
    mProgressBar->setMaximum( theTotalSteps );
    mProgressBar->setValue( theProgress );

    if ( mProgressBar->maximum() == 0 )
    {
      // for busy indicator (when minimum equals to maximum) the oxygen Qt style (used in KDE)
      // has some issues and does not start busy indicator animation. This is an ugly fix
      // that forces creation of a temporary progress bar that somehow resumes the animations.
      // Caution: looking at the code below may introduce mild pain in stomach.
      if ( strcmp( QApplication::style()->metaObject()->className(), "Oxygen::Style" ) == 0 )
      {
        QProgressBar pb;
        pb.setAttribute( Qt::WA_DontShowOnScreen ); // no visual annoyance
        pb.setMaximum( 0 );
        pb.show();
        qApp->processEvents();
      }
    }

  }
}

void QgsClassicApp::updateCRSStatusBar()
{
  mOnTheFlyProjectionStatusButton->setText( mapCanvas()->mapSettings().destinationCrs().authid() );

  if ( mapCanvas()->mapSettings().hasCrsTransformEnabled() )
  {
    mOnTheFlyProjectionStatusButton->setText( tr( "%1 (OTF)" ).arg( mOnTheFlyProjectionStatusButton->text() ) );
    mOnTheFlyProjectionStatusButton->setToolTip(
      tr( "Current CRS: %1 (OTFR enabled)" ).arg( mapCanvas()->mapSettings().destinationCrs().description() ) );
    mOnTheFlyProjectionStatusButton->setIcon( QgsApplication::getThemeIcon( "mIconProjectionEnabled.png" ) );
  }
  else
  {
    mOnTheFlyProjectionStatusButton->setToolTip(
      tr( "Current CRS: %1 (OTFR disabled)" ).arg( mapCanvas()->mapSettings().destinationCrs().description() ) );
    mOnTheFlyProjectionStatusButton->setIcon( QgsApplication::getThemeIcon( "mIconProjectionDisabled.png" ) );
  }
  emit coordinateDisplayFormatChanged( QgsCoordinateUtils::EPSG, mapCanvas()->mapSettings().destinationCrs().authid() );
}

void QgsClassicApp::updateUndoActions()
{
  bool canUndo = false, canRedo = false;
  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( vlayer && vlayer->isEditable() )
  {
    canUndo = vlayer->undoStack()->canUndo();
    canRedo = vlayer->undoStack()->canRedo();
  }
  mActionUndo->setEnabled( canUndo );
  mActionRedo->setEnabled( canRedo );
}

void QgsClassicApp::legendLayerSelectionChanged( void )
{
  QList<QgsLayerTreeLayer*> selectedLayers = mLayerTreeView ? layerTreeView()->selectedLayerNodes() : QList<QgsLayerTreeLayer*>();

  mActionDuplicateLayer->setEnabled( selectedLayers.count() > 0 );
  mActionSetLayerScaleVisibility->setEnabled( selectedLayers.count() > 0 );
  mActionSetLayerCRS->setEnabled( selectedLayers.count() > 0 );
  mActionSetProjectCRSFromLayer->setEnabled( selectedLayers.count() == 1 );

  mActionSaveEdits->setEnabled( QgsLayerTreeUtils::layersModified( selectedLayers ) );
  mActionRollbackEdits->setEnabled( QgsLayerTreeUtils::layersModified( selectedLayers ) );
  mActionCancelEdits->setEnabled( QgsLayerTreeUtils::layersEditable( selectedLayers ) );
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  bool editable = vlayer && ( vlayer->dataProvider()->capabilities() & QgsVectorDataProvider::EditingCapabilities );
  mActionToggleEditing->setEnabled( editable );
}


void QgsClassicApp::refreshActionFeatureAction()
{
  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );

  if ( !vlayer )
  {
    return;
  }

  bool layerHasActions = vlayer->actions()->size() + QgsMapLayerActionRegistry::instance()->mapLayerActions( vlayer ).size() > 0;
  mActionFeatureAction->setEnabled( layerHasActions );
}

void QgsClassicApp::clipboardDataChanged()
{
  // Enables the paste menu element
  mActionPasteStyle->setEnabled( clipboard()->hasFormat( QGSCLIPBOARD_STYLE_MIME ) );
}

void QgsClassicApp::activateDeactivateLayerRelatedActions( QgsMapLayer* layer )
{
  bool enableMove = false, enableRotate = false, enablePin = false, enableShowHide = false, enableChange = false;

  QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
  for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
  {
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( it.value() );
    if ( !vlayer || !vlayer->isEditable() ||
         ( !vlayer->diagramRenderer() && vlayer->customProperty( "labeling" ).toString() != QString( "pal" ) ) )
      continue;

    int colX, colY, colShow, colAng;
    enablePin =
      enablePin ||
      ( qobject_cast<QgsMapToolPinLabels*>( mMapTools.mPinLabels ) &&
        qobject_cast<QgsMapToolPinLabels*>( mMapTools.mPinLabels )->layerCanPin( vlayer, colX, colY ) );

    enableShowHide =
      enableShowHide ||
      ( qobject_cast<QgsMapToolShowHideLabels*>( mMapTools.mShowHideLabels ) &&
        qobject_cast<QgsMapToolShowHideLabels*>( mMapTools.mShowHideLabels )->layerCanShowHide( vlayer, colShow ) );

    enableMove =
      enableMove ||
      ( qobject_cast<QgsMapToolMoveLabel*>( mMapTools.mMoveLabel ) &&
        ( qobject_cast<QgsMapToolMoveLabel*>( mMapTools.mMoveLabel )->labelMoveable( vlayer, colX, colY )
          || qobject_cast<QgsMapToolMoveLabel*>( mMapTools.mMoveLabel )->diagramMoveable( vlayer, colX, colY ) )
      );

    enableRotate =
      enableRotate ||
      ( qobject_cast<QgsMapToolRotateLabel*>( mMapTools.mRotateLabel ) &&
        qobject_cast<QgsMapToolRotateLabel*>( mMapTools.mRotateLabel )->layerIsRotatable( vlayer, colAng ) );

    enableChange = true;

    if ( enablePin && enableShowHide && enableMove && enableRotate && enableChange )
      break;
  }

  mActionPinLabels->setEnabled( enablePin );
  mActionShowHideLabels->setEnabled( enableShowHide );
  mActionMoveLabel->setEnabled( enableMove );
  mActionRotateLabel->setEnabled( enableRotate );
  mActionChangeLabelProperties->setEnabled( enableChange );

  mMenuPasteAs->setEnabled( clipboard() && !clipboard()->empty() );
  mActionPasteAsNewVector->setEnabled( clipboard() && !clipboard()->empty() );
  mActionPasteAsNewMemoryVector->setEnabled( clipboard() && !clipboard()->empty() );

  updateLayerModifiedActions();

  if ( !layer )
  {
    mActionSelectFeatures->setEnabled( false );
    mActionSelectPolygon->setEnabled( false );
    mActionSelectFreehand->setEnabled( false );
    mActionSelectRadius->setEnabled( false );
    mActionIdentify->setEnabled( QSettings().value( "/Map/identifyMode", 0 ).toInt() != 0 );
    mActionSelectByExpression->setEnabled( false );
    mActionLabeling->setEnabled( false );
    mActionOpenTable->setEnabled( false );
    mActionOpenFieldCalc->setEnabled( false );
    mActionToggleEditing->setEnabled( false );
    mActionToggleEditing->setChecked( false );
    mActionSaveLayerEdits->setEnabled( false );
    mActionSaveLayerDefinition->setEnabled( false );
    mActionLayerSaveAs->setEnabled( false );
    mActionLayerProperties->setEnabled( false );
    mActionLayerSubsetString->setEnabled( false );
    mActionAddToOverview->setEnabled( false );
    mActionFeatureAction->setEnabled( false );
    mActionAddFeature->setEnabled( false );
    mActionCircularStringCurvePoint->setEnabled( false );
    mActionCircularStringRadius->setEnabled( false );
    mActionMoveFeature->setEnabled( false );
    mActionRotateFeature->setEnabled( false );
    mActionOffsetCurve->setEnabled( false );
    mActionNodeTool->setEnabled( false );
    mActionDeleteSelected->setEnabled( false );
    mActionCutFeatures->setEnabled( false );
    mActionCopyFeatures->setEnabled( false );
    mActionPasteFeatures->setEnabled( false );
    mActionCopyStyle->setEnabled( false );
    mActionPasteStyle->setEnabled( false );

    mUndoWidget->dockContents()->setEnabled( false );
    mActionUndo->setEnabled( false );
    mActionRedo->setEnabled( false );
    mActionSimplifyFeature->setEnabled( false );
    mActionAddRing->setEnabled( false );
    mActionFillRing->setEnabled( false );
    mActionAddPart->setEnabled( false );
    mActionDeleteRing->setEnabled( false );
    mActionDeletePart->setEnabled( false );
    mActionReshapeFeatures->setEnabled( false );
    mActionOffsetCurve->setEnabled( false );
    mActionSplitFeatures->setEnabled( false );
    mActionSplitParts->setEnabled( false );
    mActionMergeFeatures->setEnabled( false );
    mActionMergeFeatureAttributes->setEnabled( false );
    mActionRotatePointSymbols->setEnabled( false );

    mActionPinLabels->setEnabled( false );
    mActionShowHideLabels->setEnabled( false );
    mActionMoveLabel->setEnabled( false );
    mActionRotateLabel->setEnabled( false );
    mActionChangeLabelProperties->setEnabled( false );

    mActionLocalHistogramStretch->setEnabled( false );
    mActionFullHistogramStretch->setEnabled( false );
    mActionLocalCumulativeCutStretch->setEnabled( false );
    mActionFullCumulativeCutStretch->setEnabled( false );
    mActionIncreaseBrightness->setEnabled( false );
    mActionDecreaseBrightness->setEnabled( false );
    mActionIncreaseContrast->setEnabled( false );
    mActionDecreaseContrast->setEnabled( false );
    mActionZoomActualSize->setEnabled( false );
    mActionZoomToLayer->setEnabled( false );
    return;
  }

  mActionLayerProperties->setEnabled( QgsProject::instance()->layerIsEmbedded( layer->id() ).isEmpty() );
  mActionAddToOverview->setEnabled( true );
  mActionZoomToLayer->setEnabled( true );

  mActionCopyStyle->setEnabled( true );
  mActionPasteStyle->setEnabled( clipboard()->hasFormat( QGSCLIPBOARD_STYLE_MIME ) );

  /***********Vector layers****************/
  if ( layer->type() == QgsMapLayer::VectorLayer )
  {
    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( layer );
    QgsVectorDataProvider* dprovider = vlayer->dataProvider();

    bool isEditable = vlayer->isEditable();
    bool layerHasSelection = vlayer->selectedFeatureCount() > 0;
    bool layerHasActions = vlayer->actions()->size() + QgsMapLayerActionRegistry::instance()->mapLayerActions( vlayer ).size() > 0;

    mActionLocalHistogramStretch->setEnabled( false );
    mActionFullHistogramStretch->setEnabled( false );
    mActionLocalCumulativeCutStretch->setEnabled( false );
    mActionFullCumulativeCutStretch->setEnabled( false );
    mActionIncreaseBrightness->setEnabled( false );
    mActionDecreaseBrightness->setEnabled( false );
    mActionIncreaseContrast->setEnabled( false );
    mActionDecreaseContrast->setEnabled( false );
    mActionZoomActualSize->setEnabled( false );
    mActionLabeling->setEnabled( true );

    mActionSelectFeatures->setEnabled( true );
    mActionSelectPolygon->setEnabled( true );
    mActionSelectFreehand->setEnabled( true );
    mActionSelectRadius->setEnabled( true );
    mActionIdentify->setEnabled( true );
    mActionSelectByExpression->setEnabled( true );
    mActionOpenTable->setEnabled( true );
    mActionSaveLayerDefinition->setEnabled( true );
    mActionLayerSaveAs->setEnabled( true );
    mActionCopyFeatures->setEnabled( layerHasSelection );
    mActionFeatureAction->setEnabled( layerHasActions );

    if ( !isEditable && mMapCanvas->mapTool()
         && mMapCanvas->mapTool()->isEditTool() && !mSaveRollbackInProgress )
    {
      mMapCanvas->setMapTool( mNonEditMapTool );
    }

    if ( dprovider )
    {
      bool canChangeAttributes = dprovider->capabilities() & QgsVectorDataProvider::ChangeAttributeValues;
      bool canDeleteFeatures = dprovider->capabilities() & QgsVectorDataProvider::DeleteFeatures;
      bool canAddFeatures = dprovider->capabilities() & QgsVectorDataProvider::AddFeatures;
      bool canSupportEditing = dprovider->capabilities() & QgsVectorDataProvider::EditingCapabilities;
      bool canChangeGeometry = dprovider->capabilities() & QgsVectorDataProvider::ChangeGeometries;

      mActionLayerSubsetString->setEnabled( !isEditable && dprovider->supportsSubsetString() );

      mActionToggleEditing->setEnabled( canSupportEditing && !vlayer->isReadOnly() );
      mActionToggleEditing->setChecked( canSupportEditing && isEditable );
      mActionSaveLayerEdits->setEnabled( canSupportEditing && isEditable && vlayer->isModified() );
      mUndoWidget->dockContents()->setEnabled( canSupportEditing && isEditable );
      mActionUndo->setEnabled( canSupportEditing );
      mActionRedo->setEnabled( canSupportEditing );

      //start editing/stop editing
      if ( canSupportEditing )
      {
        updateUndoActions();
      }

      mActionPasteFeatures->setEnabled( isEditable && canAddFeatures && !clipboard()->empty() );

      mActionAddFeature->setEnabled( isEditable && canAddFeatures );
      mActionCircularStringCurvePoint->setEnabled( isEditable && ( canAddFeatures || canChangeGeometry ) && vlayer->geometryType() != QGis::Point );
      mActionCircularStringRadius->setEnabled( isEditable && ( canAddFeatures || canChangeGeometry ) );

      //does provider allow deleting of features?
      mActionDeleteSelected->setEnabled( isEditable && canDeleteFeatures && layerHasSelection );
      mActionCutFeatures->setEnabled( isEditable && canDeleteFeatures && layerHasSelection );

      //merge tool needs editable layer and provider with the capability of adding and deleting features
      if ( isEditable && canChangeAttributes )
      {
        mActionMergeFeatures->setEnabled( layerHasSelection && canDeleteFeatures && canAddFeatures );
        mActionMergeFeatureAttributes->setEnabled( layerHasSelection );
      }
      else
      {
        mActionMergeFeatures->setEnabled( false );
        mActionMergeFeatureAttributes->setEnabled( false );
      }

      bool isMultiPart = QGis::isMultiType( vlayer->wkbType() ) || !dprovider->doesStrictFeatureTypeCheck();

      // moving enabled if geometry changes are supported
      mActionAddPart->setEnabled( isEditable && canChangeGeometry );
      mActionDeletePart->setEnabled( isEditable && canChangeGeometry );
      mActionMoveFeature->setEnabled( isEditable && canChangeGeometry );
      mActionRotateFeature->setEnabled( isEditable && canChangeGeometry );
      mActionNodeTool->setEnabled( isEditable && canChangeGeometry );

      if ( vlayer->geometryType() == QGis::Point )
      {
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionCapturePoint.png" ) );

        mActionAddRing->setEnabled( false );
        mActionFillRing->setEnabled( false );
        mActionReshapeFeatures->setEnabled( false );
        mActionSplitFeatures->setEnabled( false );
        mActionSplitParts->setEnabled( false );
        mActionSimplifyFeature->setEnabled( false );
        mActionDeleteRing->setEnabled( false );
        mActionRotatePointSymbols->setEnabled( false );
        mActionOffsetCurve->setEnabled( false );

        if ( isEditable && canChangeAttributes )
        {
          if ( QgsMapToolRotatePointSymbols::layerIsRotatable( vlayer ) )
          {
            mActionRotatePointSymbols->setEnabled( true );
          }
        }
      }
      else if ( vlayer->geometryType() == QGis::Line )
      {
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionCaptureLine.png" ) );

        mActionReshapeFeatures->setEnabled( isEditable && canAddFeatures );
        mActionSplitFeatures->setEnabled( isEditable && canAddFeatures );
        mActionSplitParts->setEnabled( isEditable && canChangeGeometry && isMultiPart );
        mActionSimplifyFeature->setEnabled( isEditable && canAddFeatures );
        mActionOffsetCurve->setEnabled( isEditable && canAddFeatures && canChangeAttributes );

        mActionAddRing->setEnabled( false );
        mActionFillRing->setEnabled( false );
        mActionDeleteRing->setEnabled( false );
      }
      else if ( vlayer->geometryType() == QGis::Polygon )
      {
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionCapturePolygon.png" ) );

        mActionAddRing->setEnabled( isEditable && canChangeGeometry );
        mActionFillRing->setEnabled( isEditable && canChangeGeometry );
        mActionReshapeFeatures->setEnabled( isEditable && canChangeGeometry );
        mActionSplitFeatures->setEnabled( isEditable && canAddFeatures );
        mActionSplitParts->setEnabled( isEditable && canChangeGeometry && isMultiPart );
        mActionSimplifyFeature->setEnabled( isEditable && canChangeGeometry );
        mActionDeleteRing->setEnabled( isEditable && canChangeGeometry );
        mActionOffsetCurve->setEnabled( false );
      }
      else if ( vlayer->geometryType() == QGis::NoGeometry )
      {
        mActionAddFeature->setIcon( QgsApplication::getThemeIcon( "/mActionNewTableRow.png" ) );
      }

      mActionOpenFieldCalc->setEnabled( true );

      return;
    }
    else
    {
      mUndoWidget->dockContents()->setEnabled( false );
      mActionUndo->setEnabled( false );
      mActionRedo->setEnabled( false );
    }

    mActionLayerSubsetString->setEnabled( false );
  } //end vector layer block
  /*************Raster layers*************/
  else if ( layer->type() == QgsMapLayer::RasterLayer )
  {
    const QgsRasterLayer *rlayer = qobject_cast<const QgsRasterLayer *>( layer );
    if ( rlayer->dataProvider()->dataType( 1 ) != QGis::ARGB32
         && rlayer->dataProvider()->dataType( 1 ) != QGis::ARGB32_Premultiplied )
    {
      if ( rlayer->dataProvider()->capabilities() & QgsRasterDataProvider::Size )
      {
        mActionFullHistogramStretch->setEnabled( true );
      }
      else
      {
        // it would hang up reading the data for WMS for example
        mActionFullHistogramStretch->setEnabled( false );
      }
      mActionLocalHistogramStretch->setEnabled( true );
    }
    else
    {
      mActionLocalHistogramStretch->setEnabled( false );
      mActionFullHistogramStretch->setEnabled( false );
    }

    mActionLocalCumulativeCutStretch->setEnabled( true );
    mActionFullCumulativeCutStretch->setEnabled( true );
    mActionIncreaseBrightness->setEnabled( true );
    mActionDecreaseBrightness->setEnabled( true );
    mActionIncreaseContrast->setEnabled( true );
    mActionDecreaseContrast->setEnabled( true );

    mActionLayerSubsetString->setEnabled( false );
    mActionFeatureAction->setEnabled( false );
    mActionSelectFeatures->setEnabled( false );
    mActionSelectPolygon->setEnabled( false );
    mActionSelectFreehand->setEnabled( false );
    mActionSelectRadius->setEnabled( false );
    mActionZoomActualSize->setEnabled( true );
    mActionOpenTable->setEnabled( false );
    mActionOpenFieldCalc->setEnabled( false );
    mActionToggleEditing->setEnabled( false );
    mActionToggleEditing->setChecked( false );
    mActionSaveLayerEdits->setEnabled( false );
    mUndoWidget->dockContents()->setEnabled( false );
    mActionUndo->setEnabled( false );
    mActionRedo->setEnabled( false );
    mActionSaveLayerDefinition->setEnabled( true );
    mActionLayerSaveAs->setEnabled( true );
    mActionAddFeature->setEnabled( false );
    mActionCircularStringCurvePoint->setEnabled( false );
    mActionCircularStringRadius->setEnabled( false );
    mActionDeleteSelected->setEnabled( false );
    mActionAddRing->setEnabled( false );
    mActionFillRing->setEnabled( false );
    mActionAddPart->setEnabled( false );
    mActionNodeTool->setEnabled( false );
    mActionMoveFeature->setEnabled( false );
    mActionRotateFeature->setEnabled( false );
    mActionOffsetCurve->setEnabled( false );
    mActionCopyFeatures->setEnabled( false );
    mActionCutFeatures->setEnabled( false );
    mActionPasteFeatures->setEnabled( false );
    mActionRotatePointSymbols->setEnabled( false );
    mActionDeletePart->setEnabled( false );
    mActionDeleteRing->setEnabled( false );
    mActionSimplifyFeature->setEnabled( false );
    mActionReshapeFeatures->setEnabled( false );
    mActionSplitFeatures->setEnabled( false );
    mActionSplitParts->setEnabled( false );
    mActionLabeling->setEnabled( false );

    //NOTE: This check does not really add any protection, as it is called on load not on layer select/activate
    //If you load a layer with a provider and idenitfy ability then load another without, the tool would be disabled for both

    //Enable the Identify tool ( GDAL datasets draw without a provider )
    //but turn off if data provider exists and has no Identify capabilities
    mActionIdentify->setEnabled( true );

    QSettings settings;
    int identifyMode = settings.value( "/Map/identifyMode", 0 ).toInt();
    if ( identifyMode == 0 )
    {
      const QgsRasterLayer *rlayer = qobject_cast<const QgsRasterLayer *>( layer );
      const QgsRasterDataProvider* dprovider = rlayer->dataProvider();
      if ( dprovider )
      {
        // does provider allow the identify map tool?
        if ( dprovider->capabilities() & QgsRasterDataProvider::Identify )
        {
          mActionIdentify->setEnabled( true );
        }
        else
        {
          mActionIdentify->setEnabled( false );
        }
      }
    }
  }
}

void QgsClassicApp::updateLayerModifiedActions()
{
  bool enableSaveLayerEdits = false;
  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( vlayer )
  {
    QgsVectorDataProvider* dprovider = vlayer->dataProvider();
    if ( dprovider )
    {
      enableSaveLayerEdits = ( dprovider->capabilities() & QgsVectorDataProvider::ChangeAttributeValues
                               && vlayer->isEditable()
                               && vlayer->isModified() );
    }
  }
  mActionSaveLayerEdits->setEnabled( enableSaveLayerEdits );

  QList<QgsLayerTreeLayer*> selectedLayerNodes = layerTreeView() ? layerTreeView()->selectedLayerNodes() : QList<QgsLayerTreeLayer*>();

  mActionSaveEdits->setEnabled( QgsLayerTreeUtils::layersModified( selectedLayerNodes ) );
  mActionRollbackEdits->setEnabled( QgsLayerTreeUtils::layersModified( selectedLayerNodes ) );
  mActionCancelEdits->setEnabled( QgsLayerTreeUtils::layersEditable( selectedLayerNodes ) );

  bool hasEditLayers = ( editableLayers().count() > 0 );
  mActionAllEdits->setEnabled( hasEditLayers );
  mActionCancelAllEdits->setEnabled( hasEditLayers );

  bool hasModifiedLayers = ( editableLayers( true ).count() > 0 );
  mActionSaveAllEdits->setEnabled( hasModifiedLayers );
  mActionRollbackAllEdits->setEnabled( hasModifiedLayers );
}
