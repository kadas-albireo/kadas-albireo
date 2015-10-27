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

QgsRedliningTextDialog::QgsRedliningTextDialog( const QString &text, QString fontstr, double rotation, QWidget *parent )
    : QDialog( parent )
{
  ui.setupUi( this );
  ui.lineEditText->setText( text );
  if ( fontstr.isEmpty() )
  {
    fontstr = QSettings().value( "/Redlining/font", font().toString() ).toString();
  }
  QFont textFont;
  textFont.fromString( fontstr );
  ui.checkBoxBold->setChecked( textFont.bold() );
  ui.checkBoxItalic->setChecked( textFont.italic() );
  textFont.setBold( false );
  textFont.setItalic( false );
  textFont.setPointSize( font().pointSize() );
  ui.fontComboBox->setCurrentFont( textFont );
  ui.spinBoxRotation->setValue( rotation );

  connect( this, SIGNAL( accepted() ), this, SLOT( saveFont() ) );
}

QFont QgsRedliningTextDialog::currentFont() const
{
  QFont font = ui.fontComboBox->currentFont();
  font.setBold( ui.checkBoxBold->isChecked() );
  font.setItalic( ui.checkBoxItalic->isChecked() );
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
