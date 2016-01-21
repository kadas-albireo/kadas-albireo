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
#include <QPair>
#include <QPolygonF>

class QgsDistanceArea;
class QgsPoint;
class QgsRectangle;

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

    static int getZoneNumber( double lon, double lat );
    static QString getHemisphereLetter( double lat );

    typedef QPair<QPointF, QString> GridLabel;
    static void computeGrid( const QgsRectangle& bbox, double mapScale,
                             QList<QPolygonF>& zoneLines, QList<QPolygonF>& subZoneLines, QList<QPolygonF>& gridLines,
                             QList<GridLabel>& zoneLabels, QList<GridLabel>& subZoneLabels, QList<GridLabel>& gridLabels );

  private:
    static const int NUM_100K_SETS;
    static const QString SET_ORIGIN_COLUMN_LETTERS;
    static const QString SET_ORIGIN_ROW_LETTERS;

    static QString getLetter100kID( int column, int row, int parm );
    static double getMinNorthing( int zoneLetter );
    typedef GridLabel( zoneLabelCallback_t )( double, double );
    typedef GridLabel( gridLabelCallback_t )( double, double, double, bool );
    static void computeSubGrid( double cellSize, const QgsDistanceArea &da, const QgsRectangle &bbox, double x1, double x2, double y1, double y2, double xMin, double xMax, double yMin, double yMax, QList<QPolygonF>& gridLines, QList<GridLabel>* gridLabels = 0, zoneLabelCallback_t* zoneLabelCallback = 0, gridLabelCallback_t* lineLabelCallback = 0 );
    static GridLabel mgrs100kIDLabelCallback( double lon, double lat );
    static GridLabel mgrsGridLabelCallback( double lon, double lat, double cellSize, bool horiz );
};

#endif // QGSLATLONTOUTM_H
