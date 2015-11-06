/***************************************************************************
    qgsselectgrouplayerdialog.h
    ----------------------------
    begin                : January 2015
    copyright            : (C) 2015 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSELECTGROUPLAYERDIALOG_H
#define QGSSELECTGROUPLAYERDIALOG_H

#include "ui_qgsselectgrouplayerdialogbase.h"

class QgsLayerTreeModel;

class QgsSelectGroupLayerDialog: public QDialog, private Ui::QgsSelectGroupLayerDialogBase
{
    Q_OBJECT
  public:
    QgsSelectGroupLayerDialog( QgsLayerTreeModel* model, QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsSelectGroupLayerDialog();

    QStringList selectedGroups() const;
    QStringList selectedLayerIds() const;
    QStringList selectedLayerNames() const;
};

#endif // QGSSELECTGROUPLAYERDIALOG_H
