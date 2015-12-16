/***************************************************************************
 *  qgscoordinatedisplayer.h                                            *
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

#ifndef QGSCOORDINATEDISPAYER_H
#define QGSCOORDINATEDISPAYER_H

#include <QWidget>
#include <qgspoint.h>

class QComboBox;
class QLabel;
class QLineEdit;
class QgsCoordinateReferenceSystem;
class QgsMapCanvas;

class GUI_EXPORT QgsCoordinateDisplayer : public QWidget
{
    Q_OBJECT
  public:
    QgsCoordinateDisplayer( QComboBox* crsComboBox, QLineEdit* coordLineEdit, QgsMapCanvas* mapCanvas, QWidget* parent = 0 );
    ~QgsCoordinateDisplayer();
    QString getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs );
    double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs , QGis::UnitType unit );

  private:
    QgsMapCanvas* mMapCanvas;
    QComboBox* mCRSSelectionCombo;
    QLabel* mIconLabel;
    QLineEdit* mCoordinateLineEdit;

  signals:
    void displayFormatChanged();

  private slots:
    void displayCoordinates( const QgsPoint& p );
    void syncProjectCrs();
};

#endif // QGSCOORDINATEDISPAYER_H
