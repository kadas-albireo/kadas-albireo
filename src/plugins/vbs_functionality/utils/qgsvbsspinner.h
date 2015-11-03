/***************************************************************************
 *  qgsvbsspinner.h                                                        *
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

#ifndef QGSVBSSPINNER_H
#define QGSVBSSPINNER_H

#include <QWidget>

class QgsVBSSpinner : public QWidget
{
    Q_OBJECT
  public:
    QgsVBSSpinner( QWidget* parent = 0 );
    void setVisible( bool visible ) override;

  private:
    int mTicks;
    int mCounter;
    int mTimer;

    void timerEvent( QTimerEvent * );
    void paintEvent( QPaintEvent * );
};

#endif // QGSVBSSPINNER_H
