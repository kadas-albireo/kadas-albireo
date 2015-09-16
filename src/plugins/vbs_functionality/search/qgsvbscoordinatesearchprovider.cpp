/***************************************************************************
 *  qgsvbscoordinatesearchprovider.cpp                                     *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
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

#include "qgsvbscoordinatesearchprovider.h"
#include "qgscoordinatetransform.h"
#include "qgslatlontoutm.h"

const QString QgsVBSCoordinateSearchProvider::sCategoryName = QgsVBSCoordinateSearchProvider::tr( "Coordinates" );

QgsVBSCoordinateSearchProvider::QgsVBSCoordinateSearchProvider( QgisInterface *iface )
    : QgsVBSSearchProvider( iface )
{
  QString degChar = QString( "%1" ).arg( QChar( 0x00B0 ) );
  QString minChars = QString( "'%1%2%3" ).arg( QChar( 0x2032 ) ).arg( QChar( 0x02BC ) ).arg( QChar( 0x2019 ) );
  QString secChars = QString( "\"%1" ).arg( QChar( 0x2019 ) );

  mPatLVDD = QRegExp( QString( "^(\\d+\\.?\\d*)(%1)?[,\\s]\\s*(\\d+\\.?\\d*)(%1)?$" ).arg( degChar ) );
  mPatDM = QRegExp( QString( "^(\\d+)%1(\\d+\\.?\\d*)[%2]([NnSsEeWw]),?[\\s]*(\\d+)%1(\\d+\\.?\\d*)[%2]([NnSsEeWw])$" ).arg( degChar ).arg( minChars ) );
  mPatDMS = QRegExp( QString( "^(\\d+)%1(\\d+)[%2](\\d+\\.?\\d*)[%3]([NnSsEeWw]),?[\\s]*(\\d+)%1(\\d+)[%2](\\d+\\.?\\d*)[%3]([NnSsEeWw])$" ).arg( degChar ).arg( minChars ).arg( secChars ) );
  mPatUTM = QRegExp( "^(\\d+\\.?\\d*)[,\\s]\\s*(\\d+\\.?\\d*)\\s*\\(\\w+\\s+(\\d+)([A-Za-z])\\)$" );
  mPatUTM2 = QRegExp( "^(\\d+)\\s*([A-Za-z])\\s+(\\d+\\.?\\d*)[,\\s]\\s*(\\d+\\.?\\d*)$" );
  mPatMGRS = QRegExp( "^(\\d+)\\s*(\\w)\\s*(\\w\\w)\\s+(\\d+)[,\\s]\\s*(\\d+)$" );
}

void QgsVBSCoordinateSearchProvider::startSearch( const QString &searchtext, const SearchRegion &/*searchRegion*/ )
{
  SearchResult searchResult;
  searchResult.zoomScale = 1000;
  searchResult.category = sCategoryName;
  bool valid = true;

  if ( mPatLVDD.exactMatch( searchtext ) )
  {
    // LV03, LV93 or decimal degrees
    double lon = mPatLVDD.cap( 1 ).toDouble();
    double lat = mPatLVDD.cap( 3 ).toDouble();
    searchResult.pos = QgsPoint( lon, lat );
    bool haveDeg = !mPatLVDD.cap( 2 ).isEmpty() && mPatLVDD.cap( 4 ).isEmpty();
    if (( lon >= -180. && lon <= 180. ) && ( lat >= -90. && lat <= 90. ) )
    {
      searchResult.text = searchResult.pos.toDegreesMinutesSeconds( 2 );
      searchResult.crs.createFromString( "EPSG:4326" );
    }
    else if ( !haveDeg && (( lon >= 470000. && lon <= 850000. ) && ( lat >= 60000. && lat <= 310000. ) ) )
    {
      searchResult.text = searchResult.pos.toString() + " (LV03)";
      searchResult.crs.createFromString( "EPSG:21781" );
    }
    else if ( !haveDeg && ( lon >= 2450000. && lon <= 2850000. ) && ( lat >= 1050000. && lat <= 1300000. ) )
    {
      searchResult.text = searchResult.pos.toString() + " (LV95)";
      searchResult.crs.createFromString( "EPSG:2056" );
    }
    else
    {
      valid = false;
    }
  }
  else if ( mPatDM.exactMatch( searchtext ) )
  {
    QString NS = "NnSs";
    if (( NS.indexOf( mPatDM.cap( 3 ) ) != -1 ) + ( NS.indexOf( mPatDM.cap( 6 ) ) != -1 ) == 1 )
    {
      double lon = mPatDM.cap( 1 ).toDouble() + 1. / 60. * mPatDM.cap( 2 ).toDouble();
      double lat = mPatDM.cap( 4 ).toDouble() + 1. / 60. * mPatDM.cap( 5 ).toDouble();
      if (( NS.indexOf( mPatDM.cap( 3 ) ) != -1 ) )
        qSwap( lat, lon );

      searchResult.crs.createFromString( "EPSG:4326" );
      searchResult.pos = QgsPoint( lon, lat );
      searchResult.text = searchResult.pos.toDegreesMinutesSeconds( 2 );
    }
  }
  else if ( mPatDMS.exactMatch( searchtext ) )
  {
    QString NS = "NnSs";
    if (( NS.indexOf( mPatDMS.cap( 4 ) ) != -1 ) + ( NS.indexOf( mPatDMS.cap( 8 ) ) != -1 ) == 1 )
    {
      double lon = mPatDMS.cap( 1 ).toDouble() + 1. / 60. * ( mPatDMS.cap( 2 ).toDouble() + 1. / 60. * mPatDMS.cap( 3 ).toDouble() );
      double lat = mPatDMS.cap( 5 ).toDouble() + 1. / 60. * ( mPatDMS.cap( 6 ).toDouble() + 1. / 60. * mPatDMS.cap( 7 ).toDouble() );
      if (( NS.indexOf( mPatDMS.cap( 4 ) ) != -1 ) )
        qSwap( lat, lon );

      searchResult.crs.createFromString( "EPSG:4326" );
      searchResult.pos = QgsPoint( lon, lat );
      searchResult.text = searchResult.pos.toDegreesMinutesSeconds( 2 );
    }
  }
  else if ( mPatUTM.exactMatch( searchtext ) )
  {
    QgsLatLonToUTM::UTMCoo utm;
    utm.easting = mPatUTM.cap( 1 ).toInt();
    utm.northing = mPatUTM.cap( 2 ).toInt();
    utm.zoneNumber = mPatUTM.cap( 3 ).toInt();
    utm.zoneLetter = mPatUTM.cap( 4 );
    bool ok = false;
    searchResult.pos = QgsLatLonToUTM::UTM2LL( utm, ok );
    if ( !ok )
    {
      valid = false;
    }
    searchResult.crs.createFromString( "EPSG:4326" );
    searchResult.text = QString( "%1, %2 (%3 %4%5)" )
                        .arg( utm.easting ).arg( utm.northing ).arg( tr( "zone" ) ).arg( utm.zoneNumber ).arg( utm.zoneLetter );
  }
  else if ( mPatUTM2.exactMatch( searchtext ) )
  {
    QgsLatLonToUTM::UTMCoo utm;
    utm.easting = mPatUTM2.cap( 3 ).toInt();
    utm.northing = mPatUTM2.cap( 4 ).toInt();
    utm.zoneNumber = mPatUTM2.cap( 1 ).toInt();
    utm.zoneLetter = mPatUTM2.cap( 2 );
    bool ok = false;
    searchResult.pos = QgsLatLonToUTM::UTM2LL( utm, ok );
    if ( !ok )
    {
      valid = false;
    }
    searchResult.crs.createFromString( "EPSG:4326" );
    searchResult.text = QString( "%1, %2 (%3 %4%5)" )
                        .arg( utm.easting ).arg( utm.northing ).arg( tr( "zone" ) ).arg( utm.zoneNumber ).arg( utm.zoneLetter );
  }
  else if ( mPatMGRS.exactMatch( searchtext ) )
  {
    QgsLatLonToUTM::MGRSCoo mgrs;
    mgrs.easting = mPatMGRS.cap( 4 ).toInt();
    mgrs.northing = mPatMGRS.cap( 5 ).toInt();
    mgrs.zoneNumber = mPatMGRS.cap( 1 ).toInt();
    mgrs.zoneLetter = mPatMGRS.cap( 2 );
    mgrs.letter100kID = mPatMGRS.cap( 3 );
    bool ok = false;
    QgsLatLonToUTM::UTMCoo utm = QgsLatLonToUTM::MGRS2UTM( mgrs, ok );
    if ( !ok )
    {
      valid = false;
    }
    else
    {
      searchResult.pos = QgsLatLonToUTM::UTM2LL( utm, ok );
      if ( !ok )
      {
        valid = false;
      }
      else
      {
        searchResult.crs.createFromString( "EPSG:4326" );
        searchResult.text = QString( "%1%2%3 %4 %5" )
                            .arg( mgrs.zoneNumber ).arg( mgrs.zoneLetter ).arg( mgrs.letter100kID ).arg( mgrs.easting ).arg( mgrs.northing );
      }
    }
  }
  else
  {
    valid = false;
  }

  if ( valid )
  {
    emit searchResultFound( searchResult );
  }
  emit searchFinished();
  return;
}
