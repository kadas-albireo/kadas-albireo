/***************************************************************************
 *  qgscoordinateutils.h                                                   *
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

#ifndef QGSCOORDINATEUTILS_H
#define QGSCOORDINATEUTILS_H

#include <QObject>
#include "qgscoordinatereferencesystem.h"

class QgsPoint;
class QgsCoordinateReferenceSystem;

class CORE_EXPORT QgsCoordinateUtils : public QObject
{
  public:
    enum TargetFormat
    {
      EPSG,
      DegMinSec,
      DegMin,
      DecDeg,
      UTM,
      MGRS
    };

    /** Returns the height at the specified position */
    static double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs, QGis::UnitType unit, QString* errMsg = 0 );
    static QString getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& sSrs, TargetFormat targetFormat, const QString& targetEpsg = QString() );
};

#endif // QGSCOORDINATEUTILS_H
