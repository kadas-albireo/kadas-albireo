/***************************************************************************
               QgsCoordinateTransform.h  - Coordinate Transforms
                             -------------------
    begin                : Dec 2004
    copyright            : (C) 2004 Tim Sutton
    email                : tim at linfiniti.com
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
#ifndef QGSCOORDINATETRANSFORM_H
#define QGSCOORDINATETRANSFORM_H

//qt includes
#include <QObject>

//qgis includes
#include "qgspoint.h"
#include "qgsrectangle.h"
#include "qgscsexception.h"
#include "qgscoordinatereferencesystem.h"
class QDomNode;
class QDomDocument;

//non qt includes
#include <iostream>
#include <vector>

typedef void* projPJ;
class QString;

/** \ingroup core
* Class for doing transforms between two map coordinate systems.
*
* This class can convert map coordinates to a different coordinate reference system.
* It is normally associated with a map layer and is used to transform between the
* layer's coordinate system and the coordinate system of the map canvas, although
* it can be used in a more general sense to transform coordinates.
*
* All references to source and destination coordinate systems refer to
* layer and map canvas respectively. All operations are from the perspective
* of the layer. For example, a forward transformation transforms coordinates from the
* layers coordinate system to the map canvas.
*/
class CORE_EXPORT QgsCoordinateTransform: public QObject
{
    Q_OBJECT
  public:
    /*! Default constructor. Make sure you use initialised() manually if you use this one! */
    QgsCoordinateTransform() ;

    /** Constructs a QgsCoordinateTransform using QgsCoordinateReferenceSystem objects.
    * @param theSource CRS, typically of the layer's coordinate system
    * @param theDest CRS, typically of the map canvas coordinate system
    */
    QgsCoordinateTransform( const QgsCoordinateReferenceSystem& theSource,
                            const QgsCoordinateReferenceSystem& theDest );

    /** Constructs a QgsCoordinateTransform using CRS ID of source and destination CRS */
    QgsCoordinateTransform( long theSourceSrsId, long theDestSrsId );

    /*!
     * Constructs a QgsCoordinateTransform using the Well Known Text representation
     * of the layer and map canvas coordinate systems
     * @param theSourceWkt Wkt, typically of the layer's coordinate system
     * @param theDestWkt Wkt, typically of the map canvas coordinate system
     */
    QgsCoordinateTransform( QString theSourceWkt, QString theDestWkt );

    /*!
     * Constructs a QgsCoordinateTransform using a Spatial Reference Id
     * of the layer and map canvas coordinate system as Wkt
     * @param theSourceSrid Spatial Ref Id of the layer's coordinate system
     * @param theSourceWkt Wkt of the map canvas coordinate system
     * @param theSourceCRSType On of the enum members defined in QgsCoordinateReferenceSystem::CrsType
     */
    QgsCoordinateTransform( long theSourceSrid,
                            QString theDestWkt,
                            QgsCoordinateReferenceSystem::CrsType theSourceCRSType = QgsCoordinateReferenceSystem::PostgisCrsId );

    //! destructor
    ~QgsCoordinateTransform();

    //! Enum used to indicate the direction (forward or inverse) of the transform
    enum TransformDirection
    {
      ForwardTransform,     /*!< Transform from source to destination CRS. */
      ReverseTransform      /*!< Transform from destination to source CRS. */
    };

    /*!
     * Set the source (layer) QgsCoordinateReferenceSystem
     * @param theCRS QgsCoordinateReferenceSystem representation of the layer's coordinate system
     */
    void setSourceCrs( const QgsCoordinateReferenceSystem& theCRS );

    /*!
     * Mutator for dest QgsCoordinateReferenceSystem
     * @param theCRS of the destination coordinate system
     */
    void setDestCRS( const QgsCoordinateReferenceSystem& theCRS );

    /*!
     * Get the QgsCoordinateReferenceSystem representation of the layer's coordinate system
     * @return QgsCoordinateReferenceSystem of the layer's coordinate system
     */
    QgsCoordinateReferenceSystem& sourceCrs() { return mSourceCRS; }

    /*!
     * Get the QgsCoordinateReferenceSystem representation of the map canvas coordinate system
     * @return QgsCoordinateReferenceSystem of the map canvas coordinate system
     */
    QgsCoordinateReferenceSystem& destCRS() { return mDestCRS; }

    /*! Transform the point from Source Coordinate System to Destination Coordinate System
    * If the direction is ForwardTransform then coordinates are transformed from layer CS --> map canvas CS,
    * otherwise points are transformed from map canvas CS to layerCS.
    * @param p Point to transform
    * @param direction TransformDirection (defaults to ForwardTransform)
    * @return QgsPoint in Destination Coordinate System
     */
    QgsPoint transform( const QgsPoint p, TransformDirection direction = ForwardTransform ) const;

    /*! Transform the point specified by x,y from Source Coordinate System to Destination Coordinate System
    * If the direction is ForwardTransform then coordinates are transformed from layer CS --> map canvas CS,
    * otherwise points are transformed from map canvas CS to layerCS.
    * @param x x cordinate of point to transform
    * @param y y coordinate of point to transform
    * @param direction TransformDirection (defaults to ForwardTransform)
    * @return QgsPoint in Destination Coordinate System
     */
    QgsPoint transform( const double x, const double y, TransformDirection direction = ForwardTransform ) const;

    /*! Transform a QgsRectangle to the dest Coordinate system
    * If the direction is ForwardTransform then coordinates are transformed from layer CS --> map canvas CS,
    * otherwise points are transformed from map canvas CS to layerCS.
    * It assumes that rect is a bounding box, and creates a bounding box
    * in the proejcted CS, so that all points in source rectangle is within
    * returned rectangle.
    * @param QgsRectangle rect to transform
    * @param direction TransformDirection (defaults to ForwardTransform)
    * @return QgsRectangle in Destination Coordinate System
     */
    QgsRectangle transformBoundingBox( const QgsRectangle theRect, TransformDirection direction = ForwardTransform ) const;

    // Same as for the other transform() functions, but alters the x
    // and y variables in place. The second one works with good old-fashioned
    // C style arrays.
    void transformInPlace( double& x, double& y, double &z, TransformDirection direction = ForwardTransform ) const;

    void transformInPlace( std::vector<double>& x, std::vector<double>& y, std::vector<double>& z,
                           TransformDirection direction = ForwardTransform ) const;

    /*! Transform a QgsRectangle to the dest Coordinate system
    * If the direction is ForwardTransform then coordinates are transformed from layer CS --> map canvas CS,
    * otherwise points are transformed from map canvas CS to layerCS.
    * @param QgsRectangle rect to transform
    * @param direction TransformDirection (defaults to ForwardTransform)
    * @return QgsRectangle in Destination Coordinate System
     */
    QgsRectangle transform( const QgsRectangle theRect, TransformDirection direction = ForwardTransform ) const;

    /*! Transform an array of coordinates to a different Coordinate System
    * If the direction is ForwardTransform then coordinates are transformed from layer CS --> map canvas CS,
    * otherwise points are transformed from map canvas CS to layerCS.
    * @param x x cordinate of point to transform
    * @param y y coordinate of point to transform
    * @param direction TransformDirection (defaults to ForwardTransform)
    * @return QgsRectangle in Destination Coordinate System
     */
    void transformCoords( const int &numPoint, double *x, double *y, double *z, TransformDirection direction = ForwardTransform ) const;

    /*!
     * Flag to indicate whether the coordinate systems have been initialised
     * @return true if initialised, otherwise false
     */
    bool isInitialised() const {return mInitialisedFlag;};

    /*! See if the transform short circuits because src and dest are equivalent
     * @return bool True if it short circuits
     */
    bool isShortCircuited() {return mShortCircuit;};

    /*! Change the destination coordinate system by passing it a qgis srsid
    * A QGIS srsid is a unique key value to an entry on the tbl_srs in the
    * srs.db sqlite database.
    * @note This slot will usually be called if the
    * project properties change and a different coordinate system is
    * selected.
    * @note This coord transform will be reinitialised when this slot is called
    * to check if short circuiting is needed or not etc.
    * @param theCRSID -  A long representing the srsid of the srs to be used */
    void setDestCRSID( long theCRSID );

  public slots:
    //!initialise is used to actually create the Transformer instance
    void initialise();

    /*! Restores state from the given Dom node.
    * @param theNode The node from which state will be restored
    * @return bool True on success, False on failure
    */
    bool readXML( QDomNode & theNode );

    /*! Stores state to the given Dom node in the given document
    * @param theNode The node in which state will be restored
    * @param theDom The document in which state will be stored
    * @return bool True on success, False on failure
    */
    bool writeXML( QDomNode & theNode, QDomDocument & theDoc );

  signals:
    /** Signal when an invalid pj_transform() has occured */
    void  invalidTransformInput() const;

  private:

    /*!
     * Flag to indicate that the source and destination coordinate systems are
     * equal and not transformation needs to be done
     */
    bool mShortCircuit;

    /*!
     * flag to show whether the transform is properly initialised or not
     */
    bool mInitialisedFlag;

    /*!
     * QgsCoordinateReferenceSystem of the source (layer) coordinate system
     */
    QgsCoordinateReferenceSystem mSourceCRS;

    /*!
     * QgsCoordinateReferenceSystem of the destination (map canvas) coordinate system
     */
    QgsCoordinateReferenceSystem mDestCRS;

    /*!
     * Proj4 data structure of the source projection (layer coordinate system)
     */
    projPJ mSourceProjection;

    /*!
     * Proj4 data structure of the destination projection (map canvas coordinate system)
     */
    projPJ mDestinationProjection;

    /*!
     * Finder for PROJ grid files.
     */
    void setFinder();
};

//! Output stream operator
inline std::ostream& operator << ( std::ostream& os, const QgsCoordinateTransform &r )
{
  QString mySummary( "\n%%%%%%%%%%%%%%%%%%%%%%%%\nCoordinate Transform def begins:" );
  mySummary += "\n\tInitialised? : ";
  //prevent warnings
  if ( r.isInitialised() )
  {
    //do nothing this is a dummy
  }
  /*
    if (r.isInitialised())
    {
      mySummary += "Yes";
    }
    else
    {
      mySummary += "No" ;
    }
    mySummary += "\n\tShort Circuit?  : " ;
    if (r.isShortCircuited())
    {
      mySummary += "Yes";
    }
    else
    {
      mySummary += "No" ;
    }

    mySummary += "\n\tSource Spatial Ref Sys  : ";
    if (r.sourceCrs())
    {
      mySummary << r.sourceCrs();
    }
    else
    {
      mySummary += "Undefined" ;
    }

    mySummary += "\n\tDest Spatial Ref Sys  : " ;
    if (r.destCRS())
    {
      mySummary << r.destCRS();
    }
    else
    {
      mySummary += "Undefined" ;
    }
  */
  mySummary += ( "\nCoordinate Transform def ends \n%%%%%%%%%%%%%%%%%%%%%%%%\n" );
  return os << mySummary.toLocal8Bit().data() << std::endl;
}


#endif // QGSCOORDINATETRANSFORM_H
