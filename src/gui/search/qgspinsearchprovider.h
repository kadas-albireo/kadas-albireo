/***************************************************************************
 *  qgspinsearchprovider.h                                                 *
 *  -------------------                                                    *
 *  begin                : March, 2016                                     *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSPINSEARCHPROVIDER_H
#define QGSPINSEARCHPROVIDER_H

#include "qgssearchprovider.h"
#include <QRegExp>

class GUI_EXPORT QgsPinSearchProvider : public QgsSearchProvider
{
    Q_OBJECT
  public:
    QgsPinSearchProvider( QgsMapCanvas *mapCanvas ) : QgsSearchProvider( mapCanvas ) {}
    void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) override;

  private:
    static const QString sCategoryName;
};

#endif // QGSPINSEARCHPROVIDER_H
