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
#include "qgsapplication.h"
#include "qgscomposerview.h"
#include "qgscomposition.h"
#include "qgscompositionwidget.h"
#include "qgscomposerarrow.h"
#include "qgscomposerarrowwidget.h"
#include "qgscomposerframe.h"
#include "qgscomposerhtml.h"
#include "qgscomposerhtmlwidget.h"
#include "qgscomposerlabel.h"
#include "qgscomposerlabelwidget.h"
#include "qgscomposerlegend.h"
#include "qgscomposerlegendwidget.h"
#include "qgscomposermap.h"
#include "qgscomposermapwidget.h"
#include "qgscomposerpicture.h"
#include "qgscomposerpicturewidget.h"
#include "qgscomposerscalebar.h"
#include "qgscomposerscalebarwidget.h"
#include "qgscomposershape.h"
#include "qgscomposershapewidget.h"
#include "qgscomposerattributetable.h"
#include "qgscomposertablewidget.h"
#include "qgsexception.h"
#include "qgslogger.h"
#include "qgsproject.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsmessageviewer.h"
#include "qgscontexthelp.h"
#include "qgscursors.h"

#include <QCloseEvent>
#include <QCheckBox>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QImageWriter>
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
#include <QToolBar>
#include <QToolButton>
#include <QUndoView>
#include <QPaintEngine>


QgsComposer::QgsComposer( QgisApp *qgis, const QString& title )
    : QMainWindow()
    , mTitle( title )
    , mUndoView( 0 )
{
  setupUi( this );
  setWindowTitle( mTitle );
  setupTheme();
  connect( mButtonBox, SIGNAL( rejected() ), this, SLOT( close() ) );

  QSettings settings;
  int size = settings.value( "/IconSize", QGIS_ICON_SIZE ).toInt();
  setIconSize( QSize( size, size ) );

#ifndef Q_WS_MAC
  setFontSize( settings.value( "/fontPointSize", QGIS_DEFAULT_FONTSIZE ).toInt() );
#endif

  QToolButton* orderingToolButton = new QToolButton( this );
  orderingToolButton->setPopupMode( QToolButton::InstantPopup );
  orderingToolButton->setAutoRaise( true );
  orderingToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  orderingToolButton->addAction( mActionRaiseItems );
  orderingToolButton->addAction( mActionLowerItems );
  orderingToolButton->addAction( mActionMoveItemsToTop );
  orderingToolButton->addAction( mActionMoveItemsToBottom );
  orderingToolButton->setDefaultAction( mActionRaiseItems );
  toolBar->addWidget( orderingToolButton );

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
  toolBar->addWidget( alignToolButton );

  QToolButton* shapeToolButton = new QToolButton( toolBar );
  shapeToolButton->setCheckable( true );
  shapeToolButton->setPopupMode( QToolButton::InstantPopup );
  shapeToolButton->setAutoRaise( true );
  shapeToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  shapeToolButton->addAction( mActionAddRectangle );
  shapeToolButton->addAction( mActionAddTriangle );
  shapeToolButton->addAction( mActionAddEllipse );
  shapeToolButton->setDefaultAction( mActionAddEllipse );
  toolBar->insertWidget( mActionAddArrow, shapeToolButton );

  QActionGroup* toggleActionGroup = new QActionGroup( this );
  toggleActionGroup->addAction( mActionMoveItemContent );
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
  toggleActionGroup->addAction( mActionAddTable );
  toggleActionGroup->addAction( mActionAddHtml );
  toggleActionGroup->setExclusive( true );


  mActionAddNewMap->setCheckable( true );
  mActionAddNewLabel->setCheckable( true );
  mActionAddNewLegend->setCheckable( true );
  mActionSelectMoveItem->setCheckable( true );
  mActionAddNewScalebar->setCheckable( true );
  mActionAddImage->setCheckable( true );
  mActionMoveItemContent->setCheckable( true );
  mActionAddArrow->setCheckable( true );

#ifdef Q_WS_MAC
  QMenu *appMenu = menuBar()->addMenu( tr( "QGIS" ) );
  appMenu->addAction( QgisApp::instance()->actionAbout() );
  appMenu->addAction( QgisApp::instance()->actionOptions() );
#endif

  QMenu *fileMenu = menuBar()->addMenu( tr( "File" ) );
  fileMenu->addAction( mActionLoadFromTemplate );
  fileMenu->addAction( mActionSaveAsTemplate );
  fileMenu->addSeparator();
  fileMenu->addAction( mActionExportAsImage );
  fileMenu->addAction( mActionExportAsPDF );
  fileMenu->addAction( mActionExportAsSVG );
  fileMenu->addSeparator();
  fileMenu->addAction( mActionPageSetup );
  fileMenu->addAction( mActionPrint );
  fileMenu->addSeparator();
  fileMenu->addAction( mActionQuit );
  QObject::connect( mActionQuit, SIGNAL( triggered() ), this, SLOT( close() ) );

  QMenu *viewMenu = menuBar()->addMenu( tr( "View" ) );
  viewMenu->addAction( mActionZoomIn );
  viewMenu->addAction( mActionZoomOut );
  viewMenu->addAction( mActionZoomAll );
  viewMenu->addSeparator();
  viewMenu->addAction( mActionRefreshView );

  // Panel and toolbar submenus
  mPanelMenu = new QMenu( tr( "Panels" ), this );
  mPanelMenu->setObjectName( "mPanelMenu" );
  mToolbarMenu = new QMenu( tr( "Toolbars" ), this );
  mToolbarMenu->setObjectName( "mToolbarMenu" );
  viewMenu->addSeparator();
  viewMenu->addMenu( mPanelMenu );
  viewMenu->addMenu( mToolbarMenu );
  // toolBar already exists, add other widgets as they are created
  mToolbarMenu->addAction( toolBar->toggleViewAction() );

  QMenu *layoutMenu = menuBar()->addMenu( tr( "Layout" ) );
  layoutMenu->addAction( mActionUndo );
  layoutMenu->addAction( mActionRedo );
  layoutMenu->addSeparator();
  layoutMenu->addAction( mActionAddNewMap );
  layoutMenu->addAction( mActionAddNewLabel );
  layoutMenu->addAction( mActionAddNewScalebar );
  layoutMenu->addAction( mActionAddNewLegend );
  layoutMenu->addAction( mActionAddImage );
  layoutMenu->addAction( mActionSelectMoveItem );
  layoutMenu->addAction( mActionMoveItemContent );

  layoutMenu->addAction( mActionAddArrow );
  layoutMenu->addAction( mActionAddTable );
  layoutMenu->addSeparator();
  layoutMenu->addAction( mActionGroupItems );
  layoutMenu->addAction( mActionUngroupItems );
  layoutMenu->addAction( mActionRaiseItems );
  layoutMenu->addAction( mActionLowerItems );
  layoutMenu->addAction( mActionMoveItemsToTop );
  layoutMenu->addAction( mActionMoveItemsToBottom );

#ifdef Q_WS_MAC
#ifndef Q_WS_MAC64 /* assertion failure in NSMenuItem setSubmenu (Qt 4.5.0-snapshot-20080830) */
  menuBar()->addMenu( QgisApp::instance()->windowMenu() );

  menuBar()->addMenu( QgisApp::instance()->helpMenu() );
#endif
#endif

  mQgis = qgis;
  mFirstTime = true;

  // Create action to select this window
  mWindowAction = new QAction( windowTitle(), this );
  connect( mWindowAction, SIGNAL( triggered() ), this, SLOT( activate() ) );

  QgsDebugMsg( "entered." );

  setMouseTracking( true );
  mViewFrame->setMouseTracking( true );

  //create composer view
  mView = new QgsComposerView( mViewFrame );

  //init undo/redo buttons
  mComposition  = new QgsComposition( mQgis->mapCanvas()->mapRenderer() );

  mActionUndo->setEnabled( false );
  mActionRedo->setEnabled( false );
  if ( mComposition->undoStack() )
  {
    connect( mComposition->undoStack(), SIGNAL( canUndoChanged( bool ) ), mActionUndo, SLOT( setEnabled( bool ) ) );
    connect( mComposition->undoStack(), SIGNAL( canRedoChanged( bool ) ), mActionRedo, SLOT( setEnabled( bool ) ) );
  }

  connectSlots();


  mComposition->setParent( mView );
  mView->setComposition( mComposition );

  setTabPosition( Qt::AllDockWidgetAreas, QTabWidget::North );
  mGeneralDock = new QDockWidget( tr( "Composition" ), this );
  mGeneralDock->setObjectName( "CompositionDock" );
  mPanelMenu->addAction( mGeneralDock->toggleViewAction() );
  mItemDock = new QDockWidget( tr( "Item Properties" ), this );
  mItemDock->setObjectName( "ItemDock" );
  mPanelMenu->addAction( mItemDock->toggleViewAction() );
  mUndoDock = new QDockWidget( tr( "Command history" ), this );
  mUndoDock->setObjectName( "CommandDock" );
  mPanelMenu->addAction( mUndoDock->toggleViewAction() );

  mGeneralDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
  mItemDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
  mUndoDock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );


  QgsCompositionWidget* compositionWidget = new QgsCompositionWidget( mGeneralDock, mComposition );
  connect( mComposition, SIGNAL( paperSizeChanged() ), compositionWidget, SLOT( displayCompositionWidthHeight() ) );
  mGeneralDock->setWidget( compositionWidget );

  //undo widget
  mUndoView = new QUndoView( mComposition->undoStack(), this );
  mUndoDock->setWidget( mUndoView );

  addDockWidget( Qt::RightDockWidgetArea, mItemDock );
  addDockWidget( Qt::RightDockWidgetArea, mGeneralDock );
  addDockWidget( Qt::RightDockWidgetArea, mUndoDock );

  mItemDock->show();
  mGeneralDock->show();
  mUndoDock->show();

  tabifyDockWidget( mGeneralDock, mUndoDock );
  tabifyDockWidget( mItemDock, mUndoDock );
  tabifyDockWidget( mGeneralDock, mItemDock );

  mGeneralDock->raise();

  QGridLayout *l = new QGridLayout( mViewFrame );
  l->setMargin( 0 );
  l->addWidget( mView, 0, 0 );

  // Create size grip (needed by Mac OS X for QMainWindow if QStatusBar is not visible)
  mSizeGrip = new QSizeGrip( this );
  mSizeGrip->resize( mSizeGrip->sizeHint() );
  mSizeGrip->move( rect().bottomRight() - mSizeGrip->rect().bottomRight() );

  restoreWindowState();
  setSelectionTool();

  mView->setFocus();

  //connect with signals from QgsProject to write project files
  if ( QgsProject::instance() )
  {
    connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeXML( QDomDocument& ) ) );
  }
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
  mActionLoadFromTemplate->setIcon( QgsApplication::getThemeIcon( "/mActionFileOpen.png" ) );
  mActionSaveAsTemplate->setIcon( QgsApplication::getThemeIcon( "/mActionFileSaveAs.png" ) );
  mActionExportAsImage->setIcon( QgsApplication::getThemeIcon( "/mActionSaveMapAsImage.png" ) );
  mActionExportAsSVG->setIcon( QgsApplication::getThemeIcon( "/mActionSaveAsSVG.png" ) );
  mActionExportAsPDF->setIcon( QgsApplication::getThemeIcon( "/mActionSaveAsPDF.png" ) );
  mActionPrint->setIcon( QgsApplication::getThemeIcon( "/mActionFilePrint.png" ) );
  mActionZoomAll->setIcon( QgsApplication::getThemeIcon( "/mActionZoomFullExtent.png" ) );
  mActionZoomIn->setIcon( QgsApplication::getThemeIcon( "/mActionZoomIn.png" ) );
  mActionZoomOut->setIcon( QgsApplication::getThemeIcon( "/mActionZoomOut.png" ) );
  mActionRefreshView->setIcon( QgsApplication::getThemeIcon( "/mActionDraw.png" ) );
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
  mActionAddHtml->setIcon( QgsApplication::getThemeIcon( "/mActionAddHtml.png" ) );
  mActionSelectMoveItem->setIcon( QgsApplication::getThemeIcon( "/mActionSelectPan.png" ) );
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

void QgsComposer::setFontSize( int fontSize )
{
  setStyleSheet( QString( "font-size: %1pt; " ).arg( fontSize ) );
}

void QgsComposer::connectSlots()
{
  connect( mView, SIGNAL( selectedItemChanged( QgsComposerItem* ) ), this, SLOT( showItemOptions( QgsComposerItem* ) ) );
  connect( mView, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( deleteItem( QgsComposerItem* ) ) );
  connect( mView, SIGNAL( actionFinished() ), this, SLOT( setSelectionTool() ) );

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
  connect( mComposition, SIGNAL( itemRemoved( QgsComposerItem* ) ), this, SLOT( deleteItem( QgsComposerItem* ) ) );
}

void QgsComposer::open( void )
{
  if ( mFirstTime )
  {
    //mComposition->createDefault();
    mFirstTime = false;
    show();
    zoomFull(); // zoomFull() does not work properly until we have called show()
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
  show();
  raise();
  setWindowState( windowState() & ~Qt::WindowMinimized );
  activateWindow();
}

#ifdef Q_WS_MAC
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

QgsMapCanvas *QgsComposer::mapCanvas( void )
{
  return mQgis->mapCanvas();
}

QgsComposerView *QgsComposer::view( void )
{
  return mView;
}

/*QgsComposition *QgsComposer::composition(void)
{
  return mComposition;
  }*/

void QgsComposer::zoomFull( void )
{
  if ( mView )
  {
    int nPages = mComposition->numPages();
    if ( nPages < 1 )
    {
      return;
    }
    double height = mComposition->paperHeight() * nPages + mComposition->spaceBetweenPages() * ( nPages - 1 );
    mView->fitInView( 0, 0, mComposition->paperWidth() + 1, height + 1, Qt::KeepAspectRatio );
  }
}

void QgsComposer::on_mActionZoomAll_triggered()
{
  zoomFull();
  mView->update();
  emit zoomLevelChanged();
}

void QgsComposer::on_mActionZoomIn_triggered()
{
  mView->scale( 2, 2 );
  mView->update();
  emit zoomLevelChanged();
}

void QgsComposer::on_mActionZoomOut_triggered()
{
  mView->scale( .5, .5 );
  mView->update();
  emit zoomLevelChanged();
}

void QgsComposer::on_mActionRefreshView_triggered()
{
  if ( !mComposition )
  {
    return;
  }

  //refresh preview of all composer maps
  QMap<QgsComposerItem*, QWidget*>::iterator it = mItemWidgetMap.begin();
  for ( ; it != mItemWidgetMap.end(); ++it )
  {
    QgsComposerMap* map = dynamic_cast<QgsComposerMap*>( it.key() );
    if ( map && !map->isDrawing() )
    {
      map->cache();
      map->update();
    }
  }

  mComposition->update();
}

void QgsComposer::on_mActionExportAsPDF_triggered()
{
  if ( !mComposition || !mView )
  {
    return;
  }

  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  QSettings myQSettings;  // where we keep last used filter in persistent state
  QString lastUsedFile = myQSettings.value( "/UI/lastSaveAsPdfFile", "qgis.pdf" ).toString();
  QFileInfo file( lastUsedFile );

  QString outputFileName = QFileDialog::getSaveFileName(
                             this,
                             tr( "Choose a file name to save the map as" ),
                             file.path(),
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

  QApplication::setOverrideCursor( Qt::BusyCursor );
  mView->setPaintingEnabled( false );
  mComposition->exportAsPDF( outputFileName );
  mView->setPaintingEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionPrint_triggered()
{
  if ( !mComposition || !mView )
  {
    return;
  }

  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  //orientation and page size are already set to QPrinter in the page setup dialog
  QPrintDialog printDialog( &mPrinter, 0 );
  if ( printDialog.exec() != QDialog::Accepted )
  {
    return;
  }

  QApplication::setOverrideCursor( Qt::BusyCursor );
  mView->setPaintingEnabled( false );
  mComposition->print( mPrinter );
  mView->setPaintingEnabled( true );
  QApplication::restoreOverrideCursor();
}

void QgsComposer::on_mActionExportAsImage_triggered()
{
  if ( !mComposition || !mView )
  {
    return;
  }

  if ( containsWMSLayer() )
  {
    showWMSPrintingWarning();
  }

  // Image size
  int width = ( int )( mComposition->printResolution() * mComposition->paperWidth() / 25.4 );
  int height = ( int )( mComposition-> printResolution() * mComposition->paperHeight() / 25.4 );

  int memuse = width * height * 3 / 1000000;  // pixmap + image
  QgsDebugMsg( QString( "Image %1x%2" ).arg( width ).arg( height ) );
  QgsDebugMsg( QString( "memuse = %1" ).arg( memuse ) );

  if ( memuse > 200 )   // about 4500x4500
  {
    int answer = QMessageBox::warning( 0, tr( "Big image" ),
                                       tr( "To create image %1x%2 requires about %3 MB of memory. Proceed?" )
                                       .arg( width ).arg( height ).arg( memuse ),
                                       QMessageBox::Ok | QMessageBox::Cancel,  QMessageBox::Ok );

    raise();
    if ( answer == QMessageBox::Cancel )
      return;
  }

  QPair<QString, QString> fileNExt = QgisGui::getSaveAsImageName( this, tr( "Choose a file name to save the map image as" ) );

  if ( fileNExt.first.isEmpty() )
  {
    return;
  }

  mView->setPaintingEnabled( false );

  for ( int i = 0; i < mComposition->numPages(); ++i )
  {
    QImage image = mComposition->printPageAsRaster( i );
    if ( i == 0 )
    {
      image.save( fileNExt.first, fileNExt.second.toLocal8Bit().constData() );
    }
    else
    {
      QFileInfo fi( fileNExt.first );
      QString outputFilePath = fi.absolutePath() + "/" + fi.baseName() + "_" + QString::number( i + 1 ) + "." + fi.suffix();
      image.save( outputFilePath, fileNExt.second.toLocal8Bit().constData() );
    }
  }
  mView->setPaintingEnabled( true );
}


void QgsComposer::on_mActionExportAsSVG_triggered()
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

  QString lastUsedFile = settings.value( "/UI/lastSaveAsSvgFile", "qgis.svg" ).toString();
  QFileInfo file( lastUsedFile );

  QString outputFileName = QFileDialog::getSaveFileName(
                             this,
                             tr( "Choose a file name to save the map as" ),
                             file.path(),
                             tr( "SVG Format" ) + " (*.svg *.SVG)" );
  if ( outputFileName.isEmpty() )
    return;

  if ( !outputFileName.endsWith( ".svg", Qt::CaseInsensitive ) )
  {
    outputFileName += ".svg";
  }

  settings.setValue( "/UI/lastSaveAsSvgFile", outputFileName );

  mView->setPaintingEnabled( false );
  for ( int i = 0; i < mComposition->numPages(); ++i )
  {
    QSvgGenerator generator;
#if QT_VERSION >= 0x040500
    generator.setTitle( QgsProject::instance()->title() );
#endif
    if ( i == 0 )
    {
      generator.setFileName( outputFileName );
    }
    else
    {
      QFileInfo fi( outputFileName );
      generator.setFileName( fi.absolutePath() + "/" + fi.baseName() + "_" + QString::number( i + 1 ) + "." + fi.suffix() );
    }

    //width in pixel
    int width = ( int )( mComposition->paperWidth() * mComposition->printResolution() / 25.4 );
    //height in pixel
    int height = ( int )( mComposition->paperHeight() * mComposition->printResolution() / 25.4 );
    generator.setSize( QSize( width, height ) );
#if QT_VERSION >= 0x040500
    generator.setViewBox( QRect( 0, 0, width, height ) );
#endif
    generator.setResolution( mComposition->printResolution() ); //because the rendering is done in mm, convert the dpi

    QPainter p( &generator );

    mComposition->renderPage( &p, i );
    p.end();
  }
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
  settings.setValue( "UI/LastComposerTemplateDir", saveFileInfo.absolutePath() );

  QFile templateFile( saveFileName );
  if ( !templateFile.open( QIODevice::WriteOnly ) )
  {
    return;
  }

  QDomDocument saveDocument;
  writeXML( saveDocument, saveDocument );

  if ( templateFile.write( saveDocument.toByteArray() ) == -1 )
  {
    QMessageBox::warning( 0, tr( "Save error" ), tr( "Error, could not save file" ) );
  }
}

void QgsComposer::on_mActionLoadFromTemplate_triggered()
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
    QMessageBox::warning( 0, tr( "Read error" ), tr( "Error, could not read file" ) );
    return;
  }

  if ( mComposition )
  {
    QDomDocument templateDoc;
    if ( templateDoc.setContent( &templateFile ) )
    {
      mComposition->loadFromTemplate( templateDoc, 0, false );
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
  mSizeGrip->move( rect().bottomRight() - mSizeGrip->rect().bottomRight() );

  saveWindowState();
}

void QgsComposer::showEvent( QShowEvent* event )
{
  if ( event->spontaneous() ) //event from the window system
  {
    restoreComposerMapStates();
    initialiseComposerPicturePreviews();
  }

#ifdef Q_WS_MAC
  // add to menu if (re)opening window (event not due to unminimize)
  if ( !event->spontaneous() )
  {
    QgisApp::instance()->addWindow( mWindowAction );
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

void QgsComposer::restoreWindowState()
{
  QSettings settings;
  if ( ! restoreState( settings.value( "/ComposerUI/state" ).toByteArray() ) )
  {
    QgsDebugMsg( "RESTORE STATE FAILED!!" );
  }
  restoreGeometry( settings.value( "/Composer/geometry" ).toByteArray() );
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

  //delete composer view and composition
  delete mView;
  mView = 0;
  //delete every child of mViewFrame
  QObjectList viewFrameChildren = mViewFrame->children();
  QObjectList::iterator it = viewFrameChildren.begin();
  for ( ; it != viewFrameChildren.end(); ++it )
  {
    delete( *it );
  }
  //delete composition widget
  QgsCompositionWidget* oldCompositionWidget = qobject_cast<QgsCompositionWidget *>( mGeneralDock->widget() );
  delete oldCompositionWidget;

  mView = new QgsComposerView( mViewFrame );

  //read composition settings
  mComposition = new QgsComposition( mQgis->mapCanvas()->mapRenderer() );
  QDomNodeList compositionNodeList = composerElem.elementsByTagName( "Composition" );
  if ( compositionNodeList.size() > 0 )
  {
    QDomElement compositionElem = compositionNodeList.at( 0 ).toElement();
    mComposition->readXML( compositionElem, doc );
  }

  connectSlots();

  QGridLayout *l = new QGridLayout( mViewFrame );
  l->setMargin( 0 );
  l->addWidget( mView, 0, 0 );

  //create compositionwidget
  QgsCompositionWidget* compositionWidget = new QgsCompositionWidget( mGeneralDock, mComposition );
  QObject::connect( mComposition, SIGNAL( paperSizeChanged() ), compositionWidget, SLOT( displayCompositionWidthHeight() ) );
  mGeneralDock->setWidget( compositionWidget );

  //read and restore all the items
  if ( mComposition )
  {
    mComposition->addItemsFromXML( composerElem, doc, &mMapsToRestore );
  }

  mComposition->sortZList();
  mView->setComposition( mComposition );

  if ( mUndoView )
  {
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



  setSelectionTool();
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
  if ( isVisible() )
  {
    pWidget->addStandardDirectoriesToPreview();
  }
  else
  {
    mPicturePreviews.append( pWidget );
  }
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
  delete( it.value() );
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
          QgsRectangle canvasExtent = mapItem->mapRenderer()->extent();
          //adapt min y of extent such that the size of the map item stays the same
          double newCanvasExtentHeight = currentHeight / currentWidth * canvasExtent.width();
          canvasExtent.setYMinimum( canvasExtent.yMaximum() - newCanvasExtentHeight );
          mapItem->setNewExtent( canvasExtent );
        }
      }
    }
  }

  restoreComposerMapStates();
  initialiseComposerPicturePreviews();
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

void QgsComposer::initialiseComposerPicturePreviews()
{
  //create composer picture widget previews
  QList< QgsComposerPictureWidget* >::iterator picIt = mPicturePreviews.begin();
  for ( ; picIt != mPicturePreviews.end(); ++picIt )
  {
    ( *picIt )->addStandardDirectoriesToPreview();
  }
  mPicturePreviews.clear();
}
