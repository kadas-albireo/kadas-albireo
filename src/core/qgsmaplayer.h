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

#include "qgsrectangle.h"

class QgsRenderContext;
class QgsCoordinateReferenceSystem;

class QDomNode;
class QDomDocument;
class QKeyEvent;
class QPainter;

/** \ingroup core
 * Base class for all map layer types.
 * This is the base class for all map layer types (vector, raster).
 */
class CORE_EXPORT QgsMapLayer : public QObject
{
    Q_OBJECT

  public:
    /** Layers enum defining the types of layers that can be added to a map */
    enum LayerType
    {
      VectorLayer,
      RasterLayer
    };

    /** Constructor
     * @param type Type of layer as defined in QgsMapLayer::LayerType enum
     * @param lyrname Display Name of the layer
     */
    QgsMapLayer( QgsMapLayer::LayerType type = VectorLayer, QString lyrname = QString::null, QString source = QString::null );

    /** Destructor */
    virtual ~QgsMapLayer();

    /** Get the type of the layer
     * @return Integer matching a value in the QgsMapLayer::LayerType enum
     */
    QgsMapLayer::LayerType type() const;

    /** Get this layer's unique ID, this ID is used to access this layer from map layer registry */
    QString getLayerID() const;

    /** Set the display name of the layer
     * @param name New name for the layer
     */
    void setLayerName( const QString & name );

    /** Get the display name of the layer
     * @return the layer name
     */
    QString const & name() const;

    virtual bool draw( QgsRenderContext& rendererContext );

    /** Draw labels
     * @TODO to be removed: used only in vector layers
     */
    virtual void drawLabels( QgsRenderContext& rendererContext );

    /** Return the extent of the layer as a QRect */
    QgsRectangle extent() const;

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
    virtual void setLayerOrder( QStringList layers );

    /** Set the visibility of the given sublayer name */
    virtual void setSubLayerVisibility( QString name, bool vis );


    /** True if the layer can be edited */
    virtual bool isEditable() const = 0;

    /** sets state from Dom document
       @param layer_node is Dom node corresponding to ``maplayer'' tag
       @note

       The Dom node corresponds to a Dom document project file XML element read
       by QgsProject.

       This, in turn, calls readXml(), which is over-rideable by sub-classes so
       that they can read their own specific state from the given Dom node.

       Invoked by QgsProject::read().

       @returns true if successful
     */
    bool readXML( QDomNode & layer_node );


    /** stores state in Dom node
       @param layer_node is Dom node corresponding to ``projectlayers'' tag
       @note

       The Dom node corresponds to a Dom document project file XML element to be
       written by QgsProject.

       This, in turn, calls writeXml(), which is over-rideable by sub-classes so
       that they can write their own specific state to the given Dom node.

       Invoked by QgsProject::write().

       @returns true if successful
    */
    bool writeXML( QDomNode & layer_node, QDomDocument & document );

    /** Copies the symbology settings from another layer. Returns true in case of success */
    virtual bool copySymbologySettings( const QgsMapLayer& other ) = 0;

    /** Returns true if this layer can be in the same symbology group with another layer */
    virtual bool hasCompatibleSymbology( const QgsMapLayer& other ) const = 0;

    /** Accessor for transparency level. */
    unsigned int getTransparency();

    /** Mutator for transparency level. Should be between 0 and 255 */
    void setTransparency( unsigned int );

    /**
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     */
    virtual QString lastErrorTitle();

    /**
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     */
    virtual QString lastError();

    /** Returns layer's spatial reference system */
    const QgsCoordinateReferenceSystem& srs();

    /** Sets layer's spatial reference system */
    void setCrs( const QgsCoordinateReferenceSystem& srs );


    /** A convenience function to capitalise the layer name */
    static QString capitaliseLayerName( const QString name );

    /** Retrieve the default style for this layer if one
     * exists (either as a .qml file on disk or as a
     * record in the users style table in their personal qgis.db)
     * @param a reference to a flag that will be set to false if
     * we did not manage to load the default style.
     * @return a QString with any status messages
     * @see also loadNamedStyle ();
     */
    virtual QString loadDefaultStyle( bool & theResultFlag );

    /** Retrieve a named style for this layer if one
     * exists (either as a .qml file on disk or as a
     * record in the users style table in their personal qgis.db)
     * @param QString theURI - the file name or other URI for the
     * style file. First an attempt will be made to see if this
     * is a file and load that, if that fails the qgis.db styles
     * table will be consulted to see if there is a style who's
     * key matches the URI.
     * @param a reference to a flag that will be set to false if
     * we did not manage to load the default style.
     * @return a QString with any status messages
     * @see also loadDefaultStyle ();
     */
    virtual QString loadNamedStyle( const QString theURI, bool & theResultFlag );

    virtual bool loadNamedStyleFromDb( const QString db, const QString theURI, QString &qml );

    /** Save the properties of this layer as the default style
     * (either as a .qml file on disk or as a
     * record in the users style table in their personal qgis.db)
     * @param a reference to a flag that will be set to false if
     * we did not manage to save the default style.
     * @return a QString with any status messages
     * @see also loadNamedStyle () and saveNamedStyle()
     */
    virtual QString saveDefaultStyle( bool & theResultFlag );

    /** Save the properties of this layer as a named style
     * (either as a .qml file on disk or as a
     * record in the users style table in their personal qgis.db)
     * @param QString theURI - the file name or other URI for the
     * style file. First an attempt will be made to see if this
     * is a file and save to that, if that fails the qgis.db styles
     * table will be used to create a style entry who's
     * key matches the URI.
     * @param a reference to a flag that will be set to false if
     * we did not manage to save the default style.
     * @return a QString with any status messages
     * @see also saveDefaultStyle ();
     */
    virtual QString saveNamedStyle( const QString theURI, bool & theResultFlag );

    /** Read the symbology for the current layer from the Dom node supplied.
     * @param QDomNode node that will contain the symbology definition for this layer.
     * @param errorMessage reference to string that will be updated with any error messages
     * @return true in case of success.
    */
    virtual bool readSymbology( const QDomNode& node, QString& errorMessage ) = 0;

    /** Write the symbology for the layer into the docment provided.
     *  @param QDomNode the node that will have the style element added to it.
     *  @param QDomDocument the document that will have the QDomNode added.
     * @param errorMessage reference to string that will be updated with any error messages
     *  @return true in case of success.
     */
    virtual bool writeSymbology( QDomNode&, QDomDocument& doc, QString& errorMessage ) const = 0;

  public slots:

    /** Event handler for when a coordinate transform fails due to bad vertex error */
    virtual void invalidTransformInput();

    /** Accessor and mutator for the minimum scale member */
    void setMinimumScale( float theMinScale );
    float minimumScale();

    /** Accessor and mutator for the maximum scale member */
    void setMaximumScale( float theMaxScale );
    float maximumScale();

    /** Accessor and mutator for the scale based visilibility flag */
    void toggleScaleBasedVisibility( bool theVisibilityFlag );
    bool hasScaleBasedVisibility();

  signals:

    /** Emit a signal to notify of a progress event */
    void drawingProgress( int theProgress, int theTotalSteps );

    /** Emit a signal with status (e.g. to be caught by QgisApp and display a msg on status bar) */
    void statusChanged( QString theStatus );

    /** Emit a signal that layer name has been changed */
    void layerNameChanged();

    /** This signal should be connected with the slot QgsMapCanvas::refresh()
     * @TODO: to be removed - GUI dependency
     */
    void repaintRequested();

    /**The layer emits this signal when a screen update is requested.
     This signal should be connected with the slot QgsMapCanvas::updateMap()*/
    void screenUpdateRequested();

    /** This is used to send a request that any mapcanvas using this layer update its extents */
    void recalculateExtents();

  protected:

    /** called by readXML(), used by children to read state specific to them from
        project files.
    */
    virtual bool readXml( QDomNode & layer_node );

    /** called by writeXML(), used by children to write state specific to them to
        project files.
    */
    virtual bool writeXml( QDomNode & layer_node, QDomDocument & document );

    /** debugging member - invoked when a connect() is made to this object */
    void connectNotify( const char * signal );

    /** Transparency level for this layer should be 0-255 (255 being opaque) */
    unsigned int mTransparencyLevel;

    /** Extent of the layer */
    QgsRectangle mLayerExtent;

    /** Indicates if the layer is valid and can be drawn */
    bool mValid;

    /** data source description string, varies by layer type */
    QString mDataSource;

    /** Name of the layer - used for display */
    QString mLayerName;

    /** layer's Spatial reference system */
    QgsCoordinateReferenceSystem* mCRS;

  private:

    /** private copy constructor - QgsMapLayer not copyable */
    QgsMapLayer( QgsMapLayer const & );

    /** private assign operator - QgsMapLayer not copyable */
    QgsMapLayer & operator=( QgsMapLayer const & );

    /** Unique ID of this layer - used to refer to this layer in map layer registry */
    QString mID;

    /** Type of the layer (eg. vector, raster) */
    QgsMapLayer::LayerType mLayerType;

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
