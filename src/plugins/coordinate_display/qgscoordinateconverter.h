/***************************************************************************
 *  qgscoordinateconverter.h                                               *
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

#ifndef QGSCOORDINATECONVERTER_H
#define QGSCOORDINATECONVERTER_H

#include <QObject>

class QgsPoint;
class QgsCoordinateReferenceSystem;

class QgsCoordinateConverter : public QObject
{
  public:
    QgsCoordinateConverter( QObject* parent ) : QObject( parent ) {}
    virtual ~QgsCoordinateConverter() {}
    virtual QString convert( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, int prec ) const = 0;
};


class QgsEPSGCoordinateConverter : public QgsCoordinateConverter
{
  public:
    QgsEPSGCoordinateConverter( const QString& targetEPSG, QObject* parent );
    ~QgsEPSGCoordinateConverter();
    QString convert( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, int prec ) const override;
  private:
    QgsCoordinateReferenceSystem* mDestSrs;
};


class QgsWGS84CoordinateConverter : public QgsCoordinateConverter
{
  public:
    enum Format { DegMinSec, DegMin, DecDeg };
    QgsWGS84CoordinateConverter( Format format, QObject* parent );
    ~QgsWGS84CoordinateConverter();
    QString convert( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, int prec ) const override;
  private:
    Format mFormat;
    QgsCoordinateReferenceSystem* mDestSrs;
};


class QgsUTMCoordinateConverter : public QgsCoordinateConverter
{
  public:
    QgsUTMCoordinateConverter( QObject* parent );
    ~QgsUTMCoordinateConverter();
    QString convert( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, int prec ) const override;
  private:
    QgsCoordinateReferenceSystem* mWgs84Srs;

  protected:
    struct UTMCoo
    {
      int easting;
      int northing;
      int zoneNumber;
      QString zoneLetter;
    };

    UTMCoo getUTMCoordinate( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs ) const;
    QString getHemisphereLetter( double lat ) const;
};

class QgsMGRSCoordinateConverter : public QgsUTMCoordinateConverter
{
  public:
    QgsMGRSCoordinateConverter( QObject* parent ) : QgsUTMCoordinateConverter( parent ) {}
    QString convert( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, int prec ) const override;

  private:
    static const int NUM_100K_SETS;
    static const QString SET_ORIGIN_COLUMN_LETTERS;
    static const QString SET_ORIGIN_ROW_LETTERS;

    QString getLetter100kID( int column, int row, int parm ) const;
};

#endif // QGSCOORDINATECONVERTER_H
