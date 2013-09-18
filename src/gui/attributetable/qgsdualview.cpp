/***************************************************************************
    qgsdualview.cpp
     --------------------------------------
    Date                 : 10.2.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdualview.h"
#include "qgsmapcanvas.h"
#include "qgsvectorlayercache.h"
#include "qgsattributetablemodel.h"
#include "qgsfeaturelistmodel.h"
#include "qgsattributedialog.h"
#include "qgsapplication.h"
#include "qgsexpressionbuilderdialog.h"
#include "qgsattributeaction.h"
#include "qgsvectordataprovider.h"
#include "qgsmessagelog.h"

#include <QDialog>
#include <QMenu>
#include <QProgressDialog>
#include <QMessageBox>

QgsDualView::QgsDualView( QWidget* parent )
    : QStackedWidget( parent )
    , mAttributeDialog( NULL )
    , mProgressDlg( NULL )
{
  setupUi( this );

  mPreviewActionMapper = new QSignalMapper( this );

  mPreviewColumnsMenu = new QMenu( this );
  mActionPreviewColumnsMenu->setMenu( mPreviewColumnsMenu );

  // Set preview icon
  mActionExpressionPreview->setIcon( QgsApplication::getThemeIcon( "/mIconExpressionPreview.svg" ) );

  // Connect layer list preview signals
  connect( mActionExpressionPreview, SIGNAL( triggered() ), SLOT( previewExpressionBuilder() ) );
  connect( mPreviewActionMapper, SIGNAL( mapped( QObject* ) ), SLOT( previewColumnChanged( QObject* ) ) );
  connect( mFeatureList, SIGNAL( displayExpressionChanged( QString ) ), this, SLOT( previewExpressionChanged( QString ) ) );
  connect( this, SIGNAL( currentChanged( int ) ), this, SLOT( saveEditChanges() ) );
}

QgsDualView::~QgsDualView()
{
  delete mAttributeDialog;
}

void QgsDualView::init( QgsVectorLayer* layer, QgsMapCanvas* mapCanvas, QgsDistanceArea myDa, const QgsFeatureRequest &request )
{
  mDistanceArea = myDa;

  connect( mTableView, SIGNAL( willShowContextMenu( QMenu*, QModelIndex ) ), this, SLOT( viewWillShowContextMenu( QMenu*, QModelIndex ) ) );

  initLayerCache( layer );
  initModels( mapCanvas, request );

  mTableView->setModel( mFilterModel );
  mFeatureList->setModel( mFeatureListModel );

  mAttributeDialog = new QgsAttributeDialog( layer, 0, false, myDa );
  if ( mAttributeDialog->dialog() )
    mAttributeEditorLayout->addWidget( mAttributeDialog->dialog() );

  columnBoxInit();
}

void QgsDualView::columnBoxInit()
{
  // load fields
  QList<QgsField> fields = mLayerCache->layer()->pendingFields().toList();

  QString defaultField;

  // default expression: saved value
  QString displayExpression = mLayerCache->layer()->displayExpression();

  // if no display expression is saved: use display field instead
  if ( displayExpression == "" )
  {
    if ( mLayerCache->layer()->displayField() != "" )
    {
      defaultField = mLayerCache->layer()->displayField();
      displayExpression = QString( "COALESCE(\"%1\", '<NULL>')" ).arg( defaultField );
    }
  }

  // if neither diaplay expression nor display field is saved...
  if ( displayExpression == "" )
  {
    QgsAttributeList pkAttrs = mLayerCache->layer()->pendingPkAttributesList();

    if ( pkAttrs.size() > 0 )
    {
      if ( pkAttrs.size() == 1 )
        defaultField = pkAttrs.at( 0 );

      // ... If there are primary key(s) defined
      QStringList pkFields;

      Q_FOREACH( int attr, pkAttrs )
      {
        pkFields.append( "COALESCE(\"" + fields[attr].name() + "\", '<NULL>')" );
      }

      displayExpression = pkFields.join( "||', '||" );
    }
    else if ( fields.size() > 0 )
    {
      if ( fields.size() == 1 )
        defaultField = fields.at( 0 ).name();

      // ... concat all fields
      QStringList fieldNames;
      foreach ( QgsField field, fields )
      {
        fieldNames.append( "COALESCE(\"" + field.name() + "\", '<NULL>')" );
      }

      displayExpression = fieldNames.join( "||', '||" );
    }
    else
    {
      // ... there isn't really much to display
      displayExpression = "'[Please define preview text]'";
    }
  }

  // now initialise the menu
  QList< QAction* > previewActions = mFeatureListPreviewButton->actions();
  foreach ( QAction* a, previewActions )
  {
    if ( a != mActionExpressionPreview )
    {
      mPreviewActionMapper->removeMappings( a );
      delete a;
    }
  }

  mFeatureListPreviewButton->addAction( mActionExpressionPreview );
  mFeatureListPreviewButton->addAction( mActionPreviewColumnsMenu );

  foreach ( const QgsField field, fields )
  {
    if ( mLayerCache->layer()->editType( mLayerCache->layer()->fieldNameIndex( field.name() ) ) != QgsVectorLayer::Hidden )
    {
      QIcon icon = QgsApplication::getThemeIcon( "/mActionNewAttribute.png" );
      QString text = field.name();

      // Generate action for the preview popup button of the feature list
      QAction* previewAction = new QAction( icon, text, mFeatureListPreviewButton );
      mPreviewActionMapper->setMapping( previewAction, previewAction );
      connect( previewAction, SIGNAL( triggered() ), mPreviewActionMapper, SLOT( map() ) );
      mPreviewColumnsMenu->addAction( previewAction );

      if ( text == defaultField )
      {
        mFeatureListPreviewButton->setDefaultAction( previewAction );
      }
    }
  }

  // If there is no single field found as preview
  if ( !mFeatureListPreviewButton->defaultAction() )
  {
    mFeatureList->setDisplayExpression( displayExpression );
    mFeatureListPreviewButton->setDefaultAction( mActionExpressionPreview );
  }
  else
  {
    mFeatureListPreviewButton->defaultAction()->trigger();
  }
}

void QgsDualView::hideEvent( QHideEvent* event )
{
  saveEditChanges();
  QStackedWidget::hideEvent( event );
}

void QgsDualView::focusOutEvent( QFocusEvent* event )
{
  saveEditChanges();
  QStackedWidget::focusOutEvent( event );
}

void QgsDualView::setView( QgsDualView::ViewMode view )
{
  setCurrentIndex( view );
}

void QgsDualView::setFilterMode( QgsAttributeTableFilterModel::FilterMode filterMode )
{
  mFilterModel->setFilterMode( filterMode );
}

void QgsDualView::setSelectedOnTop( bool selectedOnTop )
{
  mFilterModel->setSelectedOnTop( selectedOnTop );
}

void QgsDualView::initLayerCache( QgsVectorLayer* layer )
{
  // Initialize the cache
  QSettings settings;
  int cacheSize = qMax( 1, settings.value( "/qgis/attributeTableRowCache", "10000" ).toInt() );
  mLayerCache = new QgsVectorLayerCache( layer, cacheSize, this );
  mLayerCache->setCacheGeometry( false );
  if ( 0 == ( QgsVectorDataProvider::SelectAtId & mLayerCache->layer()->dataProvider()->capabilities() ) )
  {
    connect( mLayerCache, SIGNAL( progress( int, bool & ) ), this, SLOT( progress( int, bool & ) ) );
    connect( mLayerCache, SIGNAL( finished() ), this, SLOT( finished() ) );

    mLayerCache->setFullCache( true );
  }

  connect( layer, SIGNAL( editingStarted() ), this, SLOT( editingToggled() ) );
  connect( layer, SIGNAL( beforeCommitChanges() ), this, SLOT( editingToggled() ) );
  connect( layer, SIGNAL( editingStopped() ), this, SLOT( editingToggled() ) );
  connect( layer, SIGNAL( attributeAdded( int ) ), this, SLOT( attributeAdded( int ) ) );
  connect( layer, SIGNAL( attributeDeleted( int ) ), this, SLOT( attributeDeleted( int ) ) );
  connect( layer, SIGNAL( featureDeleted( QgsFeatureId ) ), this, SLOT( featureDeleted( QgsFeatureId ) ) );
}

void QgsDualView::initModels( QgsMapCanvas* mapCanvas, const QgsFeatureRequest& request )
{
  mMasterModel = new QgsAttributeTableModel( mLayerCache, this );
  mMasterModel->setRequest( request );

  connect( mMasterModel, SIGNAL( progress( int, bool & ) ), this, SLOT( progress( int, bool & ) ) );
  connect( mMasterModel, SIGNAL( finished() ), this, SLOT( finished() ) );

  mMasterModel->loadLayer();

  mFilterModel = new QgsAttributeTableFilterModel( mapCanvas, mMasterModel, mMasterModel );

  connect( mFeatureList, SIGNAL( displayExpressionChanged( QString ) ), this, SIGNAL( displayExpressionChanged( QString ) ) );

  mFeatureListModel = new QgsFeatureListModel( mFilterModel, mFilterModel );
}

void QgsDualView::on_mFeatureList_currentEditSelectionChanged( const QgsFeature &feat )
{
  if ( !feat.isValid() )
    return;

  // Backup old dialog and delete only after creating the new dialog, so we can "hot-swap" the contained QgsFeature
  QgsAttributeDialog* oldDialog = mAttributeDialog;

  if ( mAttributeDialog && mAttributeDialog->dialog() )
  {
    saveEditChanges();
    mAttributeEditorLayout->removeWidget( mAttributeDialog->dialog() );
  }

  mAttributeDialog = new QgsAttributeDialog( mLayerCache->layer(), new QgsFeature( feat ), true, mDistanceArea, this, false );
  mAttributeEditorLayout->addWidget( mAttributeDialog->dialog() );
  mAttributeDialog->dialog()->setVisible( true );

  delete oldDialog;
}

void QgsDualView::setCurrentEditSelection( const QgsFeatureIds& fids )
{
  mFeatureList->setEditSelection( fids );
}

void QgsDualView::saveEditChanges()
{
  if ( mAttributeDialog && mAttributeDialog->dialog() )
  {
    if ( mLayerCache->layer() && mLayerCache->layer()->isEditable() )
    {
      // Get the current (unedited) feature
      QgsFeature srcFeat;
      QgsFeatureId fid = mAttributeDialog->feature()->id();
      mLayerCache->featureAtId( fid, srcFeat );
      QgsAttributes src = srcFeat.attributes();

      // Let the dialog write the edited widget values to it's feature
      mAttributeDialog->accept();
      // Get the edited feature
      const QgsAttributes &dst = mAttributeDialog->feature()->attributes();

      if ( src.count() != dst.count() )
      {
        // bail out
        return;
      }

      mLayerCache->layer()->beginEditCommand( tr( "Attributes changed" ) );

      for ( int i = 0; i < dst.count(); ++i )
      {
        if ( dst[i] != src[i] )
        {
          mLayerCache->layer()->changeAttributeValue( fid, i, dst[i] );
        }
      }

      mLayerCache->layer()->endEditCommand();
    }
  }
}

void QgsDualView::previewExpressionBuilder()
{
  // Show expression builder
  QgsExpressionBuilderDialog dlg( mLayerCache->layer(), mFeatureList->displayExpression() , this );
  dlg.setWindowTitle( tr( "Expression based preview" ) );
  dlg.setExpressionText( mFeatureList->displayExpression() );

  if ( dlg.exec() == QDialog::Accepted )
  {
    mFeatureList->setDisplayExpression( dlg.expressionText() );
    mFeatureListPreviewButton->setDefaultAction( mActionExpressionPreview );
    mFeatureListPreviewButton->setPopupMode( QToolButton::MenuButtonPopup );
  }
}

void QgsDualView::previewColumnChanged( QObject* action )
{
  QAction* previewAction = qobject_cast< QAction* >( action );

  if ( previewAction )
  {
    if ( !mFeatureList->setDisplayExpression( QString( "COALESCE( \"%1\", '<NULL>' )" ).arg( previewAction->text() ) ) )
    {
      QMessageBox::warning( this
                            , tr( "Could not set preview column" )
                            , tr( "Could not set column '%1' as preview column.\nParser error:\n%2" )
                            .arg( previewAction->text() )
                            .arg( mFeatureList->parserErrorString() )
                          );
    }
    else
    {
      mFeatureListPreviewButton->setDefaultAction( previewAction );
      mFeatureListPreviewButton->setPopupMode( QToolButton::InstantPopup );
    }
  }

  Q_ASSERT( previewAction );
}

void QgsDualView::editingToggled()
{
  // Reload the attribute dialog widget and commit changes if any
  if ( mAttributeDialog && mAttributeDialog->dialog() && mAttributeDialog->feature() )
  {
    on_mFeatureList_currentEditSelectionChanged( *mAttributeDialog->feature() );
  }
}

int QgsDualView::featureCount()
{
  return mMasterModel->rowCount();
}

int QgsDualView::filteredFeatureCount()
{
  return mFilterModel->rowCount();
}

void QgsDualView::viewWillShowContextMenu( QMenu* menu, QModelIndex atIndex )
{
  QModelIndex sourceIndex = mFilterModel->mapToSource( atIndex );

  if ( mLayerCache->layer()->actions()->size() != 0 )
  {

    QAction *a = menu->addAction( tr( "Run action" ) );
    a->setEnabled( false );

    for ( int i = 0; i < mLayerCache->layer()->actions()->size(); i++ )
    {
      const QgsAction &action = mLayerCache->layer()->actions()->at( i );

      if ( !action.runable() )
        continue;

      QgsAttributeTableAction *a = new QgsAttributeTableAction( action.name(), this, i, sourceIndex );
      menu->addAction( action.name(), a, SLOT( execute() ) );
    }
  }

  QgsAttributeTableAction *a = new QgsAttributeTableAction( tr( "Open form" ), this, -1, sourceIndex );
  menu->addAction( tr( "Open form" ), a, SLOT( featureForm() ) );
}

void QgsDualView::previewExpressionChanged( const QString expression )
{
  mLayerCache->layer()->setDisplayExpression( expression );
}

void QgsDualView::attributeDeleted( int attribute )
{
  if ( mAttributeDialog && mAttributeDialog->dialog() )
  {
    // Let the dialog write the edited widget values to it's feature
    mAttributeDialog->accept();
    // Get the edited feature
    QgsFeature* feat = mAttributeDialog->feature();
    feat->deleteAttribute( attribute );

    // Backup old dialog and delete only after creating the new dialog, so we can "hot-swap" the contained QgsFeature
    QgsAttributeDialog* oldDialog = mAttributeDialog;

    mAttributeEditorLayout->removeWidget( mAttributeDialog->dialog() );

    mAttributeDialog = new QgsAttributeDialog( mLayerCache->layer(), new QgsFeature( *feat ), true, mDistanceArea, this, false );
    mAttributeEditorLayout->addWidget( mAttributeDialog->dialog() );

    delete oldDialog;
  }
}

void QgsDualView::attributeAdded( int attribute )
{
  if ( mAttributeDialog && mAttributeDialog->dialog() )
  {
    // Let the dialog write the edited widget values to it's feature
    mAttributeDialog->accept();
    // Get the edited feature
    QgsFeature* feat = mAttributeDialog->feature();

    // Get the feature including the newly added attribute
    QgsFeature newFeat;
    mLayerCache->featureAtId( feat->id(), newFeat );

    int offset = 0;
    for ( int idx = 0; idx < newFeat.attributes().count(); ++idx )
    {
      if ( idx == attribute )
      {
        offset = 1;
      }
      else
      {
        newFeat.setAttribute( idx, feat->attribute( idx - offset ) );
      }
    }

    *feat = newFeat;

    // Backup old dialog and delete only after creating the new dialog, so we can "hot-swap" the contained QgsFeature
    QgsAttributeDialog* oldDialog = mAttributeDialog;

    mAttributeEditorLayout->removeWidget( mAttributeDialog->dialog() );

    mAttributeDialog = new QgsAttributeDialog( mLayerCache->layer(), new QgsFeature( *feat ), true, mDistanceArea, this, false );
    mAttributeEditorLayout->addWidget( mAttributeDialog->dialog() );

    delete oldDialog;
  }
}

void QgsDualView::featureDeleted( QgsFeatureId fid )
{
  if ( mAttributeDialog && mAttributeDialog->dialog() )
  {
    QgsFeature* feat = mAttributeDialog->feature();
    if ( feat )
    {
      if ( fid == feat->id() )
      {
        delete( mAttributeDialog );
        mAttributeDialog = NULL;
      }
    }
  }
}

void QgsDualView::reloadAttribute( const int& idx )
{
  if ( mAttributeDialog && mAttributeDialog->dialog() )
  {
    // Let the dialog write the edited widget values to it's feature
    mAttributeDialog->accept();
    // Get the edited feature
    QgsFeature* feat = mAttributeDialog->feature();

    // Get the feature including the changed attribute
    QgsFeature newFeat;
    mLayerCache->featureAtId( feat->id(), newFeat );

    // Update the attribute
    feat->setAttribute( idx, newFeat.attribute( idx ) );

    // Backup old dialog and delete only after creating the new dialog, so we can "hot-swap" the contained QgsFeature
    QgsAttributeDialog* oldDialog = mAttributeDialog;

    mAttributeEditorLayout->removeWidget( mAttributeDialog->dialog() );

    mAttributeDialog = new QgsAttributeDialog( mLayerCache->layer(), new QgsFeature( *feat ), true, mDistanceArea, this, false );
    mAttributeEditorLayout->addWidget( mAttributeDialog->dialog() );

    delete oldDialog;
  }
}

void QgsDualView::setFilteredFeatures( QgsFeatureIds filteredFeatures )
{
  mFilterModel->setFilteredFeatures( filteredFeatures );
}

void QgsDualView::setRequest( const QgsFeatureRequest& request )
{
  mMasterModel->setRequest( request );
}

void QgsDualView::progress( int i, bool& cancel )
{
  if ( !mProgressDlg )
  {
    mProgressDlg = new QProgressDialog( tr( "Loading features..." ), tr( "Abort" ), 0, 0, this );
    mProgressDlg->setWindowTitle( tr( "Attribute table" ) );
    mProgressDlg->setWindowModality( Qt::WindowModal );
    mProgressDlg->show();
  }

  mProgressDlg->setValue( i );
  mProgressDlg->setLabelText( tr( "%1 features loaded." ).arg( i ) );

  QCoreApplication::processEvents();

  cancel = mProgressDlg->wasCanceled();
}

void QgsDualView::finished()
{
  delete mProgressDlg;
  mProgressDlg = 0;
}

/*
 * QgsAttributeTableAction
 */

void QgsAttributeTableAction::execute()
{
  mDualView->masterModel()->executeAction( mAction, mFieldIdx );
}

void QgsAttributeTableAction::featureForm()
{
  QgsFeatureIds editedIds;
  editedIds << mDualView->masterModel()->rowToId( mFieldIdx.row() );
  mDualView->setCurrentEditSelection( editedIds );
  mDualView->setView( QgsDualView::AttributeEditor );
}
