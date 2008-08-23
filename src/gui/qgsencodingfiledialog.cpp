/***************************************************************************
    qgsencodingfiledialog.cpp - File dialog which queries the encoding type
     --------------------------------------
    Date                 : 16-Feb-2005
    Copyright            : (C) 2005 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsencodingfiledialog.h"
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QTextCodec>


QgsEncodingFileDialog::QgsEncodingFileDialog( QWidget * parent,
    const QString & caption, const QString & directory,
    const QString & filter, const QString & encoding )
    : QFileDialog( parent, caption, directory, filter )
{
  mEncodingComboBox = new QComboBox( this );
  QLabel* l = new QLabel( tr( "Encoding:" ), this );
  layout()->addWidget( l );
  layout()->addWidget( mEncodingComboBox );
  mEncodingComboBox->addItem( "BIG5" );
  mEncodingComboBox->addItem( "BIG5-HKSCS" );
  mEncodingComboBox->addItem( "EUCJP" );
  mEncodingComboBox->addItem( "EUCKR" );
  mEncodingComboBox->addItem( "GB2312" );
  mEncodingComboBox->addItem( "GBK" );
  mEncodingComboBox->addItem( "GB18030" );
  mEncodingComboBox->addItem( "JIS7" );
  mEncodingComboBox->addItem( "SHIFT-JIS" );
  mEncodingComboBox->addItem( "TSCII" );
  mEncodingComboBox->addItem( "UTF-8" );
  mEncodingComboBox->addItem( "UTF-16" );
  mEncodingComboBox->addItem( "KOI8-R" );
  mEncodingComboBox->addItem( "KOI8-U" );
  mEncodingComboBox->addItem( "ISO8859-1" );
  mEncodingComboBox->addItem( "ISO8859-2" );
  mEncodingComboBox->addItem( "ISO8859-3" );
  mEncodingComboBox->addItem( "ISO8859-4" );
  mEncodingComboBox->addItem( "ISO8859-5" );
  mEncodingComboBox->addItem( "ISO8859-6" );
  mEncodingComboBox->addItem( "ISO8859-7" );
  mEncodingComboBox->addItem( "ISO8859-8" );
  mEncodingComboBox->addItem( "ISO8859-8-I" );
  mEncodingComboBox->addItem( "ISO8859-9" );
  mEncodingComboBox->addItem( "ISO8859-10" );
  mEncodingComboBox->addItem( "ISO8859-13" );
  mEncodingComboBox->addItem( "ISO8859-14" );
  mEncodingComboBox->addItem( "ISO8859-15" );
  mEncodingComboBox->addItem( "IBM 850" );
  mEncodingComboBox->addItem( "IBM 866" );
  mEncodingComboBox->addItem( "CP874" );
  mEncodingComboBox->addItem( "CP1250" );
  mEncodingComboBox->addItem( "CP1251" );
  mEncodingComboBox->addItem( "CP1252" );
  mEncodingComboBox->addItem( "CP1253" );
  mEncodingComboBox->addItem( "CP1254" );
  mEncodingComboBox->addItem( "CP1255" );
  mEncodingComboBox->addItem( "CP1256" );
  mEncodingComboBox->addItem( "CP1257" );
  mEncodingComboBox->addItem( "CP1258" );
  mEncodingComboBox->addItem( "Apple Roman" );
  mEncodingComboBox->addItem( "TIS-620" );
  if ( encoding.isEmpty() )
  {
    mEncodingComboBox->setItemText( mEncodingComboBox->currentIndex(), QString( QTextCodec::codecForLocale()->name() ) );
  }
  else
  {
    mEncodingComboBox->setItemText( mEncodingComboBox->currentIndex(), encoding );
  }

  // if this dialog is being invoked from QgisApp::findFiles_(), then we
  // need to force selection of the first filter since that corresponds to
  // the file name we're looking for; even if we're not here from
  // findFiles_(), it won't hurt to force selection of the first file filter
  selectFilter( filters().at( 0 ) );

}

QgsEncodingFileDialog::~QgsEncodingFileDialog()
{

}

QString QgsEncodingFileDialog::encoding() const
{
  return mEncodingComboBox->currentText();
}
