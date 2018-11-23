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

#include "qgisapp.h"
#include "qgsclassicapp.h"
#include "qgsapplication.h"
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
#include "qgsmapcanvas.h"
#include "qgsmessageviewer.h"
#include "qgsmaplayeractionregistry.h"
#include "qgsprevieweffect.h"
#include "ui_qgssvgexportoptions.h"
#include "ui_defaults.h"

#include <QDesktopWidget>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageWriter>
#include <QLabel>
#include <QMessageBox>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPrinter>
#include <QSettings>
#include <QToolBar>
#include <QToolButton>
#include <QUndoView>
#include <QProgressDialog>
#include <QShortcut>

#ifdef ENABLE_MODELTEST
#include "modeltest.h"
#endif

QgsComposer::QgsComposer( QgsComposerView* view )
    : QMainWindow()
    , mView( view )
    , mComposition( view->composition() )
    , mAtlasFeatureAction( 0 )
{
  setupUi( this );
  setTitle( mComposition->title() );
  setStyleSheet( QgisApp::instance()->styleSheet() );

  QSettings settings;
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
  toggleActionGroup->addAction( mActionAddAttributeTable );
  toggleActionGroup->addAction( mActionAddHtml );
  toggleActionGroup->setExclusive( true );

  connect( mActionQuit, SIGNAL( triggered() ), this, SLOT( close() ) );

  connect( mComposition->undoStack(), SIGNAL( canUndoChanged( bool ) ), mActionUndo, SLOT( setEnabled( bool ) ) );
  connect( mActionUndo, SIGNAL( triggered( bool ) ), mComposition->undoStack(), SLOT( undo() ) );

  connect( mComposition->undoStack(), SIGNAL( canRedoChanged( bool ) ), mActionRedo, SLOT( setEnabled( bool ) ) );
  connect( mActionRedo, SIGNAL( triggered( bool ) ), mComposition->undoStack(), SLOT( redo() ) );

  mActionDeleteSelection->setShortcuts( QKeySequence::Backspace );

  mActionCut->setShortcuts( QKeySequence::Cut );
  connect( mActionCut, SIGNAL( triggered() ), this, SLOT( actionCutTriggered() ) );

  mActionCopy->setShortcuts( QKeySequence::Copy );
  connect( mActionCopy, SIGNAL( triggered() ), this, SLOT( actionCopyTriggered() ) );

  mActionPaste->setShortcuts( QKeySequence::Paste );
  connect( mActionPaste, SIGNAL( triggered() ), this, SLOT( actionPasteTriggered() ) );

  connect( mActionPreviewModeOff, SIGNAL( triggered() ), this, SLOT( disablePreviewMode() ) );
  connect( mActionPreviewModeGrayscale, SIGNAL( triggered() ), this, SLOT( activateGrayscalePreview() ) );
  connect( mActionPreviewModeMono, SIGNAL( triggered() ), this, SLOT( activateMonoPreview() ) );
  connect( mActionPreviewProtanope, SIGNAL( triggered() ), this, SLOT( activateProtanopePreview() ) );
  connect( mActionPreviewDeuteranope, SIGNAL( triggered() ), this, SLOT( activateDeuteranopePreview() ) );

  QActionGroup* mPreviewGroup = new QActionGroup( this );
  mPreviewGroup->setExclusive( true );
  mActionPreviewModeOff->setActionGroup( mPreviewGroup );
  mActionPreviewModeGrayscale->setActionGroup( mPreviewGroup );
  mActionPreviewModeMono->setActionGroup( mPreviewGroup );
  mActionPreviewProtanope->setActionGroup( mPreviewGroup );
  mActionPreviewDeuteranope->setActionGroup( mPreviewGroup );

  mActionZoomIn->setShortcut( QKeySequence( "Ctrl+=" ) );

  QMenu* toolbarMenu = new QMenu( tr( "&Toolbars" ), this );
  toolbarMenu->addAction( mComposerToolbar->toggleViewAction() );
  toolbarMenu->addAction( mPaperNavToolbar->toggleViewAction() );
  toolbarMenu->addAction( mItemActionToolbar->toggleViewAction() );
  toolbarMenu->addAction( mItemToolbar->toggleViewAction() );
  mViewMenu->addMenu( toolbarMenu );

  QMenu* panelMenu = new QMenu( tr( "P&anels" ), this );
  mViewMenu->addMenu( panelMenu );

  QToolButton* atlasExportToolButton = new QToolButton( mAtlasToolbar );
  atlasExportToolButton->setPopupMode( QToolButton::InstantPopup );
  atlasExportToolButton->setAutoRaise( true );
  atlasExportToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  atlasExportToolButton->addAction( mActionExportAtlasAsImage );
  atlasExportToolButton->addAction( mActionExportAtlasAsSVG );
  atlasExportToolButton->addAction( mActionExportAtlasAsPDF );
  atlasExportToolButton->setDefaultAction( mActionExportAtlasAsImage );
  mAtlasToolbar->insertWidget( mActionAtlasSettings, atlasExportToolButton );

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
  foreach ( double zoomLvl, mStatusZoomLevelsList )
  {
    mStatusZoomCombo->insertItem( 0, tr( "%1%" ).arg( zoomLvl * 100.0, 0, 'f', 1 ) );
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
  mHorizontalRuler = new QgsComposerRuler( QgsComposerRuler::Horizontal );
  mVerticalRuler = new QgsComposerRuler( QgsComposerRuler::Vertical );
  mRulerLayoutFix = new QWidget();
  mRulerLayoutFix->setAttribute( Qt::WA_NoMousePropagation );
  mRulerLayoutFix->setBackgroundRole( QPalette::Window );
  mRulerLayoutFix->setFixedSize( mVerticalRuler->rulerSize(), mHorizontalRuler->rulerSize() );
  mViewLayout->addWidget( mRulerLayoutFix, 0, 0 );
  mViewLayout->addWidget( mHorizontalRuler, 0, 1 );
  mViewLayout->addWidget( mVerticalRuler, 1, 0 );

  mView->setContentsMargins( 0, 0, 0, 0 );
  mView->setHorizontalRuler( mHorizontalRuler );
  mView->setVerticalRuler( mVerticalRuler );
  mViewLayout->addWidget( mView, 1, 1 );

  // Toggle panels on tab, instead of moving focus
  mView->setFocusPolicy( Qt::ClickFocus );
  QShortcut* tab = new QShortcut( Qt::Key_Tab, mView );
  tab->setContext( Qt::WidgetWithChildrenShortcut );
  connect( tab, SIGNAL( activated() ), mActionHidePanels, SLOT( trigger() ) );

  // Initial state of rulers
  QSettings myQSettings;
  bool showRulers = myQSettings.value( "/Composer/showRulers", true ).toBool();
  mActionShowRulers->blockSignals( true );
  mActionShowRulers->setChecked( showRulers );
  mHorizontalRuler->setVisible( showRulers );
  mVerticalRuler->setVisible( showRulers );
  mRulerLayoutFix->setVisible( showRulers );
  mActionShowRulers->blockSignals( false );
  connect( mActionShowRulers, SIGNAL( triggered( bool ) ), this, SLOT( toggleRulers( bool ) ) );

  // Restore grid settings
  mActionSnapGrid->setChecked( mComposition->snapToGridEnabled() );
  mActionShowGrid->setChecked( mComposition->gridVisible() );
  // Restore guide settings
  mActionShowGuides->setChecked( mComposition->snapLinesVisible() );
  mActionSnapGuides->setChecked( mComposition->alignmentSnap() );
  mActionSmartGuides->setChecked( mComposition->smartGuidesEnabled() );
  // General view settings
  mActionShowBoxes->setChecked( mComposition->boundingBoxesVisible() );

  // Connect view slots
  connect( mView, SIGNAL( selectedItemChanged( QgsComposerItem* ) ), this, SLOT( showItemOptions( QgsComposerItem* ) ) );
  connect( mView, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( deleteItem( QgsComposerItem* ) ) );
  connect( mView, SIGNAL( actionFinished() ), this, SLOT( setSelectionTool() ) );
  connect( mView, SIGNAL( cursorPosChanged( QPointF ) ), this, SLOT( updateStatusCursorPos( QPointF ) ) );
  connect( mView, SIGNAL( zoomLevelChanged() ), this, SLOT( updateStatusZoom() ) );

  // Connect composition slots
  connect( mComposition, SIGNAL( selectedItemChanged( QgsComposerItem* ) ), this, SLOT( showItemOptions( QgsComposerItem* ) ) );
  connect( mComposition, SIGNAL( composerItemAdded( QgsComposerItem* ) ), this, SLOT( createComposerItemWidget( QgsComposerItem* ) ) );
  connect( mComposition, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( deleteItem( QgsComposerItem* ) ) );
  connect( mComposition, SIGNAL( paperSizeChanged() ), mHorizontalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( paperSizeChanged() ), mVerticalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( nPagesChanged() ), mHorizontalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( nPagesChanged() ), mVerticalRuler, SLOT( update() ) );
  connect( mComposition, SIGNAL( statusMsgChanged( QString ) ), mStatusCompositionLabel, SLOT( setText( QString ) ) );
  connect( mComposition, SIGNAL( titleChanged( QString ) ), this, SLOT( setTitle( QString ) ) );

  connect( mHorizontalRuler, SIGNAL( cursorPosChanged( QPointF ) ), this, SLOT( updateStatusCursorPos( QPointF ) ) );
  connect( mVerticalRuler, SIGNAL( cursorPosChanged( QPointF ) ), this, SLOT( updateStatusCursorPos( QPointF ) ) );
  connect( this, SIGNAL( zoomLevelChanged() ), this, SLOT( updateStatusZoom() ) );

  int minDockWidth = 335;

  setTabPosition( Qt::AllDockWidgetAreas, QTabWidget::North );
  mGeneralDock = new QDockWidget( tr( "Composition" ), this );
  mGeneralDock->setObjectName( "CompositionDock" );
  mGeneralDock->setMinimumWidth( minDockWidth );
  panelMenu->addAction( mGeneralDock->toggleViewAction() );
  mItemDock = new QDockWidget( tr( "Item properties" ), this );
  mItemDock->setObjectName( "ItemDock" );
  mItemDock->setMinimumWidth( minDockWidth );
  panelMenu->addAction( mItemDock->toggleViewAction() );
  mUndoDock = new QDockWidget( tr( "Command history" ), this );
  mUndoDock->setObjectName( "CommandDock" );
  panelMenu->addAction( mUndoDock->toggleViewAction() );
  mAtlasDock = new QDockWidget( tr( "Atlas generation" ), this );
  mAtlasDock->setObjectName( "AtlasDock" );
  panelMenu->addAction( mAtlasDock->toggleViewAction() );
  mItemsDock = new QDockWidget( tr( "Items" ), this );
  mItemsDock->setObjectName( "ItemsDock" );
  panelMenu->addAction( mItemsDock->toggleViewAction() );

  QList<QDockWidget *> docks = findChildren<QDockWidget *>();
  foreach ( QDockWidget* dock, docks )
  {
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    connect( dock, SIGNAL( visibilityChanged( bool ) ), this, SLOT( dockVisibilityChanged( bool ) ) );
  }

  QgsCompositionWidget* compositionWidget = new QgsCompositionWidget( mGeneralDock, mComposition );
  connect( mComposition, SIGNAL( paperSizeChanged() ), compositionWidget, SLOT( displayCompositionWidthHeight() ) );
  connect( this, SIGNAL( printAsRasterChanged( bool ) ), compositionWidget, SLOT( setPrintAsRasterCheckBox( bool ) ) );
  mGeneralDock->setWidget( compositionWidget );

  //undo widget
  mUndoDock->setWidget( new QUndoView( mComposition->undoStack(), this ) );

  //items tree widget
  QTreeView* itemsTreeView = new QTreeView( mItemsDock );
  itemsTreeView->setModel( mComposition->itemsModel() );
#ifdef ENABLE_MODELTEST
  new ModelTest( mComposition->itemsModel(), this );
#endif

  itemsTreeView->setColumnWidth( 0, 30 );
  itemsTreeView->setColumnWidth( 1, 30 );
  itemsTreeView->header()->setResizeMode( 0, QHeaderView::Fixed );
  itemsTreeView->header()->setResizeMode( 1, QHeaderView::Fixed );
  itemsTreeView->header()->setMovable( false );

  itemsTreeView->setDragEnabled( true );
  itemsTreeView->setAcceptDrops( true );
  itemsTreeView->setDropIndicatorShown( true );
  itemsTreeView->setDragDropMode( QAbstractItemView::InternalMove );

  itemsTreeView->setIndentation( 0 );
  mItemsDock->setWidget( itemsTreeView );
  connect( itemsTreeView->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), mComposition->itemsModel(), SLOT( setSelected( QModelIndex ) ) );

  addDockWidget( Qt::RightDockWidgetArea, mItemDock );
  addDockWidget( Qt::RightDockWidgetArea, mGeneralDock );
  addDockWidget( Qt::RightDockWidgetArea, mUndoDock );
  addDockWidget( Qt::RightDockWidgetArea, mAtlasDock );
  addDockWidget( Qt::RightDockWidgetArea, mItemsDock );

  mAtlasDock->setWidget( new QgsAtlasCompositionWidget( mGeneralDock, mComposition ) );

  tabifyDockWidget( mGeneralDock, mUndoDock );
  tabifyDockWidget( mItemDock, mUndoDock );
  tabifyDockWidget( mGeneralDock, mItemDock );
  tabifyDockWidget( mItemDock, mAtlasDock );
  tabifyDockWidget( mItemDock, mItemsDock );

  mGeneralDock->raise();

  //set initial state of atlas controls
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  toggleAtlasControls( atlasMap->enabled() );
  connect( atlasMap, SIGNAL( statusMsgChanged( QString ) ), mStatusAtlasLabel, SLOT( setText( QString ) ) );
  connect( atlasMap, SIGNAL( toggled( bool ) ), this, SLOT( toggleAtlasControls( bool ) ) );
  connect( atlasMap, SIGNAL( coverageLayerChanged( QgsVectorLayer* ) ), this, SLOT( updateAtlasMapLayerAction( QgsVectorLayer * ) ) );

  connect( this, SIGNAL( atlasPreviewFeatureChanged() ), QgisApp::instance(), SLOT( refreshMapCanvas() ) );

  restoreState( settings.value( "/ComposerUI/state", QByteArray::fromRawData(( char * )defaultComposerUIstate, sizeof defaultComposerUIstate ) ).toByteArray() );
  restoreGeometry( settings.value( "/Composer/geometry", QByteArray::fromRawData(( char * )defaultComposerUIgeometry, sizeof defaultComposerUIgeometry ) ).toByteArray() );

  setSelectionTool();

  // Add widgets for all composer items
  foreach ( QGraphicsItem* item, mComposition->items() )
  {
    if ( QgsComposerItem* composerItem = dynamic_cast<QgsComposerItem*>( item ) )
    {
      createComposerItemWidget( composerItem );
    }
  }

  mView->setFocus();
  mItemDock->show();
  mGeneralDock->show();
  mUndoDock->show();
  mAtlasDock->show();
  mItemsDock->show();
}

QgsComposer::~QgsComposer()
{
  qDeleteAll( mItemWidgetMap );
}

void QgsComposer::setIconSizes( int size )
{
  setIconSize( QSize( size, size ) );
  foreach ( QToolBar * toolbar, findChildren<QToolBar *>() )
  {
    toolbar->setIconSize( QSize( size, size ) );
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

void QgsComposer::setTitle( const QString& title )
{
  setWindowTitle( title );
  //update atlas map layer action name if required
  if ( mAtlasFeatureAction )
  {
    mAtlasFeatureAction->setText( QString( tr( "Set as atlas feature for %1" ) ).arg( title ) );
  }
}

void QgsComposer::updateStatusCursorPos( QPointF cursorPosition )
{
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
  mView->setZoomLevel( selectedZoom );
  //update zoom combobox text for correct format (one decimal place, trailing % sign)
  mStatusZoomCombo->blockSignals( true );
  mStatusZoomCombo->lineEdit()->setText( tr( "%1%" ).arg( selectedZoom * 100.0, 0, 'f', 1 ) );
  mStatusZoomCombo->blockSignals( false );
}

void QgsComposer::statusZoomCombo_zoomEntered()
{
  //need to remove spaces and "%" characters from input text
  QString zoom = mStatusZoomCombo->currentText().remove( QChar( '%' ) ).trimmed();
  mView->setZoomLevel( zoom.toDouble() / 100 );
}

void QgsComposer::showItemOptions( QgsComposerItem* item )
{
  if ( !item )
  {
    mItemDock->setWidget( 0 );
    return;
  }

  QWidget* newWidget = mItemWidgetMap.value( item, 0 );
  if ( newWidget != mItemDock->widget() )
  {
    mItemDock->setWidget( newWidget );
  }
}

void QgsComposer::on_mActionOptions_triggered()
{
  QgisApp::instance()->showOptionsDialog( this, "mOptionsPageComposer" );
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
    atlasMap->loadPredefinedScalesFromProject();
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
    QgisApp::instance()->mapCanvas()->stopRendering();
    emit atlasPreviewFeatureChanged();
  }
  else
  {
    mStatusAtlasLabel->setText( QString() );
  }
}

void QgsComposer::on_mActionAtlasNext_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( atlasMap->enabled() )
  {
    QgisApp::instance()->mapCanvas()->stopRendering();
    atlasMap->loadPredefinedScalesFromProject();
    atlasMap->nextFeature();
    emit atlasPreviewFeatureChanged();
  }
}

void QgsComposer::on_mActionAtlasPrev_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( atlasMap->enabled() )
  {
    QgisApp::instance()->mapCanvas()->stopRendering();
    atlasMap->loadPredefinedScalesFromProject();
    atlasMap->prevFeature();
    emit atlasPreviewFeatureChanged();
  }
}

void QgsComposer::on_mActionAtlasFirst_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( atlasMap->enabled() )
  {
    QgisApp::instance()->mapCanvas()->stopRendering();
    atlasMap->loadPredefinedScalesFromProject();
    atlasMap->firstFeature();
    emit atlasPreviewFeatureChanged();
  }
}

void QgsComposer::on_mActionAtlasLast_triggered()
{
  QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
  if ( atlasMap->enabled() )
  {
    QgisApp::instance()->mapCanvas()->stopRendering();
    atlasMap->loadPredefinedScalesFromProject();
    atlasMap->lastFeature();
    emit atlasPreviewFeatureChanged();
  }
}

void QgsComposer::zoomFull( void )
{
  mView->fitInView( mComposition->sceneRect(), Qt::KeepAspectRatio );
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
  mView->setCurrentTool( QgsComposerView::Zoom );
}

void QgsComposer::on_mActionRefreshView_triggered()
{
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
  mComposition->setGridVisible( checked );
}

void QgsComposer::on_mActionSnapGrid_triggered( bool checked )
{
  mComposition->setSnapToGridEnabled( checked );
}

void QgsComposer::on_mActionShowGuides_triggered( bool checked )
{
  mComposition->setSnapLinesVisible( checked );
}

void QgsComposer::on_mActionSnapGuides_triggered( bool checked )
{
  mComposition->setAlignmentSnap( checked );
}

void QgsComposer::on_mActionSmartGuides_triggered( bool checked )
{
  mComposition->setSmartGuidesEnabled( checked );
}

void QgsComposer::on_mActionShowBoxes_triggered( bool checked )
{
  mComposition->setBoundingBoxesVisible( checked );
}

void QgsComposer::on_mActionClearGuides_triggered()
{
  mComposition->clearSnapLines();
}

void QgsComposer::toggleRulers( bool checked )
{
  mHorizontalRuler->setVisible( checked );
  mVerticalRuler->setVisible( checked );
  mRulerLayoutFix->setVisible( checked );
  QSettings().setValue( "/Composer/showRulers", checked );
}

void QgsComposer::on_mActionAtlasSettings_triggered()
{
  mAtlasDock->show();
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
  mView->setPreviewModeEnabled( false );
}

void QgsComposer::activateGrayscalePreview()
{
  mView->setPreviewMode( QgsPreviewEffect::PreviewGrayscale );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::activateMonoPreview()
{
  mView->setPreviewMode( QgsPreviewEffect::PreviewMono );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::activateProtanopePreview()
{
  mView->setPreviewMode( QgsPreviewEffect::PreviewProtanope );
  mView->setPreviewModeEnabled( true );
}

void QgsComposer::activateDeuteranopePreview()
{
  mView->setPreviewMode( QgsPreviewEffect::PreviewDeuteranope );
  mView->setPreviewModeEnabled( true );
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

void QgsComposer::on_mActionExportAsPDF_triggered()
{
  exportCompositionAsPDF( QgsComposition::Single );
}

void QgsComposer::on_mActionExportAtlasAsPDF_triggered()
{
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  exportCompositionAsPDF( QgsComposition::Atlas );
  mComposition->setAtlasMode( previousMode );

  if ( mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //after atlas output, jump back to preview first feature
    QgsAtlasComposition* atlasMap = &mComposition->atlasComposition();
    atlasMap->firstFeature();
  }
}

void QgsComposer::exportCompositionAsPDF( QgsComposition::OutputMode mode )
{
  if ( containsWMSLayer() )
    showWMSPrintingWarning();

  if ( containsAdvancedEffects() )
    showAdvancedEffectsWarning();

  QSettings settings;
  QString outputFileName = QFileDialog::getSaveFileName(
                             this,
                             tr( "Choose a file name to save the map as" ),
                             settings.value( "/UI/lastSaveAsPdfFile", QDir::homePath() ).toString(),
                             tr( "PDF Format" ) + " (*.pdf *.PDF)" );
  if ( outputFileName.isEmpty() )
  {
    return;
  }
  if ( !outputFileName.endsWith( ".pdf", Qt::CaseInsensitive ) )
  {
    outputFileName += ".pdf";
  }
  settings.setValue( "/UI/lastSaveAsPdfFile", outputFileName );

  setUpdatesEnabled( false );
  mView->setPaintingEnabled( false );
  QApplication::setOverrideCursor( Qt::BusyCursor );

  QProgressDialog progressDialog( this );
  QString errorMsg;
  if ( !mComposition->printToPdf( outputFileName, mode, &progressDialog, &errorMsg ) )
  {
    QMessageBox::warning( this, tr( "PDF Export Failed" ), errorMsg, QMessageBox::Ok );
  }

  mView->setPaintingEnabled( true );
  setUpdatesEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionPrint_triggered()
{
  //print only current feature
  printComposition( QgsComposition::Single );
}

void QgsComposer::on_mActionPrintAtlas_triggered()
{
  //print whole atlas
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  printComposition( QgsComposition::Atlas );
  mComposition->setAtlasMode( previousMode );
}

void QgsComposer::printComposition( QgsComposition::OutputMode mode )
{
  if ( containsWMSLayer() )
    showWMSPrintingWarning();

  if ( containsAdvancedEffects() )
    showAdvancedEffectsWarning();

  QApplication::setOverrideCursor( Qt::BusyCursor );
  mView->setPaintingEnabled( false );

  QProgressDialog progressDialog( this );
  QString errorMsg;
  if ( !mComposition->print( 0, mode, true, &progressDialog, &errorMsg ) && !errorMsg.isEmpty() )
  {
    QMessageBox::warning( this, tr( "PDF Export Failed" ), errorMsg, QMessageBox::Ok );
  }
  mView->setPaintingEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionExportAtlasAsImage_triggered()
{
  //print whole atlas
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  exportCompositionAsImage( QgsComposition::Atlas );
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
  exportCompositionAsImage( QgsComposition::Single );
}

void QgsComposer::exportCompositionAsImage( QgsComposition::OutputMode mode )
{
  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  QSettings settings;
  QString output;
  QString format;
  if ( mode == QgsComposition::Single )
  {
    QString lastUsedDir = settings.value( "/UI/lastSaveAsImageDir", QDir::homePath() ).toString();
    QPair<QString, QString> fileNExt = QgisGui::getSaveAsImageName( this, tr( "Choose a file name to save the map image as" ), lastUsedDir );

    if ( fileNExt.first.isEmpty() )
    {
      return;
    }
    output = fileNExt.first;
    format = fileNExt.second;
    settings.setValue( "/UI/lastSaveAtlasAsImagesDir", QFileInfo( output ).path() );
    settings.setValue( "/UI/lastSaveAtlasAsImagesFormat", format );
  }
  else
  {
    QString lastUsedFormat = settings.value( "/UI/lastSaveAtlasAsImagesFormat", "JPG" ).toString();
    QString lastUsedDir = settings.value( "/UI/lastSaveAtlasAsImagesDir", QDir::homePath() ).toString();

    QFileDialog dlg( this, tr( "Choose Output Folder" ) );
    dlg.setOption( QFileDialog::DontUseNativeDialog );
    dlg.setFileMode( QFileDialog::Directory );
    dlg.setOption( QFileDialog::ShowDirsOnly, true );
    dlg.setDirectory( QFileInfo( lastUsedDir ).absolutePath() );
    dlg.selectFile( QFileInfo( lastUsedDir ).fileName() );

    // Add file format selection combo
    QComboBox *box = new QComboBox();
    QHBoxLayout* hlayout = new QHBoxLayout();
    QWidget* widget = new QWidget();

    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    int selectedFormat = 0;
    for ( int i = 0; i < formats.size(); ++i )
    {
      QString format = QString( formats.at( i ) );
      if ( format.compare( lastUsedFormat, Qt::CaseInsensitive ) == 0 )
      {
        selectedFormat = i;
      }
      box->addItem( format.toUpper() );
    }
    box->setCurrentIndex( selectedFormat );

    hlayout->setMargin( 0 );
    hlayout->addWidget( new QLabel( tr( "Image format: " ) ) );
    hlayout->addWidget( box );
    widget->setLayout( hlayout );
    dlg.layout()->addWidget( widget );

    if ( !dlg.exec() || dlg.selectedFiles().isEmpty() )
    {
      return;
    }
    output = dlg.selectedFiles().front();
    format = box->currentText().toLower();

    if ( !QFileInfo( output ).isWritable() )
    {
      QMessageBox::warning( this, tr( "Image Export Failed" ), tr( "The specified output directory is not writable." ), QMessageBox::Ok, QMessageBox::Ok );
      return;
    }

    settings.setValue( "/UI/lastSaveAtlasAsImagesDir", output );
    settings.setValue( "/UI/lastSaveAtlasAsImagesFormat", format );
  }

  QApplication::setOverrideCursor( Qt::BusyCursor );
  setUpdatesEnabled( false );
  mView->setPaintingEnabled( false );
  QProgressDialog progressDialog( this );
  QString errorMsg;
  if ( !mComposition->printToImage( output, format, false, mode, &progressDialog, &errorMsg ) )
  {
    QMessageBox::warning( this, tr( "Image Export Failed" ), errorMsg, QMessageBox::Ok );
  }
  mView->setPaintingEnabled( true );
  setUpdatesEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionExportAtlasAsSVG_triggered()
{
  QgsComposition::AtlasMode previousMode = mComposition->atlasMode();
  mComposition->setAtlasMode( QgsComposition::ExportAtlas );
  exportCompositionAsSVG( QgsComposition::Atlas );
  mComposition->setAtlasMode( previousMode );

  if ( mComposition->atlasMode() == QgsComposition::PreviewAtlas )
  {
    //after atlas output, jump back to preview first feature
    mComposition->atlasComposition().firstFeature();
  }
}

void QgsComposer::on_mActionExportAsSVG_triggered()
{
  exportCompositionAsSVG( QgsComposition::Single );
}

void QgsComposer::exportCompositionAsSVG( QgsComposition::OutputMode mode )
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
    QgsMessageViewer m( this, QgisGui::ModalDialogFlags, false );
    m.setWindowTitle( tr( "SVG warning" ) );
    m.setCheckBoxText( tr( "Don't show this message again" ) );
    m.setCheckBoxState( Qt::Unchecked );
    m.setCheckBoxVisible( true );
    m.setCheckBoxQSettingsLabel( settingsLabel );
    m.setMessageAsHtml( tr( "<p>The SVG export function in QGIS has several "
                            "problems due to bugs and deficiencies in the " )
                        + tr( "Qt4 svg code. In particular, there are problems "
                              "with layers not being clipped to the map "
                              "bounding box.</p>" )
                        + tr( "If you require a vector-based output file from "
                              "Qgis it is suggested that you try printing "
                              "to PostScript if the SVG output is not "
                              "satisfactory."
                              "</p>" ) );
    m.exec();
  }

  QString output;

  if ( mode == QgsComposition::Single )
  {
    QString lastUsedDir = settings.value( "/UI/lastSaveAsSvgDir", QDir::homePath() ).toString();
    output = QFileDialog::getSaveFileName( this, tr( "Choose Output File" ), lastUsedDir, tr( "SVG Images" ) + " (*.svg)" );
    if ( output.isEmpty() )
      return;

    if ( !output.endsWith( ".svg", Qt::CaseInsensitive ) )
    {
      output += ".svg";
    }
    settings.setValue( "/UI/lastSaveAsSvgDir", QFileInfo( output ).path() );
  }
  else
  {
    QString lastUsedDir = settings.value( "/UI/lastSaveAtlasAsSvgDir", QDir::homePath() ).toString();
    output = QFileDialog::getExistingDirectory( this, tr( "Choose Output Folder" ), lastUsedDir, QFileDialog::ShowDirsOnly );
    if ( output.isEmpty() )
    {
      return;
    }
    if ( !QFileInfo( output ).isWritable() )
    {
      QMessageBox::warning( this, tr( "SVG Export Failed" ), tr( "The specified output directory is not writable." ), QMessageBox::Ok, QMessageBox::Ok );
      return;
    }
    settings.setValue( "/UI/lastSaveAtlasAsSvgDir", output );
  }

  QDialog svgOptionsDialog;
  Ui::QgsSvgExportOptionsDialog svgOptionsUi;
  svgOptionsUi.setupUi( &svgOptionsDialog );
  svgOptionsDialog.exec();
  bool groupLayers = svgOptionsUi.chkMapLayersAsGroup->isChecked();

  QApplication::setOverrideCursor( Qt::BusyCursor );
  setUpdatesEnabled( false );
  mView->setPaintingEnabled( false );
  QProgressDialog progressDialog( this );
  QString errorMsg;
  if ( !mComposition->printToImage( output, "svg", groupLayers, mode, &progressDialog, &errorMsg ) )
  {
    QMessageBox::warning( this, tr( "SVG Export Failed" ), errorMsg, QMessageBox::Ok );
  }
  mView->setPaintingEnabled( true );
  setUpdatesEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionSelectMoveItem_triggered()
{
  mView->setCurrentTool( QgsComposerView::Select );
}

void QgsComposer::on_mActionAddNewMap_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddMap );
}

void QgsComposer::on_mActionAddNewLegend_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddLegend );
}

void QgsComposer::on_mActionAddNewLabel_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddLabel );
}

void QgsComposer::on_mActionAddNewScalebar_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddScalebar );
}

void QgsComposer::on_mActionAddImage_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddPicture );
}

void QgsComposer::on_mActionAddRectangle_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddRectangle );
}

void QgsComposer::on_mActionAddTriangle_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddTriangle );
}

void QgsComposer::on_mActionAddEllipse_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddEllipse );
}

void QgsComposer::on_mActionAddAttributeTable_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddAttributeTable );
}

void QgsComposer::on_mActionAddHtml_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddHtml );
}

void QgsComposer::on_mActionAddArrow_triggered()
{
  mView->setCurrentTool( QgsComposerView::AddArrow );
}

void QgsComposer::on_mActionSaveProject_triggered()
{
  QgisApp::instance()->fileSave();
}

void QgsComposer::on_mActionNewComposer_triggered()
{
  QString title = QgisApp::instance()->uniqueComposerTitle( this, true );
  if ( !title.isNull() )
  {
    QgsComposition* composition = QgisApp::instance()->createNewComposition( title );
    QgisApp::instance()->showComposer( composition );
  }
}

void QgsComposer::on_mActionDuplicateComposer_triggered()
{
  QString newTitle = QgisApp::instance()->uniqueComposerTitle( this, false, mComposition->title() + tr( " copy" ) );
  if ( newTitle.isNull() )
  {
    return;
  }
  QgsComposition* composition = QgisApp::instance()->duplicateComposition( mComposition, newTitle );
  QgisApp::instance()->showComposer( composition );
}

void QgsComposer::on_mActionComposerManager_triggered()
{
  QgisApp::instance()->showComposerManager();
}

void QgsComposer::on_mActionSaveAsTemplate_triggered()
{
  QSettings settings;
  QString lastSaveDir = settings.value( "UI/lastComposerTemplateDir", "" ).toString();
  QString saveFileName = QFileDialog::getSaveFileName( this, tr( "Save template" ), lastSaveDir, tr( "Composer templates" ) + " (*.qpt)" );
  if ( saveFileName.isEmpty() )
    return;

  QFileInfo saveFileInfo( saveFileName );
  //check if suffix has been added
  if ( saveFileInfo.suffix().isEmpty() )
  {
    saveFileInfo = QFileInfo( saveFileName.append( ".qpt" ) );
  }
  settings.setValue( "UI/lastComposerTemplateDir", saveFileInfo.absolutePath() );

  QFile templateFile( saveFileName );
  if ( !templateFile.open( QIODevice::WriteOnly ) )
  {
    QMessageBox::warning( 0, tr( "Save error" ), tr( "Error, could not save file" ) );
  }
  QDomDocument saveDocument;
  mComposition->writeXML( saveDocument, saveDocument );
  templateFile.write( saveDocument.toByteArray() );
}

void QgsComposer::on_mActionLoadFromTemplate_triggered()
{
  QSettings settings;
  QString openFileDir = settings.value( "UI/lastComposerTemplateDir", "" ).toString();
  QString openFileString = QFileDialog::getOpenFileName( 0, tr( "Load template" ), openFileDir, tr( "Composer templates" ) + " (*.qpt)" );
  if ( openFileString.isEmpty() )
    return;

  QFileInfo openFileInfo( openFileString );
  settings.setValue( "UI/LastComposerTemplateDir", openFileInfo.absolutePath() );

  QFile templateFile( openFileString );
  if ( !templateFile.open( QIODevice::ReadOnly ) )
  {
    QMessageBox::warning( this, tr( "Read error" ), tr( "Error, could not read file" ) );
    return;
  }

  QDomDocument templateDoc;
  if ( templateDoc.setContent( &templateFile ) )
  {
    setUpdatesEnabled( false );
    mComposition->loadFromTemplate( templateDoc, 0, false );
    setUpdatesEnabled( true );
  }
}

void QgsComposer::on_mActionMoveItemContent_triggered()
{
  mView->setCurrentTool( QgsComposerView::MoveItemContent );
}

void QgsComposer::on_mActionPan_triggered()
{
  mView->setCurrentTool( QgsComposerView::Pan );
}

void QgsComposer::on_mActionGroupItems_triggered()
{
  mView->groupItems();
}

void QgsComposer::on_mActionUngroupItems_triggered()
{
  mView->ungroupItems();
}

void QgsComposer::on_mActionLockItems_triggered()
{
  mComposition->lockSelectedItems();
}

void QgsComposer::on_mActionUnlockAll_triggered()
{
  mComposition->unlockAllItems();
}

void QgsComposer::actionCutTriggered()
{
  mView->copyItems( QgsComposerView::ClipboardModeCut );
}

void QgsComposer::actionCopyTriggered()
{
  mView->copyItems( QgsComposerView::ClipboardModeCopy );
}

void QgsComposer::actionPasteTriggered()
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

void QgsComposer::on_mActionPasteInPlace_triggered()
{
  mView->pasteItems( QgsComposerView::PasteModeInPlace );
}

void QgsComposer::on_mActionDeleteSelection_triggered()
{
  mView->deleteSelectedItems();
}

void QgsComposer::on_mActionSelectAll_triggered()
{
  mView->selectAll();
}

void QgsComposer::on_mActionDeselectAll_triggered()
{
  mView->selectNone();
}

void QgsComposer::on_mActionInvertSelection_triggered()
{
  mView->selectInvert();
}

void QgsComposer::on_mActionSelectNextAbove_triggered()
{
  mComposition->selectNextByZOrder( QgsComposition::ZValueAbove );
}

void QgsComposer::on_mActionSelectNextBelow_triggered()
{
  mComposition->selectNextByZOrder( QgsComposition::ZValueBelow );
}

void QgsComposer::on_mActionRaiseItems_triggered()
{
  mComposition->raiseSelectedItems();
}

void QgsComposer::on_mActionLowerItems_triggered()
{
  mComposition->lowerSelectedItems();
}

void QgsComposer::on_mActionMoveItemsToTop_triggered()
{
  mComposition->moveSelectedItemsToTop();
}

void QgsComposer::on_mActionMoveItemsToBottom_triggered()
{
  mComposition->moveSelectedItemsToBottom();
}

void QgsComposer::on_mActionAlignLeft_triggered()
{
  mComposition->alignSelectedItemsLeft();
}

void QgsComposer::on_mActionAlignHCenter_triggered()
{
  mComposition->alignSelectedItemsHCenter();
}

void QgsComposer::on_mActionAlignRight_triggered()
{
  mComposition->alignSelectedItemsRight();
}

void QgsComposer::on_mActionAlignTop_triggered()
{
  mComposition->alignSelectedItemsTop();
}

void QgsComposer::on_mActionAlignVCenter_triggered()
{
  mComposition->alignSelectedItemsVCenter();
}

void QgsComposer::on_mActionAlignBottom_triggered()
{
  mComposition->alignSelectedItemsBottom();
}

void QgsComposer::closeEvent( QCloseEvent */*e*/ )
{
  QSettings settings;
  settings.setValue( "/Composer/geometry", saveGeometry() );
  settings.setValue( "/ComposerUI/state", saveState() );
}

void QgsComposer::createComposerItemWidget( QgsComposerItem* item )
{
  if ( QgsComposerArrow* arrow = dynamic_cast<QgsComposerArrow*>( item ) )
  {
    mItemWidgetMap.insert( arrow, new QgsComposerArrowWidget( arrow ) );
  }
  else if ( QgsComposerLabel* label = dynamic_cast<QgsComposerLabel*>( item ) )
  {
    mItemWidgetMap.insert( label, new QgsComposerLabelWidget( label ) );
  }
  else if ( QgsComposerMap* map = dynamic_cast<QgsComposerMap*>( item ) )
  {
    connect( this, SIGNAL( zoomLevelChanged() ), map, SLOT( renderModeUpdateCachedImage() ) );
    mItemWidgetMap.insert( map, new QgsComposerMapWidget( map ) );
  }
  else if ( QgsComposerScaleBar* scaleBar = dynamic_cast<QgsComposerScaleBar*>( item ) )
  {
    mItemWidgetMap.insert( scaleBar, new QgsComposerScaleBarWidget( scaleBar ) );
  }
  else if ( QgsComposerLegend* legend = dynamic_cast<QgsComposerLegend*>( item ) )
  {
    mItemWidgetMap.insert( legend, new QgsComposerLegendWidget( legend ) );
  }
  else if ( QgsComposerPicture* picture = dynamic_cast<QgsComposerPicture*>( item ) )
  {
    mItemWidgetMap.insert( picture, new QgsComposerPictureWidget( picture ) );
  }
  else if ( QgsComposerShape* shape = dynamic_cast<QgsComposerShape*>( item ) )
  {
    mItemWidgetMap.insert( shape, new QgsComposerShapeWidget( shape ) );
  }
  else if ( QgsComposerAttributeTable* table = dynamic_cast<QgsComposerAttributeTable*>( item ) )
  {
    mItemWidgetMap.insert( table, new QgsComposerTableWidget( table ) );
  }
  else if ( QgsComposerFrame* frame = dynamic_cast<QgsComposerFrame*>( item ) )
  {
    QgsComposerMultiFrame* mf = frame->multiFrame();
    if ( QgsComposerHtml* html = dynamic_cast<QgsComposerHtml*>( mf ) )
    {
      mItemWidgetMap.insert( frame, new QgsComposerHtmlWidget( html, frame ) );
    }
    else if ( QgsComposerAttributeTableV2* table = dynamic_cast<QgsComposerAttributeTableV2*>( mf ) )
    {
      mItemWidgetMap.insert( frame, new QgsComposerAttributeTableWidget( table, frame ) );
    }
  }
}

void QgsComposer::deleteItem( QgsComposerItem* item )
{
  QWidget* widget = mItemWidgetMap.take( item );
  if ( widget )
  {
    widget->deleteLater();
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
  for ( ; item_it != mItemWidgetMap.constEnd(); ++item_it )
  {
    QgsComposerItem* currentItem = item_it.key();
    QgsComposerMap* currentMap = dynamic_cast<QgsComposerMap *>( currentItem );
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
  for ( ; item_it != mItemWidgetMap.constEnd(); ++item_it )
  {
    QgsComposerItem* currentItem = item_it.key();
    // Check composer item's blend mode
    if ( currentItem->blendMode() != QPainter::CompositionMode_SourceOver )
    {
      return true;
    }
    // If item is a composer map, check if it contains any advanced effects
    QgsComposerMap* currentMap = dynamic_cast<QgsComposerMap *>( currentItem );
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
    QgsMessageViewer m( this, QgisGui::ModalDialogFlags, false );
    m.setWindowTitle( tr( "Project contains WMS layers" ) );
    m.setMessage( tr( "Some WMS servers (e.g. UMN mapserver) have a limit for the WIDTH and HEIGHT parameter. Printing layers from such servers may exceed this limit. If this is the case, the WMS layer will not be printed" ), QgsMessageOutput::MessageText );
    m.setCheckBoxText( tr( "Don't show this message again" ) );
    m.setCheckBoxState( Qt::Unchecked );
    m.setCheckBoxVisible( true );
    m.setCheckBoxQSettingsLabel( myQSettingsLabel );
    m.exec();
  }
}

void QgsComposer::showAdvancedEffectsWarning()
{
  if ( ! mComposition->printAsRaster() )
  {
    QgsMessageViewer m( this, QgisGui::ModalDialogFlags, false );
    m.setWindowTitle( tr( "Project contains composition effects" ) );
    m.setMessage( tr( "Advanced composition effects such as blend modes or vector layer transparency are enabled in this project, which cannot be printed as vectors. Printing as a raster is recommended." ), QgsMessageOutput::MessageText );
    m.setCheckBoxText( tr( "Print as raster" ) );
    m.setCheckBoxState( Qt::Checked );
    m.setCheckBoxVisible( true );
    m.showMessage( true );

    mComposition->setPrintAsRaster( m.checkBoxState() == Qt::Checked );
    emit printAsRasterChanged( m.checkBoxState() == Qt::Checked );
  }
}

void QgsComposer::on_mActionPageSetup_triggered()
{
  QPageSetupDialog( mComposition->defaultPrinter(), this ).exec();
}

void QgsComposer::setAtlasFeature( QgsMapLayer* layer, const QgsFeature& feat )
{
  //check if composition atlas settings match
  QgsAtlasComposition& atlas = mComposition->atlasComposition();
  if ( !atlas.enabled() || atlas.coverageLayer() != layer )
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

  QgisApp::instance()->mapCanvas()->stopRendering();

  //set current preview feature id
  atlas.prepareForFeature( &feat );
  emit atlasPreviewFeatureChanged();
}

void QgsComposer::updateAtlasMapLayerAction( QgsVectorLayer *coverageLayer )
{
  delete mAtlasFeatureAction;
  mAtlasFeatureAction = 0;

  if ( coverageLayer )
  {
    mAtlasFeatureAction = new QgsMapLayerAction( QString( tr( "Set as atlas feature for %1" ) ).arg( mComposition->title() ),
        this, coverageLayer, QgsMapLayerAction::SingleFeature ,
        QgsApplication::getThemeIcon( "/mIconAtlas.svg" ) );
    QgsMapLayerActionRegistry::instance()->addMapLayerAction( mAtlasFeatureAction );
    connect( mAtlasFeatureAction, SIGNAL( triggeredForFeature( QgsMapLayer*, const QgsFeature& ) ), this, SLOT( setAtlasFeature( QgsMapLayer*, const QgsFeature& ) ) );
  }
}

void QgsComposer::updateAtlasMapLayerAction( bool atlasEnabled )
{
  delete mAtlasFeatureAction;
  mAtlasFeatureAction = 0;

  if ( atlasEnabled )
  {
    QgsAtlasComposition& atlas = mComposition->atlasComposition();
    mAtlasFeatureAction = new QgsMapLayerAction( QString( tr( "Set as atlas feature for %1" ) ).arg( mComposition->title() ),
        this, atlas.coverageLayer(), QgsMapLayerAction::SingleFeature ,
        QgsApplication::getThemeIcon( "/mIconAtlas.svg" ) );
    QgsMapLayerActionRegistry::instance()->addMapLayerAction( mAtlasFeatureAction );
    connect( mAtlasFeatureAction, SIGNAL( triggeredForFeature( QgsMapLayer*, const QgsFeature& ) ), this, SLOT( setAtlasFeature( QgsMapLayer*, const QgsFeature& ) ) );
  }
}
