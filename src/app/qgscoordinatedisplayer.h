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
#include "qgscoordinateformat.h"

class QLabel;
class QLineEdit;
class QToolButton;
class QgsCoordinateReferenceSystem;
class QgsMapCanvas;

class APP_EXPORT QgsCoordinateDisplayer : public QWidget
{
    Q_OBJECT
  public:
    QgsCoordinateDisplayer( QToolButton *crsButton, QLineEdit* coordLineEdit, QToolButton *heightButton, QgsMapCanvas* mapCanvas, QWidget* parent = 0 );
    void getCoordinateDisplayFormat( QgsCoordinateFormat::Format &format, QString& epsg );
    QString getDisplayString( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs );
    double getHeightAtPos( const QgsPoint& p, const QgsCoordinateReferenceSystem& crs , QGis::UnitType unit );

  private:
    enum TargetFormat { LV03, LV95, DMS, DM, DD, UTM, MGRS };
    QgsMapCanvas* mMapCanvas;
    QToolButton* mCRSSelectionButton;
    QToolButton* mHeightSelectionButton;
    QLabel* mIconLabel;
    QLineEdit* mCoordinateLineEdit;
    QAction* mActionDisplayLV03;
    QAction* mActionDisplayLV95;
    QAction* mActionDisplayDMS;

  private slots:
    void displayCoordinates( const QgsPoint& p );
    void syncProjectCrs();
    void displayFormatChanged( QAction* action );
    void heightUnitChanged( QAction *action );
};

#endif // QGSCOORDINATEDISPAYER_H
