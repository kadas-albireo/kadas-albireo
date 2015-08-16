/***************************************************************************
 *  qgsvbscoordinatedisplayer.h                                            *
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

#ifndef QGSVBSCOORDINATEDISPAYER_H
#define QGSVBSCOORDINATEDISPAYER_H

#include <QWidget>
#include <qgspoint.h>

class QgisInterface;
class QComboBox;
class QLabel;
class QLineEdit;
class QgsCoordinateReferenceSystem;

class QgsVBSCoordinateDisplayer : public QWidget
{
    Q_OBJECT
  public:
    QgsVBSCoordinateDisplayer( QgisInterface* iface, QWidget* parent = 0 );
    ~QgsVBSCoordinateDisplayer();
    QString getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs );

  private:
    QgisInterface* mQGisIface;
    QComboBox* mCRSSelectionCombo;
    QLabel* mIconLabel;
    QLineEdit* mCoordinateLineEdit;

  signals:
    void displayFormatChanged();

  private slots:
    void displayCoordinates( const QgsPoint& p );
    void syncProjectCrs();
};

#endif // QGSVBSCOORDINATEDISPAYER_H
