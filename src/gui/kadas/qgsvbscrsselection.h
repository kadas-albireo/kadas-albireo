/***************************************************************************
 *  qgsvbscrsselection.h                                                   *
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

#ifndef QGSVBSCRSSELECTION_H
#define QGSVBSCRSSELECTION_H

#include <QToolButton>

class QgsMapCanvas;

class GUI_EXPORT QgsVBSCrsSelection : public QToolButton
{
    Q_OBJECT
  public:
    QgsVBSCrsSelection( QWidget* parent = 0 );
    ~QgsVBSCrsSelection();

    void setMapCanvas( QgsMapCanvas* canvas );

  private:
    QgsMapCanvas* mMapCanvas;

  private slots:
    void syncCrsButton();
    void setMapCrs();
    void selectMapCrs();
};

#endif // QGSVBSCRSSELECTION_H
