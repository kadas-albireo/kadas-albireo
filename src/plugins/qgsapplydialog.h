/***************************************************************************
                         qgsapplydialog.h  -  description
                         --------------------------------
    begin                : November 2008
    copyright            : (C) 2008 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSAPPLYDIALOG_H
#define QGSAPPLYDIALOG_H

#include <QDialog>

/** \brief Interface class for dialogs that have an apply operation (e.g. for symbology)*/
class QgsApplyDialog: public QDialog
{
  public:
    QgsApplyDialog(): QDialog() {}
    ~QgsApplyDialog() {}
    virtual void apply() const = 0;
};

#endif
