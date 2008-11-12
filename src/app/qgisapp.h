/***************************************************************************
                          qgisapp.h  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id$ */

#ifndef QGISAPP_H
#define QGISAPP_H

class QActionGroup;
class QCheckBox;
class QCursor;
class QFileInfo;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QMenu;
class QPixmap;
class QProgressBar;
class QPushButton;
class QRect;
class QSettings;
class QSplashScreen;
class QStringList;
class QToolButton;
class QTcpSocket;
class QValidator;

class QgisAppInterface;
class QgsClipboard;
class QgsComposer;
class QgsHelpViewer;
class QgsLegend;
class QgsMapCanvas;
class QgsMapLayer;
class QgsMapTip;
class QgsMapTool;
class QgsPoint;
class QgsProviderRegistry;
class QgsPythonDialog;
class QgsPythonUtils;
class QgsRasterLayer;
class QgsRectangle;
class QgsVectorLayer;

#include <QMainWindow>
#include <QToolBar>
#include <QAbstractSocket>
#include <QPointer>

#include "qgsconfig.h"
#include "qgspoint.h"

/*! \class QgisApp
 * \brief Main window for the Qgis application
 */
class QgisApp : public QMainWindow
{
    Q_OBJECT
  public:
    //! Constructor
    QgisApp( QSplashScreen *splash, QWidget * parent = 0, Qt::WFlags fl = Qt::Window );
    //! Destructor
    ~QgisApp();
    /**
     * Add a vector layer to the canvas, returns pointer to it
     */
    QgsVectorLayer* addVectorLayer( QString vectorLayerPath, QString baseName, QString providerKey );

    /** \brief overloaded vesion of the privat addLayer method that takes a list of
     * file names instead of prompting user with a dialog.
     @param enc encoding type for the layer
     @returns true if successfully added layer
     */
    bool addVectorLayers( QStringList const & theLayerQStringList, const QString& enc );

    /** overloaded vesion of the private addRasterLayer()
      Method that takes a list of file names instead of prompting
      user with a dialog.
      @returns true if successfully added layer(s)
      */
    bool addRasterLayers( QStringList const & theLayerQStringList, bool guiWarning = true );

    /** Open a raster layer using the Raster Data Provider.
     *  Note this is included to support WMS layers only at this stage,
     *  GDAL layer support via a Provider is not yet implemented.
     */
    QgsRasterLayer* addRasterLayer( QString const & rasterLayerPath,
                                    QString const & baseName,
                                    QString const & providerKey,
                                    QStringList const & layers,
                                    QStringList const & styles,
                                    QString const & format,
                                    QString const & crs );

    /** open a raster layer for the given file
      @returns false if unable to open a raster layer for rasterFile
      @note
      This is essentially a simplified version of the above
      */
    QgsRasterLayer* addRasterLayer( QString const & rasterFile, QString const & baseName, bool guiWarning = true );

    /** Add a 'pre-made' map layer to the project */
    void addMapLayer( QgsMapLayer *theMapLayer );

    /** Set the extents of the map canvas */
    void setExtent( QgsRectangle theRect );
    //! Remove all layers from the map and legend - reimplements same method from qgisappbase
    void removeAllLayers();
    /** Open a raster or vector file; ignore other files.
      Used to process a commandline argument or OpenDocument AppleEvent.
      @returns true if the file is successfully opened
      */
    bool openLayer( const QString & fileName );
    /** Open the specified project file; prompt to save previous project if necessary.
      Used to process a commandline argument or OpenDocument AppleEvent.
      */
    void openProject( const QString & fileName );
    /** opens a qgis project file
      @returns false if unable to open the project
      */
    bool addProject( QString projectFile );
    //!Overloaded version of the private function with same name that takes the imagename as a parameter
    void saveMapAsImage( QString, QPixmap * );
    /** Get the mapcanvas object from the app */
    QgsMapCanvas * mapCanvas() { return mMapCanvas; };
    //! Set theme (icons)
    void setTheme( QString themeName = "default" );
    //! Setup the toolbar popup menus for a given theme
    void setupToolbarPopups( QString themeName );
    //! Returns a pointer to the internal clipboard
    QgsClipboard * clipboard();

    /** Setup the proxy settings from the QSettings environment.
      * This is not called by default in the constructor. Rather,
      * the application must explicitly call setupProx(). If
      * you write your own application and wish to explicitly
      * set up your own proxy rather, you should e.g.
      *  QNetworkProxy proxy;
      *  proxy.setType(QNetworkProxy::Socks5Proxy);
      *  proxy.setHostName("proxy.example.com");
      *  proxy.setPort(1080);
      *  proxy.setUser("username");
      *  proxy.setPassword("password");
      *  QNetworkProxy::setApplicationProxy(proxy);
      *
      *  (as documented in Qt documentation.
    */
    void setupProxy();

    static QgisApp *instance() { return smInstance; }

    //! Helper to get a theme icon. It will fall back to the
    //default theme if the active theme does not have the required
    //icon.
    static QIcon getThemeIcon( const QString theName );
    //! Helper to get a theme icon as a pixmap. It will fall back to the
    //default theme if the active theme does not have the required
    //icon.
    static QPixmap getThemePixmap( const QString theName );

    /** Add a dock widget to the main window. Overloaded from QMainWindow.
     * After adding the dock widget to the ui (by delegating to the QMainWindow
     * parent class, it will also add it to the View menu list of docks.*/
    void addDockWidget( Qt::DockWidgetArea area, QDockWidget * dockwidget );
    /** Add a toolbar to the main window. Overloaded from QMainWindow.
     * After adding the toolbar to the ui (by delegating to the QMainWindow
     * parent class, it will also add it to the View menu list of toolbars.*/
    QToolBar *addToolBar( QString name );

    /** Add window to Window menu. The action title is the window title
     * and the action should raise, unminimize and activate the window. */
    void addWindow( QAction *action );
    /** Remove window from Window menu. Calling this is necessary only for
     * windows which are hidden rather than deleted when closed. */
    void removeWindow( QAction *action );

    //! Actions to be inserted in menus and toolbars
    QAction *actionNewProject() { return mActionNewProject; }
    QAction *actionOpenProject() { return mActionOpenProject; }
    QAction *actionFileSeparator1() { return mActionFileSeparator1; }
    QAction *actionSaveProject() { return mActionSaveProject; }
    QAction *actionSaveProjectAs() { return mActionSaveProjectAs; }
    QAction *actionSaveMapAsImage() { return mActionSaveMapAsImage; }
    QAction *actionFileSeparator2() { return mActionFileSeparator2; }
    QAction *actionProjectProperties() { return mActionProjectProperties; }
    QAction *actionFileSeparator3() { return mActionFileSeparator3; }
    QAction *actionPrintComposer() { return mActionPrintComposer; }
    QAction *actionFileSeparator4() { return mActionFileSeparator4; }
    QAction *actionExit() { return mActionExit; }

    QAction *actionCutFeatures() { return mActionCutFeatures; }
    QAction *actionCopyFeatures() { return mActionCopyFeatures; }
    QAction *actionPasteFeatures() { return mActionPasteFeatures; }
    QAction *actionEditSeparator1() { return mActionEditSeparator1; }
    QAction *actionCapturePoint() { return mActionCapturePoint; }
    QAction *actionCaptureLine() { return mActionCaptureLine; }
    QAction *actionCapturePologon() { return mActionCapturePolygon; }
    QAction *actionDeleteSelected() { return mActionDeleteSelected; }
    QAction *actionMoveFeature() { return mActionMoveFeature; }
    QAction *actionSplitFeatures() { return mActionSplitFeatures; }
    QAction *actionAddVertex() { return mActionAddVertex; }
    QAction *actionDelerteVertex() { return mActionDeleteVertex; }
    QAction *actioMoveVertex() { return mActionMoveVertex; }
    QAction *actionAddRing() { return mActionAddRing; }
    QAction *actionAddIsland() { return mActionAddIsland; }
    QAction *actionEditSeparator2() { return mActionEditSeparator2; }

    QAction *actionPan() { return mActionPan; }
    QAction *actionZoomIn() { return mActionZoomIn; }
    QAction *actionZoomOut() { return mActionZoomOut; }
    QAction *actionSelect() { return mActionSelect; }
    QAction *actionIdentify() { return mActionIdentify; }
    QAction *actionMeasure() { return mActionMeasure; }
    QAction *actionMeasureArea() { return mActionMeasureArea; }
    QAction *actionViewSeparator1() { return mActionViewSeparator1; }
    QAction *actionZoomFullExtent() { return mActionZoomFullExtent; }
    QAction *actionZoomToLayer() { return mActionZoomToLayer; }
    QAction *actionZoomToSelected() { return mActionZoomToSelected; }
    QAction *actionZoomLast() { return mActionZoomLast; }
    QAction *actionZoomActualSize() { return mActionZoomActualSize; }
    QAction *actionViewSeparator2() { return mActionViewSeparator2; }
    QAction *actionMapTips() { return mActionMapTips; }
    QAction *actionNewBookmark() { return mActionNewBookmark; }
    QAction *actionShowBookmarks() { return mActionShowBookmarks; }
    QAction *actionDraw() { return mActionDraw; }
    QAction *actionViewSeparator3() { return mActionViewSeparator3; }

    QAction *actionNewVectorLayer() { return mActionNewVectorLayer; }
    QAction *actionAddOgrLayer() { return mActionAddOgrLayer; }
    QAction *actionAddRasterLayer() { return mActionAddRasterLayer; }
    QAction *actionAddPgLayer() { return mActionAddPgLayer; }
    QAction *actionAddWmsLayer() { return mActionAddWmsLayer; }
    QAction *actionLayerSeparator1() { return mActionLayerSeparator1; }
    QAction *actionOpenTable() { return mActionOpenTable; }
    QAction *actionToggleEditing() { return mActionToggleEditing; }
    QAction *actionLayerSaveAs() { return mActionLayerSaveAs; }
    QAction *actionLayerSelectionSaveAs() { return mActionLayerSelectionSaveAs; }
    QAction *actionRemoveLayer() { return mActionRemoveLayer; }
    QAction *actionLayerProperties() { return mActionLayerProperties; }
    QAction *actionLayerSeparator2() { return mActionLayerSeparator2; }
    QAction *actionAddToOverview() { return mActionAddToOverview; }
    QAction *actionAddAllToOverview() { return mActionAddAllToOverview; }
    QAction *actionRemoveAllFromOverview() { return mActionRemoveAllFromOverview; }
    QAction *actionLayerSeparator3() { return mActionLayerSeparator3; }
    QAction *actionHideAllLayers() { return mActionHideAllLayers; }
    QAction *actionShowAllLayers() { return mActionShowAllLayers; }

    QAction *actionManagePlugins() { return mActionManagePlugins; }
    QAction *actionPluginSeparator1() { return mActionPluginSeparator1; }
    QAction *actionPluginListSeparator() { return mActionPluginSeparator1; }
    QAction *actionPluginSeparator2() { return mActionPluginSeparator2; }
    QAction *actionPluginPythonSeparator() { return mActionPluginSeparator2; }
    QAction *actionShowPythonDialog() { return mActionShowPythonDialog; }

    QAction *actionToggleFullScreen() { return mActionToggleFullScreen; }
    QAction *actionSettingsSeparator1() { return mActionSettingsSeparator1; }
    QAction *actionOptions() { return mActionOptions; }
    QAction *actionCustomProjection() { return mActionCustomProjection; }

#ifdef Q_WS_MAC
    QAction *actionWindowMinimize() { return mActionWindowMinimize; }
    QAction *actionWindowZoom() { return mActionWindowZoom; }
    QAction *actionWindowSeparator1() { return mActionWindowSeparator1; }
    QAction *actionWindowAllToFront() { return mActionWindowAllToFront; }
    QAction *actionWindowSeparator2() { return mActionWindowSeparator2; }
#endif

    QAction *actionHelpContents() { return mActionHelpContents; }
    QAction *actionHelpSeparator1() { return mActionHelpSeparator1; }
    QAction *actionQgisHomePage() { return mActionQgisHomePage; }
    QAction *actionCheckQgisVersion() { return mActionCheckQgisVersion; }
    QAction *actionHelpSeparator2() { return mActionHelpSeparator2; }
    QAction *actionAbout() { return mActionAbout; }

    //! Menus
    QMenu *fileMenu() { return mFileMenu; }
    QMenu *editMenu() { return mEditMenu; }
    QMenu *viewMenu() { return mViewMenu; }
    QMenu *layerMenu() { return mLayerMenu; }
    QMenu *settingsMenu() { return mSettingsMenu; }
    QMenu *pluginMenu() { return mPluginMenu; }
#ifdef Q_WS_MAC
    QMenu *firstRightStandardMenu() { return mWindowMenu; }
    QMenu *windowMenu() { return mWindowMenu; }
#else
    QMenu *firstRightStandardMenu() { return mHelpMenu; }
    QMenu *windowMenu() { return NULL; }
#endif
    QMenu *helpMenu() { return mHelpMenu; }

    //! Toolbars
    /** Get a reference to a toolbar. Mainly intended
    *   to be used by plugins that want to specifically add
    *   an icon into the file toolbar for consistency e.g.
    *   addWFS and GPS plugins.
    */
    QToolBar *fileToolBar() { return mFileToolBar; }
    QToolBar *layerToolBar() { return mLayerToolBar; }
    QToolBar *mapNavToolToolBar() { return mMapNavToolBar; }
    QToolBar *digitizeToolBar() { return mDigitizeToolBar; }
    QToolBar *attributesToolBar() { return mAttributesToolBar; }
    QToolBar *pluginToolBar() { return mPluginToolBar; }
    QToolBar *helpToolBar() { return mHelpToolBar; }

  public slots:
    //! Zoom to full extent
    void zoomFull();
    //! Zoom to the previous extent
    void zoomToPrevious();
    //! Zoom to selected features
    void zoomToSelected();

    //! cuts selected features on the active layer to the clipboard
    /**
       \param layerContainingSelection  The layer that the selection will be taken from
                                        (defaults to the active layer on the legend)
     */
    void editCut( QgsMapLayer * layerContainingSelection = 0 );
    //! copies selected features on the active layer to the clipboard
    /**
       \param layerContainingSelection  The layer that the selection will be taken from
                                        (defaults to the active layer on the legend)
     */
    void editCopy( QgsMapLayer * layerContainingSelection = 0 );
    //! copies features on the clipboard to the active layer
    /**
       \param destinationLayer  The layer that the clipboard will be pasted to
                                (defaults to the active layer on the legend)
     */
    void editPaste( QgsMapLayer * destinationLayer = 0 );

  protected:

    //! Handle state changes (WindowTitleChange)
    virtual void changeEvent( QEvent * event );
    //! Have some control over closing of the application
    virtual void closeEvent( QCloseEvent * event );

    virtual void dragEnterEvent( QDragEnterEvent * event );
    virtual void dropEvent( QDropEvent * event );

    //! reimplements widget keyPress event so we can check if cancel was pressed
    virtual void keyPressEvent( QKeyEvent * event );

  private slots:
    //! About QGis
    void about();
    //! Add a raster layer to the map (will prompt user for file name using dlg )
    void addRasterLayer();
    //#ifdef HAVE_POSTGRESQL
    //! Add a databaselayer to the map
    void addDatabaseLayer();
    //#endif
    /** toggles whether the current selected layer is in overview or not */
    void isInOverview();
    //! Slot to show the map coordinate position of the mouse cursor
    void showMouseCoordinate( QgsPoint & );
    //! Slot to show current map scale;
    void showScale( double theScale );
    //! Slot to handle user scale input;
    void userScale();
    //! Remove a layer from the map and legend
    void removeLayer();
    //! zoom to extent of layer
    void zoomToLayerExtent();
    //! zoom to actual size of raster layer
    void zoomActualSize();
    //! plugin manager
    void showPluginManager();
    //! load python support if possible
    void loadPythonSupport();
    //! Find the QMenu with the given name (ie the user visible text on the menu item)
    QMenu* getPluginMenu( QString menuName );
    //! Add the action to the submenu with the given name under the plugin menu
    void addPluginToMenu( QString name, QAction* action );
    //! Remove the action to the submenu with the given name under the plugin menu
    void removePluginMenu( QString name, QAction* action );
    //! Add an icon to the plugin toolbar
    int addPluginToolBarIcon( QAction * qAction );
    //! Remove an icon from the plugin toolbar
    void removePluginToolBarIcon( QAction *qAction );
    //! Save window state
    void saveWindowState();
    //! Restore the window and toolbar state
    void restoreWindowState();
    //! Save project. Returns true if the user selected a file to save to, false if not.
    bool fileSave();
    //! Save project as
    void fileSaveAs();
    //! Open the project file corresponding to the
    //! text)= of the given action.
    void openProject( QAction *action );
    //! Save the map view as an image - user is prompted for image name using a dialog
    void saveMapAsImage();
    //! Open a project
    void fileOpen();
    //! Create a new project
    void fileNew();
    //! As above but allows forcing without prompt
    void fileNew( bool thePromptToSaveFlag );
    //! Create a new empty vector layer
    void newVectorLayer();
    //! Print the current map view frame
    void filePrint();
    //! Add all loaded layers into the overview - overides qgisappbase method
    void addAllToOverview();
    //! Remove all loaded layers from the overview - overides qgisappbase method
    void removeAllFromOverview();
    //reimplements method from base (gui) class
    void hideAllLayers();
    //reimplements method from base (gui) class
    void showAllLayers();
    // TODO: remove exportMapServer declaration once the mapserver export plugin is complete
    // and tested
    /*
    //! Export current view as a mapserver map file
    void exportMapServer();
    */
    //! Return pointer to the active layer
    QgsMapLayer *activeLayer();
    //! Open the help contents in a browser
    void helpContents();
    //! Open the QGIS homepage in users browser
    void helpQgisHomePage();
    //! Open a url in the users configured browser
    void openURL( QString url, bool useQgisDocDirectory = true );
    //! Check qgis version against the qgis version server
    void checkQgisVersion();
    //!Invoke the custom projection dialog
    void customProjection();
    //! options dialog slot
    void options();
    //! Whats-this help slot
    void whatsThis();
    void socketConnected();
    void socketConnectionClosed();
    void socketReadyRead();
    void socketError( QAbstractSocket::SocketError e );
    //! Set project properties, including map untis
    void projectProperties();
    //! Open project properties dialog and show the projections tab
    void projectPropertiesProjections();
    /*  void urlData(); */
    //! Show the spatial bookmarks dialog
    void showBookmarks();
    //! Create a new spatial bookmark
    void newBookmark();
    //! activates the capture point tool
    void capturePoint();
    //! activates the capture line tool
    void captureLine();
    //! activates the capture polygon tool
    void capturePolygon();
    /**Deletes the selected attributes for the currently selected vector layer*/
    void deleteSelected();
    //! activates the move feature tool
    void moveFeature();
    //! activates the split features tool
    void splitFeatures();
    //! activates the add vertex tool
    void addVertex();
    //! activates the move vertex tool
    void moveVertex();
    //! activates the delete vertex tool
    void deleteVertex();
    //! activates the add ring tool
    void addRing();
    //! activates the add island tool
    void addIsland();

    //! activates the selection tool
    void select();
    //! refresh map canvas
    void refreshMapCanvas();
    //! returns pointer to map legend
    QgsLegend *legend() { return mMapLegend; }

    //! starts/stops editing mode of the current layer
    void toggleEditing();

    //! map tool changed
    void mapToolChanged( QgsMapTool *tool );

    /** Activates or deactivates actions depending on the current maplayer type.
    Is called from the legend when the current legend item has changed*/
    void activateDeactivateLayerRelatedActions( QgsMapLayer* layer );

    void showProgress( int theProgress, int theTotalSteps );
    void extentsViewToggled( bool theFlag );
    void showExtents();
    void showStatusMessage( QString theMessage );
    void updateMouseCoordinatePrecision();
    void hasCrsTransformEnabled( bool theFlag );
    void destinationSrsChanged();
    //    void debugHook();
    //! Add a vector layer to the map
    void addVectorLayer();
    //! Exit Qgis
    void fileExit();
    //! Add a WMS layer to the map
    void addWmsLayer();
    //! Set map tool to Zoom out
    void zoomOut();
    //! Set map tool to Zoom in
    void zoomIn();
    //! Set map tool to pan
    void pan();
    //! Identify feature(s) on the currently selected layer
    void identify();
    //! Measure distance
    void measure();
    //! Measure area
    void measureArea();

    //! show the attribute table for the currently selected layer
    void attributeTable();

    //! starts/stops editing mode of a layer
    void toggleEditing( QgsMapLayer *layer );

    //! save current vector layer
    void saveAsShapefile();
    void saveSelectionAsShapefile();

    //! open the properties dialog for the currently selected layer
    void layerProperties();

    //! show python console
    void showPythonDialog();

    //! Shows a warning when an old project file is read.
    void oldProjectVersionWarning( QString );

    //! Toggle map tips on/off
    void toggleMapTips();

    //! Show the map tip
    void showMapTip();

    //! Toggle full screen mode
    void toggleFullScreen();

    //! Set minimized mode of active window
    void showActiveWindowMinimized();

    //! Toggle maximized mode of active window
    void toggleActiveWindowMaximized();

    //! Raise, unminimize and activate this window
    void activate();

    //! Bring forward all open windows
    void bringAllToFront();

    //! Stops rendering of the main map
    void stopRendering();

  signals:
    /** emitted when a key is pressed and we want non widget sublasses to be able
      to pick up on this (e.g. maplayer) */
    void keyPressed( QKeyEvent *e );

    /** emitted when a project file is successfully read
      @note
      This is useful for plug-ins that store properties with project files.  A
      plug-in can connect to this signal.  When it is emitted, the plug-in
      knows to then check the project properties for any relevant state.
      */
    void projectRead();
    /** emitted when starting an entirely new project
      @note
      This is similar to projectRead(); plug-ins might want to be notified
      that they're in a new project.  Yes, projectRead() could have been
      overloaded to be used in the case of new projects instead.  However,
      it's probably more semantically correct to have an entirely separate
      signal for when this happens.
      */
    void newProject();

    //! emitted when a new bookmark is added
    void bookmarkAdded();

  private:
    /** Add a raster layer to the map (passed in as a ptr).
     * It won't force a refresh.
     */
    bool addRasterLayer( QgsRasterLayer * theRasterLayer );
    //@todo We should move these next two into vector layer class
    /** This helper checks to see whether the file name appears to be a valid vector file name */
    bool isValidVectorFileName( QString theFileNameQString );
    /** Overloaded version of the above function provided for convenience that takes a qstring pointer */
    bool isValidVectorFileName( QString * theFileNameQString );
    /** add this file to the recently opened/saved projects list
     *  pass settings by reference since creating more than one
     * instance simultaneously results in data loss.
     */
    void saveRecentProjectPath( QString projectPath, QSettings & settings );
    //! Update file menu with the current list of recently accessed projects
    void updateRecentProjectPaths();
    //! Read Well Known Binary stream from PostGIS
    //void readWKB(const char *, QStringList tables);
    //! shows the paste-transformations dialog
    void pasteTransformations();
    //! check to see if file is dirty and if so, prompt the user th save it
    bool saveDirty();

    /// QgisApp aren't copyable
    QgisApp( QgisApp const & );
    /// QgisApp aren't copyable
    QgisApp & operator=( QgisApp const & );

    void readSettings();
    void writeSettings();
    void createActions();
    void createActionGroups();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void setupConnections();
    void createLegend();
    void createOverview();
    void createCanvas();
    bool createDB();
    void createMapTips();

    // toolbars ---------------------------------------
    QToolBar *mFileToolBar;
    QToolBar *mLayerToolBar;
    QToolBar *mMapNavToolBar;
    QToolBar *mDigitizeToolBar;
    QToolBar *mAttributesToolBar;
    QToolBar *mPluginToolBar;
    QToolBar *mHelpToolBar;

    // actions for menus and toolbars -----------------

    QAction *mActionNewProject;
    QAction *mActionOpenProject;
    QAction *mActionFileSeparator1;
    QAction *mActionSaveProject;
    QAction *mActionSaveProjectAs;
    QAction *mActionSaveMapAsImage;
    QAction *mActionFileSeparator2;
    QAction *mActionProjectProperties;
    QAction *mActionFileSeparator3;
    QAction *mActionPrintComposer;
    QAction *mActionFileSeparator4;
    QAction *mActionExit;

    QAction *mActionCutFeatures;
    QAction *mActionCopyFeatures;
    QAction *mActionPasteFeatures;
    QAction *mActionEditSeparator1;
    QAction *mActionCapturePoint;
    QAction *mActionCaptureLine;
    QAction *mActionCapturePolygon;
    QAction *mActionDeleteSelected;
    QAction *mActionMoveFeature;
    QAction *mActionSplitFeatures;
    QAction *mActionAddVertex;
    QAction *mActionDeleteVertex;
    QAction *mActionMoveVertex;
    QAction *mActionAddRing;
    QAction *mActionAddIsland;
    QAction *mActionEditSeparator2;

    QAction *mActionPan;
    QAction *mActionZoomIn;
    QAction *mActionZoomOut;
    QAction *mActionSelect;
    QAction *mActionIdentify;
    QAction *mActionMeasure;
    QAction *mActionMeasureArea;
    QAction *mActionViewSeparator1;
    QAction *mActionZoomFullExtent;
    QAction *mActionZoomToLayer;
    QAction *mActionZoomToSelected;
    QAction *mActionZoomLast;
    QAction *mActionZoomActualSize;
    QAction *mActionViewSeparator2;
    QAction *mActionMapTips;
    QAction *mActionNewBookmark;
    QAction *mActionShowBookmarks;
    QAction *mActionDraw;
    QAction *mActionViewSeparator3;

    QAction *mActionNewVectorLayer;
    QAction *mActionAddOgrLayer;
    QAction *mActionAddRasterLayer;
    QAction *mActionAddPgLayer;
    QAction *mActionAddWmsLayer;
    QAction *mActionLayerSeparator1;
    QAction *mActionOpenTable;
    QAction *mActionToggleEditing;
    QAction *mActionLayerSaveAs;
    QAction *mActionLayerSelectionSaveAs;
    QAction *mActionRemoveLayer;
    QAction *mActionLayerProperties;
    QAction *mActionLayerSeparator2;
    QAction *mActionAddToOverview;
    QAction *mActionAddAllToOverview;
    QAction *mActionRemoveAllFromOverview;
    QAction *mActionLayerSeparator3;
    QAction *mActionHideAllLayers;
    QAction *mActionShowAllLayers;

    QAction *mActionManagePlugins;
    QAction *mActionPluginSeparator1;
    QAction *mActionPluginSeparator2;
    QAction *mActionShowPythonDialog;

    QAction *mActionToggleFullScreen;
    QAction *mActionSettingsSeparator1;
    QAction *mActionOptions;
    QAction *mActionCustomProjection;

#ifdef Q_WS_MAC
    QAction *mActionWindowMinimize;
    QAction *mActionWindowZoom;
    QAction *mActionWindowSeparator1;
    QAction *mActionWindowAllToFront;
    QAction *mActionWindowSeparator2;
    QActionGroup *mWindowActions;
#endif

    QAction *mActionHelpContents;
    QAction *mActionHelpSeparator1;
    QAction *mActionQgisHomePage;
    QAction *mActionCheckQgisVersion;
    QAction *mActionHelpSeparator2;
    QAction *mActionAbout;

    // action groups ----------------------------------
    QActionGroup *mMapToolGroup;

    // menus ------------------------------------------
    QMenu *mFileMenu;
    QMenu *mEditMenu;
    QMenu *mRecentProjectsMenu;
    QMenu *mViewMenu;
    QMenu *mPanelMenu;
    QMenu *mToolbarMenu;
    QMenu *mLayerMenu;
    QMenu *mSettingsMenu;
#ifdef Q_WS_MAC
    QMenu *mWindowMenu;
#endif
    QMenu *mHelpMenu;

    // docks ------------------------------------------
    QDockWidget *mLegendDock;
    QDockWidget *mOverviewDock;

#ifdef Q_WS_MAC
    //! Window menu action to select this window
    QAction *mWindowAction;
#endif

    class Tools
    {
      public:
        QgsMapTool* mZoomIn;
        QgsMapTool* mZoomOut;
        QgsMapTool* mPan;
        QgsMapTool* mIdentify;
        QgsMapTool* mMeasureDist;
        QgsMapTool* mMeasureArea;
        QgsMapTool* mCapturePoint;
        QgsMapTool* mCaptureLine;
        QgsMapTool* mCapturePolygon;
        QgsMapTool* mMoveFeature;
        QgsMapTool* mSplitFeatures;
        QgsMapTool* mSelect;
        QgsMapTool* mVertexAdd;
        QgsMapTool* mVertexMove;
        QgsMapTool* mVertexDelete;
        QgsMapTool* mAddRing;
        QgsMapTool* mAddIsland;
    } mMapTools;

    QgsMapTool *mNonEditMapTool;

    //! Widget that will live on the statusbar to display "scale 1:"
    QLabel * mScaleLabel;
    //! Widget that will live on the statusbar to display scale value
    QLineEdit * mScaleEdit;
    //! The validator for the mScaleEdit
    QValidator * mScaleEditValidator;
    //! Widget that will live in the statusbar to display coords
    QLabel * mCoordsLabel;
    //! Widget that will live in the statusbar to show progress of operations
    QProgressBar * mProgressBar;
    //! Widget used to suppress rendering
    QCheckBox * mRenderSuppressionCBox;
    //! A toggle to switch between mouse coords and view extents display
    QToolButton * mToggleExtentsViewButton;
    //! Button used to stop rendering
    QToolButton* mStopRenderButton;
    //! Widget in status bar used to show status of on the fly projection
    QToolButton * mOnTheFlyProjectionStatusButton;
    //! Popup menu
    QMenu * mPopupMenu;
    //! Top level plugin menu
    QMenu *mPluginMenu;
    //! Popup menu for the map overview tools
    QMenu *toolPopupOverviews;
    //! Popup menu for the display tools
    QMenu *toolPopupDisplay;
    //! Popup menu for the capture tools
    QMenu *toolPopupCapture;
    //! Map canvas
    QgsMapCanvas *mMapCanvas;
    //! Table of contents (legend) for the map
    QgsLegend *mMapLegend;
    //! Cursor for the overview map
    QCursor *mOverviewMapCursor;
    //! scale factor
    double mScaleFactor;
    //! Current map window extent in real-world coordinates
    QRect *mMapWindow;
    //! The previously selected non zoom map tool.
    int mPreviousNonZoomMapTool;
    //QCursor *mCursorZoomIn; //doesnt seem to be used anymore (TS)
    QString mStartupPath;
    //! full path name of the current map file (if it has been saved or loaded)
    QString mFullPathName;

    //! interface to QgisApp for plugins
    QgisAppInterface *mQgisInterface;
    friend class QgisAppInterface;

    QTcpSocket *mSocket;
    QString mVersionMessage;
    QSplashScreen *mSplash;
    //! help viewer
    QgsHelpViewer *mHelpViewer;
    //! list of recently opened/saved project files
    QStringList mRecentProjectPaths;
    //! Map composer
    QgsComposer *mComposer;
    //! How to determine the number of decimal places used to
    //! display the mouse position
    bool mMousePrecisionAutomatic;
    //! The number of decimal places to use if not automatic
    unsigned int mMousePrecisionDecimalPlaces;
    /** QGIS-internal vector feature clipboard */
    QgsClipboard* mInternalClipboard;
    //! Flag to indicate how the project properties dialog was summoned
    bool mShowProjectionTab;
    /** String containing supporting vector file formats
      Suitable for a QFileDialog file filter.  Build in ctor.
      */
    QString mVectorFileFilter;
    /** String containing supporting raster file formats
      Suitable for a QFileDialog file filter.  Build in ctor.
      */
    QString mRasterFileFilter;

    /** Timer for map tips
     */
    QTimer *mpMapTipsTimer;

    /** Point of last mouse position in map coordinates (used with MapTips)
     */
    QgsPoint mLastMapPosition;

    /* Maptip object
     */
    QgsMapTip *  mpMaptip;

    // Flag to indicate if maptips are on or off
    bool mMapTipsVisible;

    //!flag to indicate whether we are in fullscreen mode or not
    bool mFullScreenMode;

    //!flag to indicate that the previous screen mode was 'maximised'
    bool mPrevScreenModeMaximized;

    QgsPythonDialog* mPythonConsole;
    QgsPythonUtils* mPythonUtils;

    static QgisApp *smInstance;
};

#endif
