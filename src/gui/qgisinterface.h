/***************************************************************************
                          qgisinterface.h
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
#ifndef QGISINTERFACE_H
#define QGISINTERFACE_H

class QAction;
class QMenu;
class QToolBar;
class QDockWidget;
class QMainWindow;
class QWidget;

class QgsComposerView;
class QgsMapLayer;
class QgsMapCanvas;
class QgsRasterLayer;
class QgsVectorLayer;
class QgsLegendInterface;
class QgsFeature;
class QgsMessageBar;

#include <QObject>
#include <QPair>
#include <map>

#include <qgis.h>

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4996 )  // was declared deprecated
#endif

/** \ingroup gui
 * QgisInterface
 * Abstract base class defining interfaces exposed by QgisApp and
 * made available to plugins.
 *
 * Only functionality exposed by QgisInterface can be used in plugins.
 * This interface has to be implemented with application specific details.
 *
 * QGIS implements it in QgisAppInterface class, 3rd party applications
 * could provide their own implementation to be able to use plugins.
 */

class GUI_EXPORT QgisInterface : public QObject
{
    Q_OBJECT

  public:

    /** Constructor */
    QgisInterface();

    /** Virtual destructor */
    virtual ~QgisInterface();

    /** Get pointer to legend interface
      \note added in 1.4
     */
    virtual QgsLegendInterface* legendInterface() = 0;


  public slots: // TODO: do these functions really need to be slots?

    //! Zoom to full extent of map layers
    virtual void zoomFull() = 0;

    //! Zoom to previous view extent
    virtual void zoomToPrevious() = 0;

    //! Zoom to next view extent
    virtual void zoomToNext() = 0;

    //! Zoom to extent of the active layer
    virtual void zoomToActiveLayer() = 0;

    //! Add a vector layer
    virtual QgsVectorLayer* addVectorLayer( QString vectorLayerPath, QString baseName, QString providerKey ) = 0;

    //! Add a raster layer given a raster layer file name
    virtual QgsRasterLayer* addRasterLayer( QString rasterLayerPath, QString baseName = QString() ) = 0;

    //! Add a WMS layer
    virtual QgsRasterLayer* addRasterLayer( const QString& url, const QString& layerName, const QString& providerKey ) = 0;

    //! Add a project
    virtual bool addProject( QString theProject ) = 0;
    //! Start a blank project
    virtual void newProject( bool thePromptToSaveFlag = false ) = 0;

    //! Get pointer to the active layer (layer selected in the legend)
    virtual QgsMapLayer *activeLayer() = 0;

    //! Set the active layer (layer gets selected in the legend)
    //! returns true if the layer exists, false otherwise
    //! added in 1.4
    virtual bool setActiveLayer( QgsMapLayer * ) = 0;

    //! Add an icon to the plugins toolbar
    virtual int addToolBarIcon( QAction *qAction ) = 0;

    //! Remove an action (icon) from the plugin toolbar
    virtual void removeToolBarIcon( QAction *qAction ) = 0;

    //! Add an icon to the Raster toolbar
    //! @note added in 2.0
    virtual int addRasterToolBarIcon( QAction *qAction ) = 0;

    //! Remove an action (icon) from the Raster toolbar
    //! @note added in 2.0
    virtual void removeRasterToolBarIcon( QAction *qAction ) = 0;

    //! Add an icon to the Vector toolbar
    //! @note added in 2.0
    virtual int addVectorToolBarIcon( QAction *qAction ) = 0;

    //! Remove an action (icon) from the Vector toolbar
    //! @note added in 2.0
    virtual void removeVectorToolBarIcon( QAction *qAction ) = 0;

    //! Add an icon to the Database toolbar
    //! @note added in 2.0
    virtual int addDatabaseToolBarIcon( QAction *qAction ) = 0;

    //! Remove an action (icon) from the Database toolbar
    //! @note added in 2.0
    virtual void removeDatabaseToolBarIcon( QAction *qAction ) = 0;

    //! Add an icon to the Web toolbar
    //! @note added in 2.0
    virtual int addWebToolBarIcon( QAction *qAction ) = 0;

    //! Remove an action (icon) from the Web toolbar
    //! @note added in 2.0
    virtual void removeWebToolBarIcon( QAction *qAction ) = 0;

    //! Add toolbar with specified name
    virtual QToolBar * addToolBar( QString name ) = 0;

    /** Return a pointer to the map canvas */
    virtual QgsMapCanvas * mapCanvas() = 0;

    /** Return a pointer to the main window (instance of QgisApp in case of QGIS) */
    virtual QWidget * mainWindow() = 0;

    /** Return the message bar of the main app */
    virtual QgsMessageBar * messageBar() = 0;

    /**Return mainwindows / composer views of running composer instances (currently only one)*/
    virtual QList<QgsComposerView*> activeComposers() = 0;

    /** Add action to the plugins menu */
    virtual void addPluginToMenu( QString name, QAction* action ) = 0;

    /** Remove action from the plugins menu */
    virtual void removePluginMenu( QString name, QAction* action ) = 0;

    /** Add "add layer" action to layer menu
     * @note added in 1.7
     */
    virtual void insertAddLayerAction( QAction *action ) = 0;

    /** Remove "add layer" action from layer menu
     * @note added in 1.7
     */
    virtual void removeAddLayerAction( QAction *action ) = 0;

    /** Add action to the Database menu
     * @note added in 1.7
     */
    virtual void addPluginToDatabaseMenu( QString name, QAction* action ) = 0;

    /** Remove action from the Database menu
     * @note added in 1.7
     */
    virtual void removePluginDatabaseMenu( QString name, QAction* action ) = 0;

    /** Add action to the Raster menu
     * @note added in 2.0
     */
    virtual void addPluginToRasterMenu( QString name, QAction* action ) = 0;

    /** Remove action from the Raster menu
     * @note added in 2.0
     */
    virtual void removePluginRasterMenu( QString name, QAction* action ) = 0;

    /** Add action to the Vector menu
     * @note added in 2.0
     */
    virtual void addPluginToVectorMenu( QString name, QAction* action ) = 0;

    /** Remove action from the Vector menu
     * @note added in 2.0
     */
    virtual void removePluginVectorMenu( QString name, QAction* action ) = 0;

    /** Add action to the Web menu
     * @note added in 2.0
     */
    virtual void addPluginToWebMenu( QString name, QAction* action ) = 0;

    /** Remove action from the Web menu
     * @note added in 2.0
     */
    virtual void removePluginWebMenu( QString name, QAction* action ) = 0;


    /** Add a dock widget to the main window */
    virtual void addDockWidget( Qt::DockWidgetArea area, QDockWidget * dockwidget ) = 0;

    /** Remove specified dock widget from main window (doesn't delete it).
     * @note Added in 1.1
     */
    virtual void removeDockWidget( QDockWidget * dockwidget ) = 0;

    /** refresh the legend of a layer
     \deprecated use QgsLegendInterface::refreshLayerSymbology
     */
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual void refreshLegend( QgsMapLayer *l ) = 0;

    /** open layer properties dialog
     \note added in 1.5
     */
    virtual void showLayerProperties( QgsMapLayer *l ) = 0;

    /** open attribute table dialog
     \note added in 1.7
     */
    virtual void showAttributeTable( QgsVectorLayer *l ) = 0;

    /** Add window to Window menu. The action title is the window title
     * and the action should raise, unminimize and activate the window. */
    virtual void addWindow( QAction *action ) = 0;

    /** Remove window from Window menu. Calling this is necessary only for
     * windows which are hidden rather than deleted when closed. */
    virtual void removeWindow( QAction *action ) = 0;

    /** Register action to the shortcuts manager so its shortcut can be changed in GUI
      \note added in 1.2
    */
    virtual bool registerMainWindowAction( QAction* action, QString defaultShortcut ) = 0;

    /** Unregister a previously registered action. (e.g. when plugin is going to be unloaded)
      \note added in 1.2
    */
    virtual bool unregisterMainWindowAction( QAction* action ) = 0;

    // @todo is this deprecated in favour of QgsContextHelp?
    /** Open a url in the users browser. By default the QGIS doc directory is used
     * as the base for the URL. To open a URL that is not relative to the installed
     * QGIS documentation, set useQgisDocDirectory to false.
     * @param url URL to open
     * @param useQgisDocDirectory If true, the URL will be formed by concatenating
     * url to the QGIS documentation directory path (prefix/share/doc)
     */
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual void openURL( QString url, bool useQgisDocDirectory = true ) = 0;


    /** Accessors for inserting items into menus and toolbars.
     * An item can be inserted before any existing action.
     */

    //! Menus
    virtual QMenu *fileMenu() = 0;
    virtual QMenu *editMenu() = 0;
    virtual QMenu *viewMenu() = 0;
    virtual QMenu *layerMenu() = 0;
    virtual QMenu *settingsMenu() = 0;
    virtual QMenu *pluginMenu() = 0;
    virtual QMenu *rasterMenu() = 0;
    /** \note added in 1.7
    */
    virtual QMenu *databaseMenu() = 0;
    /** \note added in 2.0
    */
    virtual QMenu *vectorMenu() = 0;
    /** \note added in 2.0
    */
    virtual QMenu *webMenu() = 0;
    virtual QMenu *firstRightStandardMenu() = 0;
    virtual QMenu *windowMenu() = 0;
    virtual QMenu *helpMenu() = 0;

    //! ToolBars
    virtual QToolBar *fileToolBar() = 0;
    virtual QToolBar *layerToolBar() = 0;
    virtual QToolBar *mapNavToolToolBar() = 0;
    virtual QToolBar *digitizeToolBar() = 0;
    virtual QToolBar *advancedDigitizeToolBar() = 0;  // added in v1.5
    virtual QToolBar *attributesToolBar() = 0;
    virtual QToolBar *pluginToolBar() = 0;
    virtual QToolBar *helpToolBar() = 0;
    /** \note added in 1.7
    */
    virtual QToolBar *rasterToolBar() = 0;
    /** \note added in 2.0
    */
    virtual QToolBar *vectorToolBar() = 0;
    /** \note added in 2.0
    */
    virtual QToolBar *databaseToolBar() = 0;
    /** \note added in 2.0
    */
    virtual QToolBar *webToolBar() = 0;

    //! File menu actions
    virtual QAction *actionNewProject() = 0;
    virtual QAction *actionOpenProject() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionFileSeparator1() = 0;
    virtual QAction *actionSaveProject() = 0;
    virtual QAction *actionSaveProjectAs() = 0;
    virtual QAction *actionSaveMapAsImage() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionFileSeparator2() = 0;
    virtual QAction *actionProjectProperties() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionFileSeparator3() = 0;
    virtual QAction *actionPrintComposer() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionFileSeparator4() = 0;
    virtual QAction *actionExit() = 0;

    //! Edit menu actions
    virtual QAction *actionCutFeatures() = 0;
    virtual QAction *actionCopyFeatures() = 0;
    virtual QAction *actionPasteFeatures() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionEditSeparator1() = 0;
    virtual QAction *actionAddFeature() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionCapturePoint() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionCaptureLine() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionCapturePolygon() = 0;
    virtual QAction *actionDeleteSelected() = 0;
    virtual QAction *actionMoveFeature() = 0;
    virtual QAction *actionSplitFeatures() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionAddVertex() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionDeleteVertex() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionMoveVertex() = 0;
    virtual QAction *actionAddRing() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionAddIsland() = 0;
    virtual QAction *actionAddPart() = 0;
    virtual QAction *actionSimplifyFeature() = 0;
    virtual QAction *actionDeleteRing() = 0;
    virtual QAction *actionDeletePart() = 0;
    virtual QAction *actionNodeTool() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionEditSeparator2() = 0;

    //! View menu actions
    virtual QAction *actionPan() = 0;
    virtual QAction *actionTouch() = 0;
    virtual QAction *actionPanToSelected() = 0;
    virtual QAction *actionZoomIn() = 0;
    virtual QAction *actionZoomOut() = 0;
    virtual QAction *actionSelect() = 0;
    virtual QAction *actionSelectRectangle() = 0;
    virtual QAction *actionSelectPolygon() = 0;
    virtual QAction *actionSelectFreehand() = 0;
    virtual QAction *actionSelectRadius() = 0;
    virtual QAction *actionIdentify() = 0;
    virtual QAction *actionMeasure() = 0;
    virtual QAction *actionMeasureArea() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionViewSeparator1() = 0;
    virtual QAction *actionZoomFullExtent() = 0;
    virtual QAction *actionZoomToLayer() = 0;
    virtual QAction *actionZoomToSelected() = 0;
    virtual QAction *actionZoomLast() = 0;
    virtual QAction *actionZoomActualSize() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionViewSeparator2() = 0;
    virtual QAction *actionMapTips() = 0;
    virtual QAction *actionNewBookmark() = 0;
    virtual QAction *actionShowBookmarks() = 0;
    virtual QAction *actionDraw() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionViewSeparator3() = 0;

    //! Layer menu actions
    virtual QAction *actionNewVectorLayer() = 0;
    virtual QAction *actionAddOgrLayer() = 0;
    virtual QAction *actionAddRasterLayer() = 0;
    virtual QAction *actionAddPgLayer() = 0;
    virtual QAction *actionAddWmsLayer() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionLayerSeparator1() = 0;
    virtual QAction *actionOpenTable() = 0;
    virtual QAction *actionToggleEditing() = 0;
    virtual QAction *actionLayerSaveAs() = 0;
    virtual QAction *actionLayerSelectionSaveAs() = 0;
    virtual QAction *actionRemoveLayer() = 0;
    virtual QAction *actionLayerProperties() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionLayerSeparator2() = 0;
    virtual QAction *actionAddToOverview() = 0;
    virtual QAction *actionAddAllToOverview() = 0;
    virtual QAction *actionRemoveAllFromOverview() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionLayerSeparator3() = 0;
    virtual QAction *actionHideAllLayers() = 0;
    virtual QAction *actionShowAllLayers() = 0;

    //! Plugin menu actions
    virtual QAction *actionManagePlugins() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionPluginSeparator1() = 0;
    virtual QAction *actionPluginListSeparator() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionPluginSeparator2() = 0;
    virtual QAction *actionPluginPythonSeparator() = 0;
    virtual QAction *actionShowPythonDialog() = 0;

    //! Settings menu actions
    virtual QAction *actionToggleFullScreen() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionSettingsSeparator1() = 0;
    virtual QAction *actionOptions() = 0;
    virtual QAction *actionCustomProjection() = 0;

    //! Help menu actions
    virtual QAction *actionHelpContents() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionHelpSeparator1() = 0;
    virtual QAction *actionQgisHomePage() = 0;
    virtual QAction *actionCheckQgisVersion() = 0;
#ifndef Q_MOC_RUN
    Q_DECL_DEPRECATED
#endif
    virtual QAction *actionHelpSeparator2() = 0;
    virtual QAction *actionAbout() = 0;

    //! Open feature form
    // returns true when dialog was accepted
    // @added in 1.6
    virtual bool openFeatureForm( QgsVectorLayer *vlayer, QgsFeature &f, bool updateFeatureOnly = false ) = 0;

  signals:
    /** Emited whenever current (selected) layer changes.
     *  The pointer to layer can be null if no layer is selected
     */
    void currentLayerChanged( QgsMapLayer * layer );
    /**This signal is emitted when a new composer instance has been created
      * @note added in 1.4
      */
    void composerAdded( QgsComposerView* v );
    /**This signal is emitted before a new composer instance is going to be removed
      *  @note added in 1.4
      */
    void composerWillBeRemoved( QgsComposerView* v );
    /**This signal is emitted when the initialization is complete
      * @note added in version 1.6
      */
    void initializationCompleted();
    /** emitted when a project file is successfully read
        @note
        This is useful for plug-ins that store properties with project files.  A
        plug-in can connect to this signal.  When it is emitted, the plug-in
        knows to then check the project properties for any relevant state.

        Added in v1.6
     */
    void projectRead();
    /** emitted when starting an entirely new project
        @note
        This is similar to projectRead(); plug-ins might want to be notified
        that they're in a new project.  Yes, projectRead() could have been
        overloaded to be used in the case of new projects instead.  However,
        it's probably more semantically correct to have an entirely separate
        signal for when this happens.

        Added in v1.6
      */
    void newProjectCreated();

};

#ifdef _MSC_VER
#  pragma warning( pop )
#  pragma warning( disable: 4190 )
#endif

// FIXME: also in core/qgis.h
#ifndef QGISEXTERN
#ifdef WIN32
#  define QGISEXTERN extern "C" __declspec( dllexport )
#else
#  define QGISEXTERN extern "C"
#endif
#endif

#endif //#ifndef QGISINTERFACE_H
