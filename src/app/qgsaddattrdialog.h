/***************************************************************************
                         qgsaddattrdialog.h  -  description
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSADDATTRDIALOG_H
#define QGSADDATTRDIALOG_H

#include "ui_qgsaddattrdialogbase.h"
#include "qgisgui.h"

class QgsVectorDataProvider;

class QgsAddAttrDialog: public QDialog, private Ui::QgsAddAttrDialogBase
{
    Q_OBJECT
  public:
    QgsAddAttrDialog( QgsVectorDataProvider* provider,
                      QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags );
    QgsAddAttrDialog( const std::list<QString>& typelist,
                      QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags );
    QString name() const;
    QString type() const;
  protected:
    QgsVectorDataProvider* mDataProvider;
};

#endif
