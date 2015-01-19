/***************************************************************************
  qgsmaplayerstyleguiutils.h
  --------------------------------------
  Date                 : January 2015
  Copyright            : (C) 2015 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPLAYERSTYLEGUIUTILS_H
#define QGSMAPLAYERSTYLEGUIUTILS_H

#include <QObject>

#include "qgssingleton.h"

class QgsMapLayer;

class QAction;
class QMenu;

/** Various GUI utility functions for dealing with map layer's style manager */
class QgsMapLayerStyleGuiUtils : public QObject, public QgsSingleton<QgsMapLayerStyleGuiUtils>
{
    Q_OBJECT
  public:

    QAction* actionAddStyle( QgsMapLayer* layer, QObject* parent = 0 );
    QAction* actionRemoveStyle( QgsMapLayer* layer, QObject* parent = 0 );
    QAction* actionRenameStyle( QgsMapLayer* layer, QObject* parent = 0 );
    QList<QAction*> actionsUseStyle( QgsMapLayer* layer, QObject* parent = 0 );

    //! Add actions for the given map layer to the menu
    void addStyleManagerActions( QMenu* m, QgsMapLayer* layer );

  private:
    QString defaultStyleName();

  private slots:
    void addStyle();
    void useStyle();
    void removeStyle();
    void renameStyle();
};

#endif // QGSMAPLAYERSTYLEGUIUTILS_H
