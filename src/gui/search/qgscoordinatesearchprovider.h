/***************************************************************************
 *  qgscoordinatesearchprovider.h                                       *
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

#ifndef QGSCOORDINATESEARCHPROVIDER_HPP
#define QGSCOORDINATESEARCHPROVIDER_HPP

#include "qgssearchprovider.h"
#include <QRegExp>

class GUI_EXPORT QgsCoordinateSearchProvider : public QgsSearchProvider
{
    Q_OBJECT
  public:
    QgsCoordinateSearchProvider( QgsMapCanvas *mapCanvas );
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

#endif // QGSCOORDINATESEARCHPROVIDER_HPP
