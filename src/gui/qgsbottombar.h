/***************************************************************************
    qgsbottombar.h
    ------------------
    begin                : May 2017
    copyright            : (C) 2017 Sandro Mani
    email                : smani@sourcepole.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSBOTTOMBAR_H
#define QGSBOTTOMBAR_H

#include <QFrame>

class QgsMapCanvas;

class GUI_EXPORT QgsBottomBar : public QFrame
{
  public:
    QgsBottomBar( QgsMapCanvas* canvas, const QString& color = "orange" );
    bool eventFilter( QObject* obj, QEvent* event ) override;

  protected:
    QgsMapCanvas* mCanvas;

    void updatePosition();
};

#endif // QGSBOTTOMBAR_H
