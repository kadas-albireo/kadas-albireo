/** \ingroup MapComposer
 * Graphics scene for map printing. The class manages the paper item which always
 * is the item in the back (z-value 0). It maintains the z-Values of the items and stores
 * them in a list in ascending z-Order. This list can be changed to lower/raise items one position
 * or to bring them to front/back.
 * */
class QgsComposition : QGraphicsScene
{
%TypeHeaderCode
#include <qgscomposition.h>
%End

  public:

    /** \brief Plot type */
    enum PlotStyle
    {
      Preview,     // Use cache etc
      Print,       // Render well
      Postscript   // Fonts need different scaling!
    };

    /**Style to draw the snapping grid*/
    enum GridStyle
    {
      Solid,
      Dots,
      Crosses
    };

    enum ZValueDirection
    {
      ZValueBelow,
      ZValueAbove
    };

    enum PaperOrientation
    {
      Portrait,
      Landscape
    };

    enum OutputMode
    {
      Single = 0,
      Atlas
    };

    explicit QgsComposition( const QgsMapSettings& mapSettings, const QString& title = "", QGraphicsView* mapCanvas = 0 );

    /**Composition atlas modes*/
    enum AtlasMode
    {
      AtlasOff,     // Composition is not being controlled by an atlas
      PreviewAtlas, // An atlas composition is being previewed in the app
      ExportAtlas   // The composition is being exported as an atlas
    };

    ~QgsComposition();

    void setTitle( const QString& title );
    const QString& title() const;

    /**Changes size of paper item. Also moves all items so that they retain
     * their same relative position to the top left corner of their current page.
     * @param width page width in mm
     * @param height page height in mm
     * @see paperHeight
     * @see paperWidth
    */
    void setPaperSize( const double width, const double height );

    /**Height of paper item
     * @returns height in mm
     * @see paperWidth
     * @see setPaperSize
    */
    double paperHeight() const;

    /**Width of paper item
     * @returns width in mm
     * @see paperHeight
     * @see setPaperSize
    */
    double paperWidth() const;

    /**Returns the vertical space between pages in a composer view
     * @returns space between pages in mm
    */
    double spaceBetweenPages() const;

    /**Sets the number of pages for the composition.
     * @param pages number of pages
     * @see numPages
    */
    void setNumPages( const int pages );

    /**Returns the number of pages in the composition.
     * @returns number of pages
     * @see setNumPages
    */
    int numPages() const;

    /**Returns whether a page is empty, ie, it contains no items except for the background
     * paper item.
     * @param page page number, starting with 1
     * @returns true if page is empty
     * @note added in QGIS 2.5
     * @see numPages
     * @see setNumPages
     * @see shouldExportPage
    */
    bool pageIsEmpty( const int page ) const;

    /**Returns whether a specified page number should be included in exports of the composition.
     * @param page page number, starting with 1
     * @returns true if page should be exported
     * @note added in QGIS 2.5
     * @see numPages
     * @see pageIsEmpty
    */
    bool shouldExportPage( const int page ) const;

    /**Note: added in version 2.1*/
    void setPageStyleSymbol( QgsFillSymbolV2* symbol /Transfer/ );
    /**Note: added in version 2.1*/
    QgsFillSymbolV2* pageStyleSymbol();

    /**Returns the position within a page of a point in the composition
      @note Added in QGIS 2.1
    */
    QPointF positionOnPage( const QPointF & position ) const;

    /**Returns the page number corresponding to a point in the composition
      @note Added in QGIS 2.1
    */
    int pageNumberForPoint( const QPointF & position ) const;

    /**Sets the status bar message for the composer window
      @note Added in QGIS 2.1
    */
    void setStatusMessage( const QString & message );

    /**Refreshes the composition when composer related options change
     @note added in version 2.1
    */
    void updateSettings();

    void setSnapToGridEnabled( const bool b );
    bool snapToGridEnabled() const;

    void setGridVisible( const bool b );
    bool gridVisible() const;

    /**Hides / shows custom snap lines*/
    void setSnapLinesVisible( const bool visible );
    bool snapLinesVisible() const;

    void setAlignmentSnap( const bool s );
    bool alignmentSnap() const;

    void setSmartGuidesEnabled( const bool b );
    bool smartGuidesEnabled() const;

    /**Removes all snap lines*/
    void clearSnapLines();

    void setSnapGridResolution( const double r );
    double snapGridResolution() const;

    void setSnapGridOffsetX( const double offset );
    double snapGridOffsetX() const;

    void setSnapGridOffsetY( const double offset );
    double snapGridOffsetY() const;

    void setGridPen( const QPen& p );
    const QPen& gridPen() const;

    void setGridStyle( const GridStyle s );
    GridStyle gridStyle() const;

    /**Sets the snap tolerance to use when automatically snapping items during movement and resizing to the
     * composition grid.
     * @param tolerance snap tolerance in pixels
     * @see snapGridTolerance
     * @deprecated Use setSnapTolerance instead
    */
    void setSnapGridTolerance( const double tolerance ) /Deprecated/;

    /**Returns the snap tolerance to use when automatically snapping items during movement and resizing to the
     * composition grid.
     * @returns snap tolerance in pixels
     * @see setSnapGridTolerance
     * @deprecated Use snapTolerance instead
    */
    double snapGridTolerance() const /Deprecated/;

    /**Sets the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @param t snap tolerance in pixels
     * @see alignmentSnapTolerance
     * @deprecated Use setSnapTolerance instead
    */
    void setAlignmentSnapTolerance( double t ) /Deprecated/;

    /**Returns the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @returns snap tolerance in pixels
     * @see setAlignmentSnapTolerance
     * @deprecated Use snapTolerance instead
    */
    double alignmentSnapTolerance() const /Deprecated/;

    /**Sets the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @param snapTolerance snap tolerance in pixels
     * @see alignmentSnapTolerance
     * @note Added in QGIS 2.5
    */
    void setSnapTolerance( const int snapTolerance );

    /**Returns the snap tolerance to use when automatically snapping items during movement and resizing to guides
     * and the edges and centers of other items.
     * @returns snap tolerance in pixels
     * @see setAlignmentSnapTolerance
     * @note Added in QGIS 2.5
    */
    int snapTolerance() const;

    /**Sets whether selection bounding boxes should be shown in the composition
     * @param boundsVisible set to true to show selection bounding box
     * @see boundingBoxesVisible
     * @note added in QGIS 2.7
    */
    void setBoundingBoxesVisible( const bool boundsVisible );

    /**Returns whether selection bounding boxes should be shown in the composition
     * @returns true if selection bounding boxes should be shown
     * @see setBoundingBoxesVisible
     * @note added in QGIS 2.7
    */
    bool boundingBoxesVisible() const;

    /**Returns pointer to undo/redo command storage*/
    QUndoStack* undoStack();

    /**Returns the topmost composer item at a specified position. Ignores paper items.
     * @param position point to search for item at
     * @param ignoreLocked set to true to ignore locked items
     * @returns composer item at position
    */
    QgsComposerItem* composerItemAt( const QPointF & position, const bool ignoreLocked = false ) const;

    /**Returns the topmost composer item at a specified position which is below a specified item. Ignores paper items.
     * @param position point to search for item at
     * @param belowItem item to search below
     * @param ignoreLocked set to true to ignore locked items
     * @returns composer item at position which is below specified item
    */
    QgsComposerItem* composerItemAt( const QPointF & position, const QgsComposerItem* belowItem, const bool ignoreLocked = false ) const;

    /** Returns the page number (0-based) given a coordinate */
    int pageNumberAt( const QPointF& position ) const;

    /** Returns on which page number (0-based) is displayed an item */
    int itemPageNumber( const QgsComposerItem* ) const;

    /**Returns list of selected composer items
     * @param includeLockedItems set to true to include locked items in list
     * @returns list of selected items
    */
    QList<QgsComposerItem*> selectedComposerItems( const bool includeLockedItems = true );

%If (QLISTCONSTPTR_CONVERSION)
    /**Returns pointers to all composer maps in the scene
      @note available in python bindings only with PyQt >= 4.8.4
      */
    QList<const QgsComposerMap*> composerMapItems() const;
%End

    /**Return composer items of a specific type
     * @param itemList list of item type to store matching items in
     * @note not available in python bindings
     */
    // template<class T> void composerItems( QList<T*>& itemList );

    /**Return composer items of a specific type on a specified page
     * @param itemList list of item type to store matching items in
     * @param pageNumber page number (0 based)
     * @note not available in python bindings
     * @note added in QGIS 2.5
     */
    //template<class T> void composerItemsOnPage( QList<T*>& itemList, const int pageNumber );

    /**Returns the composer map with specified id
     @return QgsComposerMap or 0 pointer if the composer map item does not exist*/
    const QgsComposerMap* getComposerMapById( const int id ) const;

    /**Returns the composer html with specified id (a string as named in the
      composer user interface item properties).
      @param item the item.
      @return QgsComposerHtml pointer or 0 pointer if no such item exists.
     */
    const QgsComposerHtml* getComposerHtmlByItem( QgsComposerItem *item ) const /Deprecated/;

    /**Returns a composer item given its text identifier.
       Ids are not necessarely unique, but this function returns only one element.
      @param theId - A QString representing the identifier of the item to
        retrieve.
      @return QgsComposerItem pointer or 0 pointer if no such item exists.
     */
    const QgsComposerItem* getComposerItemById( const QString theId ) const;

    /**Returns a composer item given its unique identifier.
      @param theUuid A QString representing the UUID of the item to
      **/
    const QgsComposerItem* getComposerItemByUuid( const QString theUuid ) const;

    int printResolution() const;
    void setPrintResolution( const int dpi );

    bool printAsRaster() const;
    void setPrintAsRaster( const bool enabled );

    bool generateWorldFile() const;
    void setGenerateWorldFile( const bool enabled );

    const QgsComposerMap* worldFileMap();
    void setWorldFileMap( QgsComposerMap* map );

    /**Returns true if a composition should use advanced effects such as blend modes */
    bool useAdvancedEffects() const;
    /**Used to enable or disable advanced effects such as blend modes in a composition */
    void setUseAdvancedEffects( const bool effectsEnabled );

    //! Return setting of QGIS map canvas
    //! @note added in 2.4
    const QgsMapSettings& mapSettings() const;

    //! Return the QGraphicsScene which is actually the QgsMapCanvas.
    //! @note null if QgsComposition was constructed without the mapCanvas passed.
    QGraphicsView* mapCanvas() const;

    QgsComposition::PlotStyle plotStyle() const;
    void setPlotStyle( const QgsComposition::PlotStyle style );

    /**Returns the mm font size for a font that has point size set.
     * Each item that sets a font should call this function before drawing text
     * @deprecated use QgsComposerUtils::pointsToMM instead
    */
    int pixelFontSize( double pointSize ) const /Deprecated/;

    /**Does the inverse calculation and returns points for mm
     * @deprecated use QgsComposerUtils::mmToPoints instead
    */
    double pointFontSize( int pixelSize ) const /Deprecated/;

    /**Writes settings to xml (paper dimension)*/
    bool writeXML( QDomNode& parentNode, QDomDocument& doc );

    /**Reads settings from xml file
     * @note if fromTemplate is true, the title is not read from the XML*/
    bool readXML( const QDomElement& composerElem, bool fromTemplate = false );

    /**Load a template document
        @param doc template document
        @param substitutionMap map with text to replace. Text needs to be enclosed by brackets (e.g. '[text]' )
        @param clearComposition set to true to clear the existing composition and read all composition and
        atlas properties from the template. Set to false to only add new items from the template, without
        overwriting the existing items or composition settings.
      */
    bool loadFromTemplate( const QDomDocument& doc, QMap<QString, QString>* substitutionMap = 0, bool clearComposition = true );

    /**Add items from XML representation to the graphics scene (for project file reading, pasting items from clipboard)
      @param elem items parent element, e.g. \verbatim <Composer> \endverbatim or \verbatim <ComposerItemClipboard> \endverbatim
      @param doc xml document
      @param mapsToRestore for reading from project file: set preview move 'rectangle' to all maps and save the preview states to show composer maps on demand
      @param addUndoCommands insert AddItem commands if true (e.g. for copy/paste)
      @param pos item position. Optional, take position from xml if 0
      @param pasteInPlace whether the position should be kept but mapped to the page origin. (the page is the page under to the mouse cursor)
      @note parameters mapsToRestore, addUndoCommands pos and pasteInPlace not available in python bindings
     */
    void addItemsFromXML( const QDomElement& elem, const QDomDocument& doc );

    /**Adds item to z list. Usually called from constructor of QgsComposerItem*/
    void addItemToZList( QgsComposerItem* item );
    /**Removes item from z list. Usually called from destructor of QgsComposerItem*/
    void removeItemFromZList( QgsComposerItem* item );

    //functions to move selected items in hierarchy
    void raiseSelectedItems();

    //returns true if successful
    bool raiseItem( QgsComposerItem* item );
    void lowerSelectedItems();
    //returns true if successful
    bool lowerItem( QgsComposerItem* item );
    void moveSelectedItemsToTop();
    //returns true if successful
    bool moveItemToTop( QgsComposerItem* item );
    void moveSelectedItemsToBottom();
    //returns true if successful
    bool moveItemToBottom( QgsComposerItem* item );

    //functions to find items by their position in the z list
    void selectNextByZOrder( const ZValueDirection direction );
    QgsComposerItem* getComposerItemBelow( QgsComposerItem* item ) const;
    QgsComposerItem* getComposerItemAbove( QgsComposerItem* item ) const;

    //functions to align selected items
    void alignSelectedItemsLeft();
    void alignSelectedItemsHCenter();
    void alignSelectedItemsRight();
    void alignSelectedItemsTop();
    void alignSelectedItemsVCenter();
    void alignSelectedItemsBottom();

    //functions to lock and unlock items
    /**Lock the selected items*/
    void lockSelectedItems();
    /**Unlock all items*/
    void unlockAllItems();

    /**Creates a new group from a list of composer items and adds it to the composition.
     * @param items items to include in group
     * @returns QgsComposerItemGroup of grouped items, if grouping was possible
     * @note added in QGIS 2.6
    */
    QgsComposerItemGroup* groupItems( QList<QgsComposerItem*> items );

    /**Ungroups items by removing them from an item group and removing the group from the
     * composition.
     * @param group item group to ungroup
     * @returns list of items removed from the group, or an empty list if ungrouping
     * was not successful
     * @note added in QGIS 2.6
    */
    QList<QgsComposerItem*> ungroupItems( QgsComposerItemGroup* group );

    /**Sorts the zList. The only time where this function needs to be called is from QgsComposer
     * after reading all the items from xml file
     * @deprecated use refreshZList instead
    */
    void sortZList() /Deprecated/;

    /**Rebuilds the z order list by adding any item which are present in the composition
     * but missing from the z order list.
    */
    void refreshZList();

    /**Snaps a scene coordinate point to grid*/
    QPointF snapPointToGrid( const QPointF& scenePoint ) const;

    /**Returns pointer to snap lines collection*/
    QList< QGraphicsLineItem* >* snapLines();

    /**Returns pointer to selection handles
     * @note not available in python bindings
     */
    // QgsComposerMouseHandles* selectionHandles();

    /**Add a custom snap line (can be horizontal or vertical)*/
    QGraphicsLineItem* addSnapLine();
    /**Remove custom snap line (and delete the object)*/
    void removeSnapLine( QGraphicsLineItem* line );
    /**Get nearest snap line
     * @note not available in python bindings
     */
    // QGraphicsLineItem* nearestSnapLine( const bool horizontal, const double x, const double y, const double tolerance, QList< QPair< QgsComposerItem*, QgsComposerItem::ItemPositionMode > >& snappedItems );

    /**Allocates new item command and saves initial state in it
      @param item target item
      @param commandText descriptive command text
      @param c context for merge commands (unknown for non-mergeable commands)*/
    void beginCommand( QgsComposerItem* item, const QString& commandText, const QgsComposerMergeCommand::Context c = QgsComposerMergeCommand::Unknown );

    /**Saves end state of item and pushes command to the undo history*/
    void endCommand();
    /**Deletes current command*/
    void cancelCommand();

    void beginMultiFrameCommand( QgsComposerMultiFrame* multiFrame, const QString& text, const QgsComposerMultiFrameMergeCommand::Context c = QgsComposerMultiFrameMergeCommand::Unknown );
    void endMultiFrameCommand();
    /**Deletes current multi frame command*/
    void cancelMultiFrameCommand();

    /**Adds multiframe. The object is owned by QgsComposition until removeMultiFrame is called*/
    void addMultiFrame( QgsComposerMultiFrame* multiFrame );
    /**Removes multi frame (but does not delete it)*/
    void removeMultiFrame( QgsComposerMultiFrame* multiFrame );
    /**Adds a composer item to the graphics scene*/
    void addComposerItem( QgsComposerItem* item /Transfer/ );
    /**Adds map to the graphics scene and advices composer to create a widget for it (through signal)*/
    void addComposerMap( QgsComposerMap* map, const bool setDefaultPreviewStyle = true );

    /**Remove item from the graphics scene. Additionally to QGraphicsScene::removeItem, this function considers undo/redo command*/
    void removeComposerItem( QgsComposerItem* item, const bool createCommand = true, const bool removeGroupItems = true );

    /**Convenience function to create a QgsAddRemoveItemCommand, connect its signals and push it to the undo stack*/
    void pushAddRemoveCommand( QgsComposerItem* item, const QString& text, const QgsAddRemoveItemCommand::State state = QgsAddRemoveItemCommand::Added );

    /**If true, prevents any mouse cursor changes by the composition or by any composer items
      Used by QgsComposer and QgsComposerView to prevent unwanted cursor changes*/
    void setPreventCursorChange( const bool preventChange );
    bool preventCursorChange() const;

    QPrinter* defaultPrinter();

    /**
     * @brief Print composition with specified printer
     * @param printer A printer, if null the default printer is used, @see defaultPrinter
     * @param mode Either Single or Atlas
     * @param evaluateDDPageSize Whether to honour data defined page sizes
     * @param progressDialog Optional string for returning error messages
     * @param errorMsg  Optional string for returning error messages
     * @return Whether the operation succeeded
     */
    bool print( QPrinter* printer = 0, OutputMode mode = Single, bool evaluateDDPageSize = true, QProgressDialog *progressDialog = 0, QString *errorMsg = 0 ) /PyName=print_/;
    /**
     * @brief Print composition to PDF
     * @param output The output filename
     * @param mode Either Single or Atlas
     * @param progressDialog Optional progress dialog
     * @param errorMsg Optional string for returning error messages
     * @return Whether the operation succeeded
     */
    bool printToPdf( const QString& output, OutputMode mode = Single, QProgressDialog *progressDialog = 0, QString *errorMsg = 0 );
    /**
     * @brief Print composition to image
     * @param output A filename if mode is Single, a folder if mode is Atlas
     * @param format An image format supported by QImageReader, or "svg"
     * @param groupLayers Only if format is "svg": whether to export map layers as svg groups
     * @param mode Either Single or Atlas
     * @param progressDialog Optional progress dialog
     * @param errorMsg Optional string for returning error messages
     * @return Whether the operation succeeded
     */
    bool printToImage( const QString& output, const QString &format, bool groupLayers = false, OutputMode mode = Single, QProgressDialog *progressDialog = 0, QString *errorMsg = 0 );
    /**
     * @brief Render the specified page
     * @param p The painter
     * @param page The page
     */
    void renderPage( QPainter* p, int page );

    /** Compute world file parameters */
    void computeWorldFileParameters( double& a, double& b, double& c, double& d, double& e, double& f ) const;
    bool writeWorldFile( QString worldFileName ) const;

    QgsAtlasComposition& atlasComposition();

    /**Resizes a QRectF relative to the change from boundsBefore to boundsAfter
     * @deprecated use QgsComposerUtils::relativeResizeRect instead
     */
    static void relativeResizeRect( QRectF& rectToResize, const QRectF& boundsBefore, const QRectF& boundsAfter ) /Deprecated/;

    /**Returns a scaled position given a before and after range
     * @deprecated use QgsComposerUtils::relativePosition instead
     */
    static double relativePosition( double position, double beforeMin, double beforeMax, double afterMin, double afterMax ) /Deprecated/;

    /**Returns the current atlas mode of the composition
     * @returns current atlas mode
     * @see setAtlasMode
     */
    QgsComposition::AtlasMode atlasMode() const;

    /**Sets the current atlas mode of the composition.
     * @param mode atlas mode to switch to
     * @returns false if the mode could not be changed.
     * @see atlasMode
    */
    bool setAtlasMode( const QgsComposition::AtlasMode mode );

    /** Return pages in the correct order
     @note composerItems(QList< QgsPaperItem* > &) may not return pages in the correct order
     @note added in version 2.4*/
    const QList< QgsPaperItem* >& pages();

    /**Returns a reference to the data defined settings for one of the composition's data defined properties.
     * @param property data defined property to return
     * @note this method was added in version 2.5
    */
    QgsDataDefined* dataDefinedProperty( const QgsComposerObject::DataDefinedProperty property );

    /**Sets parameters for a data defined property for the composition
     * @param property data defined property to set
     * @param active true if data defined property is active, false if it is disabled
     * @param useExpression true if the expression should be used
     * @param expression expression for data defined property
     * @param field field name if the data defined property should take its value from a field
     * @note this method was added in version 2.5
    */
    void setDataDefinedProperty( const QgsComposerObject::DataDefinedProperty property, bool active, bool useExpression, const QString &expression, const QString &field );

    /**Returns the items model attached to the composition
     * @returns QgsComposerModel for composition
     * @note this method was added in version 2.5
    */
    QgsComposerModel * itemsModel();

  public slots:
    /**Casts object to the proper subclass type and calls corresponding itemAdded signal*/
    void sendItemAddedSignal( QgsComposerItem* item );

    /**Updates the scene bounds of the composition
    @note added in version 2.2*/
    void updateBounds();

    /**Forces items in the composition to refresh. For instance, this causes maps to redraw
     * and rebuild cached images, html items to reload their source url, and attribute tables
     * to refresh their contents. Calling this also triggers a recalculation of all data defined
     * attributes within the composition.
     * @note added in version 2.3*/
    void refreshItems();

    /**Clears any selected items and sets an item as the current selection.
     * @param item item to set as selected
     * @note added in version 2.3*/
    void setSelectedItem( QgsComposerItem* item );

    /**Clears any selected items in the composition. Call this method rather than
     * QGraphicsScene::clearSelection, as the latter does not correctly emit signals to allow
     * the composition's model to update.
     * @note added in version 2.5*/
    void setAllUnselected();

    /**Refreshes a data defined property for the composition by reevaluating the property's value
     * and redrawing the composition with this new value.
     * @param property data defined property to refresh. If property is set to
     * QgsComposerItem::AllProperties then all data defined properties for the composition will be
     * refreshed.
     * @note this method was added in version 2.5
    */
    void refreshDataDefinedProperty( const QgsComposerObject::DataDefinedProperty property = QgsComposerObject::AllProperties );

  signals:
    void titleChanged(const QString& newTitle);
    void paperSizeChanged();
    void nPagesChanged();

    /**Is emitted when the compositions print resolution changes*/
    void printResolutionChanged();

    /**Is emitted when selected item changed. If 0, no item is selected*/
    void selectedItemChanged( QgsComposerItem* selected );
    /**Is emitted when new composer arrow has been added to the view*/
    void composerArrowAdded( QgsComposerArrow* arrow );
    /**Is emitted when a new composer html has been added to the view*/
    void composerHtmlFrameAdded( QgsComposerHtml* html, QgsComposerFrame* frame );
    /**Is emitted when new composer label has been added to the view*/
    void composerLabelAdded( QgsComposerLabel* label );
    /**Is emitted when new composer map has been added to the view*/
    void composerMapAdded( QgsComposerMap* map );
    /**Is emitted when new composer scale bar has been added*/
    void composerScaleBarAdded( QgsComposerScaleBar* scalebar );
    /**Is emitted when a new composer legend has been added*/
    void composerLegendAdded( QgsComposerLegend* legend );
    /**Is emitted when a new composer picture has been added*/
    void composerPictureAdded( QgsComposerPicture* picture );
    /**Is emitted when a new composer shape has been added*/
    void composerShapeAdded( QgsComposerShape* shape );
    /**Is emitted when a new composer table has been added*/
    void composerTableAdded( QgsComposerAttributeTable* table );
    /**Is emitted when a new composer table frame has been added to the view*/
    void composerTableFrameAdded( QgsComposerAttributeTableV2* table, QgsComposerFrame* frame );
    /**Is emitted when a composer item has been removed from the scene*/
    void itemRemoved( QgsComposerItem* );

    /**Is emitted when item in the composition must be refreshed*/
    void refreshItemsTriggered();

    /**Is emitted when the composition has an updated status bar message for the composer window*/
    void statusMsgChanged( QString message );
};
