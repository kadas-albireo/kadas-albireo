/***************************************************************************
 *  qgscoordinateinput.cpp                                                 *
 *  ----------------------                                                 *
 *  begin                : November 2017                                   *
 *  copyright            : (C) 2017 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSCOORDINATEINPUT_H
#define QGSCOORDINATEINPUT_H

#include <QWidget>
#include "qgscoordinatereferencesystem.h"
#include "qgspoint.h"


class QComboBox;
class QLineEdit;


class GUI_EXPORT QgsCoordinateInput : public QWidget
{
    Q_OBJECT

  public:
    QgsCoordinateInput( QWidget* parent );

    const QgsPoint& getCoordinate() const { return mCoo; }
    const QgsCoordinateReferenceSystem& getCrs() const { return mCrs; }
    bool isEmpty() const { return mEmpty; }

    void setCoordinate( const QgsPoint& coo, const QgsCoordinateReferenceSystem& crs );

  private:
    static const int sFormatRole = Qt::UserRole + 1;
    static const int sAuthidRole = Qt::UserRole + 2;
    QgsPoint mCoo;
    QgsCoordinateReferenceSystem mCrs;

    bool mEmpty = true;
    QLineEdit* mLineEdit;
    QComboBox* mCrsCombo;

  signals:
    void coordinateEdited();
    void coordinateChanged();

  private slots:
    void entryChanged();
    void entryEdited();
    void crsChanged();
};

#endif // QGSCOORDINATEINPUT_H
