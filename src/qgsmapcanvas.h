/***************************************************************************
                          qgsmapcanvas.h  -  description
                             -------------------
    begin                : Sun Jun 30 2002
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
/* $Id$ */

#ifndef QGSMAPCANVAS_H
#define QGSMAPCANVAS_H

#include <list>
#include <memory>

// double sentinals to get round gcc 3.3.3 pre-processor bug

#ifndef QDOM_H
#include <qdom.h>
#endif

#ifndef QWIDGET_H
#include <qwidget.h>
#endif

#ifndef QGSRECT_H
#include <qgsrect.h>
#endif

#ifndef QGSPOINT_H
#include <qgspoint.h>
#endif

#include <qgis.h>

class QRect;
class QColor;
class QPaintDevice;
class QMouseEvent;

class QgsMapToPixel;
class QgsMapLayer;
class QgsMapLayerInterface;
class QgsLegend;
class QgsLegendView;
class QgsAcetateObject;
class QgsMeasure;


/*! \class QgsMapCanvas
 * \brief Map canvas class for displaying all GIS data types.
 */

class QgsMapCanvas : public QWidget
{
    Q_OBJECT;

 public:
    //! Constructor
    QgsMapCanvas(QWidget * parent = 0, const char *name = 0);

    //! Destructor
    ~QgsMapCanvas();

    //! Accessor for the canvas pixmap
    QPixmap * canvasPixmap();

    //! Mutator for the canvas pixmap
    void setCanvasPixmap(QPixmap * theQPixmap);

    //! Set the legend control to be used with this canvas
    void setLegend(QgsLegend *legend);

    //! Get a pointer to the legend control used with this canvas
    QgsLegend * getLegend();

    //! Get the last reported scale of the canvas
    double getScale();

    //! Clear the map canvas
    void clear();

    //! Returns the mupp (map units per pixel) for the canvas
    double mupp() const;

    //! Returns the current zoom exent of the map canvas
    QgsRect const & extent() const;
    //! Returns the combined exent for all layers on the map canvas
    QgsRect const & fullExtent() const;

    //! Set the extent of the map canvas
    void setExtent(QgsRect const & r);

    //! Zoom to the full extent of all layers
    void zoomFullExtent();

    //! Zoom to the previous extent (view)
    void zoomPreviousExtent();

    /**Zooms to the extend of the selected features*/
    void zoomToSelected();

    /** \brief Sets the map tool currently being used on the canvas */
    void setMapTool(int tool);

    /**Returns the currently active tool*/
    int mapTool();

    /** Write property of QColor bgColor. */
    virtual void setbgColor(const QColor & _newVal);

    /** Updates the full extent to include the mbr of the rectangle r */
    void updateFullExtent(QgsRect const & r);

    //! return the map layer at postion index in the layer stack
    QgsMapLayer *getZpos(int index);

    //! return the layer by name
    QgsMapLayer *layerByName(QString n);

    //! return number of layers on the map
    int layerCount() const;

    /*! Freeze/thaw the map canvas. This is used to prevent the canvas from
     * responding to events while layers are being added/removed etc.
     * @param frz Boolean specifying if the canvas should be frozen (true) or
     * thawed (false). Default is true.
     */
    void freeze(bool frz = true);

    /*! Accessor for frozen status of canvas */
    bool isFrozen();

    //! Flag the canvas as dirty and needed a refresh
    void setDirty(bool _dirty);

    //! Return the state of the canvas (dirty or not)
    bool isDirty() const;

    //! Calculate the scale and return as a string
    void currentScale(int thePrecision);

    void setZOrder( std::list<QString> );
    std::list < QString > const & zOrders() const;
    std::list < QString >       & zOrders();
    //! Set map units (needed by project properties dialog)
    void setMapUnits(QGis::units mapUnits);
    //! Get the current canvas map units

    QGis::units mapUnits() const;

    //! Get the current coordinate transform
    QgsMapToPixel * getCoordinateTransform();
    //! Declare the legend class as a friend of the map canvas
    //friend class QgsLegend;

    /** stores state in DOM node
        layerNode is DOM node corresponding to ``qgis'' tag

        The DOM node corresponds to a DOM document project file XML element to be
        written by QgsProject.

        Invoked by QgsProject::write().

        returns true if successful
    */
    bool writeXML( QDomNode & layerNode, QDomDocument & doc );

    //! true if canvas currently drawing
    bool isDrawing();

  
public slots:

    /*! Adds a layer to the map canvas.
     * @param lyr Pointer to a layer derived from QgsMapLayer
     */
    virtual void addLayer(QgsMapLayer * lyr);

    /*! \brief Add a layer from a map layer interface defined in a plugin.
      @note
         This is not currently implemented
     */
    void addLayer(QgsMapLayerInterface * lyr);

    //! remove the layer defined by key
    void remove (QString key);

    /** remove all layers from the map

        @note this does <i>not</i> iteratively call remove() since we're
        deleting all map layers reference at once; if we did this iteratively,
        then we'd be unnecessarily have the corresponding legend object
        deleting legend items.  We want the legend object to get <i>one</i>
        signal, clear(), to remove all <i>its</i> items.

        @note dirty set to true

        @note emits removedAll() signal
     */
    void removeAll();

    /**Sets dirty=true and calls render()*/
    void refresh();
    /**
     * Add an acetate object to the collection
     * @param key Key used to identify the object
     * @param obj Acetate object to add to the collection
     */
     void addAcetateObject(QString key, QgsAcetateObject *obj);

     /**Removes an acetate object from the collection and deletes the object*/
     void removeAcetateObject(const QString& key);

     /**Removes all entries of mCapturePoints and sets mLineEditing and mPolygonEditing
	to false (removes the digitising lines from the screen)
     @param norepaint true: there is no repaint at all. False: QgsMapCanvas
  decides, if a repaint is necessary or not*/
     void removeDigitizingLines(bool norepaint=false);

    //! The painter device parameter is optional - if ommitted it will default
    // to the pmCanvas (ie the gui map display). The idea is that you can pass
    // an alternative device such as one that will be used for printing or
    // saving a map view as an image file.
    virtual void render(QPaintDevice * theQPaintDevice=0);

    //! Save the convtents of the map canvas to disk as an image
    void saveAsImage(QString theFileName,QPixmap * QPixmap=0, QString="PNG" );

    //! This slot is connected to the visibility change of one or more layers
    void layerStateChange();

    //! sets z order based on order of layers in the legend
    void setZOrderFromLegend(QgsLegend *lv);

    //! Whether to suppress rendering or not
    void setRenderFlag(bool theFlag);
    //! State of render suppression flag
    bool renderFlag() {return mRenderFlag;};

    /**
    Recalculate the full extent for the map canvas. This slot is connected to
    each map layer and is "called" when the layers extent changes, either
    through editing or subsetting via SQL query or other method. The full
    extent is calculated by getting the layer collection from the map layer
    registry and iterating through it, passing the extent of each layer to
    the updateFullExtent method.
     */
    void recalculateExtents();

    /** A simple helper method to find out if on the fly projections are enabled or not */
    bool projectionsEnabled();

    /** The map units may have changed, so cope with that */
    void mapUnitsChanged();
    
signals:
    /** Let the owner know how far we are with render operations */
    void setProgress(int,int);
    /** emits current mouse position */
    void xyCoordinates(QgsPoint & p);
    /** emits mouse position when the canvas is clicked */
    void xyClickCoordinates(QgsPoint &p);
    void xyClickCoordinates(QgsPoint &p, Qt::ButtonState button);

    //! Emitted when the scale of the map changes
    void scaleChanged(QString);

    //! Emitted when the extents of the map change
    void extentsChanged(QgsRect);

    /** Emitted when the canvas has rendered.

     Passes a pointer to the painter on which the map was drawn. This is
     useful for plugins that wish to draw on the map after it has been
     rendered.  Passing the painter allows plugins to work when the map is
     being rendered onto a pixmap other than the mapCanvas own pixmap member.

    */
    void renderComplete(QPainter *);

    /** emitted whenever a layer is added to the map canvas */
    void addedLayer(QgsMapLayer * lyr);

    /** emitted whenever a layer is deleted from the map canvas
        @param the key of the deleted layer
    */
    void removedLayer( QString layer_key );

    /**
       emitted when removeAll() invoked to let observers know that the canvas is
       now empty
     */
    void removedAll();

    /** emitted when right mouse button is pressed with zoom tool
     *  QgisApp should catch it and reset tool to the last non zoom tool */
    void stopZoom();

protected:
    /// implementation struct
    class CanvasProperties;

    /// Handle pattern for implementation object
    std::auto_ptr<CanvasProperties> mCanvasProperties;

    //! lets us know whether this canvas is being used as an overview canvas or note
    bool mIsOverviewCanvas;
    
private:
    /// this class is non-copyable
    /**
       @note

       Otherwise std::auto_ptr would pass the object responsiblity on to the
       copy like a hot potato leaving the copyer in a weird state.
     */
    QgsMapCanvas( QgsMapCanvas const & );

    /**
       private to force use of ctor with arguments
     */
    QgsMapCanvas();

    /**
       List to store the points of digitised lines and polygons

       @todo XXX shouldn't this be in mCanvasProperties?
    */
    std::list<QgsPoint> mCaptureList;
    
    /**Stores the last position of the mouse cursor when digitising*/
    QgsPoint mDigitMovePoint;

    /**Stores, if mCaptureList is used for line editing
     this is needed for the editing rubber band*/
    bool mLineEditing;

    /**Stores, if mCaptureList is used for polygon editing
     this is needed for the editing rubber band*/
    bool mPolygonEditing;

    /**Draws a line from the last point of mCaptureList (if last is true, else from the first point)
       to mDigitMovePoint using Qt::XorROP. The settings for QPen are read from the QgsProject singleton*/
    void drawLineToDigitisingCursor(QPainter* paint, bool last = true);

    //! Overridden key press event
    void keyPressEvent(QKeyEvent * e);

    //! Overridden key release event
    void keyReleaseEvent(QKeyEvent * e);

    //! Overridden mouse move event
    void mouseMoveEvent(QMouseEvent * e);

    //! Overridden mouse press event
    void mousePressEvent(QMouseEvent * e);

    //! Overridden mouse release event
    void mouseReleaseEvent(QMouseEvent * e);

    //! Overridden mouse wheel event
    void wheelEvent(QWheelEvent * e);

    //! Overridden resize event
    void resizeEvent(QResizeEvent * e);

    //! Overridden paint event
    void paintEvent(QPaintEvent * pe);

    //! Gets the value used to calculated the identify search radius
    double calculateSearchRadiusValue();


    //! Increments the z order index
    void incrementZpos();

    //! Updates the z order for layers on the map
    void updateZpos();

    //! Zooms to a given center and scale 
    void zoomByScale(int x, int y, double scaleFactor);

    //! Ends pan action and redraws the canvas.
    void panActionEnd(QPoint releasePoint);

    //! Called when mouse is moving and pan is activated
    void panAction(QMouseEvent * event);

    //! Helper function to inverse project a point if projections
    // are enabled. Failsafe, returns the sent point if anything fails.
    // @whenmsg is a part fo the error message.
    QgsPoint maybeInversePoint(QgsPoint point, const char whenmsg[]);

    //! detrmines whether the user can interact with the canvas using a mouse
    //(useful for locking the overview canvas)
    bool mUserInteractionAllowed;

    //! determines whether user has requested to suppress rendering
    bool mRenderFlag;
    
  /** debugging member
      invoked when a connect() is made to this object
  */
  void connectNotify( const char * signal );

    //! Measure tool
    QgsMeasure *mMeasure;

    //! Scale factor multiple for default zoom in/out
    // TODO Make this customisable by the user
    static const double scaleDefaultMultiple;

}; // class QgsMapCanvas


#endif
