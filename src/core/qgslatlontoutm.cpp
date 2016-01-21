/***************************************************************************
 *  qgslatlontoutm.cpp                                                      *
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

#include "qgslatlontoutm.h"
#include "qgspoint.h"
#include "qgsdistancearea.h"
#include <qmath.h>
#include <QTextStream>


const int QgsLatLonToUTM::NUM_100K_SETS = 6;
const QString QgsLatLonToUTM::SET_ORIGIN_COLUMN_LETTERS = "AJSAJS";
const QString QgsLatLonToUTM::SET_ORIGIN_ROW_LETTERS = "AFAFAF";

QgsPoint QgsLatLonToUTM::UTM2LL( const UTMCoo &utm, bool& ok )
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

QgsLatLonToUTM::UTMCoo QgsLatLonToUTM::LL2UTM( const QgsPoint &pLatLong )
{
  const double a = 6378137.0; //ellip.radius;
  const double eccSqr = 0.00669438; //ellip.eccsq;
  const double k0 = 0.9996;

  double Long = pLatLong.x();
  double Lat = pLatLong.y();
  double LatRad = Lat / 180. * M_PI;
  double LongRad = Long / 180. * M_PI;
  int ZoneNumber = getZoneNumber( Long, Lat );

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

int QgsLatLonToUTM::getZoneNumber( double lon, double lat )
{
  int zoneNumber = qFloor(( lon + 180. ) / 6. ) + 1;

  //Make sure the longitude 180.00 is in Zone 60
  if ( lon >= 180.0 )
  {
    zoneNumber = 60;
  }

  // Special zone for Norway
  if ( lat >= 56.0 && lat < 64.0 && lon >= 3.0 && lon < 12.0 )
  {
    zoneNumber = 32;
  }

  // Special zones for Svalbard
  if ( lat >= 72.0 && lat < 84.0 )
  {
    if ( lon >= 0.0 && lon < 9.0 )
    {
      zoneNumber = 31;
    }
    else if ( lon >= 9.0 && lon < 21.0 )
    {
      zoneNumber = 33;
    }
    else if ( lon >= 21.0 && lon < 33.0 )
    {
      zoneNumber = 35;
    }
    else if ( lon >= 33.0 && lon < 42.0 )
    {
      zoneNumber = 37;
    }
  }
  return zoneNumber;
}

QString QgsLatLonToUTM::getHemisphereLetter( double lat )
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

QgsLatLonToUTM::MGRSCoo QgsLatLonToUTM::UTM2MGRS( const UTMCoo &utmcoo )
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

QgsLatLonToUTM::UTMCoo QgsLatLonToUTM::MGRS2UTM( const MGRSCoo &mgrs, bool& ok )
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

double QgsLatLonToUTM::getMinNorthing( int zoneLetter )
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

QString QgsLatLonToUTM::getLetter100kID( int column, int row, int parm )
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

static inline QPolygonF polyGridLineX( double x, double y1, double y2, double stepY )
{
  QPolygonF poly;
  for ( double y = y1; y <= y2; y += stepY )
  {
    poly.append( QPointF( x, y ) );
  }
  poly.append( QPointF( x, y2 ) );
  return poly;
}

static inline QPolygonF polyGridLineY( double x1, double x2, double stepX, double y )
{
  QPolygonF poly;
  for ( double x = x1; x <= x2; x += stepX )
  {
    poly.append( QPointF( x, y ) );
  }
  poly.append( QPointF( x2, y ) );
  return poly;
}

void QgsLatLonToUTM::computeGrid( const QgsRectangle &bbox, double mapScale,
                                  QList<QPolygonF> &zoneLines, QList<QPolygonF> &subZoneLines, QList<QPolygonF> &gridLines,
                                  QList<GridLabel> &zoneLabels, QList<GridLabel> &subZoneLabels, QList<GridLabel> &gridLabels )
{

  QgsDistanceArea da;
  da.setEllipsoidalMode( true );
  da.setEllipsoid( "WGS84" );

  double lats[] = { -90, -80, -72, -64, -56, -48, -40, -32, -24, -16, -8, 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 84, 90 };
  for ( int iy = 0, ny = sizeof( lats ) / sizeof( lats[0] ); iy < ny; ++iy )
  {
    for ( int ix = -30; ix < 30; ++ix )
    {
      double x1 = ix * 6;
      double x2 = ( ix + 1 ) * 6;
      double y1 = lats[iy];
      double y2 = lats[iy + 1];

      // Special zone for Norway
      if ( y1 == 56. && y2 ==  64. )
      {
        if ( x1 == 0 )
        {
          x2 = 3;
        }
        else if ( x1 == 6 )
        {
          x1 = 3;
        }
      }

      // Special zones from Svalbard
      if ( y1 == 72 && y2 == 84 )
      {
        if ( x1 == 0 )
        {
          x2 = 9;
        }
        else if ( x1 == 12 )
        {
          x1 = 9;
          x2 = 21;
        }
        else if ( x1 == 24 )
        {
          x1 = 21;
          x2 = 33;
        }
        else if ( x1 == 36 )
        {
          x1 = 33;
        }
        else if ( x1 == 6 || x1 == 18 || x1 == 30 )
        {
          continue;
        }
      }

      // Check if within area of interest
      if ( x1 > bbox.xMaximum() || x2 < bbox.xMinimum() ||
           y1 > bbox.yMaximum() || y2 < bbox.yMinimum() )
      {
        continue;
      }

      double xMin = qMax( x1, bbox.xMinimum() );
      double xMax = qMin( x2, bbox.xMaximum() );
      double yMin = qMax( y1, bbox.yMinimum() );
      double yMax = qMin( y2, bbox.yMaximum() );

      // Split box perimeter into pieces and compute lines
      zoneLines << polyGridLineX( xMin, yMin, yMax, 1 ) << polyGridLineX( xMax, yMin, yMax, 1. );
      zoneLines << polyGridLineY( xMin, xMax, 1., yMin ) << polyGridLineY( xMin, xMax, 1., yMax );
      int zoneNumber = QgsLatLonToUTM::getZoneNumber( x1, y1 );
      zoneLabels.append( qMakePair( QPointF( xMin, yMin ), QString( "%1%2" ).arg( zoneNumber ).arg( QgsLatLonToUTM::getHemisphereLetter( y1 ) ) ) );

      // Sub-grid
      if ( mapScale > 5000000 )
      {
        continue;
      }
      double cellSize = 0;
      if ( mapScale > 500000 )
      {
        computeSubGrid( 100000, da, bbox, x1, x2, y1, y2, xMin, xMax, yMin, yMax, subZoneLines, &subZoneLabels, mgrs100kIDLabelCallback );
        continue;
      }
      if ( mapScale > 50000 )
      {
        cellSize = 10000;
      }
      else if ( mapScale > 5000 )
      {
        cellSize = 1000;
      }
      else
      {
        cellSize = 100;
      }
      computeSubGrid( 100000, da, bbox, x1, x2, y1, y2, xMin, xMax, yMin, yMax, subZoneLines, &subZoneLabels, mgrs100kIDLabelCallback );
      computeSubGrid( cellSize, da, bbox, x1, x2, y1, y2, xMin, xMax, yMin, yMax, gridLines, &gridLabels, 0, mgrsGridLabelCallback );
    }
  }
}

void QgsLatLonToUTM::computeSubGrid( double cellSize, const QgsDistanceArea& da, const QgsRectangle& bbox,
                                     double x1, double x2, double y1, double y2,
                                     double xMin, double xMax, double yMin, double yMax,
                                     QList<QPolygonF> &gridLines, QList<GridLabel> *gridLabels, zoneLabelCallback_t *zoneLabelCallback, gridLabelCallback_t *lineLabelCallback )
{
  double xMid = 0.5 * ( x1 + x2 );

  double ly = da.measureLine( QgsPoint( x1, y1 ), QgsPoint( x1, y2 ) );
  double incy = ( y2 - y1 ) / ly * cellSize;
  QList<double> incx;
  QList<double> yvals;
  // Y lines (horizontal)
  double y = y1 + qFloor(( yMin - y1 ) / incy ) * incy;
  yvals.append( y );
  double lx = da.measureLine( QgsPoint( x1, yvals.back() ), QgsPoint( x2, yvals.back() ) );
  incx.append(( x2 - x1 ) / lx * cellSize );
  for ( ; y < yMax; y += incy )
  {
    if ( y < bbox.yMinimum() )
    {
      continue;
    }
    gridLines << polyGridLineY( xMin, xMax, 1., y );
    if ( lineLabelCallback )
      gridLabels->append( lineLabelCallback( xMin, y, cellSize, true ) );
    yvals.append( y );
    lx = da.measureLine( QgsPoint( x1, yvals.back() ), QgsPoint( x2, yvals.back() ) );
    incx.append(( x2 - x1 ) / lx * cellSize );
  }
  y = qMin( y2, y );
  yvals.append( y );
  lx = da.measureLine( QgsPoint( x1, yvals.back() ), QgsPoint( x2, yvals.back() ) );
  incx.append(( x2 - x1 ) / lx * cellSize );

  // X lines (vertical)
  if ( xMid >= xMin && xMid <= xMax )
  {
    gridLines << polyGridLineX( xMid, yMin, yMax, 1. );
    if ( lineLabelCallback )
      gridLabels->append( lineLabelCallback( xMid, yMin, cellSize, false ) );
  }
  if ( zoneLabelCallback )
  {
    for ( int j = 0; j < yvals.size(); ++j )
    {
      if ( xMid >= xMin - incx[j] && xMid <= xMax )
        gridLabels->append( zoneLabelCallback( qMax( xMin, xMid ), qMax( yMin, yvals[j] ) ) );
    }
  }
  int nXLines = qCeil( qMax(( xMid - x1 ) / incx.front(), ( xMid - x1 ) / incx.last() ) );
  for ( int i = 1; i <= nXLines; ++i )
  {
    QPolygonF lineLeft, lineRight;
    for ( int j = 0; j < yvals.size(); ++j )
    {
      double y = yvals[j];

      double x = xMid - i * incx[j];
      if ( x >= xMin )
      {
        lineLeft.append( QPointF( x, y ) );
        if ( zoneLabelCallback )
        {
          gridLabels->append( zoneLabelCallback( x, qMax( yMin, y ) ) );
        }
      }
      if ( zoneLabelCallback && x < xMin  && x + incx[j] > xMin )
      {
        gridLabels->append( zoneLabelCallback( xMin, qMax( yMin, y ) ) );
      }

      x = xMid + i * incx[j];
      if ( x <= xMax )
      {
        lineRight.append( QPointF( x, y ) );
        if ( zoneLabelCallback )
        {
          gridLabels->append( zoneLabelCallback( x, qMax( yMin, y ) ) );
        }
      }
      if ( zoneLabelCallback && x < xMin  && x + incx[j] > xMin )
      {
        gridLabels->append( zoneLabelCallback( xMin, qMax( yMin, y ) ) );
      }
    }
    if ( !lineLeft.isEmpty() )
    {
      gridLines << lineLeft;
      if ( lineLabelCallback )
      {
        gridLabels->append( lineLabelCallback( lineLeft.front().x(), lineLeft.front().y(), cellSize, false ) );
      }
    }
    if ( !lineRight.isEmpty() )
    {
      gridLines << lineRight;
      if ( lineLabelCallback /*&& lineRight.front().y() == yMin*/ )
        gridLabels->append( lineLabelCallback( lineRight.front().x(), lineRight.front().y(), cellSize, false ) );
    }
  }
}

QgsLatLonToUTM::GridLabel QgsLatLonToUTM::mgrs100kIDLabelCallback( double lon, double lat )
{
  UTMCoo utmcoo = LL2UTM( QgsPoint( lon + 0.01, lat + 0.01 ) );
  int setColumn = qFloor( utmcoo.easting / 100000 );
  int setRow = int( qFloor( utmcoo.northing / 100000 ) ) % 20;
  int setParm = utmcoo.zoneNumber % NUM_100K_SETS;
  if ( setParm == 0 )
  {
    setParm = NUM_100K_SETS;
  }
  return qMakePair( QPointF( lon, lat ), getLetter100kID( setColumn, setRow, setParm ) );
}

QgsLatLonToUTM::GridLabel QgsLatLonToUTM::mgrsGridLabelCallback( double lon, double lat, double cellSize, bool horiz )
{
  UTMCoo utmcoo = LL2UTM( QgsPoint( lon + 0.01, lat + 0.01 ) );
  if ( horiz )
  {
    return qMakePair( QPointF( lon, lat ), QString::number( int( utmcoo.northing / cellSize ) ) );
  }
  else
  {
    return qMakePair( QPointF( lon, lat ), QString::number( int( utmcoo.easting / cellSize ) ) );
  }
}
