/***************************************************************************
                         qgscomposer.cpp  -  description
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgscomposer.h"

#include <stdexcept>

#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgsbusyindicatordialog.h"
#include "qgscomposerruler.h"
#include "qgscomposerview.h"
#include "qgscomposition.h"
#include "qgscompositionwidget.h"
#include "qgscomposermodel.h"
#include "qgsatlascompositionwidget.h"
#include "qgscomposerarrow.h"
#include "qgscomposerarrowwidget.h"
#include "qgscomposerattributetablewidget.h"
#include "qgscomposerframe.h"
#include "qgscomposerhtml.h"
#include "qgscomposerhtmlwidget.h"
#include "qgscomposerlabel.h"
#include "qgscomposerlabelwidget.h"
#include "qgscomposerlegend.h"
#include "qgscomposerlegendwidget.h"
#include "qgscomposermap.h"
#include "qgsatlascomposition.h"
#include "qgscomposermapwidget.h"
#include "qgscomposerpicture.h"
#include "qgscomposerpicturewidget.h"
#include "qgscomposerscalebar.h"
#include "qgscomposerscalebarwidget.h"
#include "qgscomposershape.h"
#include "qgscomposershapewidget.h"
#include "qgscomposerattributetable.h"
#include "qgscomposerattributetablev2.h"
#include "qgscomposertablewidget.h"
#include "qgsexception.h"
#include "qgslogger.h"
#include "qgsproject.h"
#include "qgsmapcanvas.h"
#include "qgsmessageviewer.h"
#include "qgscontexthelp.h"
#include "qgscursors.h"
#include "qgsmaplayeractionregistry.h"
#include "qgsgeometry.h"
#include "qgspaperitem.h"
#include "qgsmaplayerregistry.h"
#include "qgsprevieweffect.h"
#include "ui_qgssvgexportoptions.h"

#include <QCloseEvent>
#include <QCheckBox>
#include <QDesktopWidget>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QImageWriter>
#include <QLabel>
#include <QMatrix>
#include <QMenuBar>
#include <QMessageBox>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPixmap>
#include <QPrintDialog>
#include <QSettings>
#include <QSizeGrip>
#include <QSvgGenerator>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUndoView>
#include <QPaintEngine>
#include <QProgressBar>
#include <QProgressDialog>
#include <QShortcut>

#ifdef ENABLE_MODELTEST
#include "modeltest.h"
#endif

// sort function for QList<QAction*>, e.g. menu listings
static bool cmpByText_( QAction* a, QAction* b )
{
  return QString::localeAwareCompare( a->text(), b->text() ) < 0;
}

QgsComposer::QgsComposer( QgisApp *qgis, const QString& title )
    : QMainWindow()
    , mTitle( title )
    , mQgis( qgis )
    , mUndoView( 0 )
    , mAtlasFeatureAction( 0 )
{
  setupUi( this );
  setWindowTitle( mTitle );
  setupTheme();

  QSettings settings;
  setStyleSheet( mQgis->styleSheet() );

  int size = settings.value( "/IconSize", QGIS_ICON_SIZE ).toInt();
  setIconSize( QSize( size, size ) );

  QToolButton* orderingToolButton = new QToolButton( this );
  orderingToolButton->setPopupMode( QToolButton::InstantPopup );
  orderingToolButton->setAutoRaise( true );
  orderingToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  orderingToolButton->addAction( mActionRaiseItems );
  orderingToolButton->addAction( mActionLowerItems );
  orderingToolButton->addAction( mActionMoveItemsToTop );
  orderingToolButton->addAction( mActionMoveItemsToBottom );
  orderingToolButton->setDefaultAction( mActionRaiseItems );
  mItemActionToolbar->addWidget( orderingToolButton );

  QToolButton* alignToolButton = new QToolButton( this );
  alignToolButton->setPopupMode( QToolButton::InstantPopup );
  alignToolButton->setAutoRaise( true );
  alignToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );

  alignToolButton->addAction( mActionAlignLeft );
  alignToolButton->addAction( mActionAlignHCenter );
  alignToolButton->addAction( mActionAlignRight );
  alignToolButton->addAction( mActionAlignTop );
  alignToolButton->addAction( mActionAlignVCenter );
  alignToolButton->addAction( mActionAlignBottom );
  alignToolButton->setDefaultAction( mActionAlignLeft );
  mItemActionToolbar->addWidget( alignToolButton );

  QToolButton* shapeToolButton = new QToolButton( mItemToolbar );
  shapeToolButton->setCheckable( true );
  shapeToolButton->setPopupMode( QToolButton::InstantPopup );
  shapeToolButton->setAutoRaise( true );
  shapeToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  shapeToolButton->addAction( mActionAddRectangle );
  shapeToolButton->addAction( mActionAddTriangle );
  shapeToolButton->addAction( mActionAddEllipse );
  shapeToolButton->setDefaultAction( mActionAddEllipse );
  mItemToolbar->insertWidget( mActionAddArrow, shapeToolButton );

  QActionGroup* toggleActionGroup = new QActionGroup( this );
  toggleActionGroup->addAction( mActionMoveItemContent );
  toggleActionGroup->addAction( mActionPan );
  toggleActionGroup->addAction( mActionMouseZoom );
  toggleActionGroup->addAction( mActionAddNewMap );
  toggleActionGroup->addAction( mActionAddNewLabel );
  toggleActionGroup->addAction( mActionAddNewLegend );
  toggleActionGroup->addAction( mActionAddNewScalebar );
  toggleActionGroup->addAction( mActionAddImage );
  toggleActionGroup->addAction( mActionSelectMoveItem );
  toggleActionGroup->addAction( mActionAddRectangle );
  toggleActionGroup->addAction( mActionAddTriangle );
  toggleActionGroup->addAction( mActionAddEllipse );
  toggleActionGroup->addAction( mActionAddArrow );
  //toggleActionGroup->addAction( mActionAddTable );
  toggleActionGroup->addAction( mActionAddAttributeTable );
  toggleActionGroup->addAction( mActionAddHtml );
  toggleActionGroup->setExclusive( true );

  mActionAddNewMap->setCheckable( true );
  mActionAddNewLabel->setCheckable( true );
  mActionAddNewLegend->setCheckable( true );
  mActionSelectMoveItem->setCheckable( true );
  mActionAddNewScalebar->setCheckable( true );
  mActionAddImage->setCheckable( true );
  mActionMoveItemContent->setCheckable( true );
  mActionPan->setCheckable( true );
  mActionMouseZoom->setCheckable( true );
  mActionAddArrow->setCheckable( true );
  mActionAddHtml->setCheckable( true );

  mActionShowGrid->setCheckable( true );
  mActionSnapGrid->setCheckable( true );
  mActionShowGuides->setCheckable( true );
  mActionSnapGuides->setCheckable( true );
  mActionSmartGuides->setCheckable( true );
  mActionShowRulers->setCheckable( true );
  mActionShowBoxes->setCheckable( true );

  mActionAtlasPreview->setCheckable( true );

#ifdef Q_OS_MAC
  mActionQuit->setText( tr( "Close" ) );
  mActionQuit->setShortcut( QKeySequence::Close );
  QMenu *appMenu = menuBar()->addMenu( tr( "QGIS" ) );
  appMenu->addAction( mQgis->actionAbout() );
  appMenu->addAction( mQgis->actionOptions() );
#endif

  QMenu *composerMenu = menuBar()->addMenu( tr( "&Composer" ) );
  composerMenu->addAction( mActionSaveProject );
  composerMenu->addSeparator();
  composerMenu->addAction( mActionNewComposer );
  composerMenu->addAction( mActionDuplicateComposer );
  composerMenu->addAction( mActionComposerManager );

  mPrintComposersMenu = new QMenu( tr( "Print &Composers" ), this );
  mPrintComposersMenu->setObjectName( "mPrintComposersMenu" );
  connect( mPrintComposersMenu, SIGNAL( aboutToShow() ), this, SLOT( populatePrintComposersMenu() ) );
  composerMenu->addMenu( mPrintComposersMenu );

  composerMenu->addSeparator();
  composerMenu->addAction( mActionLoadFromTemplate );
  composerMenu->addAction( mActionSaveAsTemplate );
  composerMenu->addSeparator();
  composerMenu->addAction( mActionExportAsImage );
  composerMenu->addAction( mActionExportAsPDF );
  composerMenu->addAction( mActionExportAsSVG );
  composerMenu->addSeparator();
  composerMenu->addAction( mActionPageSetup );
  composerMenu->addAction( mActionPrint );
  composerMenu->addSeparator();
  composerMenu->addAction( mActionQuit );
  connect( mActionQuit, SIGNAL( triggered() ), this, SLOT( close() ) );

  //cut/copy/paste actions. Note these are not included in the ui file
  //as ui files have no support for QKeySequence shortcuts
  mActionCut = new QAction( tr( "Cu&t" ), this );
  mActionCut->setShortcuts( QKeySequence::Cut );
  mActionCut->setStatusTip( tr( "Cut" ) );
  mActionCut->setIcon( QgsApplication::getThemeIcon( "/mActionEditCut.png" ) );
  connect( mActionCut, SIGNAL( triggered() ), this, SLOT( actionCutTriggered() ) );

  mActionCopy = new QAction( tr( "&Copy" ), this );
  mActionCopy->setShortcuts( QKeySequence::Copy );
  mActionCopy->setStatusTip( tr( "Copy" ) );
  mActionCopy->setIcon( QgsApplication::getThemeIcon( "/mActionEditCopy.png" ) );
  connect( mActionCopy, SIGNAL( triggered() ), this, SLOT( actionCopyTriggered() ) );

  mActionPaste = new QAction( tr( "&Paste" ), this );
  mActionPaste->setShortcuts( QKeySequence::Paste );
  mActionPaste->setStatusTip( tr( "Paste" ) );
  mActionPaste->setIcon( QgsApplication::getThemeIcon( "/mActionEditPaste.png" ) );
  connect( mActionPaste, SIGNAL( triggered() ), this, SLOT( actionPasteTriggered() ) );

  QMenu *editMenu = menuBar()->addMenu( tr( "&Edit" ) );
  editMenu->addAction( mActionUndo );
  editMenu->addAction( mActionRedo );
  editMenu->addSeparator();

  //Backspace should also trigger delete selection
  QShortcut* backSpace = new QShortcut( QKeySequence( "Backspace" ), this );
  connect( backSpace, SIGNAL( activated() ), mActionDeleteSelection, SLOT( trigger() ) );
  editMenu->addAction( mActionDeleteSelection );
  editMenu->addSeparator();

  editMenu->addAction( mActionCut );
  editMenu->addAction( mActionCopy );
  editMenu->addAction( mActionPaste );
  //TODO : "Ctrl+Shift+V" is one way to paste in place, but on some platforms you can use Shift+Ins and F18
  editMenu->addAction( mActionPasteInPlace );
  editMenu->addSeparator();
  editMenu->addAction( mActionSelectAll );
  editMenu->addAction( mActionDeselectAll );
  editMenu->addAction( mActionInvertSelection );
  editMenu->addAction( mActionSelectNextBelow );
  editMenu->addAction( mActionSelectNextAbove );

  mActionPreviewModeOff = new QAction( tr( "&Normal" ), this );
  mActionPreviewModeOff->setStatusTip( tr( "Normal" ) );
  mActionPreviewModeOff->setCheckable( true );
  mActionPreviewModeOff->setChecked( true );
  connect( mActionPreviewModeOff, SIGNAL( triggered() ), this, SLOT( disablePreviewMode() ) );
  mActionPreviewModeGrayscale = new QAction( tr( "Simulate Photocopy (&Grayscale)" ), this );
  mActionPreviewModeGrayscale->setStatusTip( tr( "Simulate photocopy (grayscale)" ) );
  mActionPreviewModeGrayscale->setCheckable( true );
  connect( mActionPreviewModeGrayscale, SIGNAL( triggered() ), this, SLOT( activateGrayscalePreview() ) );
  mActionPreviewModeMono = new QAction( tr( "Simulate Fax (&Mono)" ), this );
  mActionPreviewModeMono->setStatusTip( tr( "Simulate fax (mono)" ) );
  mActionPreviewModeMono->setCheckable( true );
  connect( mActionPreviewModeMono, SIGNAL( triggered() ), this, SLOT( activateMonoPreview() ) );
  mActionPreviewProtanope = new QAction( tr( "Simulate Color Blindness (&Protanope)" ), this );
  mActionPreviewProtanope->setStatusTip( tr( "Simulate color blindness (Protanope)" ) );
  mActionPreviewProtanope->setCheckable( true );
  connect( mActionPreviewProtanope, SIGNAL( triggered() ), this, SLOT( activateProtanopePreview() ) );
  mActionPreviewDeuteranope = new QAction( tr( "Simulate Color Blindness (&Deuteranope)" ), this );
  mActionPreviewDeuteranope->setStatusTip( tr( "Simulate color blindness (Deuteranope)" ) );
  mActionPreviewDeuteranope->setCheckable( true );
  connect( mActionPreviewDeuteranope, SIGNAL( triggered() ), this, SLOT( activateDeuteranopePreview() ) );

  QActionGroup* mPreviewGroup = new QActionGroup( this );
  mPreviewGroup->setExclusive( true );
  mActionPreviewModeOff->setActionGroup( mPreviewGroup );
  mActionPreviewModeGrayscale->setActionGroup( mPreviewGroup );
  mActionPreviewModeMono->setActionGroup( mPreviewGroup );
  mActionPreviewProtanope->setActionGroup( mPreviewGroup );
  mActionPreviewDeuteranope->setActionGroup( mPreviewGroup );

  QMenu *viewMenu = menuBar()->addMenu( tr( "&View" ) );
  //Ctrl+= should also trigger zoom in
  QShortcut* ctrlEquals = new QShortcut( QKeySequence( "Ctrl+=" ), this );
  connect( ctrlEquals, SIGNAL( activated() ), mActionZoomIn, SLOT( trigger() ) );

#ifndef Q_OS_MAC
  //disabled for OSX - see #10761
  //also see http://qt-project.org/forums/viewthread/3630 QGraphicsEffects are not well supported on OSX
  QMenu *previewMenu = viewMenu->addMenu( "&Preview" );
  previewMenu->addAction( mActionPreviewModeOff );
  previewMenu->addAction( mActionPreviewModeGrayscale );
  previewMenu->addAction( mActionPreviewModeMono );
  previewMenu->addAction( mActionPreviewProtanope );
  previewMenu->addAction( mActionPreviewDeuteranope );
#endif

  viewMenu->addSeparator();
  viewMenu->addAction( mActionZoomIn );
  viewMenu->addAction( mActionZoomOut );
  viewMenu->addAction( mActionZoomAll );
  viewMenu->addAction( mActionZoomActual );
  viewMenu->addSeparator();
  viewMenu->addAction( mActionRefreshView );
  viewMenu->addSeparator();
  viewMenu->addAction( mActionShowGrid );
  viewMenu->addAction( mActionSnapGrid );
  viewMenu->addSeparator();
  viewMenu->addAction( mActionShowGuides );
  viewMenu->addAction( mActionSnapGuides );
  viewMenu->addAction( mActionSmartGuides );
  viewMenu->addAction( mActionClearGuides );
  viewMenu->addSeparator();
  viewMenu->addAction( mActionShowBoxes );
  viewMenu->addAction( mActionShowRulers );

  // Panel and toolbar submenus
  mPanelMenu = new QMenu( tr( "P&anels" ), this );
  mPanelMenu->setObjectName( "mPanelMenu" );
  mToolbarMenu = new QMenu( tr( "&Toolbars" ), this );
  mToolbarMenu->setObjectName( "mToolbarMenu" );
  viewMenu->addSeparator();
  viewMenu->addAction( mActionToggleFullScreen );
  viewMenu->addAction( mActionHidePanels );
  viewMenu->addMenu( mPanelMenu );
  viewMenu->addMenu( mToolbarMenu );
  // toolBar already exists, add other widgets as they are created
  mToolbarMenu->addAction( mComposerToolbar->toggleViewAction() );
  mToolbarMenu->addAction( mPaperNavToolbar->toggleViewAction() );
  mToolbarMenu->addAction( mItemActionToolbar->toggleViewAction() );
  mToolbarMenu->addAction( mItemToolbar->toggleViewAction() );

  QMenu *layoutMenu = menuBar()->addMenu( tr( "&Layout" ) );
  layoutMenu->addAction( mActionAddNewMap );
  layoutMenu->addAction( mActionAddNewLabel );
  layoutMenu->addAction( mActionAddNewScalebar );
  layoutMenu->addAction( mActionAddNewLegend );
  layoutMenu->addAction( mActionAddImage );
  layoutMenu->addAction( mActionAddArrow );
  //layoutMenu->addAction( mActionAddTable );
  layoutMenu->addAction( mActionAddAttributeTable );
  layoutMenu->addAction( mActionAddHtml );
  layoutMenu->addSeparator();
  layoutMenu->addAction( mActionSelectMoveItem );
  layoutMenu->addAction( mActionMoveItemContent );
  layoutMenu->addSeparator();
  layoutMenu->addAction( mActionGroupItems );
  layoutMenu->addAction( mActionUngroupItems );
  layoutMenu->addSeparator();
  layoutMenu->addAction( mActionRaiseItems );
  layoutMenu->addAction( mActionLowerItems );
  layoutMenu->addAction( mActionMoveItemsToTop );
  layoutMenu->addAction( mActionMoveItemsToBottom );
  layoutMenu->addAction( mActionLockItems );
  layoutMenu->addAction( mActionUnlockAll );

  QMenu *atlasMenu = menuBar()->addMenu( tr( "&Atlas" ) );
  atlasMenu->addAction( mActionAtlasPreview );
  atlasMenu->addAction( mActionAtlasFirst );
  atlasMenu->addAction( mActionAtlasPrev );
  atlasMenu->addAction( mActionAtlasNext );
  atlasMenu->addAction( mActionAtlasLast );
  atlasMenu->addSeparator();
  atlasMenu->addAction( mActionPrintAtlas );
  atlasMenu->addAction( mActionExportAtlasAsImage );
  atlasMenu->addAction( mActionExportAtlasAsSVG );
  atlasMenu->addAction( mActionExportAtlasAsPDF );
  atlasMenu->addSeparator();
  atlasMenu->addAction( mActionAtlasSettings );

  QToolButton* atlasExportToolButton = new QToolButton( mAtlasToolbar );
  atlasExportToolButton->setPopupMode( QToolButton::InstantPopup );
  atlasExportToolButton->setAutoRaise( true );
  atlasExportToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  atlasExportToolButton->addAction( mActionExportAtlasAsImage );
  atlasExportToolButton->addAction( mActionExportAtlasAsSVG );
  atlasExportToolButton->addAction( mActionExportAtlasAsPDF );
  atlasExportToolButton->setDefaultAction( mActionExportAtlasAsImage );
  mAtlasToolbar->insertWidget( mActionAtlasSettings, atlasExportToolButton );

  QMenu *settingsMenu = menuBar()->addMenu( tr( "&Settings" ) );
  settingsMenu->addAction( mActionOptions );

#ifdef Q_OS_MAC
  // this doesn't work on Mac anymore: menuBar()->addMenu( mQgis->windowMenu() );
  // QgsComposer::populateWithOtherMenu should work recursively with submenus and regardless of Qt version
  mWindowMenu = new QMenu( tr( "Window" ), this );
  mWindowMenu->setObjectName( "mWindowMenu" );
  connect( mWindowMenu, SIGNAL( aboutToShow() ), this, SLOT( populateWindowMenu() ) );
  menuBar()->addMenu( mWindowMenu );

  mHelpMenu = new QMenu( tr( "Help" ), this );
  mHelpMenu->setObjectName( "mHelpMenu" );
  connect( mHelpMenu, SIGNAL( aboutToShow() ), this, SLOT( populateHelpMenu() ) );
  menuBar()->addMenu( mHelpMenu );
#endif

  mFirstTime = true;

  // Create action to select this window
  mWindowAction = new QAction( windowTitle(), this );
  connect( mWindowAction, SIGNAL( triggered() ), this, SLOT( activate() ) );

  QgsDebugMsg( "entered." );

  setMouseTracking( true );
  mViewFrame->setMouseTracking( true );

  mStatusZoomCombo = new QComboBox( mStatusBar );
  mStatusZoomCombo->setEditable( true );
  mStatusZoomCombo->setInsertPolicy( QComboBox::NoInsert );
  mStatusZoomCombo->setCompleter( 0 );
  mStatusZoomCombo->setMinimumWidth( 100 );
  //zoom combo box accepts decimals in the range 1-9999, with an optional decimal point and "%" sign
  QRegExp zoomRx( "\\s*\\d{1,4}(\\.\\d?)?\\s*%?" );
  QValidator *zoomValidator = new QRegExpValidator( zoomRx, mStatusZoomCombo );
  mStatusZoomCombo->lineEdit()->setValidator( zoomValidator );

  //add some nice zoom levels to the zoom combobox
  mStatusZoomLevelsList << 0.125 << 0.25 << 0.5 << 1.0 << 2.0 << 4.0 << 8.0;
  QList<double>::iterator zoom_it;
  for ( zoom_it = mStatusZoomLevelsList.begin(); zoom_it != mStatusZoomLevelsList.end(); ++zoom_it )
  {
    mStatusZoomCombo->insertItem( 0, tr( "%1%" ).arg( *zoom_it * 100.0, 0, 'f', 1 ) );
  }
  connect( mStatusZoomCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( statusZoomCombo_currentIndexChanged( int ) ) );
  connect( mStatusZoomCombo->lineEdit(), SIGNAL( returnPressed() ), this, SLOT( statusZoomCombo_zoomEntered() ) );

  //create status bar labels
  mStatusCursorXLabel = new QLabel( mStatusBar );
  mStatusCursorXLabel->setMinimumWidth( 100 );
  mStatusCursorYLabel = new QLabel( mStatusBar );
  mStatusCursorYLabel->setMinimumWidth( 100 );
  mStatusCursorPageLabel = new QLabel( mStatusBar );
  mStatusCursorPageLabel->setMinimumWidth( 100 );
  mStatusCompositionLabel = new QLabel( mStatusBar );
  mStatusCompositionLabel->setMinimumWidth( 350 );
  mStatusAtlasLabel = new QLabel( mStatusBar );

  //hide borders from child items in status bar under Windows
  mStatusBar->setStyleSheet( "QStatusBar::item {border: none;}" );

  mStatusBar->addWidget( mStatusCursorXLabel );
  mStatusBar->addWidget( mStatusCursorYLabel );
  mStatusBar->addWidget( mStatusCursorPageLabel );
  mStatusBar->addWidget( mStatusZoomCombo );
  mStatusBar->addWidget( mStatusCompositionLabel );
  mStatusBar->addWidget( mStatusAtlasLabel );

  //create composer view and layout with rulers
  mView = 0;
  mViewLayout = new QGridLayout();
  mViewLayout->setSpacing( 0 );
  mViewLayout->setMargin( 0 );
  mHorizontalRuler = new QgsComposerRuler( QgsComposerRuler::Horizontal );
  mVerticalRuler = new QgsComposerRuler( QgsComposerRuler::Vertical );
  mRulerLayoutFix = new QWidget();
  mRulerLayoutFix->setAttribute( Qt::WA_NoMousePropagation );
  mRulerLayoutFix->setBackgroundRole( QPalette::Window );
  mRulerLayoutFix->setFixedSize( mVerticalRuler->rulerSize(), mHorizontalRuler->rulerSize() );
  mViewLayout->addWidget( mRulerLayoutFix, 0, 0 );
  mViewLayout->addWidget( mHorizontalRuler, 0, 1 );
  mViewLayout->addWidget( mVerticalRuler, 1, 0 );
  createComposerView();
  mViewFrame->setLayout( mViewLayout );

  //initial state of rulers
  QSettings myQSettings;
  bool showRulers = myQSettings.value( "/Composer/showRulers", true ).toBool();
  mActionShowRulers->blockSignals( true );
  mActionShowRulers->setChecked( showRulers );
  mHorizontalRuler->setVisible( showRulers );
  mVerticalRuler->setVisible( showRulers );
  mRulerLayoutFix->setVisible( showRulers );
  mActionShowRulers->blockSignals( false );
  connect( mActionShowRulers, SIGNAL( triggered( bool ) ), this, SLOT( toggleRulers( bool ) ) );

  //init undo/redo buttons
  mComposition  = new QgsComposition( mQgis->mapCanvas()->mapSettings() );

  mActionUndo->setEnabled( false );
  mActionRedo->setEnabled( false );
  if ( mComposition->undoStack() )
  {
    connect( mComposition->undoStack(), SIGNAL( canUndoChanged( bool ) ), mActionUndo, SLOT( setEnabled( bool ) ) );
    connect( mComposition->undoStack(), SIGNAL( canRedoChanged( bool ) ), mActionRedo, SLOT( setEnabled( bool ) ) );
  }

  restoreGridSettings();
  connectViewSlots();
  connectCompositionSlots();
  connectOtherSlots();

  mView->setComposition( mComposition );
  //this connection is set up after setting the view's composition, as we don't want setComposition called
  //for new composers
  connect( mView, SIGNAL( compositionSet( QgsComposition* ) ), this, SLOT( setComposition( QgsComposition* ) ) );

  int minDockWidth( 335 );

  setTabPosition( Qt::AllDockWidgetAreas, QTabWidget::North );
  mGeneralDock = new QDockWidget( tr( "Composition" ), this );
  mGeneralDock->setObjectName( "CompositionDock" );
  mGeneralDock->setMinimumWidth( minDockWidth );
  mPanelMenu->addAction( mGeneralDock->toggleViewAction() );
  mItemDock = new QDockWidget( tr( "Item properties" ), this );
  mItemDock->setObjectName( "ItemDock" );
  mItemDock->setMinimumWidth( minDockWidth );
  mPanelMenu->addAction( mItemDock->toggleViewAction() );
  mUndoDock = new QDockWidget( tr( "Command history" ), this );
  mUndoDock->setObjectName( "CommandDock" );
  mPanelMenu->addAction( mUndoDock->toggleViewAction() );
  mAtlasDock = new QDockWidget( tr( "Atlas generation" ), this );
  mAtlasDock->setObjectName( "AtlasDock" );
  mPanelMenu->addAction( mAtlasDock->toggleViewAction() );
  mItemsDock = new QDockWidget( tr( "Items" ), this );
  mItemsDock->setObjectName( "ItemsDock" );
  mPanelMenu->addAction( mItemsDock->toggleViewAction() );

  QList<QDockWidget *> docks = findChildren<QDockWidget *>();
  foreach ( QDockWidget* dock, docks )
  {
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    connect( dock, SIGNAL( visibilityChanged( bool ) ), this, SLOT( dockVisibilityChanged( bool ) ) );
  }

  createCompositionWidget();

  //undo widget
  mUndoView = new QUndoView( mComposition->undoStack(), this );
  mUndoDock->setWidget( mUndoView );

  //items tree widget
  mItemsTreeView = new QTreeView( mItemsDock );
  mItemsTreeView->setModel( mComposition->itemsModel() );
#ifdef ENABLE_MODELTEST
  new ModelTest( mComposition->itemsModel(), this );
#endif

  mItemsTreeView->setColumnWidth( 0, 30 );
  mItemsTreeView->setColumnWidth( 1, 30 );
  mItemsTreeView->header()->setResizeMode( 0, QHeaderView::Fixed );
  mItemsTreeView->header()->setResizeMode( 1, QHeaderView::Fixed );
  mItemsTreeView->header()->setMovable( false );

  mItemsTreeView->setDragEnabled( true );
  mItemsTreeView->setAcceptDrops( true );
  mItemsTreeView->setDropIndicatorShown( true );
  mItemsTreeView->setDragDropMode( QAbstractItemView::InternalMove );

  mItemsTreeView->setIndentation( 0 );
  mItemsDock->setWidget( mItemsTreeView );
  connect( mItemsTreeView->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), mComposition->itemsModel(), SLOT( setSelected( QModelIndex ) ) );

  addDockWidget( Qt::RightDockWidgetArea, mItemDock );
  addDockWidget( Qt::RightDockWidgetArea, mGeneralDock );
  addDockWidget( Qt::RightDockWidgetArea, mUndoDock );
  addDockWidget( Qt::RightDockWidgetArea, mAtlasDock );
  addDockWidget( Qt::RightDockWidgetArea, mItemsDock );

  QgsAtlasCompositionWidget* atlasWidget = new QgsAtlasCompositionWidget( mGeneralDock, mComposition );
  mAtlasDock->setWidget( atlasWidget );

  mItemDock->show();
  mGeneralDock->show();
  mUndoDock->show();
  mAtlasDock->show();
  mItemsDock->show();

  tabifyDockWidget( mGeneralDock, mUndoDock );
  tabifyDockWidget( mItemDock, mUndoDock );
  tabifyDockWidget( mGeneralDock, mItemDock );
  tabifyDockWidget( mItemDock, mAtlasDock );
  tabifyDockWidget( mItemDock, mItemsDock );

  mGeneralDock->raise();

  //set initial state of atlas controls
  mActionAtlasPreview->setEnabled( false );
  mActionAtlasPreview->setChecked( false );
  mActionAtlasFirst->setEnabled( false );
  mActionAtlasLast->setEnabled( false );
  mActionAtlasNext->setEnabled( false );
  mActionAtlasPrev->setEnabled( false );
  mActionPrintAtlas->setEnabled( false );
  mActionExportAtlasAsImage->setEnabled( false );
  mActionExportAtlasAsSVG->setEnabled( false );
  mActionExportAtlasAsPDF->setEnabled( false );
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  connect( atlasMap, SIGNAL( toggled( bool ) ), this, SLOT( toggleAtlasControls( bool ) ) );
  connect( atlasMap, SIGNAL( coverageLayerChanged( QgsVectorLayer* ) ), this, SLOT( updateAtlasMapLayerAction( QgsVectorLayer * ) ) );

  //default printer page setup
  setPrinterPageDefaults();

  // Create size grip (needed by Mac OS X for QMainWindow if QStatusBar is not visible)
  //should not be needed now that composer has a status bar?
#if 0
  mSizeGrip = new QSizeGrip( this );
  mSizeGrip->resize( mSizeGrip->sizeHint() );
  mSizeGrip->move( rect().bottomRight() - mSizeGrip->rect().bottomRight() );
#endif

  restoreWindowState();
  setSelectionTool();

  mView->setFocus();

  //connect with signals from QgsProject to write project files
  if ( QgsProject::instance() )
  {
    connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeXML( QDomDocument& ) ) );
  }

#if defined(ANDROID)
  // fix for Qt Ministro hiding app's menubar in favor of native Android menus
  menuBar()->setNativeMenuBar( false );
  menuBar()->setVisible( true );
#endif
}

QgsComposer::~QgsComposer()
{
  deleteItemWidgets();
}

void QgsComposer::setupTheme()
{
  //now set all the icons - getThemeIcon will fall back to default theme if its
  //missing from active theme
  mActionQuit->setIcon( QgsApplication::getThemeIcon( "/mActionFileExit.png" ) );
  mActionSaveProject->setIcon( QgsApplication::getThemeIcon( "/mActionFileSave.svg" ) );
  mActionNewComposer->setIcon( QgsApplication::getThemeIcon( "/mActionNewComposer.svg" ) );
  mActionDuplicateComposer->setIcon( QgsApplication::getThemeIcon( "/mActionDuplicateComposer.svg" ) );
  mActionComposerManager->setIcon( QgsApplication::getThemeIcon( "/mActionComposerManager.svg" ) );
  mActionLoadFromTemplate->setIcon( QgsApplication::getThemeIcon( "/mActionFileOpen.svg" ) );
  mActionSaveAsTemplate->setIcon( QgsApplication::getThemeIcon( "/mActionFileSaveAs.svg" ) );
  mActionExportAsImage->setIcon( QgsApplication::getThemeIcon( "/mActionSaveMapAsImage.png" ) );
  mActionExportAsSVG->setIcon( QgsApplication::getThemeIcon( "/mActionSaveAsSVG.png" ) );
  mActionExportAsPDF->setIcon( QgsApplication::getThemeIcon( "/mActionSaveAsPDF.png" ) );
  mActionPrint->setIcon( QgsApplication::getThemeIcon( "/mActionFilePrint.png" ) );
  mActionZoomAll->setIcon( QgsApplication::getThemeIcon( "/mActionZoomFullExtent.svg" ) );
  mActionZoomIn->setIcon( QgsApplication::getThemeIcon( "/mActionZoomIn.svg" ) );
  mActionZoomOut->setIcon( QgsApplication::getThemeIcon( "/mActionZoomOut.svg" ) );
  mActionZoomActual->setIcon( QgsApplication::getThemeIcon( "/mActionZoomActual.svg" ) );
  mActionMouseZoom->setIcon( QgsApplication::getThemeIcon( "/mActionZoomToArea.svg" ) );
  mActionRefreshView->setIcon( QgsApplication::getThemeIcon( "/mActionDraw.svg" ) );
  mActionUndo->setIcon( QgsApplication::getThemeIcon( "/mActionUndo.png" ) );
  mActionRedo->setIcon( QgsApplication::getThemeIcon( "/mActionRedo.png" ) );
  mActionAddImage->setIcon( QgsApplication::getThemeIcon( "/mActionAddImage.png" ) );
  mActionAddNewMap->setIcon( QgsApplication::getThemeIcon( "/mActionAddMap.png" ) );
  mActionAddNewLabel->setIcon( QgsApplication::getThemeIcon( "/mActionLabel.png" ) );
  mActionAddNewLegend->setIcon( QgsApplication::getThemeIcon( "/mActionAddLegend.png" ) );
  mActionAddNewScalebar->setIcon( QgsApplication::getThemeIcon( "/mActionScaleBar.png" ) );
  mActionAddRectangle->setIcon( QgsApplication::getThemeIcon( "/mActionAddBasicShape.png" ) );
  mActionAddTriangle->setIcon( QgsApplication::getThemeIcon( "/mActionAddBasicShape.png" ) );
  mActionAddEllipse->setIcon( QgsApplication::getThemeIcon( "/mActionAddBasicShape.png" ) );
  mActionAddArrow->setIcon( QgsApplication::getThemeIcon( "/mActionAddArrow.png" ) );
  mActionAddTable->setIcon( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ) );
  mActionAddAttributeTable->setIcon( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ) );
  mActionAddHtml->setIcon( QgsApplication::getThemeIcon( "/mActionAddHtml.png" ) );
  mActionSelectMoveItem->setIcon( QgsApplication::getThemeIcon( "/mActionSelect.svg" ) );
  mActionMoveItemContent->setIcon( QgsApplication::getThemeIcon( "/mActionMoveItemContent.png" ) );
  mActionGroupItems->setIcon( QgsApplication::getThemeIcon( "/mActionGroupItems.png" ) );
  mActionUngroupItems->setIcon( QgsApplication::getThemeIcon( "/mActionUngroupItems.png" ) );
  mActionRaiseItems->setIcon( QgsApplication::getThemeIcon( "/mActionRaiseItems.png" ) );
  mActionLowerItems->setIcon( QgsApplication::getThemeIcon( "/mActionLowerItems.png" ) );
  mActionMoveItemsToTop->setIcon( QgsApplication::getThemeIcon( "/mActionMoveItemsToTop.png" ) );
  mActionMoveItemsToBottom->setIcon( QgsApplication::getThemeIcon( "/mActionMoveItemsToBottom.png" ) );
  mActionAlignLeft->setIcon( QgsApplication::getThemeIcon( "/mActionAlignLeft.png" ) );
  mActionAlignHCenter->setIcon( QgsApplication::getThemeIcon( "/mActionAlignHCenter.png" ) );
  mActionAlignRight->setIcon( QgsApplication::getThemeIcon( "/mActionAlignRight.png" ) );
  mActionAlignTop->setIcon( QgsApplication::getThemeIcon( "/mActionAlignTop.png" ) );
  mActionAlignVCenter->setIcon( QgsApplication::getThemeIcon( "/mActionAlignVCenter.png" ) );
  mActionAlignBottom->setIcon( QgsApplication::getThemeIcon( "/mActionAlignBottom.png" ) );
}

void QgsComposer::setIconSizes( int size )
{
  //Set the icon size of for all the toolbars created in the future.
  setIconSize( QSize( size, size ) );

  //Change all current icon sizes.
  QList<QToolBar *> toolbars = findChildren<QToolBar *>();
  foreach ( QToolBar * toolbar, toolbars )
  {
    toolbar->setIconSize( QSize( size, size ) );
  }
}

void QgsComposer::connectViewSlots()
{
  if ( !mView )
  {
    return;
  }

  connect( mView, SIGNAL( selectedItemChanged( QgsComposerItem* ) ), this, SLOT( showItemOptions( QgsComposerItem* ) ) );
  connect( mView, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( deleteItem( QgsComposerItem* ) ) );
  connect( mView, SIGNAL( actionFinished() ), this, SLOT( setSelectionTool() ) );

  //listen out for position updates from the QgsComposerView
  connect( mView, SIGNAL( cursorPosChanged( QPointF ) ), this, SLOT( updateStatusCursorPos( QPointF ) ) );
  connect( mView, SIGNAL( zoomLevelChanged() ), this, SLOT( updateStatusZoom() ) );
}

void QgsComposer::connectCompositionSlots()
{
  if ( !mComposition )
  {
    return;
  }

  connect( mComposition, SIGNAL( selectedItemChanged( QgsComposerItem* ) ), this, SLOT( showItemOptions( QgsComposerItem* ) ) );
  connect( mComposition, SIGNAL( composerArrowAdded( QgsComposerArrow* ) ), this, SLOT( addComposerArrow( QgsComposerArrow* ) ) );
  connect( mComposition, SIGNAL( composerHtmlFrameAdded( QgsComposerHtml*, QgsComposerFrame* ) ), this, SLOT( addComposerHtmlFrame( QgsComposerHtml*, QgsComposerFrame* ) ) );
  connect( mComposition, SIGNAL( composerLabelAdded( QgsComposerLabel* ) ), this, SLOT( addComposerLabel( QgsComposerLabel* ) ) );
  connect( mComposition, SIGNAL( composerMapAdded( QgsComposerMap* ) ), this, SLOT( addComposerMap( QgsComposerMap* ) ) );
  connect( mComposition, SIGNAL( composerScaleBarAdded( QgsComposerScaleBar* ) ), this, SLOT( addComposerScaleBar( QgsComposerScaleBar* ) ) );
  connect( mComposition, SIGNAL( composerLegendAdded( QgsComposerLegend* ) ), this, SLOT( addComposerLegend( QgsComposerLegend* ) ) );
  connect( mComposition, SIGNAL( composerPictureAdded( QgsComposerPicture* ) ), this, SLOT( addComposerPicture( QgsComposerPicture* ) ) );
  connect( mComposition, SIGNAL( composerShapeAdded( QgsComposerShape* ) ), this, SLOT( addComposerShape( QgsComposerShape* ) ) );
  connect( mComposition, SIGNAL( composerTableAdded( QgsComposerAttributeTable* ) ), this, SLOT( addComposerTable( QgsComposerAttributeTable* ) ) );
  connect( mComposition, SIGNAL( composerTableFrameAdded( QgsComposerAttributeTableV2*, QgsComposerFrame* ) ), this, SLOT( addComposerTableV2( QgsComposerAttributeTableV2*, QgsComposerFrame* ) ) );
  connect( mComposition, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( deleteItem( QgsComposerItem* ) ) );
  connect( mComposition, SIGNAL( paperSizeChanged() ), mHorizontalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( paperSizeChanged() ), mVerticalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( nPagesChanged() ), mHorizontalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( nPagesChanged() ), mVerticalRuler, SLOT( update() ) );

  //listen out to status bar updates from the atlas
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  connect( atlasMap, SIGNAL( statusMsgChanged( QString ) ), this, SLOT( updateStatusAtlasMsg( QString ) ) );

  //listen out to status bar updates from the composition
  connect( mComposition, SIGNAL( statusMsgChanged( QString ) ), this, SLOT( updateStatusCompositionMsg( QString ) ) );
}

void QgsComposer::connectOtherSlots()
{
  //also listen out for position updates from the horizontal/vertical rulers
  connect( mHorizontalRuler, SIGNAL( cursorPosChanged( QPointF ) ), this, SLOT( updateStatusCursorPos( QPointF ) ) );
  connect( mVerticalRuler, SIGNAL( cursorPosChanged( QPointF ) ), this, SLOT( updateStatusCursorPos( QPointF ) ) );
  //listen out for zoom updates
  connect( this, SIGNAL( zoomLevelChanged() ), this, SLOT( updateStatusZoom() ) );
}

void QgsComposer::open( void )
{
  if ( mFirstTime )
  {
    //mComposition->createDefault();
    mFirstTime = false;
    show();
    zoomFull(); // zoomFull() does not work properly until we have called show()
    if ( mView )
    {
      mView->updateRulers();
    }
  }

  else
  {
    show(); //make sure the window is displayed - with a saved project, it's possible to not have already called show()
    //is that a bug?
    activate(); //bring the composer window to the front
  }
}

void QgsComposer::activate()
{
  bool shown = isVisible();
  show();
  raise();
  setWindowState( windowState() & ~Qt::WindowMinimized );
  activateWindow();
  if ( !shown )
  {
    on_mActionZoomAll_triggered();
  }
}

#ifdef Q_OS_MAC
void QgsComposer::changeEvent( QEvent* event )
{
  QMainWindow::changeEvent( event );
  switch ( event->type() )
  {
    case QEvent::ActivationChange:
      if ( QApplication::activeWindow() == this )
      {
        mWindowAction->setChecked( true );
      }
      break;

    default:
      break;
  }
}
#endif

void QgsComposer::setTitle( const QString& title )
{
  mTitle = title;
  setWindowTitle( mTitle );
  if ( mWindowAction )
  {
    mWindowAction->setText( title );
  }

  //update atlas map layer action name if required
  if ( mAtlasFeatureAction )
  {
    mAtlasFeatureAction->setText( QString( tr( "Set as atlas feature for %1" ) ).arg( mTitle ) );
  }
}

void QgsComposer::updateStatusCursorPos( QPointF cursorPosition )
{
  if ( !mComposition )
  {
    return;
  }

  //convert cursor position to position on current page
  QPointF pagePosition = mComposition->positionOnPage( cursorPosition );
  int currentPage = mComposition->pageNumberForPoint( cursorPosition );

  mStatusCursorXLabel->setText( QString( tr( "x: %1 mm" ) ).arg( pagePosition.x() ) );
  mStatusCursorYLabel->setText( QString( tr( "y: %1 mm" ) ).arg( pagePosition.y() ) );
  mStatusCursorPageLabel->setText( QString( tr( "page: %3" ) ).arg( currentPage ) );
}

void QgsComposer::updateStatusZoom()
{
  double dpi = QgsApplication::desktop()->logicalDpiX();
  //monitor dpi is not always correct - so make sure the value is sane
  if (( dpi < 60 ) || ( dpi > 250 ) )
    dpi = 72;

  //pixel width for 1mm on screen
  double scale100 = dpi / 25.4;
  //current zoomLevel
  double zoomLevel = mView->transform().m11() * 100 / scale100;

  mStatusZoomCombo->blockSignals( true );
  mStatusZoomCombo->lineEdit()->setText( tr( "%1%" ).arg( zoomLevel, 0, 'f', 1 ) );
  mStatusZoomCombo->blockSignals( false );
}

void QgsComposer::statusZoomCombo_currentIndexChanged( int index )
{
  double selectedZoom = mStatusZoomLevelsList[ mStatusZoomLevelsList.count() - index - 1 ];
  if ( mView )
  {
    mView->setZoomLevel( selectedZoom );
    //update zoom combobox text for correct format (one decimal place, trailing % sign)
    mStatusZoomCombo->blockSignals( true );
    mStatusZoomCombo->lineEdit()->setText( tr( "%1%" ).arg( selectedZoom * 100.0, 0, 'f', 1 ) );
    mStatusZoomCombo->blockSignals( false );
  }
}

void QgsComposer::statusZoomCombo_zoomEntered()
{
  if ( !mView )
  {
    return;
  }

  //need to remove spaces and "%" characters from input text
  QString zoom = mStatusZoomCombo->currentText().remove( QChar( '%' ) ).trimmed();
  mView->setZoomLevel( zoom.toDouble() / 100 );
}

void QgsComposer::updateStatusCompositionMsg( QString message )
{
  mStatusCompositionLabel->setText( message );
}

void QgsComposer::updateStatusAtlasMsg( QString message )
{
  mStatusAtlasLabel->setText( message );
}

void QgsComposer::showItemOptions( QgsComposerItem* item )
{
  QWidget* currentWidget = mItemDock->widget();

  if ( !item )
  {
    mItemDock->setWidget( 0 );
    return;
  }

  QMap<QgsComposerItem*, QWidget*>::iterator it = mItemWidgetMap.find( item );
  if ( it == mItemWidgetMap.constEnd() )
  {
    return;
  }

  QWidget* newWidget = it.value();

  if ( !newWidget || newWidget == currentWidget ) //bail out if new widget does not exist or is already there
  {
    return;
  }

  mItemDock->setWidget( newWidget );
}

void QgsComposer::on_mActionOptions_triggered()
{
  mQgis->showOptionsDialog( this, QString( "mOptionsPageComposer" ) );
}

void QgsComposer::toggleAtlasControls( bool atlasEnabled )
{
  //preview defaults to unchecked
  mActionAtlasPreview->blockSignals( true );
  mActionAtlasPreview->setChecked( false );
  mActionAtlasFirst->setEnabled( false );
  mActionAtlasLast->setEnabled( false );
  mActionAtlasNext->setEnabled( false );
  mActionAtlasPrev->setEnabled( false );
  mActionAtlasPreview->blockSignals( false );
  mActionAtlasPreview->setEnabled( atlasEnabled );
  mActionPrintAtlas->setEnabled( atlasEnabled );
  mActionExportAtlasAsImage->setEnabled( atlasEnabled );
  mActionExportAtlasAsSVG->setEnabled( atlasEnabled );
  mActionExportAtlasAsPDF->setEnabled( atlasEnabled );

  updateAtlasMapLayerAction( atlasEnabled );
}

void QgsComposer::on_mActionAtlasPreview_triggered( bool checked )
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();

  //check if composition has an atlas map enabled
  if ( checked && !atlasMap->enabled() )
  {
    //no atlas current enabled
    QMessageBox::warning( 0, tr( "Enable atlas preview" ),
                          tr( "Atlas in not currently enabled for this composition!" ),
                          QMessageBox::Ok,
                          QMessageBox::Ok );
    mActionAtlasPreview->blockSignals( true );
    mActionAtlasPreview->setChecked( false );
    mActionAtlasPreview->blockSignals( false );
    mStatusAtlasLabel->setText( QString() );
    return;
  }

  //toggle other controls depending on whether atlas preview is active
  mActionAtlasFirst->setEnabled( checked );
  mActionAtlasLast->setEnabled( checked );
  mActionAtlasNext->setEnabled( checked );
  mActionAtlasPrev->setEnabled( checked );

  if ( checked )
  {
    loadAtlasPredefinedScalesFromProject();
  }

  bool previewEnabled = mComposition->setAtlasMode( checked ? QgsComposition::PreviewAtlas : QgsComposition::AtlasOff );
  if ( !previewEnabled )
  {
    //something went wrong, eg, no matching features
    QMessageBox::warning( 0, tr( "Enable atlas preview" ),
                          tr( "No matching atlas features found!" ),
                          QMessageBox::Ok,
                          QMessageBox::Ok );
    mActionAtlasPreview->blockSignals( true );
    mActionAtlasPreview->setChecked( false );
    mActionAtlasFirst->setEnabled( false );
    mActionAtlasLast->setEnabled( false );
    mActionAtlasNext->setEnabled( false );
    mActionAtlasPrev->setEnabled( false );
    mActionAtlasPreview->blockSignals( false );
    mStatusAtlasLabel->setText( QString() );
    return;
  }

  if ( checked )
  {
    mapCanvas()->stopRendering();
    emit( atlasPreviewFeatureChanged() );
  }
  else
  {
    mStatusAtlasLabel->setText( QString() );
  }

}


void QgsComposer::on_mActionAtlasNext_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( !atlasMap->enabled() )
  {
    return;
  }

  mapCanvas()->stopRendering();

  loadAtlasPredefinedScalesFromProject();
  atlasMap->nextFeature();
  emit( atlasPreviewFeatureChanged() );
}

void QgsComposer::on_mActionAtlasPrev_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( !atlasMap->enabled() )
  {
    return;
  }

  mapCanvas()->stopRendering();

  loadAtlasPredefinedScalesFromProject();
  atlasMap->prevFeature();
  emit( atlasPreviewFeatureChanged() );
}

void QgsComposer::on_mActionAtlasFirst_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( !atlasMap->enabled() )
  {
    return;
  }

  mapCanvas()->stopRendering();

  loadAtlasPredefinedScalesFromProject();
  atlasMap->firstFeature();
  emit( atlasPreviewFeatureChanged() );
}

void QgsComposer::on_mActionAtlasLast_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( !atlasMap->enabled() )
  {
    return;
  }

  mapCanvas()->stopRendering();

  loadAtlasPredefinedScalesFromProject();
  atlasMap->lastFeature();
  emit( atlasPreviewFeatureChanged() );
}

QgsMapCanvas *QgsComposer::mapCanvas( void )
{
  return mQgis->mapCanvas();
}

QgsComposerView *QgsComposer::view( void )
{
  return mView;
}

void QgsComposer::zoomFull( void )
{
  if ( mView )
  {
    mView->fitInView( mComposition->sceneRect(), Qt::KeepAspectRatio );
  }
}

void QgsComposer::on_mActionZoomAll_triggered()
{
  zoomFull();
  mView->updateRulers();
  mView->update();
  emit zoomLevelChanged();
}

void QgsComposer::on_mActionZoomIn_triggered()
{
  mView->scale( 2, 2 );
  mView->updateRulers();
  mView->update();
  emit zoomLevelChanged();
}

void QgsComposer::on_mActionZoomOut_triggered()
{
  mView->scale( .5, .5 );
  mView->updateRulers();
  mView->update();
  emit zoomLevelChanged();
}

void QgsComposer::on_mActionZoomActual_triggered()
{
  mView->setZoomLevel( 1.0 );
}

void QgsComposer::on_mActionMouseZoom_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::Zoom );
  }
}

void QgsComposer::on_mActionRefreshView_triggered()
{
  if ( !mComposition )
  {
    return;
  }

  //refresh atlas feature first, to update attributes
  if ( mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //block signals from atlas, since the later call to mComposition->refreshItems() will
    //also trigger items to refresh atlas dependent properties
    mComposition->atlasComposition().blockSignals( true );
    mComposition->atlasComposition().refreshFeature();
    mComposition->atlasComposition().blockSignals( false );
  }

  mComposition->refreshItems();
  mComposition->update();
}

void QgsComposer::on_mActionShowGrid_triggered( bool checked )
{
  //show or hide grid
  if ( mComposition )
  {
    mComposition->setGridVisible( checked );
  }
}

void QgsComposer::on_mActionSnapGrid_triggered( bool checked )
{
  //enable or disable snap items to grid
  if ( mComposition )
  {
    mComposition->setSnapToGridEnabled( checked );
  }
}

void QgsComposer::on_mActionShowGuides_triggered( bool checked )
{
  //show or hide guide lines
  if ( mComposition )
  {
    mComposition->setSnapLinesVisible( checked );
  }
}

void QgsComposer::on_mActionSnapGuides_triggered( bool checked )
{
  //enable or disable snap items to guides
  if ( mComposition )
  {
    mComposition->setAlignmentSnap( checked );
  }
}

void QgsComposer::on_mActionSmartGuides_triggered( bool checked )
{
  //enable or disable smart snapping guides
  if ( mComposition )
  {
    mComposition->setSmartGuidesEnabled( checked );
  }
}

void QgsComposer::on_mActionShowBoxes_triggered( bool checked )
{
  //show or hide bounding boxes
  if ( mComposition )
  {
    mComposition->setBoundingBoxesVisible( checked );
  }
}

void QgsComposer::on_mActionClearGuides_triggered()
{
  //clear guide lines
  if ( mComposition )
  {
    mComposition->clearSnapLines();
  }
}

void QgsComposer::toggleRulers( bool checked )
{
  //show or hide rulers
  mHorizontalRuler->setVisible( checked );
  mVerticalRuler->setVisible( checked );
  mRulerLayoutFix->setVisible( checked );

  QSettings myQSettings;
  myQSettings.setValue( "/Composer/showRulers", checked );
}

void QgsComposer::on_mActionAtlasSettings_triggered()
{
  if ( !mAtlasDock->isVisible() )
  {
    mAtlasDock->show();
  }

  mAtlasDock->raise();
}

void QgsComposer::on_mActionToggleFullScreen_triggered()
{
  if ( mActionToggleFullScreen->isChecked() )
  {
    showFullScreen();
  }
  else
  {
    showNormal();
  }
}

void QgsComposer::on_mActionHidePanels_triggered()
{
  /*
  workaround the limited Qt dock widget API
  see http://qt-project.org/forums/viewthread/1141/
  and http://qt-project.org/faq/answer/how_can_i_check_which_tab_is_the_current_one_in_a_tabbed_qdockwidget
  */

  bool showPanels = !mActionHidePanels->isChecked();
  QList<QDockWidget *> docks = findChildren<QDockWidget *>();
  QList<QTabBar *> tabBars = findChildren<QTabBar *>();

  if ( !showPanels )
  {
    mPanelStatus.clear();
    //record status of all docks

    foreach ( QDockWidget* dock, docks )
    {
      mPanelStatus.insert( dock->windowTitle(), PanelStatus( dock->isVisible(), false ) );
      dock->setVisible( false );
    }

    //record active dock tabs
    foreach ( QTabBar* tabBar, tabBars )
    {
      QString currentTabTitle = tabBar->tabText( tabBar->currentIndex() );
      mPanelStatus[ currentTabTitle ].isActive = true;
    }
  }
  else
  {
    //restore visibility of all docks
    foreach ( QDockWidget* dock, docks )
    {
      if ( ! mPanelStatus.contains( dock->windowTitle() ) )
      {
        dock->setVisible( true );
        continue;
      }
      dock->setVisible( mPanelStatus.value( dock->windowTitle() ).isVisible );
    }

    //restore previously active dock tabs
    foreach ( QTabBar* tabBar, tabBars )
    {
      //loop through all tabs in tab bar
      for ( int i = 0; i < tabBar->count(); ++i )
      {
        QString tabTitle = tabBar->tabText( i );
        if ( mPanelStatus.value( tabTitle ).isActive )
        {
          tabBar->setCurrentIndex( i );
        }
      }
    }
  }
}

void QgsComposer::disablePreviewMode()
{
  if ( !mView )
  {
    return;
  }

  mView->setPreviewModeEnabled( false );
}

void QgsComposer::activateGrayscalePreview()
{
  if ( !mView )
  {
    return;
  }

  mView->setPreviewMode( QgsPreviewEffect::PreviewGrayscale );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::activateMonoPreview()
{
  if ( !mView )
  {
    return;
  }

  mView->setPreviewMode( QgsPreviewEffect::PreviewMono );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::activateProtanopePreview()
{
  if ( !mView )
  {
    return;
  }

  mView->setPreviewMode( QgsPreviewEffect::PreviewProtanope );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::activateDeuteranopePreview()
{
  if ( !mView )
  {
    return;
  }

  mView->setPreviewMode( QgsPreviewEffect::PreviewDeuteranope );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::setComposition( QgsComposition* composition )
{
  if ( !composition )
  {
    return;
  }

  //delete composition widget
  QgsCompositionWidget* oldCompositionWidget = qobject_cast<QgsCompositionWidget *>( mGeneralDock->widget() );
  delete oldCompositionWidget;

  deleteItemWidgets();

  mComposition = composition;

  connectCompositionSlots();
  createCompositionWidget();
  restoreGridSettings();
  setupUndoView();

  //setup atlas composition widget
  QgsAtlasCompositionWidget* oldAtlasWidget = qobject_cast<QgsAtlasCompositionWidget *>( mAtlasDock->widget() );
  delete oldAtlasWidget;
  mAtlasDock->setWidget( new QgsAtlasCompositionWidget( mAtlasDock, mComposition ) );

  //set state of atlas controls
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  toggleAtlasControls( atlasMap->enabled() );
  connect( atlasMap, SIGNAL( toggled( bool ) ), this, SLOT( toggleAtlasControls( bool ) ) );
  connect( atlasMap, SIGNAL( coverageLayerChanged( QgsVectorLayer* ) ), this, SLOT( updateAtlasMapLayerAction( QgsVectorLayer * ) ) );

  //default printer page setup
  setPrinterPageDefaults();
}

void QgsComposer::dockVisibilityChanged( bool visible )
{
  if ( visible )
  {
    mActionHidePanels->blockSignals( true );
    mActionHidePanels->setChecked( false );
    mActionHidePanels->blockSignals( false );
  }
}

void QgsComposer::on_mActionExportAtlasAsPDF_triggered()
{
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  exportCompositionAsPDF( QgsComposer::Atlas );
  mComposition->setAtlasMode( previousMode );

  if ( mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //after atlas output, jump back to preview first feature
    QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
    atlasMap->firstFeature();
  }
}

void QgsComposer::on_mActionExportAsPDF_triggered()
{
  exportCompositionAsPDF( QgsComposer::Single );
}

void QgsComposer::exportCompositionAsPDF( QgsComposer::OutputMode mode )
{
  if ( !mComposition || !mView )
  {
    return;
  }

  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  if ( containsAdvancedEffects() )
  {
    showAdvancedEffectsWarning();
  }

  // If we are not printing as raster, temporarily disable advanced effects
  // as QPrinter does not support composition modes and can result
  // in items missing from the output
  if ( mComposition->printAsRaster() )
  {
    mComposition->setUseAdvancedEffects( true );
  }
  else
  {
    mComposition->setUseAdvancedEffects( false );
  }

  bool hasAnAtlas = mComposition->atlasComposition().enabled();
  bool atlasOnASingleFile = hasAnAtlas && mComposition->atlasComposition().singleFile();
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();

  QString outputFileName;
  QString outputDir;

  if ( mode == QgsComposer::Single || ( mode == QgsComposer::Atlas && atlasOnASingleFile ) )
  {
    QSettings myQSettings;  // where we keep last used filter in persistent state
    QString lastUsedFile = myQSettings.value( "/UI/lastSaveAsPdfFile", "qgis.pdf" ).toString();
    QFileInfo file( lastUsedFile );

    if ( hasAnAtlas && !atlasOnASingleFile &&
         ( mode == QgsComposer::Atlas || mComposition->atlasMode() == QgsComposition::PreviewAtlas ) )
    {
      outputFileName = QDir( file.path() ).filePath( atlasMap->currentFilename() ) + ".pdf";
    }
    else
    {
      outputFileName = file.path();
    }

    outputFileName = QFileDialog::getSaveFileName(
                       this,
                       tr( "Choose a file name to save the map as" ),
                       outputFileName,
                       tr( "PDF Format" ) + " (*.pdf *.PDF)" );
    if ( outputFileName.isEmpty() )
    {
      return;
    }

    if ( !outputFileName.endsWith( ".pdf", Qt::CaseInsensitive ) )
    {
      outputFileName += ".pdf";
    }

    myQSettings.setValue( "/UI/lastSaveAsPdfFile", outputFileName );
  }
  // else, we need to choose a directory
  else
  {
    if ( atlasMap->filenamePattern().size() == 0 )
    {
      int res = QMessageBox::warning( 0, tr( "Empty filename pattern" ),
                                      tr( "The filename pattern is empty. A default one will be used." ),
                                      QMessageBox::Ok | QMessageBox::Cancel,
                                      QMessageBox::Ok );
      if ( res == QMessageBox::Cancel )
      {
        return;
      }
      atlasMap->setFilenamePattern( "'output_'||$feature" );
    }

    QSettings myQSettings;
    QString lastUsedDir = myQSettings.value( "/UI/lastSaveAtlasAsPdfDir", "." ).toString();
    outputDir = QFileDialog::getExistingDirectory( this,
                tr( "Directory where to save PDF files" ),
                lastUsedDir,
                QFileDialog::ShowDirsOnly );
    if ( outputDir.isEmpty() )
    {
      return;
    }
    // test directory (if it exists and is writable)
    if ( !QDir( outputDir ).exists() || !QFileInfo( outputDir ).isWritable() )
    {
      QMessageBox::warning( 0, tr( "Unable to write into the directory" ),
                            tr( "The given output directory is not writable. Cancelling." ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      return;
    }

    myQSettings.setValue( "/UI/lastSaveAtlasAsPdfDir", outputDir );
  }

  mView->setPaintingEnabled( false );

  if ( mode == QgsComposer::Atlas )
  {
    QPrinter printer;

    QPainter painter;

    loadAtlasPredefinedScalesFromProject();
    if ( ! atlasMap->beginRender() && !atlasMap->featureFilterErrorString().isEmpty() )
    {
      QMessageBox::warning( this, tr( "Atlas processing error" ),
                            tr( "Feature filter parser error: %1" ).arg( atlasMap->featureFilterErrorString() ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      mView->setPaintingEnabled( true );
      return;
    }
    if ( atlasOnASingleFile )
    {
      //prepare for first feature, so that we know paper size to begin with
      atlasMap->prepareForFeature( 0 );
      mComposition->beginPrintAsPDF( printer, outputFileName );
      // set the correct resolution
      mComposition->beginPrint( printer );
      bool printReady =  painter.begin( &printer );
      if ( !printReady )
      {
        QMessageBox::warning( this, tr( "Atlas processing error" ),
                              QString( tr( "Error creating %1." ) ).arg( outputFileName ),
                              QMessageBox::Ok,
                              QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        return;
      }
    }

    QProgressDialog progress( tr( "Rendering maps..." ), tr( "Abort" ), 0, atlasMap->numFeatures(), this );
    QApplication::setOverrideCursor( Qt::BusyCursor );

    for ( int featureI = 0; featureI < atlasMap->numFeatures(); ++featureI )
    {
      progress.setValue( featureI );
      // process input events in order to allow aborting
      QCoreApplication::processEvents();
      if ( progress.wasCanceled() )
      {
        atlasMap->endRender();
        break;
      }
      if ( !atlasMap->prepareForFeature( featureI ) )
      {
        QMessageBox::warning( this, tr( "Atlas processing error" ),
                              tr( "Atlas processing error" ),
                              QMessageBox::Ok,
                              QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        QApplication::restoreOverrideCursor();
        return;
      }
      if ( !atlasOnASingleFile )
      {
        // bugs #7263 and #6856
        // QPrinter does not seem to be reset correctly and may cause generated PDFs (all except the first) corrupted
        // when transparent objects are rendered. We thus use a new QPrinter object here
        QPrinter multiFilePrinter;
        outputFileName = QDir( outputDir ).filePath( atlasMap->currentFilename() ) + ".pdf";
        mComposition->beginPrintAsPDF( multiFilePrinter, outputFileName );
        // set the correct resolution
        mComposition->beginPrint( multiFilePrinter );
        bool printReady = painter.begin( &multiFilePrinter );
        if ( !printReady )
        {
          QMessageBox::warning( this, tr( "Atlas processing error" ),
                                QString( tr( "Error creating %1." ) ).arg( outputFileName ),
                                QMessageBox::Ok,
                                QMessageBox::Ok );
          mView->setPaintingEnabled( true );
          QApplication::restoreOverrideCursor();
          return;
        }
        mComposition->doPrint( multiFilePrinter, painter );
        painter.end();
      }
      else
      {
        //start print on a new page if we're not on the first feature
        mComposition->doPrint( printer, painter, featureI > 0 );
      }
    }
    atlasMap->endRender();
    if ( atlasOnASingleFile )
    {
      painter.end();
    }
  }
  else
  {
    bool exportOk = mComposition->exportAsPDF( outputFileName );
    if ( !exportOk )
    {
      QMessageBox::warning( this, tr( "Atlas processing error" ),
                            QString( tr( "Error creating %1." ) ).arg( outputFileName ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      mView->setPaintingEnabled( true );
      QApplication::restoreOverrideCursor();
      return;
    }
  }

  if ( ! mComposition->useAdvancedEffects() )
  {
    //Switch advanced effects back on
    mComposition->setUseAdvancedEffects( true );
  }
  mView->setPaintingEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionPrint_triggered()
{
  //print only current feature
  printComposition( QgsComposer::Single );
}

void QgsComposer::on_mActionPrintAtlas_triggered()
{
  //print whole atlas
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  printComposition( QgsComposer::Atlas );
  mComposition->setAtlasMode( previousMode );
}

void QgsComposer::printComposition( QgsComposer::OutputMode mode )
{
  if ( !mComposition || !mView )
  {
    return;
  }

  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  if ( containsAdvancedEffects() )
  {
    showAdvancedEffectsWarning();
  }

  // If we are not printing as raster, temporarily disable advanced effects
  // as QPrinter does not support composition modes and can result
  // in items missing from the output
  if ( mComposition->printAsRaster() )
  {
    mComposition->setUseAdvancedEffects( true );
  }
  else
  {
    mComposition->setUseAdvancedEffects( false );
  }

  //orientation and page size are already set to QPrinter in the page setup dialog
  QPrintDialog printDialog( &mPrinter, 0 );
  if ( printDialog.exec() != QDialog::Accepted )
  {
    return;
  }

  QApplication::setOverrideCursor( Qt::BusyCursor );
  mView->setPaintingEnabled( false );

  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( mode == QgsComposer::Single )
  {
    mComposition->print( mPrinter, true );
  }
  else
  {
    //prepare for first feature, so that we know paper size to begin with
    atlasMap->prepareForFeature( 0 );

    mComposition->beginPrint( mPrinter, true );
    QPainter painter( &mPrinter );

    loadAtlasPredefinedScalesFromProject();
    if ( ! atlasMap->beginRender() && !atlasMap->featureFilterErrorString().isEmpty() )
    {
      QMessageBox::warning( this, tr( "Atlas processing error" ),
                            tr( "Feature filter parser error: %1" ).arg( atlasMap->featureFilterErrorString() ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      mView->setPaintingEnabled( true );
      QApplication::restoreOverrideCursor();
      return;
    }
    QProgressDialog progress( tr( "Rendering maps..." ), tr( "Abort" ), 0, atlasMap->numFeatures(), this );

    for ( int i = 0; i < atlasMap->numFeatures(); ++i )
    {
      progress.setValue( i );
      // process input events in order to allow cancelling
      QCoreApplication::processEvents();

      if ( progress.wasCanceled() )
      {
        atlasMap->endRender();
        break;
      }
      if ( !atlasMap->prepareForFeature( i ) )
      {
        QMessageBox::warning( this, tr( "Atlas processing error" ),
                              tr( "Atlas processing error" ),
                              QMessageBox::Ok,
                              QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        QApplication::restoreOverrideCursor();
        return;
      }

      //start print on a new page if we're not on the first feature
      mComposition->doPrint( mPrinter, painter, i > 0 );
    }
    atlasMap->endRender();
    painter.end();
  }

  if ( ! mComposition->useAdvancedEffects() )
  {
    //Switch advanced effects back on
    mComposition->setUseAdvancedEffects( true );
  }
  mView->setPaintingEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionExportAtlasAsImage_triggered()
{
  //print whole atlas
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  exportCompositionAsImage( QgsComposer::Atlas );
  mComposition->setAtlasMode( previousMode );

  if ( mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //after atlas output, jump back to preview first feature
    QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
    atlasMap->firstFeature();
  }
}

void QgsComposer::on_mActionExportAsImage_triggered()
{
  exportCompositionAsImage( QgsComposer::Single );
}

void QgsComposer::exportCompositionAsImage( QgsComposer::OutputMode mode )
{
  if ( !mComposition || !mView )
  {
    return;
  }

  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  QSettings settings;

  // Image size
  int width = ( int )( mComposition->printResolution() * mComposition->paperWidth() / 25.4 );
  int height = ( int )( mComposition-> printResolution() * mComposition->paperHeight() / 25.4 );
  int dpi = ( int )( mComposition->printResolution() );

  int memuse = width * height * 3 / 1000000;  // pixmap + image
  QgsDebugMsg( QString( "Image %1x%2" ).arg( width ).arg( height ) );
  QgsDebugMsg( QString( "memuse = %1" ).arg( memuse ) );

  if ( memuse > 200 )   // about 4500x4500
  {
    int answer = QMessageBox::warning( 0, tr( "Big image" ),
                                       tr( "To create image %1x%2 requires about %3 MB of memory. Proceed?" )
                                       .arg( width ).arg( height ).arg( memuse ),
                                       QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok );

    raise();
    if ( answer == QMessageBox::Cancel )
      return;
  }

  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( mode == QgsComposer::Single )
  {
    QString outputFileName = QString::null;

    if ( atlasMap->enabled() && mComposition->atlasMode() == QgsComposition::PreviewAtlas )
    {
      QString lastUsedDir = settings.value( "/UI/lastSaveAsImageDir", "." ).toString();
      outputFileName = QDir( lastUsedDir ).filePath( atlasMap->currentFilename() );
    }

    QPair<QString, QString> fileNExt = QgisGui::getSaveAsImageName( this, tr( "Choose a file name to save the map image as" ), outputFileName );

    if ( fileNExt.first.isEmpty() )
    {
      return;
    }

    mView->setPaintingEnabled( false );

    for ( int i = 0; i < mComposition->numPages(); ++i )
    {
      if ( !mComposition->shouldExportPage( i + 1 ) )
      {
        continue;
      }
      QImage image = mComposition->printPageAsRaster( i );
      if ( image.isNull() )
      {
        QMessageBox::warning( 0, tr( "Memory Allocation Error" ),
                              tr( "Trying to create image #%1( %2x%3 @ %4dpi ) "
                                  "may result in a memory overflow.\n"
                                  "Please try a lower resolution or a smaller papersize" )
                              .arg( i + 1 ).arg( width ).arg( height ).arg( dpi ),
                              QMessageBox::Ok, QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        return;
      }
      bool saveOk;
      if ( i == 0 )
      {
        saveOk = image.save( fileNExt.first, fileNExt.second.toLocal8Bit().constData() );
      }
      else
      {
        QFileInfo fi( fileNExt.first );
        QString outputFilePath = fi.absolutePath() + "/" + fi.baseName() + "_" + QString::number( i + 1 ) + "." + fi.suffix();
        saveOk = image.save( outputFilePath, fileNExt.second.toLocal8Bit().constData() );
      }
      if ( !saveOk )
      {
        QMessageBox::warning( this, tr( "Image export error" ),
                              QString( tr( "Error creating %1." ) ).arg( fileNExt.first ),
                              QMessageBox::Ok,
                              QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        return;
      }
    }

    //
    // Write the world file if asked to
    if ( mComposition->generateWorldFile() )
    {
      double a, b, c, d, e, f;
      mComposition->computeWorldFileParameters( a, b, c, d, e, f );

      QFileInfo fi( fileNExt.first );
      // build the world file name
      QString worldFileName = fi.absolutePath() + "/" + fi.baseName() + "."
                              + fi.suffix()[0] + fi.suffix()[fi.suffix().size()-1] + "w";

      writeWorldFile( worldFileName, a, b, c, d, e, f );
    }

    mView->setPaintingEnabled( true );
  }
  else
  {
    // else, it has an atlas to render, so a directory must first be selected
    if ( atlasMap->filenamePattern().size() == 0 )
    {
      int res = QMessageBox::warning( 0, tr( "Empty filename pattern" ),
                                      tr( "The filename pattern is empty. A default one will be used." ),
                                      QMessageBox::Ok | QMessageBox::Cancel,
                                      QMessageBox::Ok );
      if ( res == QMessageBox::Cancel )
      {
        return;
      }
      atlasMap->setFilenamePattern( "'output_'||$feature" );
    }

    QSettings myQSettings;
    QString lastUsedDir = myQSettings.value( "/UI/lastSaveAtlasAsImagesDir", "." ).toString();
    QString lastUsedFormat = myQSettings.value( "/UI/lastSaveAtlasAsImagesFormat", "jpg" ).toString();

    QFileDialog dlg( this, tr( "Directory where to save image files" ) );
    dlg.setFileMode( QFileDialog::Directory );
    dlg.setOption( QFileDialog::ShowDirsOnly, true );

    //
    // Build an augmented FileDialog with a combo box to select the output format
    QComboBox *box = new QComboBox();
    QHBoxLayout* hlayout = new QHBoxLayout();
    QWidget* widget = new QWidget();

    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    int selectedFormat = 0;
    for ( int i = 0; i < formats.size(); ++i )
    {
      QString format = QString( formats.at( i ) );
      if ( format == lastUsedFormat )
      {
        selectedFormat = i;
      }
      box->addItem( format );
    }
    box->setCurrentIndex( selectedFormat );

    hlayout->setMargin( 0 );
    hlayout->addWidget( new QLabel( tr( "Image format: " ) ) );
    hlayout->addWidget( box );
    widget->setLayout( hlayout );
    dlg.layout()->addWidget( widget );

    if ( !dlg.exec() )
    {
      return;
    }
    QStringList s = dlg.selectedFiles();
    if ( s.size() < 1 || s.at( 0 ).isEmpty() )
    {
      return;
    }
    QString dir = s.at( 0 );
    QString format = box->currentText();
    QString fileExt = "." + format;

    if ( dir.isEmpty() )
    {
      return;
    }
    // test directory (if it exists and is writable)
    if ( !QDir( dir ).exists() || !QFileInfo( dir ).isWritable() )
    {
      QMessageBox::warning( 0, tr( "Unable to write into the directory" ),
                            tr( "The given output directory is not writable. Cancelling." ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      return;
    }

    myQSettings.setValue( "/UI/lastSaveAtlasAsImagesDir", dir );

    // So, now we can render the atlas
    mView->setPaintingEnabled( false );
    QApplication::setOverrideCursor( Qt::BusyCursor );

    loadAtlasPredefinedScalesFromProject();
    if ( ! atlasMap->beginRender() && !atlasMap->featureFilterErrorString().isEmpty() )
    {
      QMessageBox::warning( this, tr( "Atlas processing error" ),
                            tr( "Feature filter parser error: %1" ).arg( atlasMap->featureFilterErrorString() ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      mView->setPaintingEnabled( true );
      QApplication::restoreOverrideCursor();
      return;
    }

    QProgressDialog progress( tr( "Rendering maps..." ), tr( "Abort" ), 0, atlasMap->numFeatures(), this );

    for ( int feature = 0; feature < atlasMap->numFeatures(); ++feature )
    {
      progress.setValue( feature );
      // process input events in order to allow cancelling
      QCoreApplication::processEvents();

      if ( progress.wasCanceled() )
      {
        atlasMap->endRender();
        break;
      }
      if ( ! atlasMap->prepareForFeature( feature ) )
      {
        QMessageBox::warning( this, tr( "Atlas processing error" ),
                              tr( "Atlas processing error" ),
                              QMessageBox::Ok,
                              QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        QApplication::restoreOverrideCursor();
        return;
      }

      QString filename = QDir( dir ).filePath( atlasMap->currentFilename() ) + fileExt;

      for ( int i = 0; i < mComposition->numPages(); ++i )
      {
        if ( !mComposition->shouldExportPage( i + 1 ) )
        {
          continue;
        }
        QImage image = mComposition->printPageAsRaster( i );
        QString imageFilename = filename;

        if ( i != 0 )
        {
          //append page number
          QFileInfo fi( filename );
          imageFilename = fi.absolutePath() + "/" + fi.baseName() + "_" + QString::number( i + 1 ) + "." + fi.suffix();
        }

        bool saveOk = image.save( imageFilename, format.toLocal8Bit().constData() );
        if ( !saveOk )
        {
          QMessageBox::warning( this, tr( "Atlas processing error" ),
                                QString( tr( "Error creating %1." ) ).arg( imageFilename ),
                                QMessageBox::Ok,
                                QMessageBox::Ok );
          mView->setPaintingEnabled( true );
          QApplication::restoreOverrideCursor();
          return;
        }
      }

      //
      // Write the world file if asked to
      if ( mComposition->generateWorldFile() )
      {
        double a, b, c, d, e, f;
        mComposition->computeWorldFileParameters( a, b, c, d, e, f );

        QFileInfo fi( filename );
        // build the world file name
        QString worldFileName = fi.absolutePath() + "/" + fi.baseName() + "."
                                + fi.suffix()[0] + fi.suffix()[fi.suffix().size()-1] + "w";

        writeWorldFile( worldFileName, a, b, c, d, e, f );
      }
    }
    atlasMap->endRender();
    mView->setPaintingEnabled( true );
    QApplication::restoreOverrideCursor();
  }
}

void QgsComposer::on_mActionExportAtlasAsSVG_triggered()
{
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  exportCompositionAsSVG( QgsComposer::Atlas );
  mComposition->setAtlasMode( previousMode );

  if ( mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //after atlas output, jump back to preview first feature
    QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
    atlasMap->firstFeature();
  }
}

void QgsComposer::on_mActionExportAsSVG_triggered()
{
  exportCompositionAsSVG( QgsComposer::Single );
}

// utility class that will hide all items until it's destroyed
struct QgsItemTempHider
{
  explicit QgsItemTempHider( const QList<QGraphicsItem *> & items )
  {
    QList<QGraphicsItem *>::const_iterator it = items.begin();
    for ( ; it != items.end(); ++it )
    {
      mItemVisibility[*it] = ( *it )->isVisible();
      ( *it )->hide();
    }
  }
  void hideAll()
  {
    QgsItemVisibilityHash::iterator it = mItemVisibility.begin();
    for ( ; it != mItemVisibility.end(); ++it ) it.key()->hide();
  }
  ~QgsItemTempHider()
  {
    QgsItemVisibilityHash::iterator it = mItemVisibility.begin();
    for ( ; it != mItemVisibility.end(); ++it )
    {
      it.key()->setVisible( it.value() );
    }
  }
private:
  Q_DISABLE_COPY( QgsItemTempHider )
  typedef QHash<QGraphicsItem*, bool> QgsItemVisibilityHash;
  QgsItemVisibilityHash mItemVisibility;
};

void QgsComposer::exportCompositionAsSVG( QgsComposer::OutputMode mode )
{
  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  QString settingsLabel = "/UI/displaySVGWarning";
  QSettings settings;

  bool displaySVGWarning = settings.value( settingsLabel, true ).toBool();

  if ( displaySVGWarning )
  {
    QgsMessageViewer* m = new QgsMessageViewer( this );
    m->setWindowTitle( tr( "SVG warning" ) );
    m->setCheckBoxText( tr( "Don't show this message again" ) );
    m->setCheckBoxState( Qt::Unchecked );
    m->setCheckBoxVisible( true );
    m->setCheckBoxQSettingsLabel( settingsLabel );
    m->setMessageAsHtml( tr( "<p>The SVG export function in QGIS has several "
                             "problems due to bugs and deficiencies in the " )
                         + tr( "Qt4 svg code. In particular, there are problems "
                               "with layers not being clipped to the map "
                               "bounding box.</p>" )
                         + tr( "If you require a vector-based output file from "
                               "Qgis it is suggested that you try printing "
                               "to PostScript if the SVG output is not "
                               "satisfactory."
                               "</p>" ) );
    m->exec();
  }

  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();

  QString outputFileName;
  QString outputDir;
  bool groupLayers = false;

  if ( mode == QgsComposer::Single )
  {
    QString lastUsedFile = settings.value( "/UI/lastSaveAsSvgFile", "qgis.svg" ).toString();
    QFileInfo file( lastUsedFile );

    if ( atlasMap->enabled() )
    {
      outputFileName = QDir( file.path() ).filePath( atlasMap->currentFilename() ) + ".svg";
    }
    else
    {
      outputFileName = file.path();
    }

    // open file dialog
    outputFileName = QFileDialog::getSaveFileName(
                       this,
                       tr( "Choose a file name to save the map as" ),
                       outputFileName,
                       tr( "SVG Format" ) + " (*.svg *.SVG)" );

    if ( outputFileName.isEmpty() )
      return;

    // open otions dialog
    {
      QDialog dialog;
      Ui::QgsSvgExportOptionsDialog options;
      options.setupUi( &dialog );
      dialog.exec();
      groupLayers = options.chkMapLayersAsGroup->isChecked();
    }

    if ( !outputFileName.endsWith( ".svg", Qt::CaseInsensitive ) )
    {
      outputFileName += ".svg";
    }

    settings.setValue( "/UI/lastSaveAsSvgFile", outputFileName );
  }
  else
  {
    // If we have an Atlas
    if ( atlasMap->filenamePattern().size() == 0 )
    {
      int res = QMessageBox::warning( 0, tr( "Empty filename pattern" ),
                                      tr( "The filename pattern is empty. A default one will be used." ),
                                      QMessageBox::Ok | QMessageBox::Cancel,
                                      QMessageBox::Ok );
      if ( res == QMessageBox::Cancel )
      {
        return;
      }
      atlasMap->setFilenamePattern( "'output_'||$feature" );
    }

    QSettings myQSettings;
    QString lastUsedDir = myQSettings.value( "/UI/lastSaveAtlasAsSvgDir", "." ).toString();

    // open file dialog
    outputDir = QFileDialog::getExistingDirectory( this,
                tr( "Directory where to save SVG files" ),
                lastUsedDir,
                QFileDialog::ShowDirsOnly );

    if ( outputDir.isEmpty() )
    {
      return;
    }
    // test directory (if it exists and is writable)
    if ( !QDir( outputDir ).exists() || !QFileInfo( outputDir ).isWritable() )
    {
      QMessageBox::warning( 0, tr( "Unable to write into the directory" ),
                            tr( "The given output directory is not writable. Cancelling." ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      return;
    }

    // open otions dialog
    {
      QDialog dialog;
      Ui::QgsSvgExportOptionsDialog options;
      options.setupUi( &dialog );
      dialog.exec();
      groupLayers = options.chkMapLayersAsGroup->isChecked();
    }


    myQSettings.setValue( "/UI/lastSaveAtlasAsSvgDir", outputDir );
  }

  mView->setPaintingEnabled( false );

  int featureI = 0;
  if ( mode == QgsComposer::Atlas )
  {
    loadAtlasPredefinedScalesFromProject();
    if ( ! atlasMap->beginRender() && !atlasMap->featureFilterErrorString().isEmpty() )
    {
      QMessageBox::warning( this, tr( "Atlas processing error" ),
                            tr( "Feature filter parser error: %1" ).arg( atlasMap->featureFilterErrorString() ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      mView->setPaintingEnabled( true );
      return;
    }
  }
  QProgressDialog progress( tr( "Rendering maps..." ), tr( "Abort" ), 0, atlasMap->numFeatures(), this );

  do
  {
    if ( mode == QgsComposer::Atlas )
    {
      if ( atlasMap->numFeatures() == 0 )
        break;

      progress.setValue( featureI );
      // process input events in order to allow aborting
      QCoreApplication::processEvents();
      if ( progress.wasCanceled() )
      {
        atlasMap->endRender();
        break;
      }
      if ( !atlasMap->prepareForFeature( featureI ) )
      {
        QMessageBox::warning( this, tr( "Atlas processing error" ),
                              tr( "Atlas processing error" ),
                              QMessageBox::Ok,
                              QMessageBox::Ok );
        mView->setPaintingEnabled( true );
        return;
      }
      outputFileName = QDir( outputDir ).filePath( atlasMap->currentFilename() ) + ".svg";
    }

    if ( !groupLayers )
    {
      for ( int i = 0; i < mComposition->numPages(); ++i )
      {
        if ( !mComposition->shouldExportPage( i + 1 ) )
        {
          continue;
        }
        QSvgGenerator generator;
        generator.setTitle( QgsProject::instance()->title() );
        QString currentFileName = outputFileName;
        if ( i == 0 )
        {
          generator.setFileName( outputFileName );
        }
        else
        {
          QFileInfo fi( outputFileName );
          currentFileName = fi.absolutePath() + "/" + fi.baseName() + "_" + QString::number( i + 1 ) + "." + fi.suffix();
          generator.setFileName( currentFileName );
        }

        //width in pixel
        int width = ( int )( mComposition->paperWidth() * mComposition->printResolution() / 25.4 );
        //height in pixel
        int height = ( int )( mComposition->paperHeight() * mComposition->printResolution() / 25.4 );
        generator.setSize( QSize( width, height ) );
        generator.setViewBox( QRect( 0, 0, width, height ) );
        generator.setResolution( mComposition->printResolution() ); //because the rendering is done in mm, convert the dpi

        QPainter p;
        bool createOk = p.begin( &generator );
        if ( !createOk )
        {
          QMessageBox::warning( this, tr( "SVG export error" ),
                                QString( tr( "Error creating %1." ) ).arg( currentFileName ),
                                QMessageBox::Ok,
                                QMessageBox::Ok );
          mView->setPaintingEnabled( true );
          return;
        }

        mComposition->renderPage( &p, i );
        p.end();
      }
    }
    else
    {
      //width and height in pixel
      const int width = ( int )( mComposition->paperWidth() * mComposition->printResolution() / 25.4 );
      const int height = ( int )( mComposition->paperHeight() * mComposition->printResolution() / 25.4 );
      QList< QgsPaperItem* > paperItems( mComposition->pages() );

      for ( int i = 0; i < mComposition->numPages(); ++i )
      {
        if ( !mComposition->shouldExportPage( i + 1 ) )
        {
          continue;
        }
        QDomDocument svg;
        QDomNode svgDocRoot;
        QgsPaperItem * paperItem = paperItems[i];
        const QRectF paperRect = QRectF( paperItem->pos().x(),
                                         paperItem->pos().y(),
                                         paperItem->rect().width(),
                                         paperItem->rect().height() );

        QList<QGraphicsItem *> items = mComposition->items( paperRect,
                                       Qt::IntersectsItemBoundingRect,
                                       Qt::AscendingOrder );
        if ( ! items.isEmpty()
             && dynamic_cast<QgsPaperGrid*>( items.last() )
             && !mComposition->gridVisible() ) items.pop_back();
        QgsItemTempHider itemsHider( items );
        int composerItemLayerIdx = 0;
        QList<QGraphicsItem *>::const_iterator it = items.begin();
        for ( unsigned svgLayerId = 1; it != items.end(); ++svgLayerId )
        {
          itemsHider.hideAll();
          QgsComposerItem * composerItem = dynamic_cast<QgsComposerItem*>( *it );
          QString layerName( "Layer " + QString::number( svgLayerId ) );
          if ( composerItem && composerItem->numberExportLayers() )
          {
            composerItem->show();
            composerItem->setCurrentExportLayer( composerItemLayerIdx );
            ++composerItemLayerIdx;
          }
          else
          {
            // show all items until the next item that renders on a separate layer
            for ( ; it != items.end(); ++it )
            {
              composerItem = dynamic_cast<QgsComposerMap*>( *it );
              if ( composerItem && composerItem->numberExportLayers() )
              {
                break;
              }
              else
              {
                ( *it )->show();
              }
            }
          }

          QBuffer svgBuffer;
          {
            QSvgGenerator generator;
            generator.setTitle( QgsProject::instance()->title() );
            generator.setOutputDevice( &svgBuffer );
            generator.setSize( QSize( width, height ) );
            generator.setViewBox( QRect( 0, 0, width, height ) );
            generator.setResolution( mComposition->printResolution() ); //because the rendering is done in mm, convert the dpi

            QPainter p( &generator );
            mComposition->renderPage( &p, i );
          }
          // post-process svg output to create groups in a single svg file
          // we create inkscape layers since it's nice and clean and free
          // and fully svg compatible
          {
            svgBuffer.close();
            svgBuffer.open( QIODevice::ReadOnly );
            QDomDocument doc;
            QString errorMsg;
            int errorLine;
            if ( ! doc.setContent( &svgBuffer, false, &errorMsg, &errorLine ) )
              QMessageBox::warning( 0, tr( "SVG error" ), tr( "There was an error in SVG output for SVG layer " ) + layerName + tr( " on page " ) + QString::number( i + 1 ) + "(" + errorMsg + ")" );
            if ( 1 == svgLayerId )
            {
              svg = QDomDocument( doc.doctype() );
              svg.appendChild( svg.importNode( doc.firstChild(), false ) );
              svgDocRoot = svg.importNode( doc.elementsByTagName( "svg" ).at( 0 ), false );
              svgDocRoot.toElement().setAttribute( "xmlns:inkscape", "http://www.inkscape.org/namespaces/inkscape" );
              svg.appendChild( svgDocRoot );
            }
            QDomNode mainGroup = svg.importNode( doc.elementsByTagName( "g" ).at( 0 ), true );
            mainGroup.toElement().setAttribute( "id", layerName );
            mainGroup.toElement().setAttribute( "inkscape:label", layerName );
            mainGroup.toElement().setAttribute( "inkscape:groupmode", "layer" );
            QDomNode defs = svg.importNode( doc.elementsByTagName( "defs" ).at( 0 ), true );
            svgDocRoot.appendChild( defs );
            svgDocRoot.appendChild( mainGroup );
          }

          if ( composerItem && composerItem->numberExportLayers() && composerItem->numberExportLayers() == composerItemLayerIdx ) // restore and pass to next item
          {
            composerItem->setCurrentExportLayer();
            composerItemLayerIdx = 0;
            ++it;
          }
        }
        QFileInfo fi( outputFileName );
        QString currentFileName = i == 0 ? outputFileName : fi.absolutePath() + "/" + fi.baseName() + "_" + QString::number( i + 1 ) + "." + fi.suffix();
        QFile out( currentFileName );
        bool openOk = out.open( QIODevice::WriteOnly | QIODevice::Text );
        if ( !openOk )
        {
          QMessageBox::warning( this, tr( "SVG export error" ),
                                QString( tr( "Error creating %1." ) ).arg( currentFileName ),
                                QMessageBox::Ok,
                                QMessageBox::Ok );
          mView->setPaintingEnabled( true );
          return;
        }

        out.write( svg.toByteArray() );
      }
    }
    featureI++;
  }
  while ( mode == QgsComposer::Atlas && featureI < atlasMap->numFeatures() );

  if ( mode == QgsComposer::Atlas )
    atlasMap->endRender();

  mView->setPaintingEnabled( true );
}

void QgsComposer::on_mActionSelectMoveItem_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::Select );
  }
}

void QgsComposer::on_mActionAddNewMap_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddMap );
  }
}

void QgsComposer::on_mActionAddNewLegend_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddLegend );
  }
}

void QgsComposer::on_mActionAddNewLabel_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddLabel );
  }
}

void QgsComposer::on_mActionAddNewScalebar_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddScalebar );
  }
}

void QgsComposer::on_mActionAddImage_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddPicture );
  }
}

void QgsComposer::on_mActionAddRectangle_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddRectangle );
  }
}

void QgsComposer::on_mActionAddTriangle_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddTriangle );
  }
}

void QgsComposer::on_mActionAddEllipse_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddEllipse );
  }
}

void QgsComposer::on_mActionAddTable_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddTable );
  }
}

void QgsComposer::on_mActionAddAttributeTable_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddAttributeTable );
  }
}

void QgsComposer::on_mActionAddHtml_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddHtml );
  }
}

void QgsComposer::on_mActionAddArrow_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::AddArrow );
  }
}

void QgsComposer::on_mActionSaveProject_triggered()
{
  mQgis->actionSaveProject()->trigger();
}

void QgsComposer::on_mActionNewComposer_triggered()
{
  QString title = mQgis->uniqueComposerTitle( this, true );
  if ( title.isNull() )
  {
    return;
  }
  mQgis->createNewComposer( title );
}

void QgsComposer::on_mActionDuplicateComposer_triggered()
{
  QString newTitle = mQgis->uniqueComposerTitle( this, false, title() + tr( " copy" ) );
  if ( newTitle.isNull() )
  {
    return;
  }

  // provide feedback, since loading of template into duplicate composer will be hidden
  QDialog* dlg = new QgsBusyIndicatorDialog( tr( "Duplicating composer..." ) );
  dlg->setStyleSheet( mQgis->styleSheet() );
  dlg->show();

  QgsComposer* newComposer = mQgis->duplicateComposer( this, newTitle );

  dlg->close();
  delete dlg;
  dlg = 0;

  if ( !newComposer )
  {
    QMessageBox::warning( this, tr( "Duplicate Composer" ),
                          tr( "Composer duplication failed." ) );
  }
}

void QgsComposer::on_mActionComposerManager_triggered()
{
  // NOTE: Avoid crash where composer that spawned modal manager from toolbar ends up
  // being deleted by user, but event loop tries to return to composer on manager close
  // (does not seem to be an issue for menu action)
  QTimer::singleShot( 0, mQgis->actionShowComposerManager(), SLOT( trigger() ) );
}

void QgsComposer::on_mActionSaveAsTemplate_triggered()
{
  //show file dialog
  QSettings settings;
  QString lastSaveDir = settings.value( "UI/lastComposerTemplateDir", "" ).toString();
  QString saveFileName = QFileDialog::getSaveFileName(
                           this,
                           tr( "Save template" ),
                           lastSaveDir,
                           tr( "Composer templates" ) + " (*.qpt *.QPT)" );
  if ( saveFileName.isEmpty() )
    return;

  QFileInfo saveFileInfo( saveFileName );
  //check if suffix has been added
  if ( saveFileInfo.suffix().isEmpty() )
  {
    QString saveFileNameWithSuffix = saveFileName.append( ".qpt" );
    saveFileInfo = QFileInfo( saveFileNameWithSuffix );
  }
  settings.setValue( "UI/lastComposerTemplateDir", saveFileInfo.absolutePath() );

  QFile templateFile( saveFileName );
  if ( !templateFile.open( QIODevice::WriteOnly ) )
  {
    return;
  }

  QDomDocument saveDocument;
  templateXML( saveDocument );

  if ( templateFile.write( saveDocument.toByteArray() ) == -1 )
  {
    QMessageBox::warning( 0, tr( "Save error" ), tr( "Error, could not save file" ) );
  }
}

void QgsComposer::on_mActionLoadFromTemplate_triggered()
{
  loadTemplate( false );
}

void QgsComposer::loadTemplate( const bool newComposer )
{
  QSettings settings;
  QString openFileDir = settings.value( "UI/lastComposerTemplateDir", "" ).toString();
  QString openFileString = QFileDialog::getOpenFileName( 0, tr( "Load template" ), openFileDir, "*.qpt" );

  if ( openFileString.isEmpty() )
  {
    return; //canceled by the user
  }

  QFileInfo openFileInfo( openFileString );
  settings.setValue( "UI/LastComposerTemplateDir", openFileInfo.absolutePath() );

  QFile templateFile( openFileString );
  if ( !templateFile.open( QIODevice::ReadOnly ) )
  {
    QMessageBox::warning( this, tr( "Read error" ), tr( "Error, could not read file" ) );
    return;
  }

  QgsComposer* c = 0;
  QgsComposition* comp = 0;

  if ( newComposer )
  {
    QString newTitle = mQgis->uniqueComposerTitle( this, true );
    if ( newTitle.isNull() )
    {
      return;
    }
    c = mQgis->createNewComposer( newTitle );
    if ( !c )
    {
      QMessageBox::warning( this, tr( "Composer error" ), tr( "Error, could not create new composer" ) );
      return;
    }
    comp = c->composition();
  }
  else
  {
    c = this;
    comp = mComposition;
  }

  if ( comp )
  {
    QDomDocument templateDoc;
    if ( templateDoc.setContent( &templateFile ) )
    {
      // provide feedback, since composer will be hidden when loading template (much faster)
      QDialog* dlg = new QgsBusyIndicatorDialog( tr( "Loading template into composer..." ) );
      dlg->setStyleSheet( mQgis->styleSheet() );
      dlg->show();

      c->setUpdatesEnabled( false );
      comp->loadFromTemplate( templateDoc, 0, false, newComposer );
      c->setUpdatesEnabled( true );

      dlg->close();
      delete dlg;
      dlg = 0;
    }
  }
}

void QgsComposer::on_mActionMoveItemContent_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::MoveItemContent );
  }
}

void QgsComposer::on_mActionPan_triggered()
{
  if ( mView )
  {
    mView->setCurrentTool( QgsComposerView::Pan );
  }
}

void QgsComposer::on_mActionGroupItems_triggered()
{
  if ( mView )
  {
    mView->groupItems();
  }
}

void QgsComposer::on_mActionUngroupItems_triggered()
{
  if ( mView )
  {
    mView->ungroupItems();
  }
}

void QgsComposer::on_mActionLockItems_triggered()
{
  if ( mComposition )
  {
    mComposition->lockSelectedItems();
  }
}

void QgsComposer::on_mActionUnlockAll_triggered()
{
  if ( mComposition )
  {
    mComposition->unlockAllItems();
  }
}

void QgsComposer::actionCutTriggered()
{
  if ( mView )
  {
    mView->copyItems( QgsComposerView::ClipboardModeCut );
  }
}

void QgsComposer::actionCopyTriggered()
{
  if ( mView )
  {
    mView->copyItems( QgsComposerView::ClipboardModeCopy );
  }
}

void QgsComposer::actionPasteTriggered()
{
  if ( mView )
  {
    QPointF pt = mView->mapToScene( mView->mapFromGlobal( QCursor::pos() ) );
    //TODO - use a better way of determining whether paste was triggered by keystroke
    //or menu item
    if (( pt.x() < 0 ) || ( pt.y() < 0 ) )
    {
      //action likely triggered by menu, paste items in center of screen
      mView->pasteItems( QgsComposerView::PasteModeCenter );
    }
    else
    {
      //action likely triggered by keystroke, paste items at cursor position
      mView->pasteItems( QgsComposerView::PasteModeCursor );
    }
  }
}

void QgsComposer::on_mActionPasteInPlace_triggered()
{
  if ( mView )
  {
    mView->pasteItems( QgsComposerView::PasteModeInPlace );
  }
}

void QgsComposer::on_mActionDeleteSelection_triggered()
{
  if ( mView )
  {
    mView->deleteSelectedItems();
  }
}

void QgsComposer::on_mActionSelectAll_triggered()
{
  if ( mView )
  {
    mView->selectAll();
  }
}

void QgsComposer::on_mActionDeselectAll_triggered()
{
  if ( mView )
  {
    mView->selectNone();
  }
}

void QgsComposer::on_mActionInvertSelection_triggered()
{
  if ( mView )
  {
    mView->selectInvert();
  }
}

void QgsComposer::on_mActionSelectNextAbove_triggered()
{
  if ( mComposition )
  {
    mComposition->selectNextByZOrder( QgsComposition::ZValueAbove );
  }
}

void QgsComposer::on_mActionSelectNextBelow_triggered()
{
  if ( mComposition )
  {
    mComposition->selectNextByZOrder( QgsComposition::ZValueBelow );
  }
}

void QgsComposer::on_mActionRaiseItems_triggered()
{
  if ( mComposition )
  {
    mComposition->raiseSelectedItems();
  }
}

void QgsComposer::on_mActionLowerItems_triggered()
{
  if ( mComposition )
  {
    mComposition->lowerSelectedItems();
  }
}

void QgsComposer::on_mActionMoveItemsToTop_triggered()
{
  if ( mComposition )
  {
    mComposition->moveSelectedItemsToTop();
  }
}

void QgsComposer::on_mActionMoveItemsToBottom_triggered()
{
  if ( mComposition )
  {
    mComposition->moveSelectedItemsToBottom();
  }
}

void QgsComposer::on_mActionAlignLeft_triggered()
{
  if ( mComposition )
  {
    mComposition->alignSelectedItemsLeft();
  }
}

void QgsComposer::on_mActionAlignHCenter_triggered()
{
  if ( mComposition )
  {
    mComposition->alignSelectedItemsHCenter();
  }
}

void QgsComposer::on_mActionAlignRight_triggered()
{
  if ( mComposition )
  {
    mComposition->alignSelectedItemsRight();
  }
}

void QgsComposer::on_mActionAlignTop_triggered()
{
  if ( mComposition )
  {
    mComposition->alignSelectedItemsTop();
  }
}

void QgsComposer::on_mActionAlignVCenter_triggered()
{
  if ( mComposition )
  {
    mComposition->alignSelectedItemsVCenter();
  }
}

void QgsComposer::on_mActionAlignBottom_triggered()
{
  if ( mComposition )
  {
    mComposition->alignSelectedItemsBottom();
  }
}

void QgsComposer::on_mActionUndo_triggered()
{
  if ( mComposition && mComposition->undoStack() )
  {
    mComposition->undoStack()->undo();
  }
}

void QgsComposer::on_mActionRedo_triggered()
{
  if ( mComposition && mComposition->undoStack() )
  {
    mComposition->undoStack()->redo();
  }
}

void QgsComposer::closeEvent( QCloseEvent *e )
{
  Q_UNUSED( e );
  saveWindowState();
}

void QgsComposer::moveEvent( QMoveEvent *e )
{
  Q_UNUSED( e );
  saveWindowState();
}

void QgsComposer::resizeEvent( QResizeEvent *e )
{
  Q_UNUSED( e );

  // Move size grip when window is resized
#if 0
  mSizeGrip->move( rect().bottomRight() - mSizeGrip->rect().bottomRight() );
#endif

  saveWindowState();
}

void QgsComposer::showEvent( QShowEvent* event )
{
  if ( event->spontaneous() ) //event from the window system
  {
    restoreComposerMapStates();
  }

#ifdef Q_OS_MAC
  // add to menu if (re)opening window (event not due to unminimize)
  if ( !event->spontaneous() )
  {
    mQgis->addWindow( mWindowAction );
  }
#endif
}

void QgsComposer::saveWindowState()
{
  QSettings settings;
  settings.setValue( "/Composer/geometry", saveGeometry() );
  // store the toolbar/dock widget settings using Qt4 settings API
  settings.setValue( "/ComposerUI/state", saveState() );
}

#include "ui_defaults.h"

void QgsComposer::restoreWindowState()
{
  // restore the toolbar and dock widgets postions using Qt4 settings API
  QSettings settings;

  if ( !restoreState( settings.value( "/ComposerUI/state", QByteArray::fromRawData(( char * )defaultComposerUIstate, sizeof defaultComposerUIstate ) ).toByteArray() ) )
  {
    QgsDebugMsg( "restore of composer UI state failed" );
  }
  // restore window geometry
  if ( !restoreGeometry( settings.value( "/Composer/geometry", QByteArray::fromRawData(( char * )defaultComposerUIgeometry, sizeof defaultComposerUIgeometry ) ).toByteArray() ) )
  {
    QgsDebugMsg( "restore of composer UI geometry failed" );
  }
}

void  QgsComposer::writeXML( QDomDocument& doc )
{

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();
  if ( qgisElem.isNull() )
  {
    return;
  }

  writeXML( qgisElem, doc );
}

void QgsComposer::writeXML( QDomNode& parentNode, QDomDocument& doc )
{
  QDomElement composerElem = doc.createElement( "Composer" );
  composerElem.setAttribute( "title", mTitle );

  //change preview mode of minimised / hidden maps before saving XML (show contents only on demand)
  QMap< QgsComposerMap*, int >::iterator mapIt = mMapsToRestore.begin();
  for ( ; mapIt != mMapsToRestore.end(); ++mapIt )
  {
    mapIt.key()->setPreviewMode(( QgsComposerMap::PreviewMode )( mapIt.value() ) );
  }
  mMapsToRestore.clear();

  //store if composer is open or closed
  if ( isVisible() )
  {
    composerElem.setAttribute( "visible", 1 );
  }
  else
  {
    composerElem.setAttribute( "visible", 0 );
  }
  parentNode.appendChild( composerElem );

  //store composition
  if ( mComposition )
  {
    mComposition->writeXML( composerElem, doc );
  }

  // store atlas
  mComposition->atlasComposition().writeXML( composerElem, doc );
}

void  QgsComposer::templateXML( QDomDocument& doc )
{
  writeXML( doc, doc );
}

void QgsComposer::readXML( const QDomDocument& doc )
{
  QDomNodeList composerNodeList = doc.elementsByTagName( "Composer" );
  if ( composerNodeList.size() < 1 )
  {
    return;
  }
  readXML( composerNodeList.at( 0 ).toElement(), doc, true );
  cleanupAfterTemplateRead();
}

void QgsComposer::createCompositionWidget()
{
  if ( !mComposition )
  {
    return;
  }

  QgsCompositionWidget* compositionWidget = new QgsCompositionWidget( mGeneralDock, mComposition );
  connect( mComposition, SIGNAL( paperSizeChanged() ), compositionWidget, SLOT( displayCompositionWidthHeight() ) );
  connect( this, SIGNAL( printAsRasterChanged( bool ) ), compositionWidget, SLOT( setPrintAsRasterCheckBox( bool ) ) );
  connect( compositionWidget, SIGNAL( pageOrientationChanged( QString ) ), this, SLOT( setPrinterPageOrientation( QString ) ) );
  mGeneralDock->setWidget( compositionWidget );
}

void QgsComposer::readXML( const QDomElement& composerElem, const QDomDocument& doc, bool fromTemplate )
{
  // Set title only if reading from project file
  if ( !fromTemplate )
  {
    if ( composerElem.hasAttribute( "title" ) )
    {
      setTitle( composerElem.attribute( "title", tr( "Composer" ) ) );
    }
  }

  //delete composition widget
  QgsCompositionWidget* oldCompositionWidget = qobject_cast<QgsCompositionWidget *>( mGeneralDock->widget() );
  delete oldCompositionWidget;

  createComposerView();

  //read composition settings
  mComposition = new QgsComposition( mQgis->mapCanvas()->mapSettings() );
  QDomNodeList compositionNodeList = composerElem.elementsByTagName( "Composition" );
  if ( compositionNodeList.size() > 0 )
  {
    QDomElement compositionElem = compositionNodeList.at( 0 ).toElement();
    mComposition->readXML( compositionElem, doc );
  }

  connectViewSlots();
  connectCompositionSlots();
  createCompositionWidget();

  //read and restore all the items
  QDomElement atlasElem;
  if ( mComposition )
  {
    // read atlas parameters - must be done before adding items
    atlasElem = composerElem.firstChildElement( "Atlas" );
    mComposition->atlasComposition().readXML( atlasElem, doc );

    mComposition->addItemsFromXML( composerElem, doc, &mMapsToRestore );
  }

  //restore grid settings
  restoreGridSettings();

  // look for world file composer map, if needed
  // Note: this must be done after maps have been added by addItemsFromXML
  if ( mComposition->generateWorldFile() )
  {
    QDomElement compositionElem = compositionNodeList.at( 0 ).toElement();
    QgsComposerMap* worldFileMap = 0;
    QList<const QgsComposerMap*> maps = mComposition->composerMapItems();
    for ( QList<const QgsComposerMap*>::const_iterator it = maps.begin(); it != maps.end(); ++it )
    {
      if (( *it )->id() == compositionElem.attribute( "worldFileMap" ).toInt() )
      {
        worldFileMap = const_cast<QgsComposerMap*>( *it );
        break;
      }
    }
    mComposition->setWorldFileMap( worldFileMap );
  }

  //make sure z values are consistent
  mComposition->refreshZList();

  //disconnect from view's compositionSet signal, since that will be emitted automatically
  disconnect( mView, SIGNAL( compositionSet( QgsComposition* ) ), this, SLOT( setComposition( QgsComposition* ) ) );
  mView->setComposition( mComposition );
  connect( mView, SIGNAL( compositionSet( QgsComposition* ) ), this, SLOT( setComposition( QgsComposition* ) ) );

  setupUndoView();

  //delete old atlas composition widget
  QgsAtlasCompositionWidget* oldAtlasWidget = qobject_cast<QgsAtlasCompositionWidget *>( mAtlasDock->widget() );
  delete oldAtlasWidget;
  mAtlasDock->setWidget( new QgsAtlasCompositionWidget( mAtlasDock, mComposition ) );

  //read atlas map parameters (for pre 2.2 templates)
  //this part must be done after adding items
  mComposition->atlasComposition().readXMLMapSettings( atlasElem, doc );

  //set state of atlas controls
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  toggleAtlasControls( atlasMap->enabled() );
  connect( atlasMap, SIGNAL( toggled( bool ) ), this, SLOT( toggleAtlasControls( bool ) ) );
  connect( atlasMap, SIGNAL( coverageLayerChanged( QgsVectorLayer* ) ), this, SLOT( updateAtlasMapLayerAction( QgsVectorLayer * ) ) );

  //default printer page setup
  setPrinterPageDefaults();

  //setup items tree view
  mItemsTreeView->setModel( mComposition->itemsModel() );
  mItemsTreeView->setColumnWidth( 0, 30 );
  mItemsTreeView->setColumnWidth( 1, 30 );
  connect( mItemsTreeView->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), mComposition->itemsModel(), SLOT( setSelected( QModelIndex ) ) );

  setSelectionTool();
}

void QgsComposer::setupUndoView()
{
  if ( !mUndoView || !mComposition )
  {
    return;
  }

  //init undo/redo buttons
  mActionUndo->setEnabled( false );
  mActionRedo->setEnabled( false );
  if ( mComposition->undoStack() )
  {
    mUndoView->setStack( mComposition->undoStack() );
    connect( mComposition->undoStack(), SIGNAL( canUndoChanged( bool ) ), mActionUndo, SLOT( setEnabled( bool ) ) );
    connect( mComposition->undoStack(), SIGNAL( canRedoChanged( bool ) ), mActionRedo, SLOT( setEnabled( bool ) ) );
  }
}

void QgsComposer::restoreGridSettings()
{
  //restore grid settings
  mActionSnapGrid->setChecked( mComposition->snapToGridEnabled() );
  mActionShowGrid->setChecked( mComposition->gridVisible() );
  //restore guide settings
  mActionShowGuides->setChecked( mComposition->snapLinesVisible() );
  mActionSnapGuides->setChecked( mComposition->alignmentSnap() );
  mActionSmartGuides->setChecked( mComposition->smartGuidesEnabled() );
  //general view settings
  mActionShowBoxes->setChecked( mComposition->boundingBoxesVisible() );
}

void QgsComposer::deleteItemWidgets()
{
  //delete all the items
  QMap<QgsComposerItem*, QWidget*>::iterator it = mItemWidgetMap.begin();
  for ( ; it != mItemWidgetMap.end(); ++it )
  {
    delete it.value();
  }
  mItemWidgetMap.clear();
}

void QgsComposer::addComposerArrow( QgsComposerArrow* arrow )
{
  if ( !arrow )
  {
    return;
  }

  QgsComposerArrowWidget* arrowWidget = new QgsComposerArrowWidget( arrow );
  mItemWidgetMap.insert( arrow, arrowWidget );
}

void QgsComposer::addComposerMap( QgsComposerMap* map )
{
  if ( !map )
  {
    return;
  }

  map->setMapCanvas( mapCanvas() ); //set canvas to composer map to have the possibility to draw canvas items
  QgsComposerMapWidget* mapWidget = new QgsComposerMapWidget( map );
  connect( this, SIGNAL( zoomLevelChanged() ), map, SLOT( renderModeUpdateCachedImage() ) );
  mItemWidgetMap.insert( map, mapWidget );
}

void QgsComposer::addComposerLabel( QgsComposerLabel* label )
{
  if ( !label )
  {
    return;
  }

  QgsComposerLabelWidget* labelWidget = new QgsComposerLabelWidget( label );
  mItemWidgetMap.insert( label, labelWidget );
}

void QgsComposer::addComposerScaleBar( QgsComposerScaleBar* scalebar )
{
  if ( !scalebar )
  {
    return;
  }

  QgsComposerScaleBarWidget* sbWidget = new QgsComposerScaleBarWidget( scalebar );
  mItemWidgetMap.insert( scalebar, sbWidget );
}

void QgsComposer::addComposerLegend( QgsComposerLegend* legend )
{
  if ( !legend )
  {
    return;
  }

  QgsComposerLegendWidget* lWidget = new QgsComposerLegendWidget( legend );
  mItemWidgetMap.insert( legend, lWidget );
}

void QgsComposer::addComposerPicture( QgsComposerPicture* picture )
{
  if ( !picture )
  {
    return;
  }

  QgsComposerPictureWidget* pWidget = new QgsComposerPictureWidget( picture );
  mItemWidgetMap.insert( picture, pWidget );
}

void QgsComposer::addComposerShape( QgsComposerShape* shape )
{
  if ( !shape )
  {
    return;
  }
  QgsComposerShapeWidget* sWidget = new QgsComposerShapeWidget( shape );
  mItemWidgetMap.insert( shape, sWidget );
}

void QgsComposer::addComposerTable( QgsComposerAttributeTable* table )
{
  if ( !table )
  {
    return;
  }

  QgsComposerTableWidget* tWidget = new QgsComposerTableWidget( table );
  mItemWidgetMap.insert( table, tWidget );
}

void QgsComposer::addComposerTableV2( QgsComposerAttributeTableV2 *table, QgsComposerFrame* frame )
{
  if ( !table )
  {
    return;
  }
  QgsComposerAttributeTableWidget* tWidget = new QgsComposerAttributeTableWidget( table, frame );
  mItemWidgetMap.insert( frame, tWidget );
}

void QgsComposer::addComposerHtmlFrame( QgsComposerHtml* html, QgsComposerFrame* frame )
{
  if ( !html )
  {
    return;
  }

  QgsComposerHtmlWidget* hWidget = new QgsComposerHtmlWidget( html, frame );
  mItemWidgetMap.insert( frame, hWidget );
}

void QgsComposer::deleteItem( QgsComposerItem* item )
{
  QMap<QgsComposerItem*, QWidget*>::iterator it = mItemWidgetMap.find( item );

  if ( it == mItemWidgetMap.end() )
  {
    return;
  }

  //the item itself is not deleted here (usually, this is done in the destructor of QgsAddRemoveItemCommand)
  it.value()->deleteLater();
  mItemWidgetMap.remove( it.key() );

  QgsComposerMap* map = dynamic_cast<QgsComposerMap*>( item );
  if ( map )
  {
    mMapsToRestore.remove( map );
  }
}

void QgsComposer::setSelectionTool()
{
  mActionSelectMoveItem->setChecked( true );
  on_mActionSelectMoveItem_triggered();
}

bool QgsComposer::containsWMSLayer() const
{
  QMap<QgsComposerItem*, QWidget*>::const_iterator item_it = mItemWidgetMap.constBegin();
  QgsComposerItem* currentItem = 0;
  QgsComposerMap* currentMap = 0;

  for ( ; item_it != mItemWidgetMap.constEnd(); ++item_it )
  {
    currentItem = item_it.key();
    currentMap = dynamic_cast<QgsComposerMap *>( currentItem );
    if ( currentMap )
    {
      if ( currentMap->containsWMSLayer() )
      {
        return true;
      }
    }
  }
  return false;
}

bool QgsComposer::containsAdvancedEffects() const
{
  // Check if composer contains any blend modes or flattened layers for transparency
  QMap<QgsComposerItem*, QWidget*>::const_iterator item_it = mItemWidgetMap.constBegin();
  QgsComposerItem* currentItem = 0;
  QgsComposerMap* currentMap = 0;

  for ( ; item_it != mItemWidgetMap.constEnd(); ++item_it )
  {
    currentItem = item_it.key();
    // Check composer item's blend mode
    if ( currentItem->blendMode() != QPainter::CompositionMode_SourceOver )
    {
      return true;
    }
    // If item is a composer map, check if it contains any advanced effects
    currentMap = dynamic_cast<QgsComposerMap *>( currentItem );
    if ( currentMap && currentMap->containsAdvancedEffects() )
    {
      return true;
    }
  }
  return false;
}

void QgsComposer::showWMSPrintingWarning()
{
  QString myQSettingsLabel = "/UI/displayComposerWMSWarning";
  QSettings myQSettings;

  bool displayWMSWarning = myQSettings.value( myQSettingsLabel, true ).toBool();
  if ( displayWMSWarning )
  {
    QgsMessageViewer* m = new QgsMessageViewer( this );
    m->setWindowTitle( tr( "Project contains WMS layers" ) );
    m->setMessage( tr( "Some WMS servers (e.g. UMN mapserver) have a limit for the WIDTH and HEIGHT parameter. Printing layers from such servers may exceed this limit. If this is the case, the WMS layer will not be printed" ), QgsMessageOutput::MessageText );
    m->setCheckBoxText( tr( "Don't show this message again" ) );
    m->setCheckBoxState( Qt::Unchecked );
    m->setCheckBoxVisible( true );
    m->setCheckBoxQSettingsLabel( myQSettingsLabel );
    m->exec();
  }
}

void QgsComposer::showAdvancedEffectsWarning()
{
  if ( ! mComposition->printAsRaster() )
  {
    QgsMessageViewer* m = new QgsMessageViewer( this, QgisGui::ModalDialogFlags, false );
    m->setWindowTitle( tr( "Project contains composition effects" ) );
    m->setMessage( tr( "Advanced composition effects such as blend modes or vector layer transparency are enabled in this project, which cannot be printed as vectors. Printing as a raster is recommended." ), QgsMessageOutput::MessageText );
    m->setCheckBoxText( tr( "Print as raster" ) );
    m->setCheckBoxState( Qt::Checked );
    m->setCheckBoxVisible( true );
    m->showMessage( true );

    if ( m->checkBoxState() == Qt::Checked )
    {
      mComposition->setPrintAsRaster( true );
      //make sure print as raster checkbox is updated
      emit printAsRasterChanged( true );
    }
    else
    {
      mComposition->setPrintAsRaster( false );
      emit printAsRasterChanged( false );
    }

    delete m;
  }
}

void QgsComposer::cleanupAfterTemplateRead()
{
  QMap<QgsComposerItem*, QWidget*>::const_iterator itemIt = mItemWidgetMap.constBegin();
  for ( ; itemIt != mItemWidgetMap.constEnd(); ++itemIt )
  {
    //update all legends completely
    QgsComposerLegend* legendItem = dynamic_cast<QgsComposerLegend *>( itemIt.key() );
    if ( legendItem )
    {
      legendItem->updateLegend();
      continue;
    }

    //update composer map extent if it does not intersect the full extent of all layers
    QgsComposerMap* mapItem = dynamic_cast<QgsComposerMap *>( itemIt.key() );
    if ( mapItem )
    {
      //test if composer map extent intersects extent of all layers
      bool intersects = false;
      QgsRectangle composerMapExtent = mapItem->extent();
      if ( mQgis )
      {
        QgsMapCanvas* canvas = mQgis->mapCanvas();
        if ( canvas )
        {
          QgsRectangle mapCanvasExtent = mQgis->mapCanvas()->fullExtent();
          if ( composerMapExtent.intersects( mapCanvasExtent ) )
          {
            intersects = true;
          }
        }
      }

      //if not: apply current canvas extent
      if ( !intersects )
      {
        double currentWidth = mapItem->rect().width();
        double currentHeight = mapItem->rect().height();
        if ( currentWidth - 0 > 0.0 ) //don't divide through zero
        {
          QgsRectangle canvasExtent = mComposition->mapSettings().visibleExtent();
          //adapt min y of extent such that the size of the map item stays the same
          double newCanvasExtentHeight = currentHeight / currentWidth * canvasExtent.width();
          canvasExtent.setYMinimum( canvasExtent.yMaximum() - newCanvasExtentHeight );
          mapItem->setNewExtent( canvasExtent );
        }
      }
    }
  }

  restoreComposerMapStates();
}

void QgsComposer::on_mActionPageSetup_triggered()
{
  if ( !mComposition )
  {
    return;
  }

  QPageSetupDialog pageSetupDialog( &mPrinter, this );
  pageSetupDialog.exec();
}

void QgsComposer::restoreComposerMapStates()
{
  //go through maps and restore original preview modes (show on demand after loading from project file)
  QMap< QgsComposerMap*, int >::iterator mapIt = mMapsToRestore.begin();
  for ( ; mapIt != mMapsToRestore.end(); ++mapIt )
  {
    mapIt.key()->setPreviewMode(( QgsComposerMap::PreviewMode )( mapIt.value() ) );
    mapIt.key()->cache();
    mapIt.key()->update();
  }
  mMapsToRestore.clear();
}

void QgsComposer::populatePrintComposersMenu()
{
  mPrintComposersMenu->clear();
  QList<QAction*> acts = mQgis->printComposersMenu()->actions();
  if ( acts.size() > 1 )
  {
    // sort actions in case main app's aboutToShow slot has not yet
    qSort( acts.begin(), acts.end(), cmpByText_ );
  }
  mPrintComposersMenu->addActions( acts );
}

void QgsComposer::populateWindowMenu()
{
  populateWithOtherMenu( mWindowMenu, mQgis->windowMenu() );
}

void QgsComposer::populateHelpMenu()
{
  populateWithOtherMenu( mHelpMenu, mQgis->helpMenu() );
}

void QgsComposer::populateWithOtherMenu( QMenu* thisMenu, QMenu* otherMenu )
{
  thisMenu->clear();
  foreach ( QAction* act, otherMenu->actions() )
  {
    if ( act->menu() )
    {
      thisMenu->addMenu( mirrorOtherMenu( act->menu() ) );
    }
    else
    {
      thisMenu->addAction( act );
    }
  }
}

QMenu* QgsComposer::mirrorOtherMenu( QMenu* otherMenu )
{
  QMenu* newMenu = new QMenu( otherMenu->title(), this );
  foreach ( QAction* act, otherMenu->actions() )
  {
    if ( act->menu() )
    {
      newMenu->addMenu( mirrorOtherMenu( act->menu() ) );
    }
    else
    {
      newMenu->addAction( act );
    }
  }
  return newMenu;
}

void QgsComposer::createComposerView()
{
  if ( !mViewLayout )
  {
    return;
  }

  delete mView;
  mView = new QgsComposerView();
  mView->setContentsMargins( 0, 0, 0, 0 );
  mView->setHorizontalRuler( mHorizontalRuler );
  mView->setVerticalRuler( mVerticalRuler );
  mViewLayout->addWidget( mView, 1, 1 );

  //view does not accept focus via tab
  mView->setFocusPolicy( Qt::ClickFocus );
  //instead, if view is focused and tab is pressed than mActionHidePanels is triggered,
  //to toggle display of panels
  QShortcut* tab = new QShortcut( Qt::Key_Tab, mView );
  tab->setContext( Qt::WidgetWithChildrenShortcut );
  connect( tab, SIGNAL( activated() ), mActionHidePanels, SLOT( trigger() ) );
}

void QgsComposer::writeWorldFile( QString worldFileName, double a, double b, double c, double d, double e, double f ) const
{
  QFile worldFile( worldFileName );
  if ( !worldFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    return;
  }
  QTextStream fout( &worldFile );

  // QString::number does not use locale settings (for the decimal point)
  // which is what we want here
  fout << QString::number( a, 'f' ) << "\r\n";
  fout << QString::number( d, 'f' ) << "\r\n";
  fout << QString::number( b, 'f' ) << "\r\n";
  fout << QString::number( e, 'f' ) << "\r\n";
  fout << QString::number( c, 'f' ) << "\r\n";
  fout << QString::number( f, 'f' ) << "\r\n";
}


void QgsComposer::setAtlasFeature( QgsMapLayer* layer, const QgsFeature& feat )
{
  //check if composition atlas settings match
  QgsAtlasComposition& atlas = mComposition->atlasComposition();
  if ( ! atlas.enabled() || atlas.coverageLayer() != layer )
  {
    //either atlas isn't enabled, or layer doesn't match
    return;
  }

  if ( mComposition->atlasMode() != QgsComposition::PreviewAtlas )
  {
    mComposition->setAtlasMode( QgsComposition::PreviewAtlas );
    //update gui controls
    mActionAtlasPreview->blockSignals( true );
    mActionAtlasPreview->setChecked( true );
    mActionAtlasPreview->blockSignals( false );
    mActionAtlasFirst->setEnabled( true );
    mActionAtlasLast->setEnabled( true );
    mActionAtlasNext->setEnabled( true );
    mActionAtlasPrev->setEnabled( true );
  }

  //bring composer window to foreground
  activate();

  mapCanvas()->stopRendering();

  //set current preview feature id
  atlas.prepareForFeature( &feat );
  emit( atlasPreviewFeatureChanged() );
}

void QgsComposer::updateAtlasMapLayerAction( QgsVectorLayer *coverageLayer )
{
  if ( mAtlasFeatureAction )
  {
    delete mAtlasFeatureAction;
    mAtlasFeatureAction = 0;
  }

  if ( coverageLayer )
  {
    mAtlasFeatureAction = new QgsMapLayerAction( QString( tr( "Set as atlas feature for %1" ) ).arg( mTitle ),
        this, coverageLayer, QgsMapLayerAction::SingleFeature ,
        QgsApplication::getThemeIcon( "/mIconAtlas.svg" ) );
    QgsMapLayerActionRegistry::instance()->addMapLayerAction( mAtlasFeatureAction );
    connect( mAtlasFeatureAction, SIGNAL( triggeredForFeature( QgsMapLayer*, const QgsFeature& ) ), this, SLOT( setAtlasFeature( QgsMapLayer*, const QgsFeature& ) ) );
  }
}

void QgsComposer::setPrinterPageOrientation( QString orientation )
{
  if ( orientation == tr( "Landscape" ) )
  {
    mPrinter.setOrientation( QPrinter::Landscape );
  }
  else
  {
    mPrinter.setOrientation( QPrinter::Portrait );
  }
}

void QgsComposer::setPrinterPageDefaults()
{
  double paperWidth = mComposition->paperWidth();
  double paperHeight = mComposition->paperHeight();

  //set printer page orientation
  if ( paperWidth > paperHeight )
  {
    mPrinter.setOrientation( QPrinter::Landscape );
  }
  else
  {
    mPrinter.setOrientation( QPrinter::Portrait );
  }
}

void QgsComposer::updateAtlasMapLayerAction( bool atlasEnabled )
{
  if ( mAtlasFeatureAction )
  {
    delete mAtlasFeatureAction;
    mAtlasFeatureAction = 0;
  }

  if ( atlasEnabled )
  {
    QgsAtlasComposition& atlas = mComposition->atlasComposition();
    mAtlasFeatureAction = new QgsMapLayerAction( QString( tr( "Set as atlas feature for %1" ) ).arg( mTitle ),
        this, atlas.coverageLayer(), QgsMapLayerAction::SingleFeature ,
        QgsApplication::getThemeIcon( "/mIconAtlas.svg" ) );
    QgsMapLayerActionRegistry::instance()->addMapLayerAction( mAtlasFeatureAction );
    connect( mAtlasFeatureAction, SIGNAL( triggeredForFeature( QgsMapLayer*, const QgsFeature& ) ), this, SLOT( setAtlasFeature( QgsMapLayer*, const QgsFeature& ) ) );
  }
}

void QgsComposer::loadAtlasPredefinedScalesFromProject()
{
  if ( !mComposition )
  {
    return;
  }
  QgsAtlasComposition& atlasMap = mComposition->atlasComposition();
  QVector<qreal> pScales;
  // first look at project's scales
  QStringList scales( QgsProject::instance()->readListEntry( "Scales", "/ScalesList" ) );
  bool hasProjectScales( QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" ) );
  if ( !hasProjectScales || scales.isEmpty() )
  {
    // default to global map tool scales
    QSettings settings;
    QString scalesStr( settings.value( "Map/scales", PROJECT_SCALES ).toString() );
    scales = scalesStr.split( "," );
  }

  for ( QStringList::const_iterator scaleIt = scales.constBegin(); scaleIt != scales.constEnd(); ++scaleIt )
  {
    QStringList parts( scaleIt->split( ':' ) );
    if ( parts.size() == 2 )
    {
      pScales.push_back( parts[1].toDouble() );
    }
  }
  atlasMap.setPredefinedScales( pScales );
}

