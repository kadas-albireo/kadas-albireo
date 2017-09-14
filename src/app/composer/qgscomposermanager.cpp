/***************************************************************************
                              qgscomposermanager.cpp
                             ------------------------
    begin                : September 11 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco at hugis dot net
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposermanager.h"
#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgsbusyindicatordialog.h"
#include "qgscomposition.h"
#include "qgslogger.h"

#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QUrl>
#include <QSettings>

QgsComposerManager::QgsComposerManager( QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f )
{
  setupUi( this );

  QSettings settings;
  restoreGeometry( settings.value( "/Windows/ComposerManager/geometry" ).toByteArray() );

  mComposerListWidget->setItemDelegate( new QgsComposerNameDelegate( mComposerListWidget ) );

  connect( mButtonBox, SIGNAL( rejected() ), this, SLOT( close() ) );
  connect( QgisApp::instance(), SIGNAL( compositionAdded( QgsComposition* ) ), this, SLOT( refreshComposers() ) );
  connect( QgisApp::instance(), SIGNAL( compositionRemoved( QgsComposition* ) ), this, SLOT( refreshComposers() ) );

  QPushButton* showBtn = new QPushButton( tr( "&Show" ) );
  mButtonBox->addButton( showBtn, QDialogButtonBox::ActionRole );
  connect( showBtn, SIGNAL( clicked() ), this, SLOT( show_clicked() ) );

  QPushButton* duplicateBtn = new QPushButton( tr( "&Duplicate" ) );
  mButtonBox->addButton( duplicateBtn, QDialogButtonBox::ActionRole );
  connect( duplicateBtn, SIGNAL( clicked() ), this, SLOT( duplicate_clicked() ) );

  QPushButton* removeBtn = new QPushButton( tr( "&Remove" ) );
  mButtonBox->addButton( removeBtn, QDialogButtonBox::ActionRole );
  connect( removeBtn, SIGNAL( clicked() ), this, SLOT( remove_clicked() ) );

  QPushButton* renameBtn = new QPushButton( tr( "Re&name" ) );
  mButtonBox->addButton( renameBtn, QDialogButtonBox::ActionRole );
  connect( renameBtn, SIGNAL( clicked() ), this, SLOT( rename_clicked() ) );

#ifdef Q_OS_MAC
  // Create action to select this window
  mWindowAction = new QAction( windowTitle(), this );
  connect( mWindowAction, SIGNAL( triggered() ), this, SLOT( activate() ) );
#endif

  mTemplate->addItem( tr( "Empty composer" ) );
  mTemplate->addItem( tr( "Specific" ) );

  mUserTemplatesDir = QgsApplication::qgisSettingsDirPath() + "/composer_templates";
  QMap<QString, QString> userTemplateMap = defaultTemplates( true );
  if ( !userTemplateMap.isEmpty() )
  {
    mTemplate->insertSeparator( mTemplate->count() );
    QMap<QString, QString>::const_iterator templateIt = userTemplateMap.constBegin();
    for ( ; templateIt != userTemplateMap.constEnd(); ++templateIt )
    {
      mTemplate->addItem( templateIt.key(), templateIt.value() );
    }
  }

  mDefaultTemplatesDir = QgsApplication::pkgDataPath() + "/composer_templates";
  QMap<QString, QString> defaultTemplateMap = defaultTemplates( false );
  if ( !defaultTemplateMap.isEmpty() )
  {
    mTemplate->insertSeparator( mTemplate->count() );
    QMap<QString, QString>::const_iterator templateIt = defaultTemplateMap.constBegin();
    for ( ; templateIt != defaultTemplateMap.constEnd(); ++templateIt )
    {
      mTemplate->addItem( templateIt.key(), templateIt.value() );
    }
  }

  mTemplatePathLineEdit->setText( settings.value( "/UI/ComposerManager/templatePath", QString( "" ) ).toString() );

  refreshComposers();
}

QgsComposerManager::~QgsComposerManager()
{
  QSettings().setValue( "/Windows/ComposerManager/geometry", saveGeometry() );
}

void QgsComposerManager::refreshComposers()
{
  QString selName = mComposerListWidget->currentItem() ? mComposerListWidget->currentItem()->text() : "";

  mComposerListWidget->clear();
  foreach ( QgsComposition* composition, QgisApp::instance()->printCompositions() )
  {
    QListWidgetItem* item = new QListWidgetItem( composition->title(), mComposerListWidget );
    item->setData( Qt::UserRole, QVariant::fromValue( composition ) );
    item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
  }
  mComposerListWidget->sortItems();

  // Restore selection
  if ( !selName.isEmpty() )
  {
    QList<QListWidgetItem*> items = mComposerListWidget->findItems( selName, Qt::MatchExactly );
    if ( !items.isEmpty() )
    {
      mComposerListWidget->setCurrentItem( items.first() );
    }
  }
}

void QgsComposerManager::activate()
{
  raise();
  setWindowState( windowState() & ~Qt::WindowMinimized );
  activateWindow();
}

QMap<QString, QString> QgsComposerManager::defaultTemplates( bool fromUser ) const
{
  QMap<QString, QString> templateMap;

  //search for default templates in $pkgDataPath/composer_templates
  // user templates in $qgisSettingsDirPath/composer_templates
  QDir defaultTemplateDir( fromUser ? mUserTemplatesDir : mDefaultTemplatesDir );
  if ( !defaultTemplateDir.exists() )
  {
    return templateMap;
  }

  foreach ( const QFileInfo& finfo, defaultTemplateDir.entryInfoList( QDir::Files ) )
  {
    templateMap.insert( finfo.baseName(), finfo.absoluteFilePath() );
  }
  return templateMap;
}

void QgsComposerManager::on_mAddButton_clicked()
{
  QFile templateFile;
  bool loadingTemplate = ( mTemplate->currentIndex() > 0 );
  if ( loadingTemplate )
  {
    if ( mTemplate->currentIndex() == 1 )
    {
      templateFile.setFileName( mTemplatePathLineEdit->text() );
    }
    else
    {
      templateFile.setFileName( mTemplate->itemData( mTemplate->currentIndex() ).toString() );
    }

    if ( !templateFile.exists() )
    {
      QMessageBox::warning( this, tr( "Template error" ), tr( "Error, template file not found" ) );
      return;
    }
    if ( !templateFile.open( QIODevice::ReadOnly ) )
    {
      QMessageBox::warning( this, tr( "Template error" ), tr( "Error, could not read file" ) );
      return;
    }
  }

  QString title = QgisApp::instance()->uniqueComposerTitle( this, true );
  if ( title.isNull() )
  {
    return;
  }
  QgsComposition* newComposition = QgisApp::instance()->createNewComposition( title );

  bool loadedOK = true;
  if ( loadingTemplate )
  {
    QDomDocument templateDoc;
    if ( templateDoc.setContent( &templateFile, false ) )
    {
      loadedOK = newComposition->loadFromTemplate( templateDoc );
      QgisApp::instance()->showComposer( newComposition );
    }
  }

  if ( !loadedOK )
  {
    QgisApp::instance()->deleteComposition( newComposition );
    QMessageBox::warning( this, tr( "Template error" ), tr( "Error, could not load template file" ) );
  }
}

void QgsComposerManager::on_mTemplate_currentIndexChanged( int indx )
{
  bool specific = ( indx == 1 ); // comes just after empty template
  mTemplatePathLineEdit->setEnabled( specific );
  mTemplatePathBtn->setEnabled( specific );
}

void QgsComposerManager::on_mTemplatePathBtn_pressed()
{
  QSettings settings;
  QString lastTmplDir = settings.value( "/UI/lastComposerTemplateDir", "." ).toString();
  QString tmplPath = QFileDialog::getOpenFileName( this,
                     tr( "Choose template" ),
                     lastTmplDir,
                     tr( "Composer templates" ) + " (*.qpt)" );
  if ( !tmplPath.isNull() )
  {
    mTemplatePathLineEdit->setText( tmplPath );
    settings.setValue( "UI/ComposerManager/templatePath", tmplPath );
    QFileInfo tmplFileInfo( tmplPath );
    settings.setValue( "UI/lastComposerTemplateDir", tmplFileInfo.absolutePath() );
  }
}

void QgsComposerManager::on_mTemplatesDefaultDirBtn_pressed()
{
  openLocalDirectory( mDefaultTemplatesDir );
}

void QgsComposerManager::on_mTemplatesUserDirBtn_pressed()
{
  openLocalDirectory( mUserTemplatesDir );
}

void QgsComposerManager::openLocalDirectory( const QString& localDirPath )
{
  QDir localDir;
  if ( !localDir.mkpath( localDirPath ) )
  {
    QMessageBox::warning( this, tr( "File system error" ), tr( "Error, could not open or create local directory" ) );
    return;
  }
  QDesktopServices::openUrl( QUrl::fromLocalFile( localDirPath ) );
}

#ifdef Q_OS_MAC
void QgsComposerManager::showEvent( QShowEvent* event )
{
  if ( !event->spontaneous() )
  {
    QgisApp::instance()->addWindow( mWindowAction );
  }
}

void QgsComposerManager::changeEvent( QEvent* event )
{
  QDialog::changeEvent( event );
  switch ( event->type() )
  {
    case QEvent::ActivationChange:
      if ( QApplication::activeWindow() == this )
      {
        mWindowAction->setChecked( true );
      }
      break;

    default:
      break;
  }
}
#endif

void QgsComposerManager::remove_clicked()
{
  QListWidgetItem* item = mComposerListWidget->currentItem();
  if ( !item )
  {
    return;
  }

  //ask for confirmation
  if ( QMessageBox::warning( this, tr( "Remove composer" ), tr( "Do you really want to remove the map composer '%1'?" ).arg( item->text() ), QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Ok )
  {
    QgisApp::instance()->deleteComposition( item->data( Qt::UserRole ).value<QgsComposition*>() );
  }
}

void QgsComposerManager::show_clicked()
{
  QListWidgetItem* item = mComposerListWidget->currentItem();
  if ( !item )
  {
    return;
  }

  QgisApp::instance()->showComposer( item->data( Qt::UserRole ).value<QgsComposition*>() );
}

void QgsComposerManager::duplicate_clicked()
{
  QListWidgetItem* item = mComposerListWidget->currentItem();
  if ( !item )
  {
    return;
  }

  QgsComposition* composition = item->data( Qt::UserRole ).value<QgsComposition*>();
  QString newTitle = QgisApp::instance()->uniqueComposerTitle( this, false, composition->title() + " copy" );
  if ( !newTitle.isNull() )
  {
    QgisApp::instance()->duplicateComposition( composition, newTitle );
  }
}

void QgsComposerManager::rename_clicked()
{
  QListWidgetItem* item = mComposerListWidget->currentItem();
  if ( !item )
  {
    return;
  }

  QgsComposition* composition = item->data( Qt::UserRole ).value<QgsComposition*>();
  QString newTitle = QgisApp::instance()->uniqueComposerTitle( this, false, composition->title() );
  if ( !newTitle.isNull() )
  {
    composition->setTitle( newTitle );
    item->setText( newTitle );
    mComposerListWidget->sortItems();
  }
}

void QgsComposerManager::on_mComposerListWidget_itemChanged( QListWidgetItem * item )
{
  QgsComposition* composition = item->data( Qt::UserRole ).value<QgsComposition*>();
  composition->setTitle( item->text() );
  mComposerListWidget->sortItems();
}

//
// QgsComposerNameDelegate
//

QWidget *QgsComposerNameDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  Q_UNUSED( option );
  Q_UNUSED( index );
  return new QLineEdit( parent );
}

void QgsComposerNameDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
  QString text = index.model()->data( index, Qt::EditRole ).toString();
  static_cast<QLineEdit*>( editor )->setText( text );
}

void QgsComposerNameDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  QString value = static_cast<QLineEdit*>( editor )->text();

  //has name changed?
  if ( model->data( index, Qt::EditRole ).toString() == value )
  {
    return;
  }

  //check if name already exists
  foreach ( QgsComposition* c, QgisApp::instance()->printCompositions() )
  {
    if ( c->title() == value )
    {
      QMessageBox::warning( 0, tr( "Rename composer" ), tr( "There is already a composer named \"%1\"" ).arg( value ) );
      return;
    }
  }

  model->setData( index, QVariant( value ), Qt::EditRole );
}

void QgsComposerNameDelegate::updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  Q_UNUSED( index );
  editor->setGeometry( option.rect );
}
