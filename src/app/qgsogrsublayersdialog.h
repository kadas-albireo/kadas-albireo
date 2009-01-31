/***************************************************************************
    qgsogrsublayersdialog.h  - dialog for selecting ogr sublayers
    ---------------------
    begin                : January 2009
    copyright            : (C) 2009 by Florian El Ahdab
    email                : felahdab at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSOGRSUBLAYERSDIALOG_H
#define QGSOGRSUBLAYERSDIALOG_H

#include <QDialog>
#include <ui_qgsogrsublayersdialogbase.h>



class QgsOGRSublayersDialog : public QDialog, private Ui::QgsOGRSublayersDialogBase
{
    Q_OBJECT
  public:
    QgsOGRSublayersDialog( QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~QgsOGRSublayersDialog();
    void populateLayerTable( QStringList theList );
    QStringList getSelection();

};

#endif
