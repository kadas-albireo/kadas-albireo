#include "qgskadasmainwidget.h"
#include "qgsapplication.h"
#include "qgsattributetabledialog.h"
#include "qgsclipboard.h"
#include "qgsdecorationgrid.h"
#include "qgskadaslayertreeviewmenuprovider.h"
#include "qgskmlexport.h"
#include "qgskmlexportdialog.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgslayertreeregistrybridge.h"
#include "qgslayertreemodel.h"
#include "qgslayertree.h"
#include "qgsapplayertreeviewmenuprovider.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmessageviewer.h"
#include "qgsmessagebar.h"
#include "qgsmessagebaritem.h"
#include "qgsnetworkaccessmanager.h"
#include "qgspluginlayer.h"
#include "qgspluginlayerregistry.h"
#include "qgsproject.h"
#include "qgsproviderregistry.h"
#include "qgsrasterlayerproperties.h"
#include "qgstemporaryfile.h"
#include "kadas/qgsvbscoordinatedisplayer.h"
#include "kadas/qgsvbscrsselection.h"
#include "kadas/qgsvbsmaptoolpinannotation.h"
#include "kadas/qgsvbspinannotationitem.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerproperties.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

//
// Map tools
//
#include "qgsmaptooladdfeature.h"
#include "qgsmaptooladdpart.h"
#include "qgsmaptooladdring.h"
#include "qgsmaptoolfillring.h"
#include "qgsmaptoolannotation.h"
#include "qgsmaptoolcircularstringcurvepoint.h"
#include "qgsmaptoolcircularstringradius.h"
#include "qgsmaptooldeletering.h"
#include "qgsmaptooldeletepart.h"
#include "qgsmaptoolfeatureaction.h"
#include "qgsmaptoolformannotation.h"
#include "qgsmaptoolhtmlannotation.h"
#include "qgsmaptoolidentifyaction.h"
#include "qgsmaptoolmeasureangle.h"
#include "qgsmaptoolmovefeature.h"
#include "qgsmaptoolrotatefeature.h"
#include "qgsmaptooloffsetcurve.h"
#include "qgsmaptoolpan.h"
#include "qgsmaptoolselect.h"
#include "qgsmaptoolselectrectangle.h"
#include "qgsmaptoolselectfreehand.h"
#include "qgsmaptoolselectpolygon.h"
#include "qgsmaptoolselectradius.h"
#include "qgsmaptoolsvgannotation.h"
#include "qgsmaptoolreshape.h"
#include "qgsmaptoolrotatepointsymbols.h"
#include "qgsmaptoolsplitfeatures.h"
#include "qgsmaptoolsplitparts.h"
#include "qgsmaptooltextannotation.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolsimplify.h"
#include "qgsmeasuretool.h"
#include "qgsmeasurecircletool.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgsmaptoolpinlabels.h"
#include "qgsmaptoolshowhidelabels.h"
#include "qgsmaptoolmovelabel.h"
#include "qgsmaptoolrotatelabel.h"
#include "qgsmaptoolchangelabelproperties.h"
#include "nodetool/qgsmaptoolnodetool.h"
#include "qgsguivectorlayertools.h"

#include "qgsvbshillshadetool.h"
#include "qgsvbsslopetool.h"
#include "qgsvbsviewshedtool.h"


QgsKadasMainWidget::QgsKadasMainWidget( QWidget* parent, Qt::WindowFlags f ): QWidget( parent, f ), mLayerTreeCanvasBridge( 0 ),
    mNonEditMapTool( 0 ), mSlopeTool( 0 ), mSaveRollbackInProgress( false )
{
  mVectorLayerTools = new QgsGuiVectorLayerTools();

  namSetup(); //setup network access manager
  setupUi( this );
  mToggleButtonGroup = new QButtonGroup( this );
  mLayerTreeView->setVisible( false );

  QgsApplication::initQgis();

  mCoordinateDisplayer = new QgsVBSCoordinateDisplayer( mCRSComboBox, mCoordinateLineEdit, mMapCanvas, this );
  mCRSSelectionButton->setMapCanvas( mMapCanvas );

  connect( mScaleComboBox, SIGNAL( scaleChanged() ), this, SLOT( userScale() ) );

  configureButtons();

  mMapCanvas->freeze();
  initLayerTreeView();
  createDecorations();
  initMapCanvas();

  mSearchWidget->init( mMapCanvas );

  // a bar to warn the user with non-blocking messages
  mInfoBar = new QgsMessageBar( mMapCanvas );
  mInfoBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
  //centralLayout->addWidget( mInfoBar, 0, 0, 1, 1 );

  /*mLayerTreeView = new QgsLayerTreeView( this );
  mLayerTreeView->setObjectName( "theLayerTreeView" ); // "theLayerTreeView" used to find this canonical instance later*/

  //initLayerTreeView

  connect( mMapCanvas, SIGNAL( scaleChanged( double ) ), this, SLOT( showScale( double ) ) );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( readProject( const QDomDocument& ) ) );

  bool otfTransformAutoEnable = QSettings().value( "/Projections/otfTransformAutoEnable", true ).toBool();
  mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );

  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument & ) ),
           this, SLOT( readProject( const QDomDocument & ) ) );

  QgsProviderRegistry::instance()->registerGuis( this );

  createCanvasTools();
  activateDeactivateLayerRelatedActions( 0 ); // after members were created

  restoreFavoriteButton( mFavoriteButton1 );
  restoreFavoriteButton( mFavoriteButton2 );
  restoreFavoriteButton( mFavoriteButton3 );
  restoreFavoriteButton( mFavoriteButton4 );

  mInternalClipboard = new QgsClipboard; // create clipboard
  //connect( mInternalClipboard, SIGNAL( changed() ), this, SLOT( clipboardChanged() ) );
}

QgsKadasMainWidget::~QgsKadasMainWidget()
{
  delete mVectorLayerTools;
  delete mInternalClipboard;
}

int QgsKadasMainWidget::messageTimeout()
{
  QSettings settings;
  return settings.value( "/qgis/messageTimeout", 5 ).toInt();
}

void QgsKadasMainWidget::resizeEvent( QResizeEvent* event )
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
  QWidget::resizeEvent( event );

  //resize mLayerTreeView and mLayerTreeViewButton
  int distanceToTopBottom = 40;
  int layerTreeHeight = mapCanvasGeometry.height() - 2 * distanceToTopBottom;
  mLayerTreeViewButton->setGeometry( mLayerTreeViewButton->pos().x(), distanceToTopBottom, mLayerTreeViewButton->geometry().width(), layerTreeHeight );
  mLayerTreeView->setGeometry( mLayerTreeView->pos().x(), distanceToTopBottom, mLayerTreeView->geometry().width(), layerTreeHeight );
}

void QgsKadasMainWidget::mousePressEvent( QMouseEvent* event )
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

void QgsKadasMainWidget::mouseMoveEvent( QMouseEvent* event )
{
  if ( event->buttons() & Qt::LeftButton )
  {
    QgsKadasRibbonButton* button = dynamic_cast<QgsKadasRibbonButton*>( childAt( event->pos() ) );
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

void QgsKadasMainWidget::dropEvent( QDropEvent* event )
{
  if ( !event || mDragStartActionName.isEmpty() )
  {
    return;
  }

  //get button under mouse
  QgsKadasRibbonButton* button = dynamic_cast<QgsKadasRibbonButton*>( childAt( event->pos() ) );
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

void QgsKadasMainWidget::dragEnterEvent( QDragEnterEvent* event )
{
  QgsKadasRibbonButton* button = dynamic_cast<QgsKadasRibbonButton*>( childAt( event->pos() ) );
  if ( button && button->acceptDrops() )
  {
    event->acceptProposedAction();
  }
}

void QgsKadasMainWidget::performDrag( const QIcon* icon )
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

void QgsKadasMainWidget::restoreFavoriteButton( QToolButton* button )
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

void QgsKadasMainWidget::configureButtons()
{
  //My maps tab

  //mActionNew
  connect( mActionNew, SIGNAL( triggered() ), this, SLOT( fileNew() ) );
  setActionToButton( mActionNew, mNewButton );
  //mActionOpen
  connect( mActionOpen, SIGNAL( triggered() ), this, SLOT( open() ) );
  setActionToButton( mActionOpen, mOpenButton );
  //mActionSave
  connect( mActionSave, SIGNAL( triggered() ), this, SLOT( save() ) );
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
  setActionToButton( mActionNewMapWindow, mNewMapWindowButton );
  setActionToButton( mAction3D, m3DButton );
  setActionToButton( mActionGrid, mGridButton );

  //draw tab

  //mActionPin
  connect( mActionPin, SIGNAL( toggled( bool ) ), this, SLOT( pin( bool ) ) );
  setActionToButton( mActionPin, mPinButton );

  //Analysis tab

  //mActionDistance
  connect( mActionDistance, SIGNAL( toggled( bool ) ), this, SLOT( distance( bool ) ) );
  setActionToButton( mActionDistance, mDistanceButton );
  //mActionArea
  connect( mActionArea, SIGNAL( toggled( bool ) ), this, SLOT( area( bool ) ) );
  setActionToButton( mActionArea, mAreaButton );
  //mActionCircle
  connect( mActionCircle, SIGNAL( toggled( bool ) ), this, SLOT( circle( bool ) ) );
  setActionToButton( mActionCircle, mMeasureCircleButton );
  //mActionProfile
  connect( mActionProfile, SIGNAL( toggled( bool ) ), this, SLOT( profile( bool ) ) );
  setActionToButton( mActionProfile, mProfileButton );
  //mActionAzimuth
  connect( mActionAzimuth, SIGNAL( toggled( bool ) ), this, SLOT( azimuth( bool ) ) );
  setActionToButton( mActionAzimuth, mAzimuthButton );
  //mActionLineOfSight
  setActionToButton( mActionLineOfSight, mLineOfSightButton );
  //mActionSlope
  connect( mActionSlope, SIGNAL( toggled( bool ) ), this, SLOT( slope( bool ) ) );
  setActionToButton( mActionSlope, mSlopeButton );
  //mActionHillshade
  connect( mActionHillshade, SIGNAL( toggled( bool ) ), this, SLOT( hillshade( bool ) ) );
  setActionToButton( mActionHillshade, mHillshadeButton );
  //mAcionViewshed
  connect( mActionViewshed, SIGNAL( toggled( bool ) ), this, SLOT( viewshed( bool ) ) );
  setActionToButton( mActionViewshed, mViewshedButton );
  //mActionWPS
  setActionToButton( mActionWPS, mWPSButton );
}

void QgsKadasMainWidget::fileNew( bool thePromptToSaveFlag, bool forceBlank )
{
  if ( thePromptToSaveFlag )
  {
    if ( !saveDirty() )
    {
      return; //cancel pressed
    }
  }

  QSettings settings;

  closeProject();

  QgsProject* prj = QgsProject::instance();
  prj->clear();

  prj->layerTreeRegistryBridge()->setNewLayersVisible( settings.value( "/qgis/new_layers_visible", true ).toBool() );

  mLayerTreeCanvasBridge->clear();

  //set the color for selections
  //the default can be set in qgisoptions
  //use project properties to override the color on a per project basis
  int myRed = settings.value( "/qgis/default_selection_color_red", 255 ).toInt();
  int myGreen = settings.value( "/qgis/default_selection_color_green", 255 ).toInt();
  int myBlue = settings.value( "/qgis/default_selection_color_blue", 0 ).toInt();
  int myAlpha = settings.value( "/qgis/default_selection_color_alpha", 255 ).toInt();
  prj->writeEntry( "Gui", "/SelectionColorRedPart", myRed );
  prj->writeEntry( "Gui", "/SelectionColorGreenPart", myGreen );
  prj->writeEntry( "Gui", "/SelectionColorBluePart", myBlue );
  prj->writeEntry( "Gui", "/SelectionColorAlphaPart", myAlpha );
  mMapCanvas->setSelectionColor( QColor( myRed, myGreen, myBlue, myAlpha ) );

  //set the canvas to the default background color
  //the default can be set in qgisoptions
  //use project properties to override the color on a per project basis
  myRed = settings.value( "/qgis/default_canvas_color_red", 255 ).toInt();
  myGreen = settings.value( "/qgis/default_canvas_color_green", 255 ).toInt();
  myBlue = settings.value( "/qgis/default_canvas_color_blue", 255 ).toInt();
  myAlpha = settings.value( "/qgis/default_canvas_color_alpha", 0 ).toInt();
  prj->writeEntry( "Gui", "/CanvasColorRedPart", myRed );
  prj->writeEntry( "Gui", "/CanvasColorGreenPart", myGreen );
  prj->writeEntry( "Gui", "/CanvasColorBluePart", myBlue );
  prj->writeEntry( "Gui", "/CanvasColorAlphaPart", myAlpha );
  mMapCanvas->setCanvasColor( QColor( myRed, myGreen, myBlue, myAlpha ) );

  prj->dirty( false );

  //setTitleBarText_( *this );

  //QgsDebugMsg("emiting new project signal");

  // emit signal so listeners know we have a new project
  emit newProject();

  mMapCanvas->freeze( false );
  mMapCanvas->refresh();
  mMapCanvas->clearExtentHistory();
  mMapCanvas->setRotation( 0.0 );
  mScaleComboBox->updateScales();

  // set project CRS
  QString defCrs = settings.value( "/Projections/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
  QgsCoordinateReferenceSystem srs;
  srs.createFromOgcWmsCrs( defCrs );
  mMapCanvas->setDestinationCrs( srs );
  // write the projections _proj string_ to project settings
  prj->writeEntry( "SpatialRefSys", "/ProjectCRSProj4String", srs.toProj4() );
  prj->writeEntry( "SpatialRefSys", "/ProjectCrs", srs.authid() );
  prj->writeEntry( "SpatialRefSys", "/ProjectCRSID", ( int ) srs.srsid() );
  prj->dirty( false );
  if ( srs.mapUnits() != QGis::UnknownUnit )
  {
    mMapCanvas->setMapUnits( srs.mapUnits() );
  }

  // enable OTF CRS transformation if necessary
  mMapCanvas->setCrsTransformEnabled( settings.value( "/Projections/otfTransformEnabled", 0 ).toBool() );

  //updateCRSStatusBar();

  /** New Empty Project Created
      (before attempting to load custom project templates/filepaths) */

  // load default template
  /* NOTE: don't open default template on launch until after initialization,
           in case a project was defined via command line */

  // don't open template if last auto-opening of a project failed
  if ( ! forceBlank )
  {
    forceBlank = ! settings.value( "/qgis/projOpenedOKAtLaunch", QVariant( true ) ).toBool();
  }

  if ( ! forceBlank && settings.value( "/qgis/newProjectDefault", QVariant( false ) ).toBool() )
  {
    fileNewFromDefaultTemplate();
  }

  //QgsVisibilityPresets::instance()->clear();

  // set the initial map tool
  mMapCanvas->setMapTool( mMapTools.mPan );
  mNonEditMapTool = mMapTools.mPan;
}

void QgsKadasMainWidget::fileNewFromDefaultTemplate()
{
  QString projectTemplate = QgsApplication::qgisSettingsDirPath() + QString( "project_default.qgs" );
  QString msgTxt;
  if ( !projectTemplate.isEmpty() && QFile::exists( projectTemplate ) )
  {
    if ( fileNewFromTemplate( projectTemplate ) )
    {
      return;
    }
    msgTxt = tr( "Default failed to open: %1" );
  }
  else
  {
    msgTxt = tr( "Default not found: %1" );
  }
  /*messageBar()->pushMessage( tr( "Open Template Project" ),
                             msgTxt.arg( projectTemplate ),
                             QgsMessageBar::WARNING );*/
}

bool QgsKadasMainWidget::fileNewFromTemplate( QString fileName )
{
  if ( !saveDirty() )
  {
    return false; //cancel pressed
  }

  QgsDebugMsg( QString( "loading project template: %1" ).arg( fileName ) );
  if ( addProject( fileName ) )
  {
    // set null filename so we don't override the template
    QgsProject::instance()->setFileName( QString() );
    return true;
  }
  return false;
}

void QgsKadasMainWidget::commitError( QgsVectorLayer *vlayer )
{
  QgsMessageViewer *mv = new QgsMessageViewer();
  mv->setWindowTitle( tr( "Commit errors" ) );
  mv->setMessageAsPlainText( tr( "Could not commit changes to layer %1" ).arg( vlayer->name() )
                             + "\n\n"
                             + tr( "Errors: %1\n" ).arg( vlayer->commitErrors().join( "\n  " ) )
                           );

  QToolButton *showMore = new QToolButton();
  // store pointer to vlayer in data of QAction
  QAction *act = new QAction( showMore );
  act->setData( QVariant( QMetaType::QObjectStar, &vlayer ) );
  act->setText( tr( "Show more" ) );
  showMore->setStyleSheet( "background-color: rgba(255, 255, 255, 0); color: black; text-decoration: underline;" );
  showMore->setCursor( Qt::PointingHandCursor );
  showMore->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
  showMore->addAction( act );
  showMore->setDefaultAction( act );
  connect( showMore, SIGNAL( triggered( QAction* ) ), mv, SLOT( exec() ) );
  connect( showMore, SIGNAL( triggered( QAction* ) ), showMore, SLOT( deleteLater() ) );

  // no timeout set, since notice needs attention and is only shown first time layer is labeled
  QgsMessageBarItem *errorMsg = new QgsMessageBarItem(
    tr( "Commit errors" ),
    tr( "Could not commit changes to layer %1" ).arg( vlayer->name() ),
    showMore,
    QgsMessageBar::WARNING,
    0,
    messageBar() );
  messageBar()->pushItem( errorMsg );
}

void QgsKadasMainWidget::initMapCanvas()
{
  if ( !mMapCanvas )
  {
    return;
  }

  QSettings mySettings;
  mMapCanvas->enableAntiAliasing( mySettings.value( "/qgis/enable_anti_aliasing", true ).toBool() );
  int action = mySettings.value( "/qgis/wheel_action", 2 ).toInt();
  double zoomFactor = mySettings.value( "/qgis/zoom_factor", 2 ).toDouble();
  mMapCanvas->setWheelAction(( QgsMapCanvas::WheelAction ) action, zoomFactor );
  mMapCanvas->setCachingEnabled( mySettings.value( "/qgis/enable_render_caching", true ).toBool() );
  mMapCanvas->setParallelRenderingEnabled( mySettings.value( "/qgis/parallel_rendering", false ).toBool() );
  mMapCanvas->setMapUpdateInterval( mySettings.value( "/qgis/map_update_interval", 250 ).toInt() );
}

void QgsKadasMainWidget::namSetup()
{
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();

  namUpdate();

#ifndef QT_NO_OPENSSL
  connect( nam, SIGNAL( sslErrorsConformationRequired( QUrl, QList<QSslError>, bool* ) ),
           this, SLOT( namConfirmSslErrors( QUrl, QList<QSslError>, bool* ) ) );
#endif
  connect( nam, SIGNAL( requestTimedOut( QNetworkReply* ) ),
           this, SLOT( namRequestTimedOut( QNetworkReply* ) ) );
}

#ifndef QT_NO_OPENSSL
void QgsKadasMainWidget::namConfirmSslErrors( const QUrl& url, const QList<QSslError> &errors, bool *ok )
{
  QString msg = tr( "SSL errors occured accessing URL %1:" ).arg( url.toString() );
  foreach ( QSslError error, errors )
  {
    msg += "\n" + error.errorString();
  }
  msg += tr( "\n\nAlways ignore these errors?" );
  *ok = QMessageBox::warning( this,
                              tr( "%1 SSL errors occured", "number of errors", errors.size() ),
                              msg,
                              QMessageBox::Yes | QMessageBox::No ) == QMessageBox::Yes;
}
#endif

void QgsKadasMainWidget::namRequestTimedOut( QNetworkReply *reply )
{
  Q_UNUSED( reply );
  QLabel *msgLabel = new QLabel( tr( "A network request timed out, any data received is likely incomplete." ) +
                                 tr( " Please check the <a href=\"#messageLog\">message log</a> for further info." ), messageBar() );
  msgLabel->setWordWrap( true );
  //connect( msgLabel, SIGNAL( linkActivated( QString ) ), mLogDock, SLOT( show() ) );
  messageBar()->pushItem( new QgsMessageBarItem( msgLabel, QgsMessageBar::WARNING, messageTimeout() ) );
}

void QgsKadasMainWidget::namUpdate()
{
  QgsNetworkAccessManager::instance()->setupDefaultProxyAndCache();
}

void QgsKadasMainWidget::initLayerTreeView()
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
  mLayerTreeView->setMenuProvider( new QgsKadasLayerTreeViewMenuProvider( mLayerTreeView, this ) );

  //setup connections
  connect( mLayerTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );

  //setupLayerTreeViewFromSettings();
}

void QgsKadasMainWidget::createDecorations()
{
  QgsDecorationGrid* mDecorationGrid = new QgsDecorationGrid( mMapCanvas, this );
  connect( mActionGrid, SIGNAL( triggered() ), mDecorationGrid, SLOT( run() ) );

  mDecorationItems.append( mDecorationGrid );
  connect( mMapCanvas, SIGNAL( renderComplete( QPainter * ) ), this, SLOT( renderDecorationItems( QPainter * ) ) );
  connect( this, SIGNAL( newProject() ), this, SLOT( projectReadDecorationItems() ) );
  connect( this, SIGNAL( projectRead() ), this, SLOT( projectReadDecorationItems() ) );
}

void QgsKadasMainWidget::renderDecorationItems( QPainter *p )
{
  foreach ( QgsDecorationItem* item, mDecorationItems )
  {
    item->render( p );
  }
}

void QgsKadasMainWidget::projectReadDecorationItems()
{
  foreach ( QgsDecorationItem* item, mDecorationItems )
  {
    item->projectRead();
  }
}

void QgsKadasMainWidget::setActionToButton( QAction* action, QToolButton* button )
{
  button->setDefaultAction( action );
  button->setProperty( "actionName", action->objectName() );
  if ( action->isCheckable() )
  {
    mToggleButtonGroup->addButton( button );
  }
}

void QgsKadasMainWidget::addToFavorites()
{
}

void QgsKadasMainWidget::fileNew()
{
  fileNew( true ); // prompts whether to save project
}

void QgsKadasMainWidget::open()
{
  // possibly save any pending work before opening a new project
  if ( saveDirty() )
  {
    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();
    QString fullPath = QFileDialog::getOpenFileName( this,
                       tr( "Choose a QGIS project file to open" ),
                       lastUsedDir,
                       tr( "QGIS files" ) + " (*.qgs *.QGS)" );
    if ( fullPath.isNull() )
    {
      return;
    }

    // Fix by Tim - getting the dirPath from the dialog
    // directly truncates the last node in the dir path.
    // This is a workaround for that
    QFileInfo myFI( fullPath );
    QString myPath = myFI.path();
    // Persist last used project dir
    settings.setValue( "/UI/lastProjectDir", myPath );

    // open the selected project
    addProject( fullPath );
  }
}

bool QgsKadasMainWidget::save()
{
  // if we don't have a file name, then obviously we need to get one; note
  // that the project file name is reset to null in fileNew()
  QFileInfo fullPath;

  // we need to remember if this is a new project so that we know to later
  // update the "last project dir" settings; we know it's a new project if
  // the current project file name is empty
  bool isNewProject = false;

  if ( QgsProject::instance()->fileName().isNull() )
  {
    isNewProject = true;

    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

    QString path = QFileDialog::getSaveFileName(
                     this,
                     tr( "Choose a QGIS project file" ),
                     lastUsedDir + "/" + QgsProject::instance()->title(),
                     tr( "QGIS files" ) + " (*.qgs *.QGS)" );
    if ( path.isEmpty() )
      return false;

    fullPath.setFile( path );

    // make sure we have the .qgs extension in the file name
    if ( "qgs" != fullPath.suffix().toLower() )
    {
      fullPath.setFile( fullPath.filePath() + ".qgs" );
    }


    QgsProject::instance()->setFileName( fullPath.filePath() );
  }
  else
  {
    QFileInfo fi( QgsProject::instance()->fileName() );
    if ( fi.exists() && ! fi.isWritable() )
    {
      messageBar()->pushMessage( tr( "Insufficient permissions" ),
                                 tr( "The project file is not writable." ),
                                 QgsMessageBar::WARNING );
      return false;
    }
  }

  if ( QgsProject::instance()->write() )
  {
    /*setTitleBarText_( *this ); // update title bar
    statusBar()->showMessage( tr( "Saved project to: %1" ).arg( QgsProject::instance()->fileName() ), 5000 );*/

    if ( isNewProject )
    {
      // add this to the list of recently used project files
      QSettings settings;
      saveRecentProjectPath( fullPath.filePath(), settings );
    }
  }
  else
  {
    QMessageBox::critical( this,
                           tr( "Unable to save project %1" ).arg( QgsProject::instance()->fileName() ),
                           QgsProject::instance()->error() );
    return false;
  }

#if 0
  // run the saved project macro
  if ( mTrustedMacros )
  {
    QgsPythonRunner::run( "qgis.utils.saveProjectMacro();" );
  }
#endif //0

  return true;
}

void QgsKadasMainWidget::saveMapAsImage()
{
  QPair< QString, QString> myFileNameAndFilter = QgisGui::getSaveAsImageName( this, tr( "Choose a file name to save the map image as" ) );
  if ( myFileNameAndFilter.first != "" )
  {
    //save the mapview to the selected file
    mMapCanvas->saveAsImage( myFileNameAndFilter.first, NULL, myFileNameAndFilter.second );
    messageBar()->pushMessage( tr( "Saved map image to %1" ).arg( myFileNameAndFilter.first ), QgsMessageBar::INFO, 5 );
    //statusBar()->showMessage( tr( "Saved map image to %1" ).arg( myFileNameAndFilter.first ) );
  }
}

void QgsKadasMainWidget::kmlExport()
{
  QgsKMLExportDialog d( mMapCanvas->mapSettings().layers() );
  if ( d.exec() == QDialog::Accepted )
  {
    QgsKMLExport kmlExport;
    kmlExport.setLayers( d.selectedLayers() );

    QString fileName = d.saveFile();
    QFile kmlFile( fileName );

    QApplication::setOverrideCursor( Qt::BusyCursor );
    if ( kmlExport.writeToDevice( &kmlFile, mMapCanvas->mapSettings() ) == 0 )
    {
      messageBar()->pushMessage( tr( "KML export completed" ), QgsMessageBar::INFO, 4 );
    }
    else
    {
      messageBar()->pushMessage( tr( "KML export failed" ), QgsMessageBar::CRITICAL, 4 );
    }
    QApplication::restoreOverrideCursor();
  }
}

void QgsKadasMainWidget::zoomToPrevious()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomToPreviousExtent();
  }
}

void QgsKadasMainWidget::zoomToNext()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomToNextExtent();
  }
}

void QgsKadasMainWidget::pin( bool enabled )
{
  if ( !mMapCanvas )
  {
    return;
  }
  mMapCanvas->setMapTool( enabled ? mMapTools.mMapToolPinAnnotation : mNonEditMapTool );
}

void QgsKadasMainWidget::profile( bool enabled )
{
  if ( !mMapCanvas )
  {
    return;
  }
  mMapCanvas->setMapTool( enabled ? mMapTools.mMeasureHeightProfile : mNonEditMapTool );
}

void QgsKadasMainWidget::distance( bool enabled )
{
  if ( !mMapCanvas )
  {
    return;
  }
  mMapCanvas->setMapTool( enabled ? mMapTools.mMeasureDist : mNonEditMapTool );
}

void QgsKadasMainWidget::area( bool enabled )
{
  if ( !mMapCanvas )
  {
    return;
  }
  mMapCanvas->setMapTool( enabled ? mMapTools.mMeasureArea : mNonEditMapTool );
}

void QgsKadasMainWidget::azimuth( bool enabled )
{
  if ( !mMapCanvas )
  {
    return;
  }
  mMapCanvas->setMapTool( enabled ? mMapTools.mMeasureAngle : mNonEditMapTool );
}

void QgsKadasMainWidget::circle( bool enabled )
{
  if ( !mMapCanvas )
  {
    return;
  }
  mMapCanvas->setMapTool( enabled ? mMapTools.mMeasureCircle : mNonEditMapTool );
}

void QgsKadasMainWidget::slope( bool enabled )
{
  if ( enabled )
  {
    mSlopeTool = new QgsVBSSlopeTool( mMapCanvas, this );
    connect( mSlopeTool, SIGNAL( finished() ), this, SLOT( setNonEditMapTool() ) );
  }
  else
  {
    delete mSlopeTool;
    mSlopeTool = 0;
    mMapCanvas->setMapTool( mNonEditMapTool );
  }
}

void QgsKadasMainWidget::hillshade( bool enabled )
{
  if ( enabled )
  {
    mHillshadeTool = new QgsVBSHillshadeTool( mMapCanvas, this );
    connect( mHillshadeTool, SIGNAL( finished() ), this, SLOT( setNonEditMapTool() ) );
  }
  else
  {
    delete mHillshadeTool;
    mHillshadeTool = 0;
    mMapCanvas->setMapTool( mNonEditMapTool );
  }
}

void QgsKadasMainWidget::viewshed( bool enabled )
{
  if ( enabled )
  {
    mViewshedTool = new QgsVBSViewshedTool( mMapCanvas, false, this );
    connect( mViewshedTool, SIGNAL( finished() ), this, SLOT( setNonEditMapTool() ) );
  }
  else
  {
    delete mViewshedTool;
    mViewshedTool = 0;
  }
}

void QgsKadasMainWidget::pinActionToggled( bool enabled )
{
  if ( !enabled )
  {
    mMapCanvas->setMapTool( mNonEditMapTool );
  }
}

void QgsKadasMainWidget::userScale()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomScale( 1.0 / mScaleComboBox->scale() );
  }
}

void QgsKadasMainWidget::showScale( double theScale )
{
  if ( mScaleComboBox )
  {
    mScaleComboBox->setScale( theScale );
  }
}

void QgsKadasMainWidget::setNonEditMapTool()
{
  if ( mMapCanvas )
  {
    mMapCanvas->setMapTool( mNonEditMapTool );
  }
}

bool QgsKadasMainWidget::addProject( const QString& projectFile )
{
  QFileInfo pfi( projectFile );
  //statusBar()->showMessage( tr( "Loading project: %1" ).arg( pfi.fileName() ) );
  qApp->processEvents();

  QApplication::setOverrideCursor( Qt::WaitCursor );

  // close the previous opened project if any
  closeProject();

  if ( ! QgsProject::instance()->read( projectFile ) )
  {
    QApplication::restoreOverrideCursor();
    //statusBar()->clearMessage();

    QMessageBox::critical( this,
                           tr( "Unable to open project" ),
                           QgsProject::instance()->error() );


    mMapCanvas->freeze( false );
    mMapCanvas->refresh();
    return false;
  }

  //setTitleBarText_( *this );
  int  myRedInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorRedPart", 255 );
  int  myGreenInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorGreenPart", 255 );
  int  myBlueInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorBluePart", 255 );
  int  myAlphaInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorAlphaPart", 0 );
  QColor myColor = QColor( myRedInt, myGreenInt, myBlueInt, myAlphaInt );
  mMapCanvas->setCanvasColor( myColor ); //this is fill color before rendering starts
  QgsDebugMsg( "Canvas background color restored..." );

  //load project scales
  bool projectScales = QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" );
  if ( projectScales )
  {
    //mScaleEdit->updateScales( QgsProject::instance()->readListEntry( "Scales", "/ScalesList" ) );
  }

  mMapCanvas->updateScale();
  QgsDebugMsg( "Scale restored..." );

  //setFilterLegendByMapEnabled( QgsProject::instance()->readBoolEntry( "Legend", "filterByMap" ) );

  QSettings settings;

#if 0 //disable project macros for security reasons
  // does the project have any macros?
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    if ( !QgsProject::instance()->readEntry( "Macros", "/pythonCode", QString::null ).isEmpty() )
    {
      int enableMacros = settings.value( "/qgis/enableMacros", 1 ).toInt();
      // 0 = never, 1 = ask, 2 = just for this session, 3 = always

      if ( enableMacros == 3 || enableMacros == 2 )
      {
        enableProjectMacros();
      }
      else if ( enableMacros == 1 ) // ask
      {
        // create the notification widget for macros


        QToolButton *btnEnableMacros = new QToolButton();
        btnEnableMacros->setText( tr( "Enable macros" ) );
        btnEnableMacros->setStyleSheet( "background-color: rgba(255, 255, 255, 0); color: black; text-decoration: underline;" );
        btnEnableMacros->setCursor( Qt::PointingHandCursor );
        btnEnableMacros->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
        connect( btnEnableMacros, SIGNAL( clicked() ), mInfoBar, SLOT( popWidget() ) );
        connect( btnEnableMacros, SIGNAL( clicked() ), this, SLOT( enableProjectMacros() ) );

        QgsMessageBarItem *macroMsg = new QgsMessageBarItem(
          tr( "Security warning" ),
          tr( "project macros have been disabled." ),
          btnEnableMacros,
          QgsMessageBar::WARNING,
          0,
          mInfoBar );
        // display the macros notification widget
        mInfoBar->pushItem( macroMsg );
      }
    }
  }
#endif //0

  //emit projectRead(); // let plug-ins know that we've read in a new
  // project so that they can check any project
  // specific plug-in state

  // add this to the list of recently used project files
  saveRecentProjectPath( projectFile, settings );

  QApplication::restoreOverrideCursor();

  mMapCanvas->freeze( false );
  mMapCanvas->refresh();

  //statusBar()->showMessage( tr( "Project loaded" ), 3000 );
  return true;
}

bool QgsKadasMainWidget::saveDirty()
{
  QString whyDirty = "";
  bool hasUnsavedEdits = false;
  // extra check to see if there are any vector layers with unsaved provider edits
  // to ensure user has opportunity to save any editing
  if ( QgsMapLayerRegistry::instance()->count() > 0 )
  {
    QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
    for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
    {
      QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() );
      if ( !vl )
      {
        continue;
      }

      hasUnsavedEdits = ( vl->isEditable() && vl->isModified() );
      if ( hasUnsavedEdits )
      {
        break;
      }
    }

    if ( hasUnsavedEdits )
    {
      markDirty();
      whyDirty = "<p style='color:darkred;'>";
      whyDirty += tr( "Project has layer(s) in edit mode with unsaved edits, which will NOT be saved!" );
      whyDirty += "</p>";
    }
  }

  QMessageBox::StandardButton answer( QMessageBox::Discard );
  mMapCanvas->freeze( true );

  //QgsDebugMsg(QString("Layer count is %1").arg(mMapCanvas->layerCount()));
  //QgsDebugMsg(QString("Project is %1dirty").arg( QgsProject::instance()->isDirty() ? "" : "not "));
  //QgsDebugMsg(QString("Map canvas is %1dirty").arg(mMapCanvas->isDirty() ? "" : "not "));

  QSettings settings;
  bool askThem = settings.value( "qgis/askToSaveProjectChanges", true ).toBool();

  if ( askThem && QgsProject::instance()->isDirty() && QgsMapLayerRegistry::instance()->count() > 0 )
  {
    // flag project as dirty since dirty state of canvas is reset if "dirty"
    // is based on a zoom or pan
    markDirty();

    // old code: mProjectIsDirtyFlag = true;

    // prompt user to save
    answer = QMessageBox::information( this, tr( "Save?" ),
                                       tr( "Do you want to save the current project? %1" )
                                       .arg( whyDirty ),
                                       QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard,
                                       hasUnsavedEdits ? QMessageBox::Cancel : QMessageBox::Save );
    if ( QMessageBox::Save == answer )
    {
      if ( !fileSave() )
        answer = QMessageBox::Cancel;
    }
  }

  mMapCanvas->freeze( false );

  return answer != QMessageBox::Cancel;
}

void QgsKadasMainWidget::markDirty()
{
  // notify the project that there was a change
  QgsProject::instance()->dirty( true );
}

bool QgsKadasMainWidget::fileSave()
{
  // if we don't have a file name, then obviously we need to get one; note
  // that the project file name is reset to null in fileNew()
  QFileInfo fullPath;

  // we need to remember if this is a new project so that we know to later
  // update the "last project dir" settings; we know it's a new project if
  // the current project file name is empty
  bool isNewProject = false;

  if ( QgsProject::instance()->fileName().isNull() )
  {
    isNewProject = true;

    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

    QString path = QFileDialog::getSaveFileName(
                     this,
                     tr( "Choose a QGIS project file" ),
                     lastUsedDir + "/" + QgsProject::instance()->title(),
                     tr( "QGIS files" ) + " (*.qgs *.QGS)" );
    if ( path.isEmpty() )
      return false;

    fullPath.setFile( path );

    // make sure we have the .qgs extension in the file name
    if ( "qgs" != fullPath.suffix().toLower() )
    {
      fullPath.setFile( fullPath.filePath() + ".qgs" );
    }


    QgsProject::instance()->setFileName( fullPath.filePath() );
  }
  else
  {
    QFileInfo fi( QgsProject::instance()->fileName() );
    if ( fi.exists() && ! fi.isWritable() )
    {
#if 0
      messageBar()->pushMessage( tr( "Insufficient permissions" ),
                                 tr( "The project file is not writable." ),
                                 QgsMessageBar::WARNING );
#endif //0
      return false;
    }
  }

  if ( QgsProject::instance()->write() )
  {
#if 0
    setTitleBarText_( *this ); // update title bar
    statusBar()->showMessage( tr( "Saved project to: %1" ).arg( QgsProject::instance()->fileName() ), 5000 );
#endif //0

    if ( isNewProject )
    {
      // add this to the list of recently used project files
      QSettings settings;
      saveRecentProjectPath( fullPath.filePath(), settings );
    }
  }
  else
  {
    QMessageBox::critical( this,
                           tr( "Unable to save project %1" ).arg( QgsProject::instance()->fileName() ),
                           QgsProject::instance()->error() );
    return false;
  }

#if 0
  // run the saved project macro
  if ( mTrustedMacros )
  {
    QgsPythonRunner::run( "qgis.utils.saveProjectMacro();" );
  }
#endif //0

  return true;
}

void QgsKadasMainWidget::saveRecentProjectPath( QString projectPath, QSettings & settings )
{
  // Get canonical absolute path
  QFileInfo myFileInfo( projectPath );
  projectPath = myFileInfo.absoluteFilePath();

  // If this file is already in the list, remove it
  mRecentProjectPaths.removeAll( projectPath );

  // Prepend this file to the list
  mRecentProjectPaths.prepend( projectPath );

  // Keep the list to 8 items by trimming excess off the bottom
  while ( mRecentProjectPaths.count() > 8 )
  {
    mRecentProjectPaths.pop_back();
  }

  // Persist the list
  settings.setValue( "/UI/recentProjectsList", mRecentProjectPaths );


  // Update menu list of paths
  updateRecentProjectPaths();
}

void QgsKadasMainWidget::updateRecentProjectPaths()
{
  //don't have this currently in kadas widget
}

void QgsKadasMainWidget::closeProject()
{
#if 0
  // unload the project macros before changing anything
  if ( mTrustedMacros )
  {
    QgsPythonRunner::run( "qgis.utils.unloadProjectMacros();" );
  }

  // remove any message widgets from the message bar
  mInfoBar->clearWidgets();

  mTrustedMacros = false;

  setFilterLegendByMapEnabled( false );

  deletePrintComposers();
  removeAnnotationItems();
#endif //0

  // clear out any stuff from project
  mMapCanvas->freeze( true );
  QList<QgsMapCanvasLayer> emptyList;
  mMapCanvas->setLayerSet( emptyList );
  mMapCanvas->clearCache();
  removeAllLayers();
  QgsTemporaryFile::clear();
}

void QgsKadasMainWidget::removeAllLayers()
{
  QgsMapLayerRegistry::instance()->removeAllMapLayers();
}

void QgsKadasMainWidget::readProject( const QDomDocument & )
{
  //projectChanged( doc );

  // force update of canvas, without automatic changes to extent and OTF projections
  bool autoEnableCrsTransform = mLayerTreeCanvasBridge->autoEnableCrsTransform();
  bool autoSetupOnFirstLayer = mLayerTreeCanvasBridge->autoSetupOnFirstLayer();
  mLayerTreeCanvasBridge->setAutoEnableCrsTransform( false );
  mLayerTreeCanvasBridge->setAutoSetupOnFirstLayer( false );

  mLayerTreeCanvasBridge->setCanvasLayers();

  if ( autoEnableCrsTransform )
    mLayerTreeCanvasBridge->setAutoEnableCrsTransform( true );

  if ( autoSetupOnFirstLayer )
    mLayerTreeCanvasBridge->setAutoSetupOnFirstLayer( true );
}

void QgsKadasMainWidget::on_mLayerTreeViewButton_clicked()
{
  if ( !mLayerTreeView )
  {
    return;
  }

  bool visible = mLayerTreeView->isVisible();
  mLayerTreeView->setVisible( !visible );

  if ( !visible )
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/images/kadas/layerbaum_unfolded.png" ) );
    mLayerTreeViewButton->move( mLayerTreeView->size().width() - 5, mLayerTreeViewButton->y() );
  }
  else
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/images/kadas/layerbaum_folded.png" ) );
    mLayerTreeViewButton->move( -2, mLayerTreeViewButton->y() );
  }
}

void QgsKadasMainWidget::on_mZoomInButton_clicked()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomIn();
  }
}

void QgsKadasMainWidget::on_mZoomOutButton_clicked()
{
  if ( mMapCanvas )
  {
    mMapCanvas->zoomOut();
  }
}

void QgsKadasMainWidget::activateDeactivateLayerRelatedActions( QgsMapLayer *layer )
{
#if 0 //disable those actions for now...
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
#endif //0
  mMapCanvas->setMapTool( mNonEditMapTool );
}

void QgsKadasMainWidget::createCanvasTools()
{
  // create tools
  mMapTools.mZoomIn = new QgsMapToolZoom( mMapCanvas, false /* zoomIn */ );
  //mMapTools.mZoomIn->setAction( mActionZoomIn );
  mMapTools.mZoomOut = new QgsMapToolZoom( mMapCanvas, true /* zoomOut */ );
  //mMapTools.mZoomOut->setAction( mActionZoomOut );
  mMapTools.mPan = new QgsMapToolPan( mMapCanvas );
  //mMapTools.mPan->setAction( mActionPan );
  mMapTools.mIdentify = new QgsMapToolIdentifyAction( mMapCanvas );
  //mMapTools.mIdentify->setAction( mActionIdentify );
  connect( mMapTools.mIdentify, SIGNAL( copyToClipboard( QgsFeatureStore & ) ),
           this, SLOT( copyFeatures( QgsFeatureStore & ) ) );
  mMapTools.mFeatureAction = new QgsMapToolFeatureAction( mMapCanvas );
  //mMapTools.mFeatureAction->setAction( mActionFeatureAction );
  mMapTools.mMeasureDist = new QgsMeasureTool( mMapCanvas, false );
  //mMapTools.mMeasureDist->setAction( mActionMeasure );
  mMapTools.mMeasureArea = new QgsMeasureTool( mMapCanvas, true );
  //mMapTools.mMeasureArea->setAction( mActionMeasureArea );
  mMapTools.mMeasureCircle = new QgsMeasureCircleTool( mMapCanvas );
  //mMapTools.mMeasureCircle->setAction( mActionMeasureCircle );
  mMapTools.mMeasureHeightProfile = new QgsMeasureHeightProfileTool( mMapCanvas );
  //mMapTools.mMeasureHeightProfile->setAction( mActionMeasureHeightProfile );
  mMapTools.mMeasureAngle = new QgsMapToolMeasureAngle( mMapCanvas );
  //mMapTools.mMeasureAngle->setAction( mActionMeasureAngle );
  mMapTools.mTextAnnotation = new QgsMapToolTextAnnotation( mMapCanvas );
  //mMapTools.mTextAnnotation->setAction( mActionTextAnnotation );
  mMapTools.mFormAnnotation = new QgsMapToolFormAnnotation( mMapCanvas );
  //mMapTools.mFormAnnotation->setAction( mActionFormAnnotation );
  mMapTools.mHtmlAnnotation = new QgsMapToolHtmlAnnotation( mMapCanvas );
  //mMapTools.mHtmlAnnotation->setAction( mActionHtmlAnnotation );
  mMapTools.mSvgAnnotation = new QgsMapToolSvgAnnotation( mMapCanvas );
  //mMapTools.mSvgAnnotation->setAction( mActionSvgAnnotation );
  mMapTools.mAnnotation = new QgsMapToolAnnotation( mMapCanvas );
  //mMapTools.mAnnotation->setAction( mActionAnnotation );
  mMapTools.mAddFeature = new QgsMapToolAddFeature( mMapCanvas );
  //mMapTools.mAddFeature->setAction( mActionAddFeature );
  mMapTools.mCircularStringCurvePoint = new QgsMapToolCircularStringCurvePoint( dynamic_cast<QgsMapToolAddFeature*>( mMapTools.mAddFeature ), mMapCanvas );
  //mMapTools.mCircularStringCurvePoint->setAction( mActionCircularStringCurvePoint );
  mMapTools.mCircularStringRadius = new QgsMapToolCircularStringRadius( dynamic_cast<QgsMapToolAddFeature*>( mMapTools.mAddFeature ), mMapCanvas );
  //mMapTools.mCircularStringRadius->setAction( mActionCircularStringRadius );
  mMapTools.mMoveFeature = new QgsMapToolMoveFeature( mMapCanvas );
  //mMapTools.mMoveFeature->setAction( mActionMoveFeature );
  mMapTools.mRotateFeature = new QgsMapToolRotateFeature( mMapCanvas );
  //mMapTools.mRotateFeature->setAction( mActionRotateFeature );
  //need at least geos 3.3 for OffsetCurve tool
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
  ((GEOS_VERSION_MAJOR>3) || ((GEOS_VERSION_MAJOR==3) && (GEOS_VERSION_MINOR>=3)))
  mMapTools.mOffsetCurve = new QgsMapToolOffsetCurve( mMapCanvas );
  //mMapTools.mOffsetCurve->setAction( mActionOffsetCurve );
#else
  mAdvancedDigitizeToolBar->removeAction( mActionOffsetCurve );
  mEditMenu->removeAction( mActionOffsetCurve );
  mMapTools.mOffsetCurve = 0;
#endif //GEOS_VERSION
  mMapTools.mReshapeFeatures = new QgsMapToolReshape( mMapCanvas );
  //mMapTools.mReshapeFeatures->setAction( mActionReshapeFeatures );
  mMapTools.mSplitFeatures = new QgsMapToolSplitFeatures( mMapCanvas );
  //mMapTools.mSplitFeatures->setAction( mActionSplitFeatures );
  mMapTools.mSplitParts = new QgsMapToolSplitParts( mMapCanvas );
  //mMapTools.mSplitParts->setAction( mActionSplitParts );
  mMapTools.mSelectFeatures = new QgsMapToolSelectFeatures( mMapCanvas );
  //mMapTools.mSelectFeatures->setAction( mActionSelectFeatures );
  mMapTools.mSelectPolygon = new QgsMapToolSelectPolygon( mMapCanvas );
  //mMapTools.mSelectPolygon->setAction( mActionSelectPolygon );
  mMapTools.mSelectFreehand = new QgsMapToolSelectFreehand( mMapCanvas );
  //mMapTools.mSelectFreehand->setAction( mActionSelectFreehand );
  mMapTools.mSelectRadius = new QgsMapToolSelectRadius( mMapCanvas );
  //mMapTools.mSelectRadius->setAction( mActionSelectRadius );
  mMapTools.mAddRing = new QgsMapToolAddRing( mMapCanvas );
  //mMapTools.mAddRing->setAction( mActionAddRing );
  mMapTools.mFillRing = new QgsMapToolFillRing( mMapCanvas );
  //mMapTools.mFillRing->setAction( mActionFillRing );
  mMapTools.mAddPart = new QgsMapToolAddPart( mMapCanvas );
  //mMapTools.mAddPart->setAction( mActionAddPart );
  mMapTools.mSimplifyFeature = new QgsMapToolSimplify( mMapCanvas );
  //mMapTools.mSimplifyFeature->setAction( mActionSimplifyFeature );
  mMapTools.mDeleteRing = new QgsMapToolDeleteRing( mMapCanvas );
  //mMapTools.mDeleteRing->setAction( mActionDeleteRing );
  mMapTools.mDeletePart = new QgsMapToolDeletePart( mMapCanvas );
  //mMapTools.mDeletePart->setAction( mActionDeletePart );
  mMapTools.mNodeTool = new QgsMapToolNodeTool( mMapCanvas );
  //mMapTools.mNodeTool->setAction( mActionNodeTool );
  mMapTools.mRotatePointSymbolsTool = new QgsMapToolRotatePointSymbols( mMapCanvas );
  //mMapTools.mRotatePointSymbolsTool->setAction( mActionRotatePointSymbols );
  mMapTools.mPinLabels = new QgsMapToolPinLabels( mMapCanvas );
  //mMapTools.mPinLabels->setAction( mActionPinLabels );
  mMapTools.mShowHideLabels = new QgsMapToolShowHideLabels( mMapCanvas );
  //mMapTools.mShowHideLabels->setAction( mActionShowHideLabels );
  mMapTools.mMoveLabel = new QgsMapToolMoveLabel( mMapCanvas );
  //mMapTools.mMoveLabel->setAction( mActionMoveLabel );

  mMapTools.mRotateLabel = new QgsMapToolRotateLabel( mMapCanvas );
  //mMapTools.mRotateLabel->setAction( mActionRotateLabel );
  mMapTools.mChangeLabelProperties = new QgsMapToolChangeLabelProperties( mMapCanvas );
  mMapTools.mMapToolPinAnnotation = new QgsVBSMapToolPinAnnotation( mMapCanvas, mCoordinateDisplayer );
  //mMapTools.mChangeLabelProperties->setAction( mActionChangeLabelProperties );
  //ensure that non edit tool is initialised or we will get crashes in some situations
  mNonEditMapTool = mMapTools.mPan;
}

void QgsKadasMainWidget::layerTreeViewDoubleClicked( const QModelIndex& index )
{
  Q_UNUSED( index )
  QSettings settings;
  switch ( settings.value( "/qgis/legendDoubleClickAction", 0 ).toInt() )
  {
    case 0:
      layerProperties();
      break;
    case 1:
      attributeTable();
      break;
    default:
      break;
  }
}

void QgsKadasMainWidget::layerProperties()
{
  showLayerProperties( activeLayer() );
}

void QgsKadasMainWidget::attributeTable()
{
  QgsVectorLayer *myLayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !myLayer )
  {
    return;
  }

  QgsAttributeTableDialog *mDialog = new QgsAttributeTableDialog( myLayer, this );
  mDialog->show();
  // the dialog will be deleted by itself on close
}

void QgsKadasMainWidget::toggleEditing()
{
  QgsVectorLayer *currentLayer = qobject_cast<QgsVectorLayer*>( activeLayer() );
  if ( currentLayer )
  {
    toggleEditing( currentLayer, true );
  }
  else
  {
    // active although there's no layer active!?
    //mActionToggleEditing->setChecked( false );
  }
}

bool QgsKadasMainWidget::toggleEditing( QgsMapLayer *layer, bool allowCancel )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer )
  {
    return false;
  }

  bool res = true;

  if ( !vlayer->isEditable() && !vlayer->isReadOnly() )
  {
    if ( !( vlayer->dataProvider()->capabilities() & QgsVectorDataProvider::EditingCapabilities ) )
    {
      //mActionToggleEditing->setChecked( false );
      //mActionToggleEditing->setEnabled( false );
      messageBar()->pushMessage( tr( "Start editing failed" ),
                                 tr( "Provider cannot be opened for editing" ),
                                 QgsMessageBar::INFO, messageTimeout() );
      return false;
    }

    vlayer->startEditing();

    QSettings settings;
    QString markerType = settings.value( "/qgis/digitizing/marker_style", "Cross" ).toString();
    bool markSelectedOnly = settings.value( "/qgis/digitizing/marker_only_for_selected", false ).toBool();

    // redraw only if markers will be drawn
    if (( !markSelectedOnly || vlayer->selectedFeatureCount() > 0 ) &&
        ( markerType == "Cross" || markerType == "SemiTransparentCircle" ) )
    {
      vlayer->triggerRepaint();
    }
  }
  else if ( vlayer->isModified() && vlayer->type() == QgsMapLayer::RedliningLayer )
  {
    vlayer->commitChanges();
  }
  else if ( vlayer->isModified() )
  {
    QMessageBox::StandardButtons buttons = QMessageBox::Save | QMessageBox::Discard;
    if ( allowCancel )
      buttons |= QMessageBox::Cancel;

    switch ( QMessageBox::information( 0,
                                       tr( "Stop editing" ),
                                       tr( "Do you want to save the changes to layer %1?" ).arg( vlayer->name() ),
                                       buttons ) )
    {
      case QMessageBox::Cancel:
        res = false;
        break;

      case QMessageBox::Save:
        QApplication::setOverrideCursor( Qt::WaitCursor );

        if ( !vlayer->commitChanges() )
        {
          commitError( vlayer );
          // Leave the in-memory editing state alone,
          // to give the user a chance to enter different values
          // and try the commit again later
          res = false;
        }

        vlayer->triggerRepaint();

        QApplication::restoreOverrideCursor();
        break;

      case QMessageBox::Discard:
        QApplication::setOverrideCursor( Qt::WaitCursor );

        mMapCanvas->freeze( true );
        if ( !vlayer->rollBack() )
        {
          messageBar()->pushMessage( tr( "Error" ),
                                     tr( "Problems during roll back" ),
                                     QgsMessageBar::CRITICAL );
          res = false;
        }
        mMapCanvas->freeze( false );

        vlayer->triggerRepaint();

        QApplication::restoreOverrideCursor();
        break;

      default:
        break;
    }
  }
  else //layer not modified
  {
    mMapCanvas->freeze( true );
    vlayer->rollBack();
    mMapCanvas->freeze( false );
    res = true;
    vlayer->triggerRepaint();
  }

  if ( !res && layer == activeLayer() )
  {
    // while also called when layer sends editingStarted/editingStopped signals,
    // this ensures correct restoring of gui state if toggling was canceled
    // or layer commit/rollback functions failed
    activateDeactivateLayerRelatedActions( layer );
  }

  return res;
}

void QgsKadasMainWidget::removeLayer()
{
  if ( !mLayerTreeView )
  {
    return;
  }

  foreach ( QgsMapLayer * layer, mLayerTreeView->selectedLayers() )
  {
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>( layer );
    if ( vlayer && vlayer->isEditable() && !toggleEditing( vlayer, true ) )
      return;
  }

  QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes( true );

  //validate selection
  if ( selectedNodes.isEmpty() )
  {
    messageBar()->pushMessage( tr( "No legend entries selected" ),
                               tr( "Select the layers and groups you want to remove in the legend." ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  bool promptConfirmation = QSettings().value( "qgis/askToDeleteLayers", true ).toBool();
  //display a warning
  if ( promptConfirmation && QMessageBox::warning( this, tr( "Remove layers and groups" ), tr( "Remove %n legend entries?", "number of legend items to remove", selectedNodes.count() ), QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
  {
    return;
  }

  foreach ( QgsLayerTreeNode* node, selectedNodes )
  {
    QgsLayerTreeGroup* parentGroup = qobject_cast<QgsLayerTreeGroup*>( node->parent() );
    if ( parentGroup )
      parentGroup->removeChildNode( node );
  }

  //showStatusMessage( tr( "%n legend entries removed.", "number of removed legend entries", selectedNodes.count() ) );

  mMapCanvas->refresh();
}

void QgsKadasMainWidget::saveEdits( QgsMapLayer *layer, bool leaveEditable, bool triggerRepaint )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer || !vlayer->isEditable() || !vlayer->isModified() )
    return;

  if ( vlayer == activeLayer() )
    mSaveRollbackInProgress = true;

  if ( !vlayer->commitChanges() )
  {
    mSaveRollbackInProgress = false;
    commitError( vlayer );
  }

  if ( leaveEditable )
  {
    vlayer->startEditing();
  }
  if ( triggerRepaint )
  {
    vlayer->triggerRepaint();
  }
}

void QgsKadasMainWidget::editCopy( QgsMapLayer * layerContainingSelection )
{
  QgsVectorLayer* selectionVectorLayer = qobject_cast<QgsVectorLayer *>( layerContainingSelection ? layerContainingSelection : activeLayer() );
  if ( !selectionVectorLayer )
    return;

  // Test for feature support in this layer
  clipboard()->replaceWithCopyOf( selectionVectorLayer );
}

void QgsKadasMainWidget::deleteSelected( QgsMapLayer *layer, QWidget* parent, bool promptConfirmation )
{
  if ( !layer )
  {
    layer = mLayerTreeView->currentLayer();
  }

  if ( !parent )
  {
    parent = this;
  }

  if ( !layer )
  {
    messageBar()->pushMessage( tr( "No Layer Selected" ),
                               tr( "To delete features, you must select a vector layer in the legend" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer )
  {
    messageBar()->pushMessage( tr( "No Vector Layer Selected" ),
                               tr( "Deleting features only works on vector layers" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  if ( !( vlayer->dataProvider()->capabilities() & QgsVectorDataProvider::DeleteFeatures ) )
  {
    messageBar()->pushMessage( tr( "Provider does not support deletion" ),
                               tr( "Data provider does not support deleting features" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  if ( !vlayer->isEditable() )
  {
    messageBar()->pushMessage( tr( "Layer not editable" ),
                               tr( "The current layer is not editable. Choose 'Start editing' in the digitizing toolbar." ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }

  //validate selection
  int numberOfSelectedFeatures = vlayer->selectedFeaturesIds().size();
  if ( numberOfSelectedFeatures == 0 )
  {
    messageBar()->pushMessage( tr( "No Features Selected" ),
                               tr( "The current layer has no selected features" ),
                               QgsMessageBar::INFO, messageTimeout() );
    return;
  }
  //display a warning
  if ( promptConfirmation && QMessageBox::warning( parent, tr( "Delete features" ), tr( "Delete %n feature(s)?", "number of features to delete", numberOfSelectedFeatures ), QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
  {
    return;
  }

  vlayer->beginEditCommand( tr( "Features deleted" ) );
  int deletedCount = 0;
  if ( !vlayer->deleteSelectedFeatures( &deletedCount ) )
  {
    messageBar()->pushMessage( tr( "Problem deleting features" ),
                               tr( "A problem occured during deletion of %1 feature(s)" ).arg( numberOfSelectedFeatures - deletedCount ),
                               QgsMessageBar::WARNING );
  }
  else
  {
    //showStatusMessage( tr( "%n feature(s) deleted.", "number of features deleted", numberOfSelectedFeatures ) );
  }

  vlayer->endEditCommand();
}

void QgsKadasMainWidget::showLayerProperties( QgsMapLayer *ml )
{
  if ( !ml )
    return;

  if ( !QgsProject::instance()->layerIsEmbedded( ml->id() ).isEmpty() )
  {
    return; //don't show properties of embedded layers
  }

  if ( ml->type() == QgsMapLayer::RasterLayer )
  {
    QgsRasterLayerProperties *rlp = new QgsRasterLayerProperties( ml, mMapCanvas, this );
    rlp->exec();
    delete rlp; // delete since dialog cannot be reused without updating code
  }
  else if ( ml->type() == QgsMapLayer::VectorLayer ) // VECTOR
  {
    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( ml );
    QgsVectorLayerProperties *vlp = new QgsVectorLayerProperties( vlayer, this, this );

    if ( vlp->exec() )
    {
      activateDeactivateLayerRelatedActions( ml );
    }
    delete vlp; // delete since dialog cannot be reused without updating code
  }
  else if ( ml->type() == QgsMapLayer::PluginLayer )
  {
    QgsPluginLayer* pl = qobject_cast<QgsPluginLayer *>( ml );
    if ( !pl )
      return;

    QgsPluginLayerType* plt = QgsPluginLayerRegistry::instance()->pluginLayerType( pl->pluginLayerType() );
    if ( !plt )
      return;

    if ( !plt->showLayerProperties( pl ) )
    {
      /*messageBar()->pushMessage( tr( "Warning" ),
                                 tr( "This layer doesn't have a properties dialog." ),
                                 QgsMessageBar::INFO, messageTimeout() );*/
    }
  }
}

QgsMapLayer* QgsKadasMainWidget::activeLayer()
{
  return mLayerTreeView ? mLayerTreeView->currentLayer() : 0;
}



