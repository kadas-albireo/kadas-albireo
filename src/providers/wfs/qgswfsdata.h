/***************************************************************************
     qgswfsdata.h
     --------------------------------------
    Date                 : Sun Sep 16 12:19:55 AKDT 2007
    Copyright            : (C) 2007 by Gary E. Sherman
    Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSWFSDATA_H
#define QGSWFSDATA_H

#include <QHttp>
#include <expat.h>
#include "qgis.h"
#include "qgsapplication.h"
#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgspoint.h"
#include <list>
#include <set>
#include <stack>
class QgsRectangle;
class QgsCoordinateReferenceSystem;


/**This class reads data from a WFS server or alternatively from a GML file. It uses the expat XML parser and an event based model to keep performance high. The parsing starts when the first data arrives, it does not wait until the request is finished*/
class QgsWFSData: public QObject
{
    Q_OBJECT
  public:
    QgsWFSData(
      const QString& uri,
      QgsRectangle* extent,
      QgsCoordinateReferenceSystem* srs,
      QList<QgsFeature*> &features,
      const QString& geometryAttribute,
      const QSet<QString>& thematicAttributes,
      QGis::WkbType* wkbType );
    ~QgsWFSData();

    /**Does the Http GET request to the wfs server
       @param query string (to define the requested typename)
       @param extent the extent of the WFS layer
       @param srs the reference system of the layer
       @param features the features of the layer
    @return 0 in case of success*/
    int getWFSData();
    /**Returns a pointer to the internal QHttp object (mainly for the purpose of making singal/slot connections*/
    const QHttp* http() const {return &mHttp;}

  private slots:
    void setFinished( bool error );

  private:

    enum parseMode
    {
      boundingBox,
      featureMember,
      attribute,
      geometry,
      coordinate,
      point,
      line,
      polygon,
      multiPoint,
      multiLine,
      multiPolygon
    };

    QgsWFSData();

    /**XML handler methods*/
    void startElement( const XML_Char* el, const XML_Char** attr );
    void endElement( const XML_Char* el );
    void characters( const XML_Char* chars, int len );
    static void start( void* data, const XML_Char* el, const XML_Char** attr )
    {
      static_cast<QgsWFSData*>( data )->startElement( el, attr );
    }
    static void end( void* data, const XML_Char* el )
    {
      static_cast<QgsWFSData*>( data )->endElement( el );
    }
    static void chars( void* data, const XML_Char* chars, int len )
    {
      static_cast<QgsWFSData*>( data )->characters( chars, len );
    }

    //helper routines
    /**Reads attribute srsName="EpsgCrsId:..."
       @param epsgNr result
       @param attr attribute strings
       @return 0 in case of success*/
    int readEpsgFromAttribute( int& epsgNr, const XML_Char** attr ) const;
    /**Reads the 'cs' (coordinate separator) attribute.
     @return the cs attribute value or the default value ","*/
    QString readCsFromAttribute( const XML_Char** attr ) const;
    /**Reads the 'ts' (tuple separator) attribute.
       @return the ts attribute value or the devault value " "*/
    QString readTsFromAttribute( const XML_Char** attr ) const;
    /**Creates a rectangle from a coordinate string.
     @return 0 in case of success*/
    int createBBoxFromCoordinateString( QgsRectangle* bb, const QString& coordString ) const;
    /**Creates a set of points from a coordinate string.
       @param points list that will contain the created points
       @param coordString the text containing the coordinates
       @param cs coortinate separator
       @param ts tuple separator
       @return 0 in case of success*/
    int pointsFromCoordinateString( std::list<QgsPoint>& points, const QString& coordString, const QString& cs, const QString& ts ) const;
    int getPointWKB( unsigned char** wkb, int* size, const QgsPoint& ) const;
    int getLineWKB( unsigned char** wkb, int* size, const std::list<QgsPoint>& lineCoordinates ) const;
    int getRingWKB( unsigned char** wkb, int* size, const std::list<QgsPoint>& ringCoordinates ) const;
    /**Creates a multiline from the information in mCurrentWKBFragments and mCurrentWKBFragmentSizes. Assign the result. The multiline is in mCurrentWKB and mCurrentWKBSize. The function deletes the memory in mCurrentWKBFragments. Returns 0 in case of success.*/
    int createMultiLineFromFragments();
    int createMultiPointFromFragments();
    int createPolygonFromFragments();
    int createMultiPolygonFromFragments();
    /**Adds all the integers contained in mCurrentWKBFragmentSizes*/
    int totalWKBFragmentSize() const;

    QString mUri;
    //results are members such that handler routines are able to manipulate them
    /**Bounding box of the layer*/
    QgsRectangle* mExtent;
    /**Source srs of the layer*/
    QgsCoordinateReferenceSystem* mSrs;
    /**The features of the layer*/
    QList<QgsFeature*> &mFeatures;
    /**Name of geometry attribute*/
    QString mGeometryAttribute;
    const QSet<QString> &mThematicAttributes;
    QGis::WkbType* mWkbType;
    /**True if the request is finished*/
    bool mFinished;
    /**The HTTP client object*/
    QHttp mHttp;
    /**Keep track about the most important nested elements*/
    std::stack<parseMode> mParseModeStack;
    /**This contains the character data if an important element has been encountered*/
    QString mStringCash;
    QgsFeature* mCurrentFeature;
    int mFeatureCount;
    /**The total WKB for a feature*/
    unsigned char* mCurrentWKB;
    /**The total WKB size for a feature*/
    int mCurrentWKBSize;
    /**WKB intermediate storage during parsing. For points and lines, no intermediate WKB is stored at all. For multipoins and multilines and polygons, only one nested list is used. For multipolygons, both nested lists are used*/
    std::list< std::list<unsigned char*> > mCurrentWKBFragments;
    /**Similar to mCurrentWKB, but only the size*/
    std::list< std::list<int> > mCurrentWKBFragmentSizes;
    QString mAttributeName;
    /**Index where the current attribute should be inserted*/
    int mAttributeIndex;
    QString mTypeName;
    QgsApplication::endian_t mEndian;
    /**Coordinate separator for coordinate strings. Usually "," */
    QString mCoordinateSeparator;
    /**Tuple separator for coordinate strings. Usually " " */
    QString mTupleSeparator;
};

#endif
