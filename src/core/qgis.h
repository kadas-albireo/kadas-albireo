/***************************************************************************
                          qgis.h - QGIS namespace
                             -------------------
    begin                : Sat Jun 30 2002
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

#ifndef QGIS_H
#define QGIS_H
#include <QEvent>
#include <QString>
/** \ingroup core
 * The QGis class provides global constants for use throughout the application.
 */
class CORE_EXPORT QGis
{
  public:
    // Version constants
    //
    // Version string
    static const char* QGIS_VERSION;
    // Version number used for comparing versions using the "Check QGIS Version" function
    static const int QGIS_VERSION_INT;
    // Release name
    static const char* QGIS_RELEASE_NAME;
    // The subversion version
    static const char* QGIS_SVN_VERSION;

    // Enumerations
    //

    //! Used for symbology operations
    // Feature types
    enum WkbType
    {
      WKBPoint = 1,
      WKBLineString,
      WKBPolygon,
      WKBMultiPoint,
      WKBMultiLineString,
      WKBMultiPolygon,
      WKBUnknown,
      WKBPoint25D = 0x80000001,
      WKBLineString25D,
      WKBPolygon25D,
      WKBMultiPoint25D,
      WKBMultiLineString25D,
      WKBMultiPolygon25D
    };
    enum GeometryType
    {
      Point,
      Line,
      Polygon,
      UnknownGeometry
    };

    // String representation of geometry types (set in qgis.cpp)
    static const char *qgisVectorGeometryType[];

    //! description strings for feature types
    static const char *qgisFeatureTypes[];

    //! map units that qgis supports
    enum UnitType
    {
      Meters,
      Feet,
      Degrees,
      UnknownUnit
    } ;

    //! User defined event types
    enum UserEvent
    {
      // These first two are useful for threads to alert their parent data providers

      //! The extents have been calculated by a provider of a layer
      ProviderExtentCalcEvent = ( QEvent::User + 1 ),

      //! The row count has been calculated by a provider of a layer
      ProviderCountCalcEvent
    };

    static const double DEFAULT_IDENTIFY_RADIUS;
};

// hack to workaround warnings when casting void pointers
// retrieved from QLibrary::resolve to function pointers.
// It's assumed that this works on all systems supporting
// QLibrary
inline void ( *cast_to_fptr( void *p ) )()
{
  union
  {
    void *p;
    void ( *f )();
  } u;

  u.p = p;
  return u.f;
}

/** Wkt string that represents a geographic coord sys */
const  QString GEOWkt =
  "GEOGCS[\"WGS 84\", "
  "  DATUM[\"WGS_1984\", "
  "    SPHEROID[\"WGS 84\",6378137,298.257223563, "
  "      AUTHORITY[\"EPSG\",7030]], "
  "    TOWGS84[0,0,0,0,0,0,0], "
  "    AUTHORITY[\"EPSG\",6326]], "
  "  PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",8901]], "
  "  UNIT[\"DMSH\",0.0174532925199433,AUTHORITY[\"EPSG\",9108]], "
  "  AXIS[\"Lat\",NORTH], "
  "  AXIS[\"Long\",EAST], "
  "  AUTHORITY[\"EPSG\",4326]]";
/** PROJ4 string that represents a geographic coord sys */
const QString GEOPROJ4 = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs";
/** Magic number for a geographic coord sys in POSTGIS SRID */
const long GEOSRID = 4326;
/** Magic number for a geographic coord sys in QGIS srs.db tbl_srs.srs_id */
const long GEOCRS_ID = 3344;
/**  Magic number for a geographic coord sys in EpsgCrsId ID format */
const long GEO_EPSG_CRS_ID = 4326;
/** The length of the string "+proj=" */
const int PROJ_PREFIX_LEN = 6;
/** The length of the string "+ellps=" */
const int ELLPS_PREFIX_LEN = 7;
/** The length of the string "+lat_1=" */
const int LAT_PREFIX_LEN = 7;
/** Magick number that determines whether a projection crsid is a system (srs.db)
 *  or user (~/.qgis.qgis.db) defined projection. */
const int USER_CRS_START_ID = 100000;

//
// Constants for point symbols
//

/** Magic number that determines the minimum allowable point size for point symbols */
const double MINIMUM_POINT_SIZE = 0.1;
/** Magic number that determines the default point size for point symbols */
const double DEFAULT_POINT_SIZE = 2.0;
const double DEFAULT_LINE_WIDTH = 0.26;

// FIXME: also in qgisinterface.h
#ifndef QGISEXTERN
#ifdef WIN32
#  define QGISEXTERN extern "C" __declspec( dllexport )
#  ifdef _MSC_VER
// do not warn about C bindings returing QString
#    pragma warning(disable:4190)
#  endif
#else
#  define QGISEXTERN extern "C"
#endif
#endif

#endif
