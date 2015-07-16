/***************************************************************************
 *  qgscoordinatedisplay.h                                                 *
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

#ifndef QGSCOORDINATEDISPLAY_H
#define QGSCOORDINATEDISPLAY_H

#include "qgisplugin.h"

#include <QObject>

class QComboBox;
class QLineEdit;
class QgsPoint;

class QgsCoordinateDisplay: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsCoordinateDisplay( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;
    QWidget* mContainerWidget;
    QComboBox* mCRSSelectionCombo;
    QLineEdit* mCoordinateLineEdit;

  private slots:
    void displayCoordinates( const QgsPoint& p );
    void syncProjectCrs();
};

#endif // QGSCOORDINATEDISPLAY_H
