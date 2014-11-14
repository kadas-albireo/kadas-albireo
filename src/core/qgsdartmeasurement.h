/***************************************************************************
    qgsdartmeasurement.h
     --------------------------------------
    Date                 : 8.11.2014
    Copyright            : (C) 2014 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSDARTMEASUREMENT_H
#define QGSDARTMEASUREMENT_H

#include <QString>

class QgsDartMeasurement
{
  public:
    enum Type
    {
      Text,
      ImagePng,
      Integer
    };

    QgsDartMeasurement()
        : mType( Text )
    {}

    QgsDartMeasurement( const QString& name, Type type, const QString& value );

    const QString toString() const;

    void send() const;

    static const QString typeToString( Type type );

  private:
    QString mName;
    Type mType;
    QString mValue;
};

#endif // QGSDARTMEASUREMENT_H
