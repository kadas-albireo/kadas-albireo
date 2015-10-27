/***************************************************************************
    qgsredliningtextdialog.h - Redlining text dialog
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSREDLININGTEXTDIALOG_H
#define QGSREDLININGTEXTDIALOG_H

#include <QDialog>

#include "ui_qgsredliningtextdialog.h"

class QgsRedliningTextDialog : public QDialog
{
    Q_OBJECT
  public:
    QgsRedliningTextDialog( const QString& text, QString fontstr, double rotation, QWidget* parent = 0 );
    QString currentText() const { return ui.lineEditText->text(); }
    QFont currentFont() const;
    double rotation() const;

  private:
    Ui::RedliningTextDialog ui;

  private slots:
    void saveFont();
};

#endif // QGSREDLININGTEXTDIALOG_H
