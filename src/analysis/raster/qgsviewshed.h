/***************************************************************************
                          qgsviewshed.h  -  Viewshed computation
                          ---------------------------
    begin                : November 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVIEWSHED_H
#define QGSVIEWSHED_H

class QgsPoint;
class QgsCoordinateReferenceSystem;
class QProgressDialog;

#include "qgis.h"

namespace QgsViewshed
{

  bool computeViewshed( const QString& inputFile,
                        const QString& outputFile, const QString& outputFormat,
                        QgsPoint observerPos, const QgsCoordinateReferenceSystem& observerPosCrs,
                        double observerHeight, double targetHeight, double radius,
                        const QGis::UnitType distanceElevUnit,
                        QProgressDialog* progress = 0 );

} // QgsViewshed

#endif // QGSVIEWSHED_H