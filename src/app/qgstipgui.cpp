/***************************************************************************
                          qgstipgui.cpp  -  description
                             -------------------
    begin                : Sat Aug 10 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QSettings>
#include <QPushButton>

#include "qgstipgui.h"
#include "qgsapplication.h"
#include <qgstip.h>
#include <qgstipfactory.h>

#ifdef Q_OS_MACX
QgsTipGui::QgsTipGui()
    : QDialog( NULL, Qt::WindowSystemMenuHint )  // Modeless dialog with close button only
#else
QgsTipGui::QgsTipGui()
    : QDialog( NULL )  // Normal dialog in non Mac-OS
#endif
{
  setupUi( this );
  init();
}

QgsTipGui::~QgsTipGui()
{
}

void QgsTipGui::init()
{

  QgsTipFactory myFactory;
  QgsTip myTip = myFactory.getTip();
  mTipPosition = myFactory.position( myTip );

  showTip( myTip );

  QPushButton *pb;
  pb = new QPushButton( tr( "&Previous" ) );
  connect( pb, SIGNAL( clicked() ), this, SLOT( prevClicked() ) );
  buttonBox->addButton( pb, QDialogButtonBox::ActionRole );

  pb = new QPushButton( tr( "&Next" ) );
  connect( pb, SIGNAL( clicked() ), this, SLOT( nextClicked() ) );
  buttonBox->addButton( pb, QDialogButtonBox::ActionRole );
}

void QgsTipGui::showTip( QgsTip myTip )
{
  // TODO - This html construction can be simplified using QStringBuilder
  //        once Qt 4.6 is the minimum required version for building QGIS.
  //
  QString content = "<img src='"
                    + QgsApplication::iconsPath()
                    + "qgis-icon-60x60.png"
                    + "' style='float:left;'>"
                    + "<h2>"
                    + myTip.title()
                    + "</h2><br clear='all'/>"
                    + myTip.content();

  txtTip->setHtml( content );
}

void QgsTipGui::on_cbxDisableTips_toggled( bool theFlag )
{
  QSettings settings;
  //note the ! below as when the cbx is checked (true) we want to
  //change the setting to false
  settings.setValue( "/qgis/showTips", !theFlag );
  hide();
}

void QgsTipGui::nextClicked()
{
  mTipPosition += 1;
  QgsTipFactory myFactory;
  if ( mTipPosition >= myFactory.count() )
  {
    mTipPosition = 0;
  }
  QgsTip myTip = myFactory.getTip( mTipPosition );
  showTip( myTip );
}

void QgsTipGui::prevClicked()
{
  mTipPosition -= 1;
  QgsTipFactory myFactory;
  if ( mTipPosition < 0 )
  {
    mTipPosition = myFactory.count() - 1;
  }
  QgsTip myTip = myFactory.getTip( mTipPosition );
  showTip( myTip );
}
