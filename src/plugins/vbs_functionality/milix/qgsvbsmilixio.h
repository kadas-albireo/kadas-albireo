/***************************************************************************
 *  qgsvbsmilixio.h                                                        *
 *  -------------------                                                    *
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

#ifndef QGSVBSMILIXIO_H
#define QGSVBSMILIXIO_H

#include <QObject>

class QgsMessageBar;
class QgsVBSMilixLayer;

class QgsVBSMilixIO : public QObject
{
  public:
    static bool save( QgsMessageBar *messageBar , QgsVBSMilixLayer *layer );
    static bool load( QgsMessageBar *messageBar );
  private:
    static void showMessageDialog( const QString& title, const QString& body, const QString& messages );
};

#endif // QGSVBSMILIXIO_H
