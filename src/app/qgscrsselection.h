/***************************************************************************
 *  qgscrsselection.h                                                   *
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

#ifndef QGSCRSSELECTION_H
#define QGSCRSSELECTION_H

#include <QToolButton>

class QgsMapCanvas;

class APP_EXPORT QgsCrsSelection : public QToolButton
{
    Q_OBJECT
  public:
    QgsCrsSelection( QWidget* parent = 0 );
    ~QgsCrsSelection();

    void setMapCanvas( QgsMapCanvas* canvas );

  private:
    QgsMapCanvas* mMapCanvas;

  private slots:
    void syncCrsButton();
    void setMapCrs();
    void selectMapCrs();
};

#endif // QGSCRSSELECTION_H
