/***************************************************************************
                          qgslegend.h  -  description
                             -------------------
    begin                : Sun Jul 28 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
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
/* $Id$ */

#ifndef QGSLEGEND_H
#define QGSLEGEND_H

#include <deque>
#include <map>
#include <set>
#include <QTreeWidget>

class QgsLegendLayer;
class QgsLegendLayerFile;
class QgsLegendItem;
class QgsMapLayer;
class QgsMapCanvas;
class QDomDocument;
class QDomNode;
class QMouseEvent;
class QTreeWidgetItem;

/**
   \class QgsLegend
   \brief A Legend treeview for QGIS
   Map legend is a specialised QListView designed to show grooups of map layers,
   map layers, and the map layer members, properties and symbols for each layer.

   The legend supports simple operations such as displaying an ordered list of
   layers in the current canvas, and complex operations such as drag/dropping
   layer symbologies and properties between layers.

   There are a variety of different items that can appear in a QgsLegend. All
   items added to a QgsLegend should be inherited from QgsLegendItem as this
   will ensure that they can perform legend specific tasks such as seeing if
   dropping of other items onto them is allowed, returning their type etc.

   The following types are defined:
   <ul>
   <li>QgsLegendGroup - a group folder for many layers (can include other groups)</li>
   <li>QgsLegendLayer - a layer that contains one or more files associated with it.
                        allowing more than one file lets you create virtual layers where
                        1 or more files can share the same symbology and properties
                        and be treated as they are one layer for things such as hiding /
                        showing, scale dependent visibility etc.</li>
   <li>QgsLegendSymbologyGroup - a collabsable node that contains symbology items. Can
                                only exist inside of a QgsLegendLayer</li>
   <li>QgsLegendSymbologyItem - a class break (vector) or pallette entry (raster) etc.
                                Double clicking on a symbology item will let you change
                                the properties for only that specific item. Can only exist
                                inside a symbology group.</li>
   <li>QgsLegendPropertyGroup - a collapsable node that shows 1 or more properties. Can
                                only exist inside of a QgsLegendLayer</li>
   <li>QgsLegendPropertyItem - A list of properties related to the layer. Double clicking
                              a property item will invoke a dialog that will let you change
                              the property settings. Can only exist inside a property group</li>
   <li>QgsLegendLayerFileGroup - each QgsLegendLayer can have one or more files associated
                              with it. This is the container group for these files. Can
                              only exist inside of a QgsLegendLayer.</li>
   <li>QgsLegendLayerFile -  A file node that relates to a file on disk. Assigning multiple
                             file nodes in a file group allows you treat them as if they are
                             one entity.</li>
   </ul>
   @note Additional group types may be defined in the future to accommodate WMS, PostGIS etc layers.
   @author Gary E.Sherman, Tim Sutton, Marco Hugentobler and Jens Oberender
*/

class QgsLegend : public QTreeWidget
{
    Q_OBJECT
  private:
    // Moved here to match access of declaration later in file.
    // Previous location raised a warning in msvc as the forward
    // declaration was public while the definition was private
    class QgsLegendPixmaps;


  public:
    /*! Constructor.
    * @param qgis_app link to qgisapp
    * @param theParent An optional parent widget
    * @param theName An optional name for the widget
    */
    QgsLegend( QWidget * parent = 0, const char *name = 0 );

    //! Destructor
    ~QgsLegend();

    /** Returns QgsLegendLayerFile associated with current layer */
    QgsLegendLayerFile* currentLayerFile();

    /*!Returns the current layer if the current item is a QgsLegendLayerFile.
    If the current item is a QgsLegendLayer, its first maplayer is returned.
    Else, 0 is returned.*/
    QgsMapLayer* currentLayer();

    /**Writes the content of the legend to a project file*/
    bool writeXML( QDomNode & layer_node, QDomDocument & document );

    /**Restores the legend from a project file*/
    bool readXML( QDomNode& legendnode );

    /**Returns true, if the y-coordinate is >= the center of the item*/
    bool yCoordAboveCenter( QgsLegendItem* it, int ycoord );

    /**Returns the first item in the hierarchy*/
    QTreeWidgetItem* firstItem();

    /**Returns the next item (next sibling or next item on level above)*/
    QTreeWidgetItem* nextItem( QTreeWidgetItem* item );

    /**Returns the next sibling of an item or 0 if there is none*/
    QTreeWidgetItem* nextSibling( QTreeWidgetItem* item );

    /**Returns the previous sibling of an item or 0 if there is none*/
    QTreeWidgetItem* previousSibling( QTreeWidgetItem* item );

    /**Finds the next dom node. This function is used by QgsLegend, but probably its not a good place here*/
    static QDomNode nextDomNode( const QDomNode& theNode );

    /**Inserts an item into another one. Stores the item specific settings of the moved item (and its subitems)
     and applies it afterwards again*/
    void insertItem( QTreeWidgetItem* move, QTreeWidgetItem* into );

    /**Moves an item after another one. Stores the item specific settings of the moved item (and its subitems)
     and applies it afterwards again*/
    void moveItem( QTreeWidgetItem* move, QTreeWidgetItem* after );

    /**Removes an item from the legend. This is e.g. necessary before shifting it to another place*/
    void removeItem( QTreeWidgetItem* item );

    /**Returns the ids of the layers contained in this legend. The order is bottom->top*/
    std::deque<QString> layerIDs();

    /**Updates layer set of map canvas*/
    void updateMapCanvasLayerSet();

    /**Updates overview*/
    void updateOverview();

    /**Show/remove all layer in/from overview*/
    void enableOverviewModeAllLayers( bool isInOverview );

    /**Adds an entry to mPixmapWidthValues*/
    void addPixmapWidthValue( int width );

    /**Adds an entry to mPixmapHeightValues*/
    void addPixmapHeightValue( int height );

    /**Removes an entry from mPixmapWidthValues*/
    void removePixmapWidthValue( int width );

    /**Removes an entry from mPixmapHeightValues*/
    void removePixmapHeightValue( int height );

    /** Sets the name of the QgsLegendLayer that is the parent of
        the given QgsLegendLayerFile */
    void setName( QgsLegendLayerFile* w, QString layerName );

    /**Sets the toggle editing action. Usually called from QgisApp*/
    void setToggleEditingAction( QAction* editingAction ) {mToggleEditingAction = editingAction;}

    /**Returns structure with legend pixmaps*/
    QgsLegendPixmaps& pixmaps() { return mPixmaps; }


    void updateCheckStates( QTreeWidgetItem* item, Qt::CheckState state ) {mStateOfCheckBoxes[item] = state;}

  public slots:

    /*!Adds a new layer group with the maplayer to the canvas*/
    void addLayer( QgsMapLayer * layer );

    void setMapCanvas( QgsMapCanvas * canvas ) {mMapCanvas = canvas;}

    /**Updates symbology items for a layer*/
    void refreshLayerSymbology( QString key, bool expandItem = true );

    /*!
      * Slot called to clear the tree of all items
      * @note Usually connected to a QgsMapCanvas that will ask its legend to clear itself.
      * @return void
      */
    void removeAll();

    /*!
     * Called when the user wishes to toggle on or off all of the layers in
     * the legend, and in the map.
     * @return void
     */
    void selectAll( bool select );

    /*!
     * Slot called when user wishes to add a new empty layer group to the legend.
     * The user will be prompted for the name of the newly added group.
     * @return void
     */
    void addGroup();
    void removeLayer( QString );

    /** called to read legend settings from project */
    void readProject( const QDomDocument & );

    /** called to write legend settings to project */
    void writeProject( QDomDocument & );

    /**Removes the current LegendLayer and all its LegendLayerFiles*/
    void legendLayerRemove();

    /**Toggle show in overview for current layer*/
    void legendLayerShowInOverview();

    /**Zooms to extent of the current legend layer (considers there may be several
    legend layer files*/
    void legendLayerZoom();

    /***Zooms so that the pixels of the raster layer occupies exactly one screen pixel.
        Only works on raster layers*/
    void legendLayerZoomNative();

    /**Show attribute table*/
    void legendLayerAttributeTable();

    /**Shows the property dialog of the first legend layer file in a legend layer*/
    void legendLayerShowProperties();

  protected:

    /*!Event handler for mouse movements.
     * Mainly intended so handle cases where user is dragging and dropping
     * items into or out of groups, or is reordering layers.
     * @note Overrides method of the same name in the QListView class.
     * @return void
     */
    void mouseMoveEvent( QMouseEvent * e );

    /*!
     * Event handler for buton mouse presses.
     * Mainly intended so handle cases where user is dragging and dropping
     * items into or out of groups, or is reordering layers.
     * @note Overrides method of the same name in the QListView class.
     * @return void
     */
    void mousePressEvent( QMouseEvent * e );

    /*!
     * Event handler for mouse button releases.
     * Mainly intended so handle cases where user is dragging and dropping
     * items into or out of groups, or is reordering layers. Each sublass of
     * QgsLegendItem has an accept method that defines behaviour rules for
     * whether another QgsLegendItem child instance can be dropped onto it.
     * <h1>Behaviour rules for dropped legend items</h1>
     * <ul>
     * <li> Symbology groups, properies groups and layers groups can only be dropped
     * onto QgsLegendLayer nodes. </li>
     * <li>Only QgsLegendGroup and QgsLegendLayer can be top level items in the view</li>
     * <li>Groups can be nested by dropping them into each other,</li>
     * <li>Each group can have one or more layers</li>
     * <li>Layers can be ordered by dragging them above or below another layer.</li>
     * <li>The order for QgsLegendSymbologyGroup, QgsLegendPropertyGroup and QgsLegendLayerGroup
     * is predefined to sort in that order.</li>
     * </ul>
     * @note Overrides method of the same name in the QListView class.
     * @return void
     */
    void mouseReleaseEvent( QMouseEvent * e );
    void mouseDoubleClickEvent( QMouseEvent* e );

    /**Stores the necessary information about the position of an item in the hierarchy. Afterwards,
    this item may be moved back to the original position with resetToInitialPosition()*/
    void storeInitialPosition( QTreeWidgetItem* li );

    /**Moves an item back to the position where storeInitialPosition has been called*/
    void resetToInitialPosition( QTreeWidgetItem* li );

    /**Returns the legend layer to which a map layer gelongs*/
    QgsLegendLayer* findLegendLayer( const QString& layerKey );

    /**Checks mPixmapWidthValues and mPixmapHeightValues and sets a new icon size if necessary*/
    void adjustIconSize();

    /**Initialize pixmaps - called when QgsLegend is constructed */
    void initPixmaps();

    /**This function compares the layer order before a drag with the current layer ordering and triggers a canvas repaint if it has changed*/
    bool checkLayerOrderUpdate();

  private slots:

    /**Calls 'handleRightClickEvent' on the item*/
    void handleRightClickEvent( QTreeWidgetItem* item, const QPoint& position );
    /**Removes the current legend group*/
    void legendGroupRemove();
    /**Sets all listview items to open*/
    void expandAll();
    /**Sets all listview items to closed*/
    void collapseAll();
    /**Just for a test*/
    void handleItemChange( QTreeWidgetItem* item, int row );
    /** delegates current layer to map canvas */
    void handleCurrentItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous );
    /**Calls openPersistentEditor for the current item*/
    void openEditor();
    /**Removes the current item and inserts it as a toplevel item at the end of the legend*/
    void makeToTopLevelItem();
    /**Show/ Hide the legend layer file groups*/
    void showLegendLayerFileGroups();

  private:

    /*! Prevent the copying of QgsLegends
    * @todo See if this is really required - we may want multiple map, canvas and
            legend support at some stage in the future.
    */
    QgsLegend( QgsLegend const & );

    /*!
     * Prevent the copying of QgsLegends
     * @todo See if this is really required - we may want multiple map, canvas and
             legend support at some stage in the future.
     */
    QgsLegend & operator=( QgsLegend const & );

    /*!
     * Position of mouse when it is pressed at the start of a drag event.
     */
    QPoint mLastPressPos;

    /**True if the mouse is pressed*/
    bool mMousePressedFlag;

    /// keep track of the Item being dragged
    QTreeWidgetItem* mItemBeingMoved;

    /*!
     * Position in the list of the item being moved as it was at the start of a drag event.
     * An item at the top of the list will be 0 and each successive item below it
     * will be 1,2 3 etc... regardless of nesting level.
     */
    int mItemBeingMovedOrigPos;

    /**Information needed by 'storeInitialPosition' and 'resetToInitialPosition'*/
    enum HIERARCHY_POSITION_TYPE
    {
      FIRST_ITEM,
      FIRST_CHILD,
      YOUNGER_SIBLING
    };
    HIERARCHY_POSITION_TYPE mRestoreInformation;
    QTreeWidgetItem* mRestoreItem;

    bool mShowLegendLayerFiles;

    /**Stores the layer ordering before a mouse Move. After the move, this is used to
     decide if the mapcanvas really has to be refreshed*/
    std::deque<QString> mLayersPriorToMove;

    /**Action for the legendlayer right click menu*/
    QAction* mToggleEditingAction;

    /*!
     * A fuction sed to determin how far down in the list an item is (starting with one for the first Item.
     *If the item is not in the legend, -1 is returned
     * @see mItemBeingMovedOrigPos
     */
    int getItemPos( QTreeWidgetItem* item );

    /**Pointer to the main canvas. Used for requiring repaints in case of legend changes*/
    QgsMapCanvas* mMapCanvas;

    /**Map that keeps track of which checkboxes are in which check state. This is necessary because QTreeView does not emit
       a signal for check state changes*/
    std::map<QTreeWidgetItem*, Qt::CheckState> mStateOfCheckBoxes;

    /**Stores the width values of the LegendSymbologyItem pixmaps. The purpose of this is that the legend may automatically change
     the global IconWidth when items are added or removed*/
    std::multiset<int> mPixmapWidthValues;

    /**Stores the width values of the LegendSymbologyItem pixmaps. The purpose of this is that the legend may automatically change
     the global IconWidth when items are added or removed*/
    std::multiset<int> mPixmapHeightValues;

    /**QgsLegend does not set the icon with/height to values lower than the minimum icon size*/
    QSize mMinimumIconSize;

    /** structure which holds pixmap which are used in legend */
    class QgsLegendPixmaps
    {
      public:
        //! Pixmap which is shown by default
        QPixmap mOriginalPixmap;

        //! Pixmap to show a bogus vertex was encoutnered in this layer (applies to vector layers only)
        QPixmap mProjectionErrorPixmap;

        //! Pixmap to show if this layer is represented in overview or now
        QPixmap mInOverviewPixmap;

        //! Pixmap to show it this layer has currently editing turned on
        QPixmap mEditablePixmap;

    } mPixmaps;



  signals:
    void zOrderChanged( QgsLegend * lv );

    //! Emited whenever current (selected) layer changes
    //  the pointer to layer can be null if no layer is selected
    void currentLayerChanged( QgsMapLayer * layer );
};
#endif
