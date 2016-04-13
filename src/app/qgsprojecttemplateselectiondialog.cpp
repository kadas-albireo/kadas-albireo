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
#include "qgisapp.h"
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
  setupUi( this );

  mModel = new QFileSystemModel( this );
  mModel->setNameFilters( QStringList() << "*.qgs" );
  mModel->setNameFilterDisables( false );
  mModel->setReadOnly( true );

  mTreeView->setModel( mModel );
  mTreeView->setRootIndex( mModel->setRootPath( QSettings().value( "/qgis/projectTemplateDir", QgsApplication::qgisSettingsDirPath() + "project_templates" ).toString() ) );
  for ( int i = 1, n = mModel->columnCount(); i < n; ++i )
  {
    mTreeView->setColumnHidden( i, true );
  }
  connect( mTreeView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( itemClicked( QModelIndex ) ) );
  connect( mTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( itemDoubleClicked( QModelIndex ) ) );

  connect( mRadioButtonGroup, SIGNAL( buttonClicked( int ) ), this, SLOT( radioChanged() ) );

  mCreateButton = mButtonBox->addButton( tr( "Create" ), QDialogButtonBox::AcceptRole );
  mCreateButton->setEnabled( false );
  connect( mCreateButton, SIGNAL( clicked() ), this, SLOT( createProject() ) );
}

void QgsProjectTemplateSelectionDialog::itemClicked( const QModelIndex& index )
{
  mCreateButton->setEnabled( mModel->fileInfo( index ).isFile() );
}

void QgsProjectTemplateSelectionDialog::itemDoubleClicked( const QModelIndex& index )
{
  if ( mModel->fileInfo( index ).isFile() )
  {
    createProject();
  }
}

void QgsProjectTemplateSelectionDialog::createProject()
{
  if ( mProjectFromTemplateRadio->isChecked() )
  {
    QString filename = mModel->fileInfo( mTreeView->currentIndex() ).absoluteFilePath();
    if ( !filename.isEmpty() )
    {
      QgisApp::instance()->fileNewFromTemplate( filename );
      accept();
    }
  }
  else
  {
    QgisApp::instance()->fileNew( true );
    accept();
  }
}

void QgsProjectTemplateSelectionDialog::radioChanged()
{
  if ( mRadioButtonGroup->checkedButton() == mProjectFromTemplateRadio )
  {
    mCreateButton->setEnabled( mModel->fileInfo( mTreeView->currentIndex() ).isFile() );
    mTreeView->setEnabled( true );
  }
  else
  {
    mCreateButton->setEnabled( true );
    mTreeView->setEnabled( false );
  }
}
