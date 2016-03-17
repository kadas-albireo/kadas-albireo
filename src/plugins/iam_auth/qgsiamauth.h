/***************************************************************************
 *  qgsiamauth.h                                                           *
 *  ------------                                                           *
 *  begin                : March 2016                                      *
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

#ifndef QGSIAMAUTH_H
#define QGSIAMAUTH_H

#include "qgisplugin.h"
#include <QObject>

class QgsIAMAuth : public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsIAMAuth( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;

  private slots:
};

#endif // QGSIAMAUTH_H
