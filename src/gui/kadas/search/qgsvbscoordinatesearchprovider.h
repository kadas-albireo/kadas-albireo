/***************************************************************************
 *  qgsvbscoordinatesearchprovider.h                                       *
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

#ifndef QGSVBSCOORDINATESEARCHPROVIDER_HPP
#define QGSVBSCOORDINATESEARCHPROVIDER_HPP

#include "qgsvbssearchprovider.h"
#include <QRegExp>

class QgsVBSCoordinateSearchProvider : public QgsVBSSearchProvider
{
    Q_OBJECT
  public:
    QgsVBSCoordinateSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) override;

  private:
    QRegExp mPatLVDD;
    QRegExp mPatDM;
    QRegExp mPatDMS;
    QRegExp mPatUTM;
    QRegExp mPatUTM2;
    QRegExp mPatMGRS;

    static const QString sCategoryName;
};

#endif // QGSVBSCOORDINATESEARCHPROVIDER_HPP
