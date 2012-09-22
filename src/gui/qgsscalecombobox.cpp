/***************************************************************************
                              qgsscalecombobox.h
                              ------------------------
  begin                : January 7, 2012
  copyright            : (C) 2012 by Alexander Bruy
  email                : alexander dot bruy at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgis.h"
#include "qgslogger.h"
#include "qgsscalecombobox.h"

#include <QAbstractItemView>
#include <QLocale>
#include <QSettings>
#include <QLineEdit>

QgsScaleComboBox::QgsScaleComboBox( QWidget* parent ) : QComboBox( parent )
{
  updateScales();

  setEditable( true );
  setInsertPolicy( QComboBox::NoInsert );
  setCompleter( 0 );
  connect( this, SIGNAL( activated( const QString & ) ), this, SLOT( fixupScale() ) );
  connect( lineEdit(), SIGNAL( editingFinished() ), this, SLOT( fixupScale() ) );
  fixupScale();
}

QgsScaleComboBox::~QgsScaleComboBox()
{
}

void QgsScaleComboBox::updateScales( const QStringList &scales )
{
  QStringList myScalesList;
  QString oldScale = currentText();

  if ( scales.isEmpty() )
  {
    QSettings settings;
    QString myScales = settings.value( "Map/scales", PROJECT_SCALES ).toString();
    if ( !myScales.isEmpty() )
    {
      myScalesList = myScales.split( "," );
    }
  }
  else
  {
    QStringList::const_iterator scaleIt = scales.constBegin();
    for ( ; scaleIt != scales.constEnd(); ++scaleIt )
    {
      myScalesList.append( *scaleIt );
    }
  }

  blockSignals( true );
  clear();
  addItems( myScalesList );
  setScaleString( oldScale );
  blockSignals( false );
}

void QgsScaleComboBox::showPopup()
{
  QComboBox::showPopup();

  if ( !currentText().contains( ':' ) )
  {
    return;
  }
  QStringList parts = currentText().split( ':' );
  bool ok;
  int idx = 0;
  int min = 999999;
  long currScale = parts.at( 1 ).toLong( &ok );
  long nextScale, delta;
  for ( int i = 0; i < count(); i++ )
  {
    parts = itemText( i ).split( ':' );
    nextScale = parts.at( 1 ).toLong( &ok );
    delta = qAbs( currScale - nextScale );
    if ( delta < min )
    {
      min = delta;
      idx = i;
    }
  }

  blockSignals( true );
  view()->setCurrentIndex( model()->index( idx, 0 ) );
  blockSignals( false );
}

//! Function to read the selected scale as text
// @note added in 2.0
QString QgsScaleComboBox::scaleString()
{
  return toString( mScale );
}

//! Function to set the selected scale from text
// @note added in 2.0
bool QgsScaleComboBox::setScaleString( QString scaleTxt )
{
  bool ok;
  double newScale = toDouble( scaleTxt, &ok );
  if ( ! ok )
  {
    return false;
  }
  else
  {
    mScale = newScale;
    setEditText( toString( mScale ) );
    clearFocus();
    return true;
  }
}

//! Function to read the selected scale as double
// @note added in 2.0
double QgsScaleComboBox::scale()
{
  return mScale;
}

//! Function to set the selected scale from double
// @note added in 2.0
void QgsScaleComboBox::setScale( double scale )
{
  setScaleString( toString( scale ) );
}

//! Slot called when QComboBox has changed
void QgsScaleComboBox::fixupScale()
{
  double newScale;
  double oldScale = mScale;
  bool ok;
  QStringList txtList;

  // QgsDebugMsg( QString( "entered with oldScale: %1" ).arg( oldScale ) );
  newScale = toDouble( currentText(), &ok );
  if ( ok )
  {
    // Valid string representation
    if ( newScale != oldScale )
    {
      // Scale has change, update.
      // QgsDebugMsg( QString( "New scale OK!: %1" ).arg( newScale ) );
      mScale = newScale;
      setScale( mScale );
      emit scaleChanged();
    }
  }
  else
  {
    // Invalid string representation
    // Reset to the old
    setScale( mScale );
  }
}

QString QgsScaleComboBox::toString( double scale )
{
  if ( scale > 1 )
  {
    return QString( "%1:1" ).arg( qRound( scale ) );
  }
  else
  {
    return QString( "1:%1" ).arg( qRound( 1.0 / scale ) );
  }
}

double QgsScaleComboBox::toDouble( QString scaleString, bool * returnOk )
{
  bool ok = false;
  QString scaleTxt( scaleString );

  double scale = QLocale::system().toDouble( scaleTxt, &ok );
  if ( ok )
  {
    // Create a text version and set that text and rescan
    // Idea is to get the same rounding.
    scaleTxt = toString( scale );
  }
  // It is now either X:Y or not valid
  ok = false;
  QStringList txtList = scaleTxt.split( ':' );
  if ( 2 == txtList.size() )
  {
    bool okX = false;
    bool okY = false;
    int x = QLocale::system().toInt( txtList[ 0 ], &okX );
    int y = QLocale::system().toInt( txtList[ 1 ], &okY );
    if ( okX && okY )
    {
      // Scale is fraction of x and y
      scale = ( double )x / ( double )y;
      ok = true;
    }
  }

  // Set up optional return flag
  if ( returnOk )
  {
    *returnOk = ok;
  }
  return scale;
}
