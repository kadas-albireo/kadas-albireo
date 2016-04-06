/***************************************************************************
    qgsredliningtextdialog.cpp - Redlining text dialog
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

#include "qgsredliningtextdialog.h"
#include <QSettings>

QgsRedliningTextDialog::QgsRedliningTextDialog( const QString &text, const QFont &font, double rotation, QWidget *parent )
    : QDialog( parent )
{
  ui.setupUi( this );
  ui.lineEditText->setText( text );
  ui.checkBoxBold->setChecked( font.bold() );
  ui.checkBoxItalic->setChecked( font.italic() );
  ui.spinBoxFontSize->setValue( font.pointSize() );
  ui.spinBoxRotation->setValue( rotation );
  // Set only family to make the text in the fontComboBox appear normal
  QFont fontComboFont;
  fontComboFont.setFamily( font.family() );
  ui.fontComboBox->setCurrentFont( fontComboFont );
  ui.lineEditText->setFocus();

  connect( this, SIGNAL( accepted() ), this, SLOT( saveFont() ) );
}

QFont QgsRedliningTextDialog::currentFont() const
{
  QFont font = ui.fontComboBox->currentFont();
  font.setBold( ui.checkBoxBold->isChecked() );
  font.setItalic( ui.checkBoxItalic->isChecked() );
  font.setPointSize( ui.spinBoxFontSize->value() );
  return font;
}

double QgsRedliningTextDialog::rotation() const
{
  return ui.spinBoxRotation->value();
}

void QgsRedliningTextDialog::saveFont()
{
  QSettings().setValue( "/Redlining/font", currentFont().toString() );
}
