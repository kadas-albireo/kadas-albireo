/***************************************************************************
                qgsattributeactiondialog.h  -  attribute action dialog
                             -------------------

This class creates and manages the Action tab of the Vector Layer
Properties dialog box. Changes made in the dialog box are propagated
back to QgsVectorLayer.

    begin                : October 2004
    copyright            : (C) 2004 by Gavin Macaulay
    email                : gavin at macaulay dot co dot nz
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSATTRIBUTEACTIONDIALOG_H
#define QGSATTRIBUTEACTIONDIALOG_H

#include "ui_qgsattributeactiondialogbase.h"
#include "qgsfield.h"
#include <vector>

class QgsAttributeAction;


class QgsAttributeActionDialog: public QWidget, private Ui::QgsAttributeActionDialogBase
{
  Q_OBJECT;
  
 public:
  QgsAttributeActionDialog(QgsAttributeAction* actions, 
			   const std::vector<QgsField>& fields,
			   QWidget* parent = 0);

  ~QgsAttributeActionDialog() {};

  void init();

 public slots:
  void moveUp();
  void moveDown();
  void browse();
  void remove();
  void insert();
  void insertField();
  void apply();
  void update();
  void rowSelected(int, int, int, const QPoint&);

 private:
 
  void insert(int pos);

  QString uniqueName(QString name);

  // Pointer to the QgsAttributeAction in the class that created us.
  QgsAttributeAction* mActions;
};

#endif
