/***************************************************************************
                         qgscomposer.h
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
#ifndef QGSCOMPOSER_H
#define QGSCOMPOSER_H

#include "ui_qgscomposerbase.h"
#include "qgscomposition.h"
#include <QDockWidget>

class QgsComposerArrow;
class QgsComposerFrame;
class QgsComposerHtml;
class QgsComposerLabel;
class QgsComposerLegend;
class QgsComposerMap;
class QgsComposerPicture;
class QgsComposerPictureWidget;
class QgsComposerRuler;
class QgsComposerScaleBar;
class QgsComposerShape;
class QgsComposerAttributeTable;
class QgsComposerAttributeTableV2;
class QgsComposerView;
class QgsMapLayerAction;

class QComboBox;
class QLabel;

/** \ingroup MapComposer
 * \brief A gui for composing a printable map.
 */
class QgsComposer: public QMainWindow, private Ui::QgsComposerBase
{
    Q_OBJECT

  public:
    QgsComposer( QgsComposerView *view );
    ~QgsComposer();

    void setIconSizes( int size );

    //! Zoom to full extent of the paper
    void zoomFull();

    //! Return current composition
    QgsComposition* composition() { return mComposition; }

  protected:
    virtual void closeEvent( QCloseEvent * ) override;

  signals:
    //! Is emitted every time the view zoom has changed
    void zoomLevelChanged();

    //! Is emitted when the atlas preview feature changes
    void atlasPreviewFeatureChanged();

  public slots:
    //! Zoom to full extent of the paper
    void on_mActionZoomAll_triggered();

    //! Zoom in
    void on_mActionZoomIn_triggered();

    //! Zoom out
    void on_mActionZoomOut_triggered();

    //! Zoom actual
    void on_mActionZoomActual_triggered();

    //! Refresh view
    void on_mActionRefreshView_triggered();

    //! Print the composition
    void on_mActionPrint_triggered();

    //! Page Setup for composition
    void on_mActionPageSetup_triggered();

    //! Print as image
    void on_mActionExportAsImage_triggered();

    //! Print as SVG
    void on_mActionExportAsSVG_triggered();

    //! Print as PDF
    void on_mActionExportAsPDF_triggered();

    //! Select item
    void on_mActionSelectMoveItem_triggered();

    //! Add arrow
    void on_mActionAddArrow_triggered();

    //! Add new map
    void on_mActionAddNewMap_triggered();

    //! Add new legend
    void on_mActionAddNewLegend_triggered();

    //! Add new label
    void on_mActionAddNewLabel_triggered();

    //! Add new scalebar
    void on_mActionAddNewScalebar_triggered();

    //! Add new picture
    void on_mActionAddImage_triggered();

    void on_mActionAddRectangle_triggered();

    void on_mActionAddTriangle_triggered();

    void on_mActionAddEllipse_triggered();

    //! Add attribute table
    void on_mActionAddAttributeTable_triggered();

    void on_mActionAddHtml_triggered();

    //! Save parent project
    void on_mActionSaveProject_triggered();

    //! Create new composer
    void on_mActionNewComposer_triggered();

    //! Duplicate current composer
    void on_mActionDuplicateComposer_triggered();

    //! Show composer manager

    void on_mActionComposerManager_triggered();

    //! Save composer as template
    void on_mActionSaveAsTemplate_triggered();

    void on_mActionLoadFromTemplate_triggered();

    //! Set tool to move item content
    void on_mActionMoveItemContent_triggered();

    //! Set tool to move item content
    void on_mActionPan_triggered();

    //! Set tool to mouse zoom
    void on_mActionMouseZoom_triggered();

    //! Group selected items
    void on_mActionGroupItems_triggered();

    //! Cut item(s)
    void actionCutTriggered();

    //! Copy item(s)
    void actionCopyTriggered();

    //! Paste item(s)
    void actionPasteTriggered();

    //! Paste in place item(s)
    void on_mActionPasteInPlace_triggered();

    //! Delete selected item(s)
    void on_mActionDeleteSelection_triggered();

    //! Select all items
    void on_mActionSelectAll_triggered();

    //! Deselect all items
    void on_mActionDeselectAll_triggered();

    //! Invert selection
    void on_mActionInvertSelection_triggered();

    //! Ungroup selected item group
    void on_mActionUngroupItems_triggered();

    //! Lock selected items
    void on_mActionLockItems_triggered();

    //! Unlock all items
    void on_mActionUnlockAll_triggered();

    //! Select next item below
    void on_mActionSelectNextAbove_triggered();

    //! Select next item above
    void on_mActionSelectNextBelow_triggered();

    //! Move selected items one position up
    void on_mActionRaiseItems_triggered();

    //!Move selected items one position down
    void on_mActionLowerItems_triggered();

    //!Move selected items to top
    void on_mActionMoveItemsToTop_triggered();

    //!Move selected items to bottom
    void on_mActionMoveItemsToBottom_triggered();

    //!Align selected composer items left
    void on_mActionAlignLeft_triggered();

    //!Align selected composere items horizontally centered
    void on_mActionAlignHCenter_triggered();

    //!Align selected composer items right
    void on_mActionAlignRight_triggered();

    //!Align selected composer items to top
    void on_mActionAlignTop_triggered();

    //!Align selected composer items vertically centered
    void on_mActionAlignVCenter_triggered();

    //!Align selected composer items to bottom
    void on_mActionAlignBottom_triggered();

    //!Show/hide grid
    void on_mActionShowGrid_triggered( bool checked );

    //!Enable or disable snap items to grid
    void on_mActionSnapGrid_triggered( bool checked );

    //!Show/hide guides
    void on_mActionShowGuides_triggered( bool checked );

    //!Enable or disable snap items to guides
    void on_mActionSnapGuides_triggered( bool checked );

    //!Enable or disable smart guides
    void on_mActionSmartGuides_triggered( bool checked );

    //!Show/hide bounding boxes
    void on_mActionShowBoxes_triggered( bool checked );

    //!Show/hide rulers
    void toggleRulers( bool checked );

    //!Clear guides
    void on_mActionClearGuides_triggered();

    //!Show options dialog
    void on_mActionOptions_triggered();

    //!Toggle atlas preview
    void on_mActionAtlasPreview_triggered( bool checked );

    //!Next atlas feature
    void on_mActionAtlasNext_triggered();

    //!Previous atlas feature
    void on_mActionAtlasPrev_triggered();

    //!First atlas feature
    void on_mActionAtlasFirst_triggered();

    //!Last atlas feature
    void on_mActionAtlasLast_triggered();

    //! Print the atlas
    void on_mActionPrintAtlas_triggered();

    //! Print atlas as image
    void on_mActionExportAtlasAsImage_triggered();

    //! Print atlas as SVG
    void on_mActionExportAtlasAsSVG_triggered();

    //! Print atlas as PDF
    void on_mActionExportAtlasAsPDF_triggered();

    //! Atlas settings
    void on_mActionAtlasSettings_triggered();

    //! Toggle full screen mode
    void on_mActionToggleFullScreen_triggered();

    //! Toggle panels
    void on_mActionHidePanels_triggered();

    void createComposerItemWidget( QgsComposerItem* item );

    /**Removes item from the item/widget map and deletes the configuration widget. Does not delete the item itself*/
    void deleteItem( QgsComposerItem* item );

    /**Shows the configuration widget for a composer item*/
    void showItemOptions( QgsComposerItem* i );


    void setSelectionTool();

    //! Raise, unminimize and activate this window
    void activate();

    //! Updates cursor position in status bar
    void updateStatusCursorPos( QPointF position );

    //! Updates zoom level in status bar
    void updateStatusZoom();

    void statusZoomCombo_currentIndexChanged( int index );

    void statusZoomCombo_zoomEntered();

  private:
    //! True if a composer map contains a WMS layer
    bool containsWMSLayer() const;

    //! True if a composer contains advanced effects, such as blend modes
    bool containsAdvancedEffects() const;

    //! Displays a warning because of possible min/max size in WMS
    void showWMSPrintingWarning();

    //! Displays a warning because of incompatibility between blend modes and QPrinter
    void showAdvancedEffectsWarning();

    //! Prints either the whole atlas or just the current feature, depending on mode
    void printComposition( QgsComposition::OutputMode mode );

    //! Exports either either the whole atlas or just the current feature as an image, depending on mode
    void exportCompositionAsImage( QgsComposition::OutputMode mode );

    //! Exports either either the whole atlas or just the current feature as an SVG, depending on mode
    void exportCompositionAsSVG( QgsComposition::OutputMode mode );

    //! Exports either either the whole atlas or just the current feature as a PDF, depending on mode
    void exportCompositionAsPDF( QgsComposition::OutputMode mode );

    //! Updates the "set as atlas feature" map layer action, removing it if atlas is disabled
    void updateAtlasMapLayerAction( bool atlasEnabled );

    /**Labels in status bar which shows current mouse position*/
    QLabel* mStatusCursorXLabel;
    QLabel* mStatusCursorYLabel;
    QLabel* mStatusCursorPageLabel;
    /**Combobox in status bar which shows/adjusts current zoom level*/
    QComboBox* mStatusZoomCombo;
    QList<double> mStatusZoomLevelsList;
    /**Label in status bar which shows messages from the composition*/
    QLabel* mStatusCompositionLabel;
    /**Label in status bar which shows atlas details*/
    QLabel* mStatusAtlasLabel;

    //! Pointer to composer view
    QgsComposerView *mView;
    QgsComposerRuler* mHorizontalRuler;
    QgsComposerRuler* mVerticalRuler;
    QWidget* mRulerLayoutFix;

    //! Current composition
    QgsComposition *mComposition;

    //! To know which item to show if selection changes
    QMap<QgsComposerItem*, QWidget*> mItemWidgetMap;

    QDockWidget* mItemDock;
    QDockWidget* mUndoDock;
    QDockWidget* mGeneralDock;
    QDockWidget* mAtlasDock;
    QDockWidget* mItemsDock;

    QgsMapLayerAction* mAtlasFeatureAction;

    struct PanelStatus
    {
      PanelStatus( bool visible = true, bool active = false ) : isVisible( visible ), isActive( active ) {}
      bool isVisible;
      bool isActive;
    };

    QMap< QString, PanelStatus > mPanelStatus;

  signals:
    void printAsRasterChanged( bool state );

  private slots:
    //! Toggles the state of the atlas preview and navigation controls
    //! @note added in 2.1
    void toggleAtlasControls( bool atlasEnabled );

    //! Sets the specified feature as the current atlas feature
    //! @note added in 2.1
    void setAtlasFeature( QgsMapLayer* layer, const QgsFeature &feat );

    //! Updates the "set as atlas feature" map layer action when atlas coverage layer changes
    void updateAtlasMapLayerAction( QgsVectorLayer* coverageLayer );

    void disablePreviewMode();
    void activateGrayscalePreview();
    void activateMonoPreview();
    void activateProtanopePreview();
    void activateDeuteranopePreview();

    void dockVisibilityChanged( bool visible );
    void setTitle( const QString& title );
};

#endif

