/***************************************************************************
    qgsfloatinginputwidget.h  -  A floating CAD-style input widget
    -------------------------------------------------------------
  begin                : Nov 26, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsfloatinginputwidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QKeyEvent>

QgsFloatingInputWidgetField::QgsFloatingInputWidgetField( QValidator* validator, QWidget* parent ) : QLineEdit( parent )
{
  setValidator( validator );
  connect( this, SIGNAL( returnPressed() ), this, SLOT( checkInputChanged() ) );
}

void QgsFloatingInputWidgetField::setText( const QString& text )
{
  QLineEdit::setText( text );
  mPrevText = text;
}

void QgsFloatingInputWidgetField::focusOutEvent( QFocusEvent *ev )
{
  QString curText = text();
  if ( curText != mPrevText )
  {
    mPrevText = curText;
    emit inputChanged();
  }
  QLineEdit::focusOutEvent( ev );
}

void QgsFloatingInputWidgetField::checkInputChanged()
{
  QString curText = text();
  if ( curText != mPrevText )
  {
    mPrevText = curText;
    emit inputChanged();
  }
  else
  {
    emit inputConfirmed();
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsFloatingInputWidget::QgsFloatingInputWidget( QWidget* parent ) : QWidget( parent ), mFocusedInput( 0 )
{
  setObjectName( "FloatingInputWidget" );
  QGridLayout* gridLayout = new QGridLayout();
  gridLayout->setContentsMargins( 2, 2, 2, 2 );
  gridLayout->setSpacing( 1 );
  setLayout( gridLayout );
}

void QgsFloatingInputWidget::addInputField( const QString &label, QgsFloatingInputWidgetField* widget, bool initiallyfocused )
{
  QGridLayout* gridLayout = static_cast<QGridLayout*>( layout() );
  int row = gridLayout->rowCount();
  gridLayout->addWidget( new QLabel( label ), row, 0, 1, 1 );;
  gridLayout->addWidget( widget, row, 1, 1, 1 );
  mInputFields.append( widget );
  if ( initiallyfocused )
  {
    setFocusedInputField( widget );
  }
}

void QgsFloatingInputWidget::setFocusedInputField( QgsFloatingInputWidgetField* widget )
{
  if ( mFocusedInput )
  {
    mFocusedInput->removeEventFilter( this );
  }
  mFocusedInput = widget;
  mFocusedInput->setFocus();
  mFocusedInput->selectAll();
  mFocusedInput->installEventFilter( this );
}

bool QgsFloatingInputWidget::eventFilter( QObject *obj, QEvent *ev )
{
  // If currently focused widget loses focus, make it receive focus again
  if ( obj == mFocusedInput && ev->type() == QEvent::FocusOut )
  {
    mFocusedInput->setFocus();
    mFocusedInput->selectAll();
    return true;
  }
  return QWidget::eventFilter( obj, ev );
}

bool QgsFloatingInputWidget::focusNextPrevChild( bool /*next*/ )
{
  // Disable automatic TAB event handling
  // http://stackoverflow.com/a/21351638/1338788
  return false;
}

void QgsFloatingInputWidget::keyPressEvent( QKeyEvent *ev )
{
  // Override tab handling to ensure only the input fields inside the widget receive focus
  if ( ev->key() == Qt::Key_Tab )
  {
    int n = mInputFields.size();
    int nextIdx = ( mInputFields.indexOf( mFocusedInput ) + 1 ) % n;
    setFocusedInputField( mInputFields[nextIdx] );
    ev->accept();
  }
  else if ( ev->key() == Qt::Key_Backtab )
  {
    int n = mInputFields.size();
    int nextIdx = ( n + mInputFields.indexOf( mFocusedInput ) - 1 ) % n;
    setFocusedInputField( mInputFields[nextIdx] );
    ev->accept();
  }
  else
  {
    QWidget::keyPressEvent(( ev ) );
  }
}
