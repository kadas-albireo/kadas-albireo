/***************************************************************************
    qgsredliningmanager.cpp
     --------------------------------------
    Date                 : Jul 2017
    Copyright            : (C) 2017 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSREDLININGMANAGER_H
#define QGSREDLININGMANAGER_H

#include <QObject>

class QgsFeature;
class QgsLabelPosition;

class QgsRedliningManager : public QObject
{
    Q_OBJECT
  public:
    QgsRedliningManager( QObject* parent );
    virtual ~QgsRedliningManager();
    virtual void editFeature( const QgsFeature &feature ) = 0;
    virtual void editLabel( const QgsLabelPosition &labelPos ) = 0;
};

#endif // QGSREDLININGMANAGER_H
