/***************************************************************************
                         qgscomposerlegenditemdialog.h
                         -----------------------------
    begin                : July 2008
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

#ifndef QGSCOMPOSERLEGENDITEMDIALOG_H
#define QGSCOMPOSERLEGENDITEMDIALOG_H

#include "ui_qgscomposerlegenditemdialogbase.h"

class QStandardItem;

/** \ingroup MapComposer
 * A dialog to enter properties of composer legend items (currently only item text)
 * */
class QgsComposerLegendItemDialog: private Ui::QgsComposerLegendItemDialogBase, public QDialog
{
  public:
    QgsComposerLegendItemDialog( const QStandardItem* item, QWidget* parent = 0 );
    ~QgsComposerLegendItemDialog();

    /**Returns the item text inserted by user*/
    QString itemText() const;

  private:
    QgsComposerLegendItemDialog(); //forbidden
};

#endif //QGSCOMPOSERLEGENDITEMDIALOG_H
