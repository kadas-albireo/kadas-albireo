/***************************************************************************
 *  qgslatlontoutm.h                                                       *
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

#ifndef QGSLATLONTOUTM_H
#define QGSLATLONTOUTM_H

#include <QString>

class QgsPoint;

class CORE_EXPORT QgsLatLonToUTM
{
  public:
    struct UTMCoo
    {
      int easting;
      int northing;
      int zoneNumber;
      QString zoneLetter;
    };

    struct MGRSCoo
    {
      int easting;
      int northing;
      int zoneNumber;
      QString zoneLetter;
      QString letter100kID;
    };

    static QgsPoint UTM2LL( const UTMCoo& utm, bool &ok );
    static UTMCoo LL2UTM( const QgsPoint& pLatLong );
    static MGRSCoo UTM2MGRS( const UTMCoo& utmcoo );
    static UTMCoo MGRS2UTM( const MGRSCoo& mgrs, bool& ok );

  private:
    static const int NUM_100K_SETS;
    static const QString SET_ORIGIN_COLUMN_LETTERS;
    static const QString SET_ORIGIN_ROW_LETTERS;

    static QString getHemisphereLetter( double lat );
    static QString getLetter100kID( int column, int row, int parm );
    static double getMinNorthing( int zoneLetter );
};

#endif // QGSLATLONTOUTM_H
