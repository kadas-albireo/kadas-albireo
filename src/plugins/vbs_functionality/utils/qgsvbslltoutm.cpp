/***************************************************************************
 *  qgsvbslltoutm.cpp                                                      *
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

#include "qgsvbslltoutm.h"
#include "qgspoint.h"
#include <qmath.h>


const int QgsVBSLLToUTM::NUM_100K_SETS = 6;
const QString QgsVBSLLToUTM::SET_ORIGIN_COLUMN_LETTERS = "AJSAJS";
const QString QgsVBSLLToUTM::SET_ORIGIN_ROW_LETTERS = "AFAFAF";

QgsPoint QgsVBSLLToUTM::UTM2LL( const UTMCoo &utm, bool& ok )
{
  ok = false;

  // check whether the ZoneNummber and ZoneLetter are valid
  if ( utm.zoneNumber < 0 || utm.zoneNumber > 60 || utm.zoneLetter.isEmpty() )
  {
    return QgsPoint();
  }

  const double k0 = 0.9996;
  const double a = 6378137.0; //ellip.radius;
  const double eccSquared = 0.00669438; //ellip.eccsq;
  double e1 = ( 1 - qSqrt( 1 - eccSquared ) ) / ( 1 + qSqrt( 1 - eccSquared ) );

  // remove 500,000 meter offset for longitude
  double x = utm.easting - 500000.0;
  double y = utm.northing;

  // We must know somehow if we are in the Northern or Southern
  // hemisphere, this is the only time we use the letter So even
  // if the Zone letter isn't exactly correct it should indicate
  // the hemisphere correctly
  if ( utm.zoneLetter.at( 0 ) < 'N' )
  {
    y -= 10000000.0; // remove 10,000,000 meter offset used
    // for southern hemisphere
  }

  // There are 60 zones with zone 1 being at West -180 to -174
  double LongOrigin = ( utm.zoneNumber - 1 ) * 6 - 180 + 3; // +3 puts origin in middle of zone

  double eccPrimeSquared = ( eccSquared ) / ( 1 - eccSquared );

  double M = y / k0;
  double mu = M / ( a * ( 1 - eccSquared / 4 - 3 * eccSquared * eccSquared / 64 - 5 * eccSquared * eccSquared * eccSquared / 256 ) );

  double phi1Rad = mu + ( 3 * e1 / 2 - 27 * e1 * e1 * e1 / 32 ) * qSin( 2 * mu ) + ( 21 * e1 * e1 / 16 - 55 * e1 * e1 * e1 * e1 / 32 ) * qSin( 4 * mu ) + ( 151 * e1 * e1 * e1 / 96 ) * qSin( 6 * mu );
  // double phi1 = ProjMath.radToDeg(phi1Rad);

  double N1 = a / qSqrt( 1 - eccSquared * qSin( phi1Rad ) * qSin( phi1Rad ) );
  double T1 = qTan( phi1Rad ) * qTan( phi1Rad );
  double C1 = eccPrimeSquared * qCos( phi1Rad ) * qCos( phi1Rad );
  double R1 = a * ( 1 - eccSquared ) / qPow( 1 - eccSquared * qSin( phi1Rad ) * qSin( phi1Rad ), 1.5 );
  double D = x / ( N1 * k0 );

  double lat = phi1Rad - ( N1 * qTan( phi1Rad ) / R1 ) * ( D * D / 2 - ( 5 + 3 * T1 + 10 * C1 - 4 * C1 * C1 - 9 * eccPrimeSquared ) * D * D * D * D / 24 + ( 61 + 90 * T1 + 298 * C1 + 45 * T1 * T1 - 252 * eccPrimeSquared - 3 * C1 * C1 ) * D * D * D * D * D * D / 720 );
  lat = lat / M_PI * 180.;

  double lon = ( D - ( 1 + 2 * T1 + C1 ) * D * D * D / 6 + ( 5 - 2 * C1 + 28 * T1 - 3 * C1 * C1 + 8 * eccPrimeSquared + 24 * T1 * T1 ) * D * D * D * D * D / 120 ) / qCos( phi1Rad );
  lon = LongOrigin + lon / M_PI * 180.;

  ok = true;
  return QgsPoint( lon, lat );
}

QgsVBSLLToUTM::UTMCoo QgsVBSLLToUTM::LL2UTM( const QgsPoint &pLatLong )
{
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

QString QgsVBSLLToUTM::getHemisphereLetter( double lat )
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

QgsVBSLLToUTM::MGRSCoo QgsVBSLLToUTM::UTM2MGRS( const UTMCoo &utmcoo )
{
  int setParm = utmcoo.zoneNumber % NUM_100K_SETS;
  if ( setParm == 0 )
  {
    setParm = NUM_100K_SETS;
  }

  int setColumn = qFloor( utmcoo.easting / 100000 );
  int setRow = int( qFloor( utmcoo.northing / 100000 ) ) % 20;

  MGRSCoo mgrscoo;
  mgrscoo.easting = utmcoo.easting % 100000;
  mgrscoo.northing = utmcoo.northing % 100000;
  mgrscoo.zoneLetter = utmcoo.zoneLetter;
  mgrscoo.zoneNumber = utmcoo.zoneNumber;
  mgrscoo.letter100kID = getLetter100kID( setColumn, setRow, setParm );
  return mgrscoo;
}

QgsVBSLLToUTM::UTMCoo QgsVBSLLToUTM::MGRS2UTM( const MGRSCoo &mgrs, bool& ok )
{
  UTMCoo utm;
  utm.zoneLetter = mgrs.zoneLetter;
  utm.zoneNumber = mgrs.zoneNumber;
  utm.easting = mgrs.easting;
  utm.northing = mgrs.northing;

  int setParam = mgrs.zoneNumber % NUM_100K_SETS;
  if ( setParam == 0 )
  {
    setParam = NUM_100K_SETS;
  }

  double easting100k;
  {
    int e = mgrs.letter100kID.at( 0 ).unicode();
    int curCol = SET_ORIGIN_COLUMN_LETTERS.at( setParam - 1 ).unicode();
    easting100k = 100000.0;
    bool rewindMarker = false;

    while ( curCol != e )
    {
      ++curCol;
      if ( curCol == 'I' )
      {
        ++curCol;
      }
      if ( curCol == 'O' )
      {
        ++curCol;
      }
      if ( curCol > 'Z' )
      {
        if ( rewindMarker )
        {
          // Bad character
          ok = false;
          return utm;
        }
        curCol = 'A';
        rewindMarker = true;
      }
      easting100k += 100000.0;
    }
  }

  double northing100k;
  {
    int n = mgrs.letter100kID.at( 1 ).unicode();
    if ( n > 'V' )
    {
      // Invalid Northing
      return utm;
    }

    // rowOrigin is the letter at the origin of the set for the
    // column
    int curRow = SET_ORIGIN_ROW_LETTERS.at( setParam - 1 ).unicode();
    northing100k = 0.0;
    bool rewindMarker = false;

    while ( curRow != n )
    {
      ++curRow;
      if ( curRow == 'I' )
      {
        ++curRow;
      }
      if ( curRow == 'O' )
      {
        ++curRow;
      }
      // fixing a bug making whole application hang in this loop
      // when 'n' is a wrong character
      if ( curRow > 'V' )
      {
        if ( rewindMarker )
        {
          // Bad character
          ok = false;
          return utm;
        }
        curRow = 'A';
        rewindMarker = true;
      }
      northing100k += 100000.0;
    }

    double minNorthing = getMinNorthing( mgrs.zoneLetter.at( 0 ).unicode() );
    if ( minNorthing < 0.0 )
    {
      // Invalid zone letter
      ok = false;
      return utm;
    }
    while ( northing100k < minNorthing )
    {
      northing100k += 2000000;
    }
  }

  utm.easting = utm.easting % 100000 + easting100k;
  utm.northing = utm.northing % 100000 + northing100k;
  ok = true;
  return utm;
}

double QgsVBSLLToUTM::getMinNorthing( int zoneLetter )
{
  double northing;
  switch ( zoneLetter )
  {
    case 'C':
      northing = 1100000.0;
      break;
    case 'D':
      northing = 2000000.0;
      break;
    case 'E':
      northing = 2800000.0;
      break;
    case 'F':
      northing = 3700000.0;
      break;
    case 'G':
      northing = 4600000.0;
      break;
    case 'H':
      northing = 5500000.0;
      break;
    case 'J':
      northing = 6400000.0;
      break;
    case 'K':
      northing = 7300000.0;
      break;
    case 'L':
      northing = 8200000.0;
      break;
    case 'M':
      northing = 9100000.0;
      break;
    case 'N':
      northing = 0.0;
      break;
    case 'P':
      northing = 800000.0;
      break;
    case 'Q':
      northing = 1700000.0;
      break;
    case 'R':
      northing = 2600000.0;
      break;
    case 'S':
      northing = 3500000.0;
      break;
    case 'T':
      northing = 4400000.0;
      break;
    case 'U':
      northing = 5300000.0;
      break;
    case 'V':
      northing = 6200000.0;
      break;
    case 'W':
      northing = 7000000.0;
      break;
    case 'X':
      northing = 7900000.0;
      break;
    default:
      northing = -1.0;
  }
  return northing;
}

QString QgsVBSLLToUTM::getLetter100kID( int column, int row, int parm )
{
  // colOrigin and rowOrigin are the letters at the origin of the set
  int index = parm - 1;
  if ( index < 0 || index >= SET_ORIGIN_COLUMN_LETTERS.length() )
  {
    return QString();
  }
  if ( index < 0 || index >= SET_ORIGIN_ROW_LETTERS.length() )
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
