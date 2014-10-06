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
#include "qgsfield.h"

class QgsVectorLayer;

class APP_EXPORT QgsAddAttrDialog: public QDialog, private Ui::QgsAddAttrDialogBase
{
    Q_OBJECT
  public:
    enum AttributeMode
    {
      ProviderField,
      VirtualField
    };

    QgsAddAttrDialog( QgsVectorLayer *vlayer,
                      QWidget *parent = 0, Qt::WindowFlags fl = QgisGui::ModalDialogFlags );

    QgsField field() const;
    const QString expression() const;
    AttributeMode mode() const;

  private slots:
    void on_mTypeBox_currentIndexChanged( int idx );
    void on_mFieldModeButtonGroup_buttonClicked( QAbstractButton* button );
    void on_mLength_editingFinished();
    void accept();

  private:
    bool mIsShapeFile;
    QgsVectorLayer* mLayer;

    void setPrecisionMinMax();
};

#endif
