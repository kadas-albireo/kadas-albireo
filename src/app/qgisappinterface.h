/***************************************************************************
                          qgisappinterface.h
 Interface class for exposing functions in QgisApp for use by plugins
                             -------------------
  begin                : 2004-02-11
  copyright            : (C) 2004 by Gary E.Sherman
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
#ifndef QGISIFACE_H
#define QGISIFACE_H

#include "qgisinterface.h"
#include "qgsapplegendinterface.h"
#include "qgsapppluginmanagerinterface.h"

class QgisApp;


/** \class QgisAppInterface
 * \brief Interface class to provide access to private methods in QgisApp
 * for use by plugins.
 *
 * Only those functions "exposed" by QgisInterface can be called from within a
 * plugin.
 */
Q_NOWARN_DEPRECATED_PUSH
class APP_EXPORT QgisAppInterface : public QgisInterface
{
    Q_OBJECT

  public:
    /**
     * Constructor.
     * @param qgis Pointer to the QgisApp object
     */
    QgisAppInterface( QgisApp *qgisapp );
    ~QgisAppInterface();

    QgsLegendInterface* legendInterface();

    QgsPluginManagerInterface* pluginManagerInterface();

    QgsLayerTreeView* layerTreeView();

    /* Exposed functions */

    //! Zoom map to full extent
    void zoomFull();
    //! Zoom map to previous extent
    void zoomToPrevious();
    //! Zoom map to next extent
    void zoomToNext();
    //! Zoom to active layer
    void zoomToActiveLayer();

    //! Add a vector layer
    QgsVectorLayer* addVectorLayer( QString vectorLayerPath, QString baseName, QString providerKey );
    //! Add a raster layer given its file name
    QgsRasterLayer* addRasterLayer( QString rasterLayerPath, QString baseName );
    //! Add a WMS layer
    QgsRasterLayer* addRasterLayer( const QString& url, const QString& baseName, const QString& providerKey );

    //! Add a project
    bool addProject( QString theProjectName );
    //! Start a new blank project
    void newProject( bool thePromptToSaveFlag = false );

    //! Get pointer to the active layer (layer selected in the legend)
    QgsMapLayer *activeLayer();

    //! set the active layer (layer selected in the legend)
    bool setActiveLayer( QgsMapLayer *layer );

    //! Add an icon to the plugins toolbar
    int addToolBarIcon( QAction *qAction );
    /**
     * Add a widget to the plugins toolbar.
     * To remove this widget again, call {@link removeToolBarIcon}
     * with the returned QAction.
     *
     * @param widget widget to add. The toolbar will take ownership of this widget
     * @return the QAction you can use to remove this widget from the toolbar
     */
    QAction* addToolBarWidget( QWidget* widget );
    //! Remove an icon (action) from the plugin toolbar
    void removeToolBarIcon( QAction *qAction );
    //! Add an icon to the Raster toolbar
    int addRasterToolBarIcon( QAction *qAction );
    /**
     * Add a widget to the raster toolbar.
     * To remove this widget again, call {@link removeRasterToolBarIcon}
     * with the returned QAction.
     *
     * @param widget widget to add. The toolbar will take ownership of this widget
     * @return the QAction you can use to remove this widget from the toolbar
     */
    QAction* addRasterToolBarWidget( QWidget* widget );
    //! Remove an icon (action) from the Raster toolbar
    void removeRasterToolBarIcon( QAction *qAction );
    //! Add an icon to the Vector toolbar
    int addVectorToolBarIcon( QAction *qAction );
    /**
     * Add a widget to the vector toolbar.
     * To remove this widget again, call {@link removeVectorToolBarIcon}
     * with the returned QAction.
     *
     * @param widget widget to add. The toolbar will take ownership of this widget
     * @return the QAction you can use to remove this widget from the toolbar
     */
    QAction* addVectorToolBarWidget( QWidget* widget );
    //! Remove an icon (action) from the Vector toolbar
    void removeVectorToolBarIcon( QAction *qAction );
    //! Add an icon to the Database toolbar
    int addDatabaseToolBarIcon( QAction *qAction );
    /**
     * Add a widget to the database toolbar.
     * To remove this widget again, call {@link removeDatabaseToolBarIcon}
     * with the returned QAction.
     *
     * @param widget widget to add. The toolbar will take ownership of this widget
     * @return the QAction you can use to remove this widget from the toolbar
     */
    QAction* addDatabaseToolBarWidget( QWidget* widget );
    //! Remove an icon (action) from the Database toolbar
    void removeDatabaseToolBarIcon( QAction *qAction );
    //! Add an icon to the Web toolbar
    int addWebToolBarIcon( QAction *qAction );
    /**
     * Add a widget to the web toolbar.
     * To remove this widget again, call {@link removeWebToolBarIcon}
     * with the returned QAction.
     *
     * @param widget widget to add. The toolbar will take ownership of this widget
     * @return the QAction you can use to remove this widget from the toolbar
     */
    QAction* addWebToolBarWidget( QWidget* widget );
    //! Remove an icon (action) from the Web toolbar
    void removeWebToolBarIcon( QAction *qAction );

    //! Add toolbar with specified name
    QToolBar* addToolBar( QString name );

    //! Add a toolbar
    //! @note added in 2.3
    void addToolBar( QToolBar* toolbar, Qt::ToolBarArea area = Qt::TopToolBarArea );

    /** Open a url in the users browser. By default the QGIS doc directory is used
     * as the base for the URL. To open a URL that is not relative to the installed
     * QGIS documentation, set useQgisDocDirectory to false.
     * @param url URL to open
     * @param useQgisDocDirectory If true, the URL will be formed by concatenating
     * url to the QGIS documentation directory path (<prefix>/share/doc)
     */
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    void openURL( QString url, bool useQgisDocDirectory = true );

    /** Return a pointer to the map canvas used by qgisapp */
    QgsMapCanvas * mapCanvas();

    /** Gives access to main QgisApp object

        Plugins don't need to know about QgisApp, as we pass it as QWidget,
        it can be used for connecting slots and using as widget's parent
    */
    QWidget * mainWindow();

    QgsMessageBar * messageBar();

    // ### QGIS 3: return QgsComposer*, not QgsComposerView*
    QList<QgsComposerView*> activeComposers();

    // ### QGIS 3: return QgsComposer*, not QgsComposerView*
    /** Create a new composer
     * @param title window title for new composer (one will be generated if empty)
     * @return pointer to composer's view
     * @note new composer window will be shown and activated
     */
    QgsComposerView* createNewComposer( QString title = QString( "" ) );

    // ### QGIS 3: return QgsComposer*, not QgsComposerView*
    /** Duplicate an existing parent composer from composer view
     * @param composerView pointer to existing composer view
     * @param title window title for duplicated composer (one will be generated if empty)
     * @return pointer to duplicate composer's view
     * @note dupicate composer window will be hidden until loaded, then shown and activated
     */
    QgsComposerView* duplicateComposer( QgsComposerView* composerView, QString title = QString( "" ) );

    /** Deletes parent composer of composer view, after closing composer window */
    void deleteComposer( QgsComposerView* composerView );

    /** Return changeable options built from settings and/or defaults */
    QMap<QString, QVariant> defaultStyleSheetOptions();

    /** Generate stylesheet
     * @param opts generated default option values, or a changed copy of them */
    void buildStyleSheet( const QMap<QString, QVariant>& opts );

    /** Save changed default option keys/values to user settings */
    void saveStyleSheetOptions( const QMap<QString, QVariant>& opts );

    /** Get reference font for initial qApp (may not be same as QgisApp) */
    QFont defaultStyleSheetFont();

    /** Add action to the plugins menu */
    void addPluginToMenu( QString name, QAction* action );
    /** Remove action from the plugins menu */
    void removePluginMenu( QString name, QAction* action );

    /** Add action to the Database menu */
    void addPluginToDatabaseMenu( QString name, QAction* action );
    /** Remove action from the Database menu */
    void removePluginDatabaseMenu( QString name, QAction* action );

    /** Add action to the Raster menu */
    void addPluginToRasterMenu( QString name, QAction* action );
    /** Remove action from the Raster menu */
    void removePluginRasterMenu( QString name, QAction* action );

    /** Add action to the Vector menu */
    void addPluginToVectorMenu( QString name, QAction* action );
    /** Remove action from the Raster menu */
    void removePluginVectorMenu( QString name, QAction* action );

    /** Add action to the Web menu */
    void addPluginToWebMenu( QString name, QAction* action );
    /** Remove action from the Web menu */
    void removePluginWebMenu( QString name, QAction* action );

    /** Add "add layer" action to the layer menu */
    void insertAddLayerAction( QAction *action );
    /** remove "add layer" action from the layer menu */
    void removeAddLayerAction( QAction *action );

    /** Add a dock widget to the main window */
    void addDockWidget( Qt::DockWidgetArea area, QDockWidget * dockwidget );

    /** Remove specified dock widget from main window (doesn't delete it). */
    void removeDockWidget( QDockWidget * dockwidget );

    /** show layer properties dialog for layer
     * @param l layer to show properties table for
     */
    virtual void showLayerProperties( QgsMapLayer *l );

    /** show layer attribute dialog for layer
     * @param l layer to show attribute table for
     */
    virtual void showAttributeTable( QgsVectorLayer *l );

    /** Add window to Window menu. The action title is the window title
     * and the action should raise, unminimize and activate the window. */
    virtual void addWindow( QAction *action );
    /** Remove window from Window menu. Calling this is necessary only for
     * windows which are hidden rather than deleted when closed. */
    virtual void removeWindow( QAction *action );

    /** Register action to the shortcuts manager so its shortcut can be changed in GUI. */
    virtual bool registerMainWindowAction( QAction* action, QString defaultShortcut );

    /** Unregister a previously registered action. (e.g. when plugin is going to be unloaded. */
    virtual bool unregisterMainWindowAction( QAction* action );

    /** Accessors for inserting items into menus and toolbars.
     * An item can be inserted before any existing action.
     */

    //! Menus
    Q_DECL_DEPRECATED virtual QMenu *fileMenu();
    virtual QMenu *projectMenu();
    virtual QMenu *editMenu();
    virtual QMenu *viewMenu();
    virtual QMenu *layerMenu();
    virtual QMenu *newLayerMenu();
    //! @note added in 2.5
    virtual QMenu *addLayerMenu();
    virtual QMenu *settingsMenu();
    virtual QMenu *pluginMenu();
    virtual QMenu *rasterMenu();
    virtual QMenu *vectorMenu();
    virtual QMenu *databaseMenu();
    virtual QMenu *webMenu();
    virtual QMenu *firstRightStandardMenu();
    virtual QMenu *windowMenu();
    virtual QMenu *helpMenu();

    //! ToolBars
    virtual QToolBar *fileToolBar();
    virtual QToolBar *layerToolBar();
    virtual QToolBar *mapNavToolToolBar();
    virtual QToolBar *digitizeToolBar();
    virtual QToolBar *advancedDigitizeToolBar();
    virtual QToolBar *attributesToolBar();
    virtual QToolBar *pluginToolBar();
    virtual QToolBar *helpToolBar();
    virtual QToolBar *rasterToolBar();
    virtual QToolBar *vectorToolBar();
    virtual QToolBar *databaseToolBar();
    virtual QToolBar *webToolBar();

    //! Project menu actions
    virtual QAction *actionNewProject();
    virtual QAction *actionOpenProject();
    virtual QAction *actionSaveProject();
    virtual QAction *actionSaveProjectAs();
    virtual QAction *actionSaveMapAsImage();
    virtual QAction *actionProjectProperties();
    virtual QAction *actionPrintComposer();
    virtual QAction *actionShowComposerManager();
    virtual QAction *actionExit();

    //! Edit menu actions
    virtual QAction *actionCutFeatures();
    virtual QAction *actionCopyFeatures();
    virtual QAction *actionPasteFeatures();
    virtual QAction *actionAddFeature();
    virtual QAction *actionDeleteSelected();
    virtual QAction *actionMoveFeature();
    virtual QAction *actionSplitFeatures();
    virtual QAction *actionSplitParts();
    virtual QAction *actionAddRing();
    virtual QAction *actionAddPart();
    virtual QAction *actionSimplifyFeature();
    virtual QAction *actionDeleteRing();
    virtual QAction *actionDeletePart();
    virtual QAction *actionNodeTool();

    //! View menu actions
    virtual QAction *actionPan();
    virtual QAction *actionTouch();
    virtual QAction *actionPanToSelected();
    virtual QAction *actionZoomIn();
    virtual QAction *actionZoomOut();
    virtual QAction *actionSelect();
    virtual QAction *actionSelectRectangle();
    virtual QAction *actionSelectPolygon();
    virtual QAction *actionSelectFreehand();
    virtual QAction *actionSelectRadius();
    virtual QAction *actionIdentify();
    virtual QAction *actionFeatureAction();
    virtual QAction *actionMeasure();
    virtual QAction *actionMeasureArea();
    virtual QAction *actionZoomFullExtent();
    virtual QAction *actionZoomToLayer();
    virtual QAction *actionZoomToSelected();
    virtual QAction *actionZoomLast();
    virtual QAction *actionZoomNext();
    virtual QAction *actionZoomActualSize();
    virtual QAction *actionMapTips();
    virtual QAction *actionNewBookmark();
    virtual QAction *actionShowBookmarks();
    virtual QAction *actionDraw();

    //! Layer menu actions
    virtual QAction *actionNewVectorLayer();
    virtual QAction *actionAddOgrLayer();
    virtual QAction *actionAddRasterLayer();
    virtual QAction *actionAddPgLayer();
    virtual QAction *actionAddWmsLayer();
    virtual QAction *actionCopyLayerStyle();
    virtual QAction *actionPasteLayerStyle();
    virtual QAction *actionOpenTable();
    virtual QAction *actionOpenFieldCalculator();
    virtual QAction *actionToggleEditing();
    virtual QAction *actionSaveActiveLayerEdits();
    virtual QAction *actionAllEdits();
    virtual QAction *actionSaveEdits();
    virtual QAction *actionSaveAllEdits();
    virtual QAction *actionRollbackEdits();
    virtual QAction *actionRollbackAllEdits();
    virtual QAction *actionCancelEdits();
    virtual QAction *actionCancelAllEdits();
    virtual QAction *actionLayerSaveAs();
    virtual QAction *actionLayerSelectionSaveAs();
    virtual QAction *actionRemoveLayer();
    virtual QAction *actionDuplicateLayer();
    virtual QAction *actionLayerProperties();
    virtual QAction *actionAddToOverview();
    virtual QAction *actionAddAllToOverview();
    virtual QAction *actionRemoveAllFromOverview();
    virtual QAction *actionHideAllLayers();
    virtual QAction *actionShowAllLayers();
    virtual QAction *actionHideSelectedLayers();
    virtual QAction *actionShowSelectedLayers();

    //! Plugin menu actions
    virtual QAction *actionManagePlugins();
    virtual QAction *actionPluginListSeparator();
    virtual QAction *actionShowPythonDialog();

    //! Settings menu actions
    virtual QAction *actionToggleFullScreen();
    virtual QAction *actionOptions();
    virtual QAction *actionCustomProjection();

    //! Help menu actions
    virtual QAction *actionHelpContents();
    virtual QAction *actionQgisHomePage();
    virtual QAction *actionCheckQgisVersion();
    virtual QAction *actionAbout();

    /**
     * Open feature form
     * returns true when dialog was accepted (if shown modal, true otherwise)
     * @param l vector layer
     * @param f feature to show/modify
     * @param updateFeatureOnly only update the feature update (don't change any attributes of the layer) [UNUSED]
     * @param showModal if true, will wait for the dialog to be executed (only shown otherwise)
     */
    virtual bool openFeatureForm( QgsVectorLayer *l, QgsFeature &f, bool updateFeatureOnly = false, bool showModal = true );

    /**
     * Returns a feature form for a given feature
     *
     * @param layer   The layer for which the dialog will be created
     * @param feature The feature for which the dialog will be created
     *
     * @return A feature form
     */
    virtual QgsAttributeDialog* getFeatureForm( QgsVectorLayer *layer, QgsFeature &feature );

    /**
     * Access the vector layer tools instance.
     * With the help of this you can access methods like addFeature, startEditing
     * or stopEditing while giving the user the appropriate dialogs.
     *
     * @return An instance of the vector layer tools
     */
    virtual QgsVectorLayerTools* vectorLayerTools();

    /** This method is only needed when using a UI form with a custom widget plugin and calling
     * openFeatureForm or getFeatureForm from Python (PyQt4) and you havn't used the info tool first.
     * Python will crash bringing QGIS wtih it
     * if the custom form is not loaded from a C++ method call.
     *
     * This method uses a QTimer to call QUiLoader in order to load the form via C++
     * you only need to call this once after that you can call openFeatureForm/getFeatureForm
     * like normal
     *
     * More information here: http://qt-project.org/forums/viewthread/27098/
     */
    virtual void preloadForm( QString uifile );

    /** Return vector layers in edit mode
     * @param modified whether to return only layers that have been modified
     * @returns list of layers in legend order, or empty list
     */
    virtual QList<QgsMapLayer *> editableLayers( bool modified = false ) const;

    /** Get timeout for timed messages: default of 5 seconds */
    virtual int messageTimeout();

  signals:
    void currentThemeChanged( QString );

  private slots:

    void cacheloadForm( QString uifile );

  private:

    /// QgisInterface aren't copied
    QgisAppInterface( QgisAppInterface const & );

    /// QgisInterface aren't copied
    QgisAppInterface & operator=( QgisAppInterface const & );

    //! Pointer to the QgisApp object
    QgisApp *qgis;

    QTimer *mTimer;

    //! Pointer to the LegendInterface object
    QgsAppLegendInterface legendIface;

    //! Pointer to the PluginManagerInterface object
    QgsAppPluginManagerInterface pluginManagerIface;
};
Q_NOWARN_DEPRECATED_POP

#endif //#define QGISAPPINTERFACE_H
