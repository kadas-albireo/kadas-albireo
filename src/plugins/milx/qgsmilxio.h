/***************************************************************************
 *  qgsmilxio.h                                                            *
 *  -----------                                                            *
 *  begin                : February 2016                                   *
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

#ifndef QGSMILXIO_H
#define QGSMILXIO_H

#include <QObject>

class QgisInterface;
class QgsMessageBar;
class QgsMilXLayer;

class QgsMilXIO : public QObject
{
    Q_OBJECT
  public:
    static bool save( QgisInterface *iface );
    static bool load( QgisInterface *iface );
  private:
    static void showMessageDialog( const QString& title, const QString& body, const QString& messages );
};

#endif // QGSMILXIO_H
