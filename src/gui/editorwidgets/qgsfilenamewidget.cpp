/***************************************************************************
    qgsfilenamewidget.cpp
     --------------------------------------
    Date                 : 5.1.2014
    Copyright            : (C) 2014 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsfilenamewidget.h"

#include "qgsfilterlineedit.h"

#include <QFileDialog>
#include <QGridLayout>

QgsFileNameWidget::QgsFileNameWidget( QgsVectorLayer* vl, int fieldIdx, QWidget* editor, QWidget* parent )
    :  QgsEditorWidgetWrapper( vl, fieldIdx, editor, parent )
{
}

QVariant QgsFileNameWidget::value()
{
  QVariant value;

  if ( mLineEdit )
    value = mLineEdit->text();

  if ( mLabel )
    value = mLabel->text();

  return value;
}

QWidget* QgsFileNameWidget::createWidget( QWidget* parent )
{
  QWidget* container = new QWidget( parent );
  container->setBackgroundRole( QPalette::Window );
  container->setAutoFillBackground( true );

  QLineEdit* le = new QgsFilterLineEdit( container );
  QPushButton* pbn = new QPushButton( tr( "..." ), container );
  QGridLayout* layout = new QGridLayout();

  layout->setMargin( 0 );
  layout->addWidget( le, 0, 0 );
  layout->addWidget( pbn, 0, 1 );

  container->setLayout( layout );

  return container;
}

void QgsFileNameWidget::initWidget( QWidget* editor )
{
  mLineEdit = qobject_cast<QLineEdit*>( editor );
  if ( !mLineEdit )
  {
    mLineEdit = editor->findChild<QLineEdit*>();
  }

  mPushButton = editor->findChild<QPushButton*>();

  if ( mPushButton )
    connect( mPushButton, SIGNAL( clicked() ), this, SLOT( selectFileName() ) );

  mLabel = qobject_cast<QLabel*>( editor );

  if ( mLineEdit )
    connect( mLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( valueChanged( QString ) ) );
}

void QgsFileNameWidget::setValue( const QVariant& value )
{
  if ( mLineEdit )
    mLineEdit->setText( value.toString() );

  if ( mLabel )
    mLabel->setText( value.toString() );
}

void QgsFileNameWidget::selectFileName()
{
  QString text;

  if ( mLineEdit )
    text = mLineEdit->text();

  if ( mLabel )
    text = mLabel->text();

  QString fileName = QFileDialog::getOpenFileName( mLineEdit, tr( "Select a file" ), QFileInfo( text ).absolutePath() );

  if ( fileName.isNull() )
    return;

  if ( mLineEdit )
    mLineEdit->setText( QDir::toNativeSeparators( fileName ) );

  if ( mLabel )
    mLineEdit->setText( QDir::toNativeSeparators( fileName ) );
}
