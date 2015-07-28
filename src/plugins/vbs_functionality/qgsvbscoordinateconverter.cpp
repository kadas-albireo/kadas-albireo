/***************************************************************************
 *  qgsvbscoordinateconverter.cpp                                          *
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

#include "qgsvbscoordinateconverter.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgspoint.h"
#include "utils/qgsvbslltoutm.h"

QgsEPSGCoordinateConverter::QgsEPSGCoordinateConverter( const QString &targetEPSG, QObject *parent )
    : QgsVBSCoordinateConverter( parent )
{
  mDestSrs = new QgsCoordinateReferenceSystem( targetEPSG );
}

QgsEPSGCoordinateConverter::~QgsEPSGCoordinateConverter()
{
  delete mDestSrs;
}

QString QgsEPSGCoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs ) const
{
  QgsPoint pOut = QgsCoordinateTransform( sSrs, *mDestSrs ).transform( p );
  return QString( "%1, %2" ).arg( pOut.x(), 0, 'f', 0 ).arg( pOut.y(), 0, 'f', 0 );
}

///////////////////////////////////////////////////////////////////////////////

QgsWGS84CoordinateConverter::QgsWGS84CoordinateConverter( Format format, QObject *parent )
    : QgsVBSCoordinateConverter( parent )
{
  mFormat = format;
  mDestSrs = new QgsCoordinateReferenceSystem( "EPSG:4326" );
}

QgsWGS84CoordinateConverter::~QgsWGS84CoordinateConverter()
{
  delete mDestSrs;
}

QString QgsWGS84CoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs ) const
{
  QgsPoint pOut = QgsCoordinateTransform( sSrs, *mDestSrs ).transform( p );
  switch ( mFormat )
  {
    case DegMinSec:
    {
      return pOut.toDegreesMinutesSeconds( 1 );
    }
    case DegMin:
    {
      return pOut.toDegreesMinutes( 3 );
    }
    case DecDeg:
    {
      return QString( "%1%2,%3%4" ).arg( pOut.x(), 0, 'f', 5 ).arg( QChar( 176 ) )
             .arg( pOut.y(), 0, 'f', 5 ).arg( QChar( 176 ) );
    }
  }
  return "";
}

///////////////////////////////////////////////////////////////////////////////

QgsUTMCoordinateConverter::QgsUTMCoordinateConverter( QObject *parent )
    : QgsVBSCoordinateConverter( parent )
{
  mWgs84Srs = new QgsCoordinateReferenceSystem( "EPSG:4326" );
}

QgsUTMCoordinateConverter::~QgsUTMCoordinateConverter()
{
  delete mWgs84Srs;
}

QString QgsUTMCoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs ) const
{
  QgsPoint pLatLong = QgsCoordinateTransform( sSrs, *mWgs84Srs ).transform( p );
  QgsVBSLLToUTM::UTMCoo coo = QgsVBSLLToUTM::LL2UTM( pLatLong );
  return QString( "%1, %2 (zone %3%4)" ).arg( coo.easting ).arg( coo.northing ).arg( coo.zoneNumber ).arg( coo.zoneLetter );
}

///////////////////////////////////////////////////////////////////////////////

QString QgsMGRSCoordinateConverter::convert( const QgsPoint &p, const QgsCoordinateReferenceSystem &sSrs ) const
{
  QgsPoint pLatLong = QgsCoordinateTransform( sSrs, *mWgs84Srs ).transform( p );
  QgsVBSLLToUTM::UTMCoo utm = QgsVBSLLToUTM::LL2UTM( pLatLong );
  QgsVBSLLToUTM::MGRSCoo mgrs = QgsVBSLLToUTM::UTM2MGRS( utm );
  if ( mgrs.letter100kID.isEmpty() )
    return QString();

  return QString( "%1%2%3 %4 %5" ).arg( mgrs.zoneNumber ).arg( mgrs.zoneLetter ).arg( mgrs.letter100kID ).arg( mgrs.easting, 5, 10, QChar( '0' ) ).arg( mgrs.northing, 5, 10, QChar( '0' ) );
}
