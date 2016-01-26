/***************************************************************************
    qgsprojecttemplateselectiondialog.cpp
    -------------------------------------
  begin                : January 2016
  copyright            : (C) 2016 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsprojecttemplateselectiondialog.h"
#include "qgsapplication.h"
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileSystemModel>
#include <QKeyEvent>
#include <QSettings>
#include <QTreeView>
#include <QVBoxLayout>

QgsProjectTemplateSelectionDialog::QgsProjectTemplateSelectionDialog( QWidget *parent )
    : QDialog( parent )
{
  setWindowTitle( tr( "Select project template" ) );

  mModel = new QFileSystemModel( this );
  mModel->setNameFilters( QStringList() << "*.qgs" );
  mModel->setNameFilterDisables( false );
  mModel->setReadOnly( true );

  mTreeView = new QTreeView( this );
  mTreeView->setModel( mModel );
  mTreeView->setRootIndex( mModel->setRootPath( QSettings().value( "/qgis/projectTemplateDir", QgsApplication::qgisSettingsDirPath() + "project_templates" ).toString() ) );
  for ( int i = 1, n = mModel->columnCount(); i < n; ++i )
  {
    mTreeView->setColumnHidden( i, true );
  }
  mTreeView->setHeaderHidden( true );
  mTreeView->setIconSize( QSize( 32, 32 ) );
  connect( mTreeView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( itemClicked( QModelIndex ) ) );
  connect( mTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( itemDoubleClicked( QModelIndex ) ) );

  mButtonBox = new QDialogButtonBox( QDialogButtonBox::Open | QDialogButtonBox::Cancel, Qt::Horizontal, this );
  mButtonBox->button( QDialogButtonBox::Open )->setEnabled( false );

  setLayout( new QVBoxLayout() );
  layout()->addWidget( mTreeView );
  layout()->addWidget( mButtonBox );

  connect( mButtonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( mButtonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
}

void QgsProjectTemplateSelectionDialog::itemClicked( const QModelIndex& index )
{
  mButtonBox->button( QDialogButtonBox::Open )->setEnabled( mModel->fileInfo( index ).isFile() );
}

void QgsProjectTemplateSelectionDialog::itemDoubleClicked( const QModelIndex& index )
{
  if ( mModel->fileInfo( index ).isFile() )
  {
    accept();
  }
}

QString QgsProjectTemplateSelectionDialog::run()
{
  if ( exec() == QDialog::Accepted )
  {
    return mModel->fileInfo( mTreeView->currentIndex() ).absoluteFilePath();
  }
  return QString();
}
