/***************************************************************************
                          qgsmaplayer.h  -  description
                             -------------------
    begin                : Fri Jun 28 2002
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

#ifndef QGSMAPLAYER_H
#define QGSMAPLAYER_H

#include <vector>
#include <map>

#include <QObject>

#include "qgsrect.h"

class QgsCoordinateTransform;
class QgsMapToPixel;
class QgsSpatialRefSys;

class QDomNode;
class QDomDocument;
class QKeyEvent;
class QPainter;

/** \class QgsMapLayer
 *  \brief Base class for all map layer types.
 * This class is the base class for all map layer types (vector, raster).
 */
class CORE_EXPORT QgsMapLayer : public QObject
{
    Q_OBJECT

public:

    /** Constructor
     * @param type Type of layer as defined in LAYERS enum
     * @param lyrname Display Name of the layer
     */
    QgsMapLayer(int type = 0, QString lyrname = QString::null, QString source = QString::null);

    /** Destructor */
    virtual ~QgsMapLayer();

    /** Get the type of the layer
     * @return Integer matching a value in the LAYERS enum
     */
    int type() const;

    /** Get this layer's unique ID, this ID is used to access this layer from map layer registry */
    QString getLayerID() const;

    /** Set the display name of the layer
     * @param name New name for the layer
     */
    void setLayerName(const QString & name);

    /** Get the display name of the layer
     * @return the layer name
     */
    QString const & name() const;

    /** Virtual function to calculate the extent of the current layer.
     * This function must be overridden in all child classes and implemented
     * based on the layer type
     */
    virtual QgsRect calculateExtent();

    /** Render the layer, to be overridden in child classes
     * @param painter Painter that to be used for rendered output
     * @param rect Extent of the layer to be drawn
     * @param mtp Transformation class
     * @return FALSE if an error occurred during drawing
     */
    virtual bool draw(QPainter* painter, QgsRect& rect, QgsMapToPixel* mtp, QgsCoordinateTransform* ct, bool);
    
    /** Draw labels
     * @TODO to be removed: used only in vector layers
     */
    virtual void drawLabels(QPainter* painter, QgsRect& rect, QgsMapToPixel* mtp, QgsCoordinateTransform* ct);

    /** Return the extent of the layer as a QRect */
    const QgsRect extent();

    /*! Return the status of the layer. An invalid layer is one which has a bad datasource
     * or other problem. Child classes set this flag when intialized
     * @return True if the layer is valid and can be accessed
     */
    bool isValid();

    /*! Gets a version of the internal layer definition that has sensitive 
      *  bits removed (for example, the password). This function should 
      * be used when displaying the source name for general viewing. 
     */ 
    QString publicSource() const;

    /** Returns the source for the layer */
    QString const & source() const;

    /**
     * Returns the sublayers of this layer
     * (Useful for providers that manage their own layers, such as WMS)
     */
    virtual QStringList subLayers();
    
    /**
     * Reorders the *previously selected* sublayers of this layer from bottom to top
     * (Useful for providers that manage their own layers, such as WMS)
     */
    virtual void setLayerOrder(QStringList layers);
    
    /** Set the visibility of the given sublayer name */
    virtual void setSubLayerVisibility(QString name, bool vis);

    /** Layers enum defining the types of layers that can be added to a map */
    enum LAYERS
    {
        VECTOR,
        RASTER
    };

    /** True if the layer can be edited */
    virtual bool isEditable() const = 0;

    /** sets state from DOM document
       @param layer_node is DOM node corresponding to ``maplayer'' tag
       @note

       The DOM node corresponds to a DOM document project file XML element read
       by QgsProject.

       This, in turn, calls readXML_(), which is over-rideable by sub-classes so
       that they can read their own specific state from the given DOM node.

       Invoked by QgsProject::read().

       @returns true if successful
     */
    bool readXML(QDomNode & layer_node);


    /** stores state in DOM node
       @param layer_node is DOM node corresponding to ``projectlayers'' tag
       @note

       The DOM node corresponds to a DOM document project file XML element to be
       written by QgsProject.

       This, in turn, calls writeXML_(), which is over-rideable by sub-classes so
       that they can write their own specific state to the given DOM node.

       Invoked by QgsProject::write().

       @returns true if successful
    */
    bool writeXML(QDomNode & layer_node, QDomDocument & document);

    /** Copies the symbology settings from another layer. Returns true in case of success */
    virtual bool copySymbologySettings(const QgsMapLayer& other) = 0;

    /** Returns true if this layer can be in the same symbology group with another layer */
    virtual bool isSymbologyCompatible(const QgsMapLayer& other) const = 0;

    /** Accessor for transparency level. */
    unsigned int getTransparency();

    /** Mutator for transparency level. Should be between 0 and 255 */
    void setTransparency(unsigned int);
    
    /**
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     */
    virtual QString errorCaptionString();
  
    /**
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     */
    virtual QString errorString();

    /** Returns layer's spatial reference system */
    const QgsSpatialRefSys& srs();
    
    /** Sets layer's spatial reference system */
    void setSrs(const QgsSpatialRefSys& srs);
    
    
    /** A convenience function to capitalise the layer name */
    static QString capitaliseLayerName(const QString name);
  
public slots:

    /** Event handler for when a coordinate transform fails due to bad vertex error */
    virtual void invalidTransformInput();

    /** Accessor and mutator for the minimum scale member */
    void setMinScale(float theMinScale);
    float minScale();

    /** Accessor and mutator for the maximum scale member */
    void setMaxScale(float theMaxScale);
    float maxScale();

    /** Accessor and mutator for the scale based visilibility flag */
    void setScaleBasedVisibility( bool theVisibilityFlag);
    bool scaleBasedVisibility();

signals:

    /** Emit a signal to notify of a progress event */
    void drawingProgress(int theProgress, int theTotalSteps);

    /** Emit a signal with status (e.g. to be caught by QgisApp and display a msg on status bar) */
    void setStatus(QString theStatusQString);
    
    /** Emit a signal that layer name has been changed */
    void layerNameChanged();

    /** This signal should be connected with the slot QgsMapCanvas::refresh()
     * @TODO: to be removed - GUI dependency
     */
    void repaintRequested();

    /** This is used to send a request that any mapcanvas using this layer update its extents */
    void recalculateExtents();

protected:

    /** called by readXML(), used by children to read state specific to them from
        project files.
    */
    virtual bool readXML_( QDomNode & layer_node );

    /** called by writeXML(), used by children to write state specific to them to
        project files.
    */
    virtual bool writeXML_( QDomNode & layer_node, QDomDocument & document );

    /** debugging member - invoked when a connect() is made to this object */
    void connectNotify( const char * signal );

    /** Transparency level for this layer should be 0-255 (255 being opaque) */
    unsigned int mTransparencyLevel;
  
    /** Extent of the layer */
    QgsRect mLayerExtent;

    /** Indicates if the layer is valid and can be drawn */
    bool mValid;

    /** data source description string, varies by layer type */
    QString mDataSource;

    /** Name of the layer - used for display */
    QString mLayerName;

    /** layer's Spatial reference system */
    QgsSpatialRefSys* mSRS;

private:

    /** private copy constructor - QgsMapLayer not copyable */
    QgsMapLayer( QgsMapLayer const & );

    /** private assign operator - QgsMapLayer not copyable */
    QgsMapLayer & operator=( QgsMapLayer const & );

    /** Unique ID of this layer - used to refer to this layer in map layer registry */
    QString mID;

    /** Type of the layer (eg. vector, raster) */
    int mLayerType;

    /** Tag for embedding additional information */
    QString mTag;

    /** Minimum scale at which this layer should be displayed */
    float mMinScale;
    /** Maximum scale at which this layer should be displayed */
    float mMaxScale;
    /** A flag that tells us whether to use the above vars to restrict layer visibility */
    bool mScaleBasedVisibility;

};

#endif
