/***************************************************************************
 *  qgscoordinateconverter.cpp                                             *
 *  -------------------                                                    *
 *  begin                : Jul 13, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscoordinateconverter.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgspoint.h"
#include <qmath.h>

QgsEPSGCoordinateConverter::QgsEPSGCoordinateConverter( const QString &targetEPSG, QObject *parent )
    : QgsCoordinateConverter( parent )
{
  mDestSrs = new QgsCoordinateReferenceSystem( targetEPSG );
}

QgsEPSGCoordinateConverter::~QgsEPSGCoordinateConverter()
{
  delete mDestSrs;
}

QString QgsEPSGCoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs, int prec ) const
{
  QgsPoint pOut = QgsCoordinateTransform( sSrs, *mDestSrs ).transform( p );
  return QString( "%1, %2" ).arg( pOut.x(), 0, 'f', prec ).arg( pOut.y(), 0, 'f', prec );
}

///////////////////////////////////////////////////////////////////////////////

QgsWGS84CoordinateConverter::QgsWGS84CoordinateConverter( Format format, QObject *parent )
    : QgsCoordinateConverter( parent )
{
  mFormat = format;
  mDestSrs = new QgsCoordinateReferenceSystem( "EPSG:4326" );
}

QgsWGS84CoordinateConverter::~QgsWGS84CoordinateConverter()
{
  delete mDestSrs;
}

QString QgsWGS84CoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs, int prec ) const
{
  QgsPoint pOut = QgsCoordinateTransform( sSrs, *mDestSrs ).transform( p );
  switch ( mFormat )
  {
    case DegMinSec:
    {
      return pOut.toDegreesMinutesSeconds( prec );
    }
    case DegMin:
    {
      return pOut.toDegreesMinutes( prec );
    }
    case DecDeg:
    {
      return QString( "%1%2, %3%4" ).arg( pOut.x(), 0, 'f', prec ).arg( QChar( 176 ) )
             .arg( pOut.y(), 0, 'f', prec ).arg( QChar( 176 ) );
    }
  }
  return "";
}

///////////////////////////////////////////////////////////////////////////////

QgsUTMCoordinateConverter::QgsUTMCoordinateConverter( QObject *parent )
    : QgsCoordinateConverter( parent )
{
  mWgs84Srs = new QgsCoordinateReferenceSystem( "EPSG:4326" );
}

QgsUTMCoordinateConverter::~QgsUTMCoordinateConverter()
{
  delete mWgs84Srs;
}

QString QgsUTMCoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs, int /*prec*/ ) const
{
  UTMCoo coo = getUTMCoordinate( p, sSrs );
  return QString( "%1 %2 %3mE %4mN" ).arg( coo.zoneNumber ).arg( coo.zoneLetter ).arg( coo.easting ).arg( coo.northing );
}

QgsUTMCoordinateConverter::UTMCoo QgsUTMCoordinateConverter::getUTMCoordinate( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs ) const
{
  QgsPoint pLatLong = QgsCoordinateTransform( sSrs, *mWgs84Srs ).transform( p );

  const double a = 6378137.0; //ellip.radius;
  const double eccSqr = 0.00669438; //ellip.eccsq;
  const double k0 = 0.9996;

  double Long = pLatLong.x();
  double Lat = pLatLong.y();
  double LatRad = Lat / 180. * M_PI;
  double LongRad = Long / 180. * M_PI;
  int ZoneNumber = qFloor(( Long + 180. ) / 6. ) + 1;

  //Make sure the longitude 180.00 is in Zone 60
  if ( Long >= 180.0 )
  {
    ZoneNumber = 60;
  }

  // Special zone for Norway
  if ( Lat >= 56.0 && Lat < 64.0 && Long >= 3.0 && Long < 12.0 )
  {
    ZoneNumber = 32;
  }

  // Special zones for Svalbard
  if ( Lat >= 72.0 && Lat < 84.0 )
  {
    if ( Long >= 0.0 && Long < 9.0 )
    {
      ZoneNumber = 31;
    }
    else if ( Long >= 9.0 && Long < 21.0 )
    {
      ZoneNumber = 33;
    }
    else if ( Long >= 21.0 && Long < 33.0 )
    {
      ZoneNumber = 35;
    }
    else if ( Long >= 33.0 && Long < 42.0 )
    {
      ZoneNumber = 37;
    }
  }

  //+3 puts origin in middle of zone
  double LongOriginRad = (( ZoneNumber - 1 ) * 6 - 180 + 3 ) / 180. * M_PI;

  double eccPrimeSquared = ( eccSqr ) / ( 1 - eccSqr );

  double N = a / qSqrt( 1 - eccSqr * qSin( LatRad ) * qSin( LatRad ) );
  double T = qTan( LatRad ) * qTan( LatRad );
  double C = eccPrimeSquared * qCos( LatRad ) * qCos( LatRad );
  double A = qCos( LatRad ) * ( LongRad - LongOriginRad );
  double M = a * (
               ( 1 - eccSqr / 4 - 3 * eccSqr * eccSqr / 64 - 5 * eccSqr * eccSqr * eccSqr / 256 ) * LatRad -
               ( 3 * eccSqr / 8 + 3 * eccSqr * eccSqr / 32 + 45 * eccSqr * eccSqr * eccSqr / 1024 ) * qSin( 2 * LatRad ) +
               ( 15 * eccSqr * eccSqr / 256 + 45 * eccSqr * eccSqr * eccSqr / 1024 ) * qSin( 4 * LatRad ) -
               ( 35 * eccSqr * eccSqr * eccSqr / 3072 ) * qSin( 6 * LatRad ) );

  UTMCoo coo;

  coo.easting = ( k0 * N * ( A + ( 1 - T + C ) * A * A * A / 6.0 + ( 5 - 18 * T + T * T + 72 * C - 58 * eccPrimeSquared ) * A * A * A * A * A / 120.0 ) + 500000.0 );

  coo.northing = ( k0 * ( M + N * qTan( LatRad ) * ( A * A / 2 + ( 5 - T + 9 * C + 4 * C * C ) * A * A * A * A / 24.0 + ( 61 - 58 * T + T * T + 600 * C - 330 * eccPrimeSquared ) * A * A * A * A * A * A / 720.0 ) ) );
  if ( Lat < 0.0 )
  {
    coo.northing += 10000000.0; //10000000 meter offset for southern hemisphere
  }

  coo.zoneNumber = ZoneNumber;
  coo.zoneLetter = getHemisphereLetter( Lat );
  return coo;
}

QString QgsUTMCoordinateConverter::getHemisphereLetter( double lat ) const
{
  if (( 84 >= lat ) && ( lat >= 72 ) )
    return "X";
  else if (( 72 > lat ) && ( lat >= 64 ) )
    return "W";
  else if (( 64 > lat ) && ( lat >= 56 ) )
    return "V";
  else if (( 56 > lat ) && ( lat >= 48 ) )
    return "U";
  else if (( 48 > lat ) && ( lat >= 40 ) )
    return "T";
  else if (( 40 > lat ) && ( lat >= 32 ) )
    return "S";
  else if (( 32 > lat ) && ( lat >= 24 ) )
    return "R";
  else if (( 24 > lat ) && ( lat >= 16 ) )
    return "Q";
  else if (( 16 > lat ) && ( lat >= 8 ) )
    return "P";
  else if (( 8 > lat ) && ( lat >= 0 ) )
    return "N";
  else if (( 0 > lat ) && ( lat >= -8 ) )
    return "M";
  else if (( -8 > lat ) && ( lat >= -16 ) )
    return "L";
  else if (( -16 > lat ) && ( lat >= -24 ) )
    return "K";
  else if (( -24 > lat ) && ( lat >= -32 ) )
    return "J";
  else if (( -32 > lat ) && ( lat >= -40 ) )
    return "H";
  else if (( -40 > lat ) && ( lat >= -48 ) )
    return "G";
  else if (( -48 > lat ) && ( lat >= -56 ) )
    return "F";
  else if (( -56 > lat ) && ( lat >= -64 ) )
    return "E";
  else if (( -64 > lat ) && ( lat >= -72 ) )
    return "D";
  else if (( -72 > lat ) && ( lat >= -80 ) )
    return "C";

  //This is an error flag to show that the Latitude is outside MGRS limits
  return "Z";
}

///////////////////////////////////////////////////////////////////////////////

const int QgsMGRSCoordinateConverter::NUM_100K_SETS = 6;
const QString QgsMGRSCoordinateConverter::SET_ORIGIN_COLUMN_LETTERS = "AJSAJS";
const QString QgsMGRSCoordinateConverter::SET_ORIGIN_ROW_LETTERS = "AFAFAF";

QString QgsMGRSCoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs, int prec ) const
{
  UTMCoo utm = getUTMCoordinate( p, sSrs );

  int setParm = utm.zoneNumber % NUM_100K_SETS;
  if ( setParm == 0 )
  {
    setParm = NUM_100K_SETS;
  }

  int setColumn = qFloor( utm.easting / 100000. );
  int setRow = int( qFloor( utm.northing / 100000. ) ) % 20;
  QString letter100kID = getLetter100kID( setColumn, setRow, setParm );
  QString seasting = QString::number( utm.easting );
  QString snorthing = QString::number( utm.northing );

  if ( letter100kID.isEmpty() )
    return QString();

  return utm.zoneNumber + utm.zoneLetter + letter100kID + seasting.mid( seasting.length() - 5, prec ) + snorthing.mid( snorthing.length() - 5, prec );
}

QString QgsMGRSCoordinateConverter::getLetter100kID( int column, int row, int parm ) const
{
  // colOrigin and rowOrigin are the letters at the origin of the set
  int index = parm - 1;
  if ( index >= SET_ORIGIN_COLUMN_LETTERS.length() )
  {
    return QString();
  }
  if ( index >= SET_ORIGIN_ROW_LETTERS.length() )
  {
    return QString();
  }

  int colOrigin = SET_ORIGIN_COLUMN_LETTERS.at( index ).unicode();
  int rowOrigin = SET_ORIGIN_ROW_LETTERS.at( index ).unicode();

  // colInt and rowInt are the letters to build to return
  int colInt = colOrigin + column - 1;
  int rowInt = rowOrigin + row;
  bool rollover = false;

  if ( colInt > 'Z' )
  {
    colInt = colInt - 'Z' + 'A' - 1;
    rollover = true;
  }

  if ( colInt == 'I' || ( colOrigin < 'I' && colInt > 'I' ) || (( colInt > 'I' || colOrigin < 'I' ) && rollover ) )
  {
    colInt++;
  }

  if ( colInt == 'O' || ( colOrigin < 'O' && colInt > 'O' ) || (( colInt > 'O' || colOrigin < 'O' ) && rollover ) )
  {
    colInt++;

    if ( colInt == 'I' )
    {
      colInt++;
    }
  }

  if ( colInt > 'Z' )
  {
    colInt = colInt - 'Z' + 'A' - 1;
  }

  if ( rowInt > 'V' )
  {
    rowInt = rowInt - 'V' + 'A' - 1;
    rollover = true;
  }
  else
  {
    rollover = false;
  }

  if ((( rowInt == 'I' ) || (( rowOrigin < 'I' ) && ( rowInt > 'I' ) ) ) || ((( rowInt > 'I' ) || ( rowOrigin < 'I' ) ) && rollover ) )
  {
    rowInt++;
  }

  if ((( rowInt == 'O' ) || (( rowOrigin < 'O' ) && ( rowInt > 'O' ) ) ) || ((( rowInt > 'O' ) || ( rowOrigin < 'O' ) ) && rollover ) )
  {
    rowInt++;

    if ( rowInt == 'I' )
    {
      rowInt++;
    }
  }

  if ( rowInt > 'V' )
  {
    rowInt = rowInt - 'V' + 'A' - 1;
  }

  return QString( "%1%2" ).arg( QChar( colInt ) ).arg( QChar( rowInt ) );
}
