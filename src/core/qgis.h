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

#ifndef QGIS_H
#define QGIS_H

#include <QEvent>
#include <QString>
#include <QMetaType>
#include <cfloat>
#include <cmath>
#include <qnumeric.h>

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
    // The development version
    static const char* QGIS_DEV_VERSION;

    // Enumerations
    //

    //! Used for symbology operations
    // Feature types
    enum WkbType
    {
      WKBUnknown = 0,
      WKBPoint = 1,
      WKBLineString,
      WKBPolygon,
      WKBMultiPoint,
      WKBMultiLineString,
      WKBMultiPolygon,
      WKBNoGeometry = 100, //attributes only
      WKBPoint25D = 0x80000001,
      WKBLineString25D,
      WKBPolygon25D,
      WKBMultiPoint25D,
      WKBMultiLineString25D,
      WKBMultiPolygon25D,
    };

    enum GeometryType
    {
      Point,
      Line,
      Polygon,
      UnknownGeometry,
      NoGeometry
    };

    // String representation of geometry types (set in qgis.cpp)
    static const char *qgisVectorGeometryType[];

    //! description strings for feature types
    static const char *qgisFeatureTypes[];

    /** Map units that qgis supports
     * @note that QGIS < 1.4 api had only Meters, Feet, Degrees and UnknownUnit
     * @note and QGIS >1.8 returns to that
     */
    enum UnitType
    {
      Meters = 0,
      Feet = 1,
      Degrees = 2, //for 1.0 api backwards compatibility
      UnknownUnit = 3,

      // for [1.4;1.8] api compatibility
      DecimalDegrees = 2,         // was 2
      DegreesMinutesSeconds = 2,  // was 4
      DegreesDecimalMinutes = 2,  // was 5
    };

    // Provides the canonical name of the type value
    static QString toLiteral( QGis::UnitType unit );
    // Converts from the canonical name to the type value
    static UnitType fromLiteral( QString  literal, QGis::UnitType defaultType = UnknownUnit );
    // Provides translated version of the type value
    static QString tr( QGis::UnitType unit );

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

  private:
    // String representation of unit types (set in qgis.cpp)
    static const char *qgisUnitTypes[];

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

//
// compare two doubles (but allow some difference)
//
inline bool doubleNear( double a, double b, double epsilon = 4 * DBL_EPSILON )
{
  const double diff = a - b;
  return diff > -epsilon && diff <= epsilon;
}

/** Wkt string that represents a geographic coord sys
 * @note added in 1.8 to replace GEOWkt
 */
const QString GEOWKT =
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
/** Wkt string that represents a geographic coord sys
 * @note deprecated in 1.8 due to violation of coding conventions (globals
 *  should be in all caps).
 */
#ifndef _MSC_VER
Q_DECL_DEPRECATED
#endif
const QString GEOWkt = GEOWKT;

const QString PROJECT_SCALES =
  "1:1000000,1:500000,1:250000,1:100000,1:50000,1:25000,"
  "1:10000,1:5000,1:2500,1:1000,1:500";

/** PROJ4 string that represents a geographic coord sys */
extern CORE_EXPORT const QString GEOPROJ4;
/** Magic number for a geographic coord sys in POSTGIS SRID */
const long GEOSRID = 4326;
/** Magic number for a geographic coord sys in QGIS srs.db tbl_srs.srs_id */
const long GEOCRS_ID = 3452;
/** Magic number for a geographic coord sys in EpsgCrsId ID format */
const long GEO_EPSG_CRS_ID = 4326;
/** Geographic coord sys from EPSG authority */
const QString GEO_EPSG_CRS_AUTHID = "EPSG:4326";
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

/** default snapping tolerance for segments (@note added in 1.8) */
const double DEFAULT_SEGMENT_EPSILON = 1e-8;

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
