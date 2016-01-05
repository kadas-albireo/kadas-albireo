/***************************************************************************
                          qgsclassicapp.h  -  description
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

#ifndef QGSCLASSICAPP_H
#define QGSCLASSICAPP_H

#include "qgisapp.h"

#include "ui_qgsclassicapp.h"

class APP_EXPORT QgsClassicApp : public QgisApp, private Ui::MainWindow
{
    Q_OBJECT
  public:

    static QgsClassicApp* instance() { return qobject_cast<QgsClassicApp*>( smInstance ); }

    QgsClassicApp( QSplashScreen *splash, bool restorePlugins = true, QWidget *parent = 0, Qt::WindowFlags fl = Qt::Window );
    QgsClassicApp();
    ~QgsClassicApp();

    QgsMapCanvas *mapCanvas() const override { return mMapCanvas; }
    QgsLayerTreeView* layerTreeView() const override { return mLayerTreeView; }
    QgsMessageBar *messageBar() const override { return mInfoBar; }
    void setTheme( QString themeName ) override;

    /** Add a toolbar to the main window. Overloaded from QMainWindow.
     * After adding the toolbar to the ui (by delegating to the QMainWindow
     * parent class, it will also add it to the View menu list of toolbars.*/
    QToolBar *addToolBar( QString name );

    /** Add a toolbar to the main window. Overloaded from QMainWindow.
     * After adding the toolbar to the ui (by delegating to the QMainWindow
     * parent class, it will also add it to the View menu list of toolbars.
     * @note added in 2.3
     */
    void addToolBar( QToolBar *toolBar, Qt::ToolBarArea area = Qt::TopToolBarArea );

    //! Actions to be inserted in menus and toolbars
    QAction *actionNewProject() { return mActionNewProject; }
    QAction *actionOpenProject() { return mActionOpenProject; }
    QAction *actionSaveProject() { return mActionSaveProject; }
    QAction *actionSaveProjectAs() { return mActionSaveProjectAs; }
    QAction *actionSaveMapAsImage() { return mActionSaveMapAsImage; }
    QAction *actionProjectProperties() { return mActionProjectProperties; }
    QAction *actionShowComposerManager() { return mActionShowComposerManager; }
    QAction *actionNewPrintComposer() { return mActionNewPrintComposer; }
    QAction *actionExit() { return mActionExit; }

    QAction *actionCutFeatures() { return mActionCutFeatures; }
    QAction *actionCopyFeatures() { return mActionCopyFeatures; }
    QAction *actionPasteFeatures() { return mActionPasteFeatures; }
    QAction *actionPasteAsNewVector() { return mActionPasteAsNewVector; }
    QAction *actionPasteAsNewMemoryVector() { return mActionPasteAsNewMemoryVector; }
    QAction *actionDeleteSelected() { return mActionDeleteSelected; }
    QAction *actionAddFeature() { return mActionAddFeature; }
    QAction *actionMoveFeature() { return mActionMoveFeature; }
    QAction *actionRotateFeature() { return mActionRotateFeature;}
    QAction *actionSplitFeatures() { return mActionSplitFeatures; }
    QAction *actionSplitParts() { return mActionSplitParts; }
    QAction *actionAddRing() { return mActionAddRing; }
    QAction *actionFillRing() { return mActionFillRing; }
    QAction *actionAddPart() { return mActionAddPart; }
    QAction *actionSimplifyFeature() { return mActionSimplifyFeature; }
    QAction *actionDeleteRing() { return mActionDeleteRing; }
    QAction *actionDeletePart() { return mActionDeletePart; }
    QAction *actionNodeTool() { return mActionNodeTool; }
    QAction *actionSnappingOptions() { return mActionSnappingOptions; }
    QAction *actionOffsetCurve() { return mActionOffsetCurve; }
    QAction *actionPan() { return mActionPan; }
    QAction *actionPanToSelected() { return mActionPanToSelected; }
    QAction *actionZoomIn() { return mActionZoomIn; }
    QAction *actionZoomOut() { return mActionZoomOut; }
    QAction *actionSelect() { return mActionSelectFeatures; }
    QAction *actionSelectRectangle() { return mActionSelectFeatures; }
    QAction *actionSelectPolygon() { return mActionSelectPolygon; }
    QAction *actionSelectFreehand() { return mActionSelectFreehand; }
    QAction *actionSelectRadius() { return mActionSelectRadius; }
    QAction *actionIdentify() { return mActionIdentify; }
    QAction *actionFeatureAction() { return mActionFeatureAction; }
    QAction *actionMeasure() { return mActionMeasure; }
    QAction *actionMeasureArea() { return mActionMeasureArea; }
    QAction *actionMeasureCircle() { return mActionMeasureCircle; }
    QAction *actionMeasureHeightProfile() { return mActionMeasureHeightProfile; }
    QAction *actionZoomFullExtent() { return mActionZoomFullExtent; }
    QAction *actionZoomToLayer() { return mActionZoomToLayer; }
    QAction *actionZoomToSelected() { return mActionZoomToSelected; }
    QAction *actionZoomLast() { return mActionZoomLast; }
    QAction *actionZoomNext() { return mActionZoomNext; }
    QAction *actionZoomActualSize() { return mActionZoomActualSize; }
    QAction *actionMapTips() { return mActionMapTips; }
    QAction *actionNewBookmark() { return mActionNewBookmark; }
    QAction *actionShowBookmarks() { return mActionShowBookmarks; }
    QAction *actionDraw() { return mActionDraw; }

    QAction *actionNewVectorLayer() { return mActionNewVectorLayer; }
    QAction *actionNewSpatialLiteLayer() { return mActionNewSpatiaLiteLayer; }
    QAction *actionEmbedLayers() { return mActionEmbedLayers; }
    QAction *actionAddOgrLayer() { return mActionAddOgrLayer; }
    QAction *actionAddRasterLayer() { return mActionAddRasterLayer; }
    QAction *actionAddPgLayer() { return mActionAddPgLayer; }
    QAction *actionAddSpatiaLiteLayer() { return mActionAddSpatiaLiteLayer; }
    QAction *actionAddWmsLayer() { return mActionAddWmsLayer; }
    QAction *actionAddWcsLayer() { return mActionAddWcsLayer; }
    QAction *actionAddWfsLayer() { return mActionAddWfsLayer; }
    QAction *actionAddAfsLayer() { return mActionAddAfsLayer; }
    QAction *actionAddAmsLayer() { return mActionAddAmsLayer; }
    QAction *actionCopyLayerStyle() { return mActionCopyStyle; }
    QAction *actionPasteLayerStyle() { return mActionPasteStyle; }
    QAction *actionOpenTable() { return mActionOpenTable; }
    QAction *actionOpenFieldCalculator() { return mActionOpenFieldCalc; }
    QAction *actionToggleEditing() { return mActionToggleEditing; }
    QAction *actionSaveActiveLayerEdits() { return mActionSaveLayerEdits; }
    QAction *actionAllEdits() { return mActionAllEdits; }
    QAction *actionSaveEdits() { return mActionSaveEdits; }
    QAction *actionSaveAllEdits() { return mActionSaveAllEdits; }
    QAction *actionRollbackEdits() { return mActionRollbackEdits; }
    QAction *actionRollbackAllEdits() { return mActionRollbackAllEdits; }
    QAction *actionCancelEdits() { return mActionCancelEdits; }
    QAction *actionCancelAllEdits() { return mActionCancelAllEdits; }
    QAction *actionLayerSaveAs() { return mActionLayerSaveAs; }
    QAction *actionRemoveLayer() { return mActionRemoveLayer; }
    QAction *actionDuplicateLayer() { return mActionDuplicateLayer; }
    /** @note added in 2.4 */
    QAction *actionSetLayerScaleVisibility() { return mActionSetLayerScaleVisibility; }
    QAction *actionSetLayerCRS() { return mActionSetLayerCRS; }
    QAction *actionSetProjectCRSFromLayer() { return mActionSetProjectCRSFromLayer; }
    QAction *actionLayerProperties() { return mActionLayerProperties; }
    QAction *actionLayerSubsetString() { return mActionLayerSubsetString; }
    QAction *actionAddToOverview() { return mActionAddToOverview; }
    QAction *actionAddAllToOverview() { return mActionAddAllToOverview; }
    QAction *actionRemoveAllFromOverview() { return mActionRemoveAllFromOverview; }
    QAction *actionHideAllLayers() { return mActionHideAllLayers; }
    QAction *actionShowAllLayers() { return mActionShowAllLayers; }
    QAction *actionHideSelectedLayers() { return mActionHideSelectedLayers; }
    QAction *actionShowSelectedLayers() { return mActionShowSelectedLayers; }

    QAction *actionManagePlugins() { return mActionManagePlugins; }
    QAction *actionPluginListSeparator() { return mActionPluginSeparator1; }
    QAction *actionPluginPythonSeparator() { return mActionPluginSeparator2; }
    QAction *actionShowPythonDialog() { return mActionShowPythonDialog; }

    QAction *actionToggleFullScreen() { return mActionToggleFullScreen; }
    QAction *actionOptions() { return mActionOptions; }
    QAction *actionCustomProjection() { return mActionCustomProjection; }
    QAction *actionConfigureShortcuts() { return mActionConfigureShortcuts; }

#ifdef Q_OS_MAC
    QAction *actionWindowMinimize() { return mActionWindowMinimize; }
    QAction *actionWindowZoom() { return mActionWindowZoom; }
    QAction *actionWindowAllToFront() { return mActionWindowAllToFront; }
#endif

    QAction *actionHelpContents() { return mActionHelpContents; }
    QAction *actionHelpAPI() { return mActionHelpAPI; }
    QAction *actionQgisHomePage() { return mActionQgisHomePage; }
    QAction *actionCheckQgisVersion() { return mActionCheckQgisVersion; }
    QAction *actionAbout() { return mActionAbout; }
    QAction *actionSponsors() { return mActionSponsors; }

    QAction *actionShowPinnedLabels() { return mActionShowPinnedLabels; }

    //! Menus
    QMenu *projectMenu() const override { return mProjectMenu; }
    QMenu *editMenu() const override { return mEditMenu; }
    QMenu *viewMenu() const override { return mViewMenu; }
    QMenu *layerMenu() const override { return mLayerMenu; }
    QMenu *newLayerMenu() const override { return mNewLayerMenu; }
    //! @note added in 2.5
    QMenu *addLayerMenu() const override { return mAddLayerMenu; }
    QMenu *settingsMenu() const override { return mSettingsMenu; }
    QMenu *pluginMenu() const override { return mPluginMenu; }
    QMenu *databaseMenu() const override { return mDatabaseMenu; }
    QMenu *rasterMenu() const override { return mRasterMenu; }
    QMenu *vectorMenu() const override { return mVectorMenu; }
    QMenu *webMenu() const override { return mWebMenu; }
#ifdef Q_OS_MAC
    QMenu *firstRightStandardMenu() const override { return mWindowMenu; }
    QMenu *windowMenu() const override { return mWindowMenu; }
#else
    QMenu *firstRightStandardMenu() const override { return mHelpMenu; }
    QMenu *windowMenu() const override { return NULL; }
#endif
    QMenu *helpMenu() const override { return mHelpMenu; }
    QMenu *printComposersMenu() const override { return mPrintComposersMenu;}
    QMenu *panelMenu() const override { return mPanelMenu; }
    QMenu *recentProjectsMenu() const override { return mRecentProjectsMenu; }
    QMenu *projectFromTemplateMenu() const override { return mProjectFromTemplateMenu; }
    QMenu *featureActionMenu() const override { return mFeatureActionMenu; }

    //! Toolbars
    /** Get a reference to a toolbar. Mainly intended to be used by plugins. */
    QToolBar *fileToolBar() { return mFileToolBar; }
    QToolBar *layerToolBar() { return mLayerToolBar; }
    QToolBar *mapNavToolToolBar() { return mMapNavToolBar; }
    QToolBar *digitizeToolBar() { return mDigitizeToolBar; }
    QToolBar *advancedDigitizeToolBar() { return mAdvancedDigitizeToolBar; }
    QToolBar *attributesToolBar() { return mAttributesToolBar; }
    QToolBar *pluginToolBar() { return mPluginToolBar; }
    QToolBar *helpToolBar() { return mHelpToolBar; }
    QToolBar *rasterToolBar() { return mRasterToolBar; }
    QToolBar *vectorToolBar() { return mVectorToolBar; }
    QToolBar *databaseToolBar() { return mDatabaseToolBar; }
    QToolBar *webToolBar() { return mWebToolBar; }

    //! Find the QMenu with the given name within plugin menu (ie the user visible text on the menu item)
    QMenu* getPluginMenu( QString menuName );
    //! Add the action to the submenu with the given name under the plugin menu
    void addPluginToMenu( QString name, QAction *action );
    //! Remove the action to the submenu with the given name under the plugin menu
    void removePluginMenu( QString name, QAction *action );
    //! Find the QMenu with the given name within the Database menu (ie the user visible text on the menu item)
    QMenu *getDatabaseMenu( QString menuName );
    //! Add the action to the submenu with the given name under the Database menu
    void addPluginToDatabaseMenu( QString name, QAction *action );
    //! Remove the action to the submenu with the given name under the Database menu
    void removePluginDatabaseMenu( QString name, QAction *action );
    //! Find the QMenu with the given name within the Raster menu (ie the user visible text on the menu item)
    QMenu *getRasterMenu( QString menuName );
    //! Add the action to the submenu with the given name under the Raster menu
    void addPluginToRasterMenu( QString name, QAction *action );
    //! Remove the action to the submenu with the given name under the Raster menu
    void removePluginRasterMenu( QString name, QAction *action );
    //! Find the QMenu with the given name within the Vector menu (ie the user visible text on the menu item)
    QMenu *getVectorMenu( QString menuName );
    //! Add the action to the submenu with the given name under the Vector menu
    void addPluginToVectorMenu( QString name, QAction *action );
    //! Remove the action to the submenu with the given name under the Vector menu
    void removePluginVectorMenu( QString name, QAction *action );
    //! Find the QMenu with the given name within the Web menu (ie the user visible text on the menu item)
    QMenu *getWebMenu( QString menuName );
    //! Add the action to the submenu with the given name under the Web menu
    void addPluginToWebMenu( QString name, QAction *action );
    //! Remove the action to the submenu with the given name under the Web menu
    void removePluginWebMenu( QString name, QAction *action );
    //! Add "add layer" action to layer menu
    void insertAddLayerAction( QAction *action );
    //! Remove "add layer" action to layer menu
    void removeAddLayerAction( QAction *action );
    //! Add an icon to the plugin toolbar
    int addPluginToolBarIcon( QAction *qAction );
    /**
     * Add a widget to the plugins toolbar.
     * To remove this widget again, call {@link removeToolBarIcon}
     * with the returned QAction.
     *
     * @param widget widget to add. The toolbar will take ownership of this widget
     * @return the QAction you can use to remove this widget from the toolbar
     */
    QAction* addPluginToolBarWidget( QWidget *widget );
    //! Remove an icon from the plugin toolbar
    void removePluginToolBarIcon( QAction *qAction );
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
    QAction *addRasterToolBarWidget( QWidget *widget );
    //! Remove an icon from the Raster toolbar
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
    QAction *addVectorToolBarWidget( QWidget *widget );
    //! Remove an icon from the Vector toolbar
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
    QAction *addDatabaseToolBarWidget( QWidget *widget );
    //! Remove an icon from the Database toolbar
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
    QAction *addWebToolBarWidget( QWidget *widget );
    //! Remove an icon from the Web toolbar
    void removeWebToolBarIcon( QAction *qAction );

  private slots:
    //! update default action of toolbutton
    void toolButtonActionTriggered( QAction * );
    void extentsViewToggled( bool theFlag );
    void showMouseCoordinate( const QgsPoint & );
    void showScale( double theScale );
    void userScale();
    void userCenter();
    void userRotation();
    void showExtents();
    void showRotation();
    void showProgress( int theProgress, int theTotalSteps );
    void updateCRSStatusBar();
    void updateUndoActions();
    void legendLayerSelectionChanged();
    void refreshActionFeatureAction();
    void clipboardDataChanged();
    void activateDeactivateLayerRelatedActions( QgsMapLayer *layer );
    void updateLayerModifiedActions() override;

  private:
    //! Map canvas
    QgsMapCanvas *mMapCanvas;
    //! Table of contents (legend) for the map
    QgsLayerTreeView *mLayerTreeView;
    //! A bar to display warnings in a non-blocker manner
    QgsMessageBar *mInfoBar;

    QAction *mActionPluginSeparator1;
    QAction *mActionPluginSeparator2;
    QAction *mActionRasterSeparator;

    QActionGroup *mMapToolGroup;
    QActionGroup *mPreviewGroup;

#ifdef Q_OS_MAC
    QAction *mActionWindowMinimize;
    QAction *mActionWindowZoom;
    QAction *mActionWindowSeparator1;
    QAction *mActionWindowAllToFront;
    QAction *mActionWindowSeparator2;
    QActionGroup *mWindowActions;
#endif

#ifdef Q_OS_MAC
    QMenu *mWindowMenu;
#endif
    QMenu *mPanelMenu;
    QMenu *mToolbarMenu;

    //! Widget that will live on the statusbar to display "scale 1:"
    QLabel *mScaleLabel;
    //! Widget that will live on the statusbar to display scale value
    QgsScaleComboBox *mScaleEdit;
    //! The validator for the mScaleEdit
    QValidator * mScaleEditValidator;
    //! Widget that will live on the statusbar to display "Coordinate / Extent"
    QLabel *mCoordsLabel;
    //! Widget that will live in the statusbar to display and edit coords
    QLineEdit *mCoordsEdit;
    //! The validator for the mCoordsEdit
    QValidator *mCoordsEditValidator;
    //! Widget that will live on the statusbar to display "Rotation"
    QLabel *mRotationLabel;
    //! Widget that will live in the statusbar to display and edit rotation
    QgsDoubleSpinBox *mRotationEdit;
    //! The validator for the mCoordsEdit
    QValidator *mRotationEditValidator;
    //! Widget that will live in the statusbar to show progress of operations
    QProgressBar *mProgressBar;
    //! Widget used to suppress rendering
    QCheckBox *mRenderSuppressionCBox;
    //! A toggle to switch between mouse coords and view extents display
    QToolButton *mToggleExtentsViewButton;
    //! Widget in status bar used to show current project CRS
    QLabel *mOnTheFlyProjectionStatusLabel;
    //! Widget in status bar used to show status of on the fly projection
    QToolButton *mOnTheFlyProjectionStatusButton;
    QToolButton *mMessageButton;
    //! Menu that contains the list of actions of the selected vector layer
    QMenu *mFeatureActionMenu;
    //! Popup menu
    QMenu *mPopupMenu;
    //! Top level database menu
    QMenu *mDatabaseMenu;
    //! Top level web menu
    QMenu *mWebMenu;
    //! Popup menu for the map overview tools
    QMenu *mToolPopupOverviews;
    //! Popup menu for the display tools
    QMenu *mToolPopupDisplay;

    void createActions();
    void createActionGroups();
    void setupMenus();
    void createToolBars();
    void createStatusBar();
    void initLayerTreeView();
    void setToolActions();
};

#endif // QGSCLASSICAPP_H

