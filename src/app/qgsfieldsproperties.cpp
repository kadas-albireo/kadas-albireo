/***************************************************************************
    qgsfieldsproperties.cpp
    ---------------------
    begin                : September 2012
    copyright            : (C) 2012 by Matthias Kuhn
    email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgseditorwidgetfactory.h"
#include "qgseditorwidgetregistry.h"
#include "qgsaddattrdialog.h"
#include "qgsaddtaborgroup.h"
#include "qgsapplication.h"
#include "qgsattributetypedialog.h"
#include "qgsfieldcalculator.h"
#include "qgsfieldsproperties.h"
#include "qgslogger.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"

#include <QTreeWidgetItem>
#include <QWidget>
#include <QMimeData>
#include <QDropEvent>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

QgsFieldsProperties::QgsFieldsProperties( QgsVectorLayer *layer, QWidget* parent )
    : QWidget( parent ), mLayer( layer )
{
  if ( !layer )
    return;

  setupUi( this );
  setupEditTypes();

  mSplitter->restoreState( QSettings().value( "/Windows/VectorLayerProperties/FieldsProperties/SplitState" ).toByteArray() );

  // Init as hidden by default, it will be enabled if project is set to
  mAttributeEditorOptionsWidget->setVisible( false );

  mAddAttributeButton->setIcon( QgsApplication::getThemeIcon( "/mActionNewAttribute.png" ) );
  mDeleteAttributeButton->setIcon( QgsApplication::getThemeIcon( "/mActionDeleteAttribute.png" ) );
  mToggleEditingButton->setIcon( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ) );
  mCalculateFieldButton->setIcon( QgsApplication::getThemeIcon( "/mActionCalculateField.png" ) );

  connect( mToggleEditingButton, SIGNAL( clicked() ), this, SIGNAL( toggleEditing() ) );
  connect( mLayer, SIGNAL( editingStarted() ), this, SLOT( editingToggled() ) );
  connect( mLayer, SIGNAL( editingStopped() ), this, SLOT( editingToggled() ) );
  connect( mLayer, SIGNAL( attributeAdded( int ) ), this, SLOT( attributeAdded( int ) ) );
  connect( mLayer, SIGNAL( attributeDeleted( int ) ), this, SLOT( attributeDeleted( int ) ) );

  // tab and group display
  mAddItemButton->setEnabled( false );

  mDesignerTree = new DesignerTree( mAttributesTreeFrame );
  mDesignerListLayout->addWidget( mDesignerTree );
  mDesignerTree->setHeaderLabels( QStringList() << tr( "Label" ) );

  mFieldsList = new DragList( mAttributesListFrame );
  mAttributesListLayout->addWidget( mFieldsList );

  mFieldsList->setColumnCount( attrColCount );
  mFieldsList->setSelectionBehavior( QAbstractItemView::SelectRows );
  mFieldsList->setDragDropMode( QAbstractItemView::DragOnly );
  mFieldsList->setHorizontalHeaderItem( attrIdCol, new QTableWidgetItem( tr( "Id" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrNameCol, new QTableWidgetItem( tr( "Name" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrTypeCol, new QTableWidgetItem( tr( "Type" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrTypeNameCol, new QTableWidgetItem( tr( "Type name" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrLengthCol, new QTableWidgetItem( tr( "Length" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrPrecCol, new QTableWidgetItem( tr( "Precision" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrCommentCol, new QTableWidgetItem( tr( "Comment" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrEditTypeCol, new QTableWidgetItem( tr( "Edit widget" ) ) );
  mFieldsList->setHorizontalHeaderItem( attrWMSCol, new QTableWidgetItem( "WMS" ) );
  mFieldsList->setHorizontalHeaderItem( attrWFSCol, new QTableWidgetItem( "WFS" ) );
  mFieldsList->setHorizontalHeaderItem( attrAliasCol, new QTableWidgetItem( tr( "Alias" ) ) );

  mFieldsList->setSortingEnabled( true );
  mFieldsList->sortByColumn( 0, Qt::AscendingOrder );
  mFieldsList->setSelectionBehavior( QAbstractItemView::SelectRows );
  mFieldsList->setSelectionMode( QAbstractItemView::ExtendedSelection );
  mFieldsList->verticalHeader()->hide();

  connect( mDesignerTree, SIGNAL( itemSelectionChanged() ), this, SLOT( onAttributeSelectionChanged() ) );
  connect( mFieldsList, SIGNAL( itemSelectionChanged() ), this, SLOT( onAttributeSelectionChanged() ) );

  leEditForm->setText( layer->editForm() );
  leEditFormInit->setText( layer->editFormInit() );

  updateButtons();
}

QgsFieldsProperties::~QgsFieldsProperties()
{
  QSettings().setValue( "/Windows/VectorLayerProperties/FieldsProperties/SplitState", mSplitter->saveState() );
}

void QgsFieldsProperties::init()
{
  loadRows();

  mEditorLayoutComboBox->setCurrentIndex( mLayer->editorLayout() );
  mFormSuppressCmbBx->setCurrentIndex( mLayer->featureFormSuppress() );

  loadAttributeEditorTree();
}

void QgsFieldsProperties::onAttributeSelectionChanged()
{
  bool isAddPossible = false;
  if ( mDesignerTree->selectedItems().count() == 1 && mFieldsList->selectedItems().count() > 0 )
    if ( mDesignerTree->selectedItems()[0]->data( 0, DesignerTreeRole ).value<DesignerTreeItemData>().type() == DesignerTreeItemData::Container )
      isAddPossible = true;
  mAddItemButton->setEnabled( isAddPossible );
}

QTreeWidgetItem *QgsFieldsProperties::loadAttributeEditorTreeItem( QgsAttributeEditorElement* const widgetDef, QTreeWidgetItem* parent )
{
  QTreeWidgetItem* newWidget = 0;
  switch ( widgetDef->type() )
  {
    case QgsAttributeEditorElement::AeTypeField:
      newWidget = mDesignerTree->addItem( parent, DesignerTreeItemData( DesignerTreeItemData::Field, widgetDef->name() ) );
      break;

    case QgsAttributeEditorElement::AeTypeContainer:
    {
      newWidget = mDesignerTree->addItem( parent, DesignerTreeItemData( DesignerTreeItemData::Container, widgetDef->name() ) );

      const QgsAttributeEditorContainer* container = dynamic_cast<const QgsAttributeEditorContainer*>( widgetDef );

      Q_FOREACH( QgsAttributeEditorElement* wdg, container->children() )
      {
        loadAttributeEditorTreeItem( wdg, newWidget );
      }
    }
    break;

    default:
      QgsDebugMsg( "Unknown attribute editor widget type encountered..." );
      break;
  }
  return newWidget;
}

void QgsFieldsProperties::loadAttributeEditorTree()
{
  // tabs and groups info
  mDesignerTree->clear();
  mDesignerTree->setSortingEnabled( false );
  mDesignerTree->setSelectionBehavior( QAbstractItemView::SelectRows );
  mDesignerTree->setDragDropMode( QAbstractItemView::InternalMove );
  mDesignerTree->setAcceptDrops( true );
  mDesignerTree->setDragDropMode( QAbstractItemView::DragDrop );

  Q_FOREACH( QgsAttributeEditorElement* wdg, mLayer->attributeEditorElements() )
  {
    loadAttributeEditorTreeItem( wdg, mDesignerTree->invisibleRootItem() );
  }
}

void QgsFieldsProperties::loadRows()
{
  disconnect( mFieldsList, SIGNAL( cellChanged( int, int ) ), this, SLOT( attributesListCellChanged( int, int ) ) );
  const QgsFields &fields = mLayer->pendingFields();

  mIndexedWidgets.clear();
  mFieldsList->setRowCount( 0 );

  for ( int i = 0; i < fields.count(); ++i )
    attributeAdded( i );

  mFieldsList->resizeColumnsToContents();
  connect( mFieldsList, SIGNAL( cellChanged( int, int ) ), this, SLOT( attributesListCellChanged( int, int ) ) );
}

void QgsFieldsProperties::setRow( int row, int idx, const QgsField &field )
{
  QTableWidgetItem* dataItem = new QTableWidgetItem( idx );
  DesignerTreeItemData itemData( DesignerTreeItemData::Field, field.name() );
  dataItem->setData( DesignerTreeRole, itemData.asQVariant() );
  mFieldsList->setItem( row, attrIdCol, dataItem );
  mIndexedWidgets.insert( idx, mFieldsList->item( row, 0 ) );
  mFieldsList->setItem( row, attrNameCol, new QTableWidgetItem( field.name() ) );
  mFieldsList->setItem( row, attrTypeCol, new QTableWidgetItem( QVariant::typeToName( field.type() ) ) );
  mFieldsList->setItem( row, attrTypeNameCol, new QTableWidgetItem( field.typeName() ) );
  mFieldsList->setItem( row, attrLengthCol, new QTableWidgetItem( QString::number( field.length() ) ) );
  mFieldsList->setItem( row, attrPrecCol, new QTableWidgetItem( QString::number( field.precision() ) ) );
  mFieldsList->setItem( row, attrCommentCol, new QTableWidgetItem( field.comment() ) );

  for ( int i = 0; i < attrEditTypeCol; i++ )
    mFieldsList->item( row, i )->setFlags( mFieldsList->item( row, i )->flags() & ~Qt::ItemIsEditable );

  FieldConfig cfg( mLayer, idx );
  QPushButton *pb;
  if ( cfg.mEditType == QgsVectorLayer::EditorWidgetV2 )
  {
    pb = new QPushButton( QgsEditorWidgetRegistry::instance()->name( cfg.mEditorWidgetV2Type ) );
    pb->setProperty( "EditWidgetV2", cfg.mEditType );
  }
  else
  {
    pb = new QPushButton( editTypeButtonText( cfg.mEditType ) );
  }

  mFieldsList->setCellWidget( row, attrEditTypeCol, pb );
  connect( pb, SIGNAL( pressed() ), this, SLOT( attributeTypeDialog( ) ) );

  cfg.mButton = pb;
  setConfigForRow( row, cfg );

  //set the alias for the attribute
  mFieldsList->setItem( row, attrAliasCol, new QTableWidgetItem( mLayer->attributeAlias( idx ) ) );

  //published WMS/WFS attributes
  QTableWidgetItem* wmsAttrItem = new QTableWidgetItem();
  wmsAttrItem->setCheckState( mLayer->excludeAttributesWMS().contains( field.name() ) ? Qt::Unchecked : Qt::Checked );
  wmsAttrItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
  mFieldsList->setItem( row, attrWMSCol, wmsAttrItem );
  QTableWidgetItem* wfsAttrItem = new QTableWidgetItem();
  wfsAttrItem->setCheckState( mLayer->excludeAttributesWFS().contains( field.name() ) ? Qt::Unchecked : Qt::Checked );
  wfsAttrItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
  mFieldsList->setItem( row, attrWFSCol, wfsAttrItem );
}

void QgsFieldsProperties::on_mAddItemButton_clicked()
{
  QList<QTableWidgetItem*> listItems = mFieldsList->selectedItems();
  QList<QTreeWidgetItem*> treeItems = mDesignerTree->selectedItems();

  if ( treeItems.count() != 1 && listItems.count() == 0 )
    return;

  QTreeWidgetItem *parent = treeItems[0];
  if ( parent->data( 0, DesignerTreeRole ).value<DesignerTreeItemData>().type() != DesignerTreeItemData::Container )
    return;

  Q_FOREACH( QTableWidgetItem* item, listItems )
  {
    if ( item->column() == 0 ) // Information is in the first column
      mDesignerTree->addItem( parent , item->data( DesignerTreeRole ).value<DesignerTreeItemData>() );
  }
}

void QgsFieldsProperties::on_mAddTabOrGroupButton_clicked()
{
  QList<QgsAddTabOrGroup::TabPair> tabList;

  for ( QTreeWidgetItemIterator it( mDesignerTree ); *it; ++it )
  {
    DesignerTreeItemData itemData = ( *it )->data( 0, DesignerTreeRole ).value<DesignerTreeItemData>();
    if ( itemData.type() == DesignerTreeItemData::Container )
    {
      tabList.append( QgsAddTabOrGroup::TabPair( itemData.name(), *it ) );
    }
  }
  QgsAddTabOrGroup addTabOrGroup( mLayer, tabList, this );

  if ( !addTabOrGroup.exec() )
    return;

  QString name = addTabOrGroup.name();
  if ( addTabOrGroup.tabButtonIsChecked() )
  {
    mDesignerTree->addContainer( mDesignerTree->invisibleRootItem(), name );
  }
  else
  {
    QTreeWidgetItem* tabItem = addTabOrGroup.tab();
    mDesignerTree->addContainer( tabItem , name );
  }
}

void QgsFieldsProperties::on_mRemoveTabGroupItemButton_clicked()
{
  qDeleteAll( mDesignerTree->selectedItems() );
}

void QgsFieldsProperties::on_mMoveDownItem_clicked()
{
  QList<QTreeWidgetItem*> itemList = mDesignerTree->selectedItems();
  if ( itemList.count() != 1 )
    return;

  QTreeWidgetItem* itemToMoveDown = itemList.first();
  QTreeWidgetItem* parent = itemToMoveDown->parent();
  if ( !parent )
  {
    parent = mDesignerTree->invisibleRootItem();
  }
  int itemIndex = parent->indexOfChild( itemToMoveDown );

  if ( itemIndex < parent->childCount() - 1 )
  {
    parent->takeChild( itemIndex );
    parent->insertChild( itemIndex + 1, itemToMoveDown );

    itemToMoveDown->setSelected( true );
    parent->child( itemIndex )->setSelected( false );
  }
}

void QgsFieldsProperties::on_mMoveUpItem_clicked()
{
  QList<QTreeWidgetItem*> itemList = mDesignerTree->selectedItems();
  if ( itemList.count() != 1 )
    return;

  QTreeWidgetItem* itemToMoveUp = itemList.first();
  QTreeWidgetItem* parent = itemToMoveUp->parent();
  if ( !parent )
  {
    parent = mDesignerTree->invisibleRootItem();
  }
  int itemIndex = parent->indexOfChild( itemToMoveUp );

  if ( itemIndex > 0 )
  {
    parent->takeChild( itemIndex );
    parent->insertChild( itemIndex - 1, itemToMoveUp );

    itemToMoveUp->setSelected( true );
    parent->child( itemIndex )->setSelected( false );
  }
}

void QgsFieldsProperties::attributeTypeDialog()
{
  QPushButton *pb = qobject_cast<QPushButton *>( sender() );
  if ( !pb )
    return;

  FieldConfig cfg;
  int index = -1;
  int row = -1;

  Q_FOREACH( QTableWidgetItem* wdg, mIndexedWidgets )
  {
    cfg = wdg->data( FieldConfigRole ).value<FieldConfig>();
    if ( cfg.mButton == pb )
    {
      index = mIndexedWidgets.indexOf( wdg );
      row = wdg->row();
      break;
    }
  }

  if ( index == -1 )
    return;

  QgsAttributeTypeDialog attributeTypeDialog( mLayer );

  attributeTypeDialog.setValueMap( cfg.mValueMap );
  attributeTypeDialog.setRange( cfg.mRange );
  attributeTypeDialog.setValueRelation( cfg.mValueRelationData );

  QPair<QString, QString> checkStates = cfg.mCheckedState;
  attributeTypeDialog.setCheckedState( checkStates.first, checkStates.second );

  attributeTypeDialog.setDateFormat( cfg.mDateFormat );
  attributeTypeDialog.setWidgetSize( cfg.mWidgetSize );
  attributeTypeDialog.setFieldEditable( cfg.mEditable );
  attributeTypeDialog.setLabelOnTop( cfg.mLabelOnTop );
  attributeTypeDialog.setWidgetV2Config( cfg.mEditorWidgetV2Config );

  attributeTypeDialog.setIndex( index, cfg.mEditType );
  attributeTypeDialog.setFieldEditableEnabled( cfg.mEditableEnabled );

  if ( !attributeTypeDialog.exec() )
    return;

  cfg.mEditType = attributeTypeDialog.editType();
  cfg.mEditable = attributeTypeDialog.fieldEditable();
  cfg.mLabelOnTop = attributeTypeDialog.labelOnTop();

  switch ( cfg.mEditType )
  {
    case QgsVectorLayer::ValueMap:
      cfg.mValueMap = attributeTypeDialog.valueMap();
      break;
    case QgsVectorLayer::EditRange:
    case QgsVectorLayer::SliderRange:
    case QgsVectorLayer::DialRange:
      cfg.mRange = attributeTypeDialog.rangeData();
      break;
    case QgsVectorLayer::CheckBox:
      cfg.mCheckedState = attributeTypeDialog.checkedState();
      break;
    case QgsVectorLayer::ValueRelation:
      cfg.mValueRelationData = attributeTypeDialog.valueRelationData();
      break;
    case QgsVectorLayer::Calendar:
      cfg.mDateFormat = attributeTypeDialog.dateFormat();
      break;
    case QgsVectorLayer::Photo:
    case QgsVectorLayer::WebView:
      cfg.mWidgetSize = attributeTypeDialog.widgetSize();
      break;
    case QgsVectorLayer::LineEdit:
    case QgsVectorLayer::TextEdit:
    case QgsVectorLayer::UniqueValues:
    case QgsVectorLayer::UniqueValuesEditable:
    case QgsVectorLayer::Classification:
    case QgsVectorLayer::FileName:
    case QgsVectorLayer::Enumeration:
    case QgsVectorLayer::Immutable:
    case QgsVectorLayer::Hidden:
    case QgsVectorLayer::UuidGenerator:
    case QgsVectorLayer::Color:
      break;

    case QgsVectorLayer::EditorWidgetV2:
      cfg.mEditorWidgetV2Type = attributeTypeDialog.editorWidgetV2Type();
      cfg.mEditorWidgetV2Config = attributeTypeDialog.editorWidgetV2Config();
      break;
  }

  if ( cfg.mEditType == QgsVectorLayer::EditorWidgetV2 )
  {
    pb->setText( attributeTypeDialog.editorWidgetV2Text() );
    pb->setProperty( "EditWidgetV2", cfg.mEditType );
  }
  else
  {
    pb->setText( editTypeButtonText( cfg.mEditType ) );
    pb->setProperty( "EditWidgetV2", QVariant() );
  }

  setConfigForRow( row, cfg );
}


void QgsFieldsProperties::attributeAdded( int idx )
{
  bool sorted = mFieldsList->isSortingEnabled();
  mFieldsList->setSortingEnabled( false );
  const QgsFields &fields = mLayer->pendingFields();
  int row = mFieldsList->rowCount();
  mFieldsList->insertRow( row );
  setRow( row, idx, fields[idx] );

  for ( int i = idx; i < mIndexedWidgets.count(); i++ )
  {
    mIndexedWidgets[i]->setData( Qt::DisplayRole, i );
  }

  mFieldsList->setCurrentCell( row, idx );
  mFieldsList->setSortingEnabled( sorted );
}


void QgsFieldsProperties::attributeDeleted( int idx )
{
  mFieldsList->removeRow( mIndexedWidgets.at( idx )->row() );
  mIndexedWidgets.removeAt( idx );
  for ( int i = idx; i < mIndexedWidgets.count(); i++ )
  {
    mIndexedWidgets[i]->setData( Qt::DisplayRole, i );
  }
}

void QgsFieldsProperties::addAttribute()
{
  QgsAddAttrDialog dialog( mLayer, this );
  if ( dialog.exec() == QDialog::Accepted )
  {
    mLayer->beginEditCommand( "Attribute added" );
    if ( !addAttribute( dialog.field() ) )
    {
      mLayer->destroyEditCommand();
      QMessageBox::information( this, tr( "Name conflict" ), tr( "The attribute could not be inserted. The name already exists in the table." ) );
    }
    else
    {
      mLayer->endEditCommand();
    }
  }
}

bool QgsFieldsProperties::addAttribute( const QgsField &field )
{
  QgsDebugMsg( "inserting attribute " + field.name() + " of type " + field.typeName() );
  mLayer->beginEditCommand( tr( "Added attribute" ) );
  if ( mLayer->addAttribute( field ) )
  {
    mLayer->endEditCommand();
    return true;
  }
  else
  {
    mLayer->destroyEditCommand();
    return false;
  }
}

void QgsFieldsProperties::editingToggled()
{
  updateButtons();
}

QgsFieldsProperties::FieldConfig QgsFieldsProperties::configForRow( int row )
{
  foreach ( QTableWidgetItem* wdg , mIndexedWidgets )
  {
    if ( wdg->row() == row )
    {
      return wdg->data( FieldConfigRole ).value<FieldConfig>();
    }
  }

  // Should never get here
  Q_ASSERT( false );
  return FieldConfig();
}

void QgsFieldsProperties::setConfigForRow( int row, QgsFieldsProperties::FieldConfig cfg )
{
  foreach ( QTableWidgetItem* wdg , mIndexedWidgets )
  {
    if ( wdg->row() == row )
    {
      wdg->setData( FieldConfigRole, QVariant::fromValue<FieldConfig>( cfg ) );
      return;
    }
  }

  // Should never get here
  Q_ASSERT( false );
}

void QgsFieldsProperties::on_mAddAttributeButton_clicked()
{
  QgsAddAttrDialog dialog( mLayer, this );
  if ( dialog.exec() == QDialog::Accepted )
  {
    mLayer->beginEditCommand( "Attribute added" );
    if ( !addAttribute( dialog.field() ) )
    {
      mLayer->destroyEditCommand();
      QMessageBox::information( this, tr( "Name conflict" ), tr( "The attribute could not be inserted. The name already exists in the table." ) );
    }
    else
    {
      mLayer->endEditCommand();
    }
  }
}

void QgsFieldsProperties::on_mDeleteAttributeButton_clicked()
{
  QSet<int> attrs;
  foreach ( QTableWidgetItem* item, mFieldsList->selectedItems() )
  {
    if ( item->column() == 0 )
    {
      attrs << mIndexedWidgets.indexOf( item );
    }
  }

  mLayer->beginEditCommand( tr( "Deleted attribute" ) );
  mLayer->deleteAttributes( attrs.toList() );
  mLayer->endEditCommand();
}

void QgsFieldsProperties::updateButtons()
{
  int cap = mLayer->dataProvider()->capabilities();

  mToggleEditingButton->setEnabled(( cap & QgsVectorDataProvider::ChangeAttributeValues ) && !mLayer->isReadOnly() );

  if ( mLayer->isEditable() )
  {
    mAddAttributeButton->setEnabled( cap & QgsVectorDataProvider::AddAttributes );
    mDeleteAttributeButton->setEnabled( cap & QgsVectorDataProvider::DeleteAttributes );
    mCalculateFieldButton->setEnabled( cap & ( QgsVectorDataProvider::ChangeAttributeValues | QgsVectorDataProvider::AddAttributes ) );
    mToggleEditingButton->setChecked( true );
  }
  else
  {
    mAddAttributeButton->setEnabled( false );
    mDeleteAttributeButton->setEnabled( false );
    mToggleEditingButton->setChecked( false );
    mCalculateFieldButton->setEnabled( false );
  }
}


void QgsFieldsProperties::attributesListCellChanged( int row, int column )
{
  if ( column == attrAliasCol && mLayer ) //only consider attribute aliases in this function
  {
    int idx = mFieldsList->item( row, attrIdCol )->text().toInt();

    const QgsFields &fields = mLayer->pendingFields();

    if ( idx >= fields.count() )
    {
      return; // index must be wrong
    }

    QTableWidgetItem *aliasItem = mFieldsList->item( row, column );
    if ( aliasItem )
    {
      mLayer->addAttributeAlias( idx, aliasItem->text() );
    }
  }
}

void QgsFieldsProperties::on_mCalculateFieldButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QgsFieldCalculator calc( mLayer );
  calc.exec();
}


//
// methods reimplemented from qt designer base class
//

QMap< QgsVectorLayer::EditType, QString > QgsFieldsProperties::editTypeMap;

void QgsFieldsProperties::setupEditTypes()
{
  if ( !editTypeMap.isEmpty() )
    return;

  editTypeMap.insert( QgsVectorLayer::LineEdit, tr( "Line edit" ) );
  editTypeMap.insert( QgsVectorLayer::UniqueValues, tr( "Unique values" ) );
  editTypeMap.insert( QgsVectorLayer::UniqueValuesEditable, tr( "Unique values editable" ) );
  editTypeMap.insert( QgsVectorLayer::Classification, tr( "Classification" ) );
  editTypeMap.insert( QgsVectorLayer::ValueMap, tr( "Value map" ) );
  editTypeMap.insert( QgsVectorLayer::EditRange, tr( "Edit range" ) );
  editTypeMap.insert( QgsVectorLayer::SliderRange, tr( "Slider range" ) );
  editTypeMap.insert( QgsVectorLayer::DialRange, tr( "Dial range" ) );
  editTypeMap.insert( QgsVectorLayer::FileName, tr( "File name" ) );
  editTypeMap.insert( QgsVectorLayer::Enumeration, tr( "Enumeration" ) );
  editTypeMap.insert( QgsVectorLayer::Immutable, tr( "Immutable" ) );
  editTypeMap.insert( QgsVectorLayer::Hidden, tr( "Hidden" ) );
  editTypeMap.insert( QgsVectorLayer::CheckBox, tr( "Checkbox" ) );
  editTypeMap.insert( QgsVectorLayer::TextEdit, tr( "Text edit" ) );
  editTypeMap.insert( QgsVectorLayer::Calendar, tr( "Calendar" ) );
  editTypeMap.insert( QgsVectorLayer::ValueRelation, tr( "Value relation" ) );
  editTypeMap.insert( QgsVectorLayer::UuidGenerator, tr( "UUID generator" ) );
  editTypeMap.insert( QgsVectorLayer::Photo, tr( "Photo" ) );
  editTypeMap.insert( QgsVectorLayer::WebView, tr( "Web view" ) );
  editTypeMap.insert( QgsVectorLayer::Color, tr( "Color" ) );
  editTypeMap.insert( QgsVectorLayer::EditorWidgetV2, tr( "Editor Widget" ) );
}

QString QgsFieldsProperties::editTypeButtonText( QgsVectorLayer::EditType type )
{
  return editTypeMap[ type ];
}

QgsVectorLayer::EditType QgsFieldsProperties::editTypeFromButton( QPushButton* btn )
{
  QVariant editWidgetId = btn->property( "EditWidgetV2" );

  if ( editWidgetId.isNull() )
    return editTypeMap.key( btn->text() );
  else
    return QgsVectorLayer::EditorWidgetV2;
}

QgsAttributeEditorElement* QgsFieldsProperties::createAttributeEditorWidget( QTreeWidgetItem* item, QObject *parent )
{
  QgsAttributeEditorElement* widgetDef;

  DesignerTreeItemData itemData = item->data( 0, DesignerTreeRole ).value<DesignerTreeItemData>();
  switch ( itemData.type() )
  {
    case DesignerTreeItemData::Field:
    {
      int idx = mLayer->fieldNameIndex( itemData.name() );
      widgetDef = new QgsAttributeEditorField( itemData.name(), idx, parent );
      break;
    }

    case DesignerTreeItemData::Container:
    {
      QgsAttributeEditorContainer* container = new QgsAttributeEditorContainer( itemData.name(), parent );

      for ( int t = 0; t < item->childCount(); t++ )
      {
        container->addChildElement( createAttributeEditorWidget( item->child( t ), container ) );
      }

      widgetDef = container;
      break;
    }
  }

  return widgetDef;
}

void QgsFieldsProperties::on_pbnSelectEditForm_clicked()
{
  QSettings myQSettings;
  QString lastUsedDir = myQSettings.value( "style/lastUIDir", "." ).toString();
  QString uifilename = QFileDialog::getOpenFileName( this, tr( "Select edit form" ), lastUsedDir, tr( "UI file" )  + " (*.ui)" );

  if ( uifilename.isNull() )
    return;

  leEditForm->setText( uifilename );
}

void QgsFieldsProperties::on_mEditorLayoutComboBox_currentIndexChanged( int index )
{
  switch ( index )
  {
    case 0:
      mAttributeEditorOptionsWidget->setVisible( false );
      break;

    case 1:
      mAttributeEditorOptionsWidget->setVisible( true );
      mAttributeEditorOptionsWidget->setCurrentIndex( 1 );
      break;

    case 2:
      mAttributeEditorOptionsWidget->setVisible( true );
      mAttributeEditorOptionsWidget->setCurrentIndex( 0 );
      break;
  }
}

void QgsFieldsProperties::apply()
{
  QSet<QString> excludeAttributesWMS, excludeAttributesWFS;

  for ( int i = 0; i < mFieldsList->rowCount(); i++ )
  {
    int idx = mFieldsList->item( i, attrIdCol )->text().toInt();
    FieldConfig cfg = configForRow( i );

    QPushButton *pb = qobject_cast<QPushButton *>( mFieldsList->cellWidget( i, attrEditTypeCol ) );
    if ( !pb )
      continue;

    mLayer->setEditType( idx, cfg.mEditType );

    mLayer->setFieldEditable( idx, cfg.mEditable );
    mLayer->setLabelOnTop( idx, cfg.mLabelOnTop );

    switch ( cfg.mEditType )
    {
      case QgsVectorLayer::ValueMap:
        mLayer->valueMap( idx ) = cfg.mValueMap;
        break;

      case QgsVectorLayer::EditRange:
      case QgsVectorLayer::SliderRange:
      case QgsVectorLayer::DialRange:
        mLayer->range( idx ) = cfg.mRange;
        break;

      case QgsVectorLayer::CheckBox:
        mLayer->setCheckedState( idx, cfg.mCheckedState.first, cfg.mCheckedState.second );
        break;

      case QgsVectorLayer::ValueRelation:
        mLayer->valueRelation( idx ) = cfg.mValueRelationData;
        break;

      case QgsVectorLayer::Calendar:
        mLayer->dateFormat( idx ) = cfg.mDateFormat;
        break;

      case QgsVectorLayer::Photo:
      case QgsVectorLayer::WebView:
        mLayer->widgetSize( idx ) = cfg.mWidgetSize;
        break;

      case QgsVectorLayer::LineEdit:
      case QgsVectorLayer::UniqueValues:
      case QgsVectorLayer::UniqueValuesEditable:
      case QgsVectorLayer::Classification:
      case QgsVectorLayer::FileName:
      case QgsVectorLayer::Enumeration:
      case QgsVectorLayer::Immutable:
      case QgsVectorLayer::Hidden:
      case QgsVectorLayer::TextEdit:
      case QgsVectorLayer::UuidGenerator:
      case QgsVectorLayer::Color:
        break;

      case QgsVectorLayer::EditorWidgetV2:
        mLayer->setEditorWidgetV2( idx, cfg.mEditorWidgetV2Type );
        mLayer->setEditorWidgetV2Config( idx, cfg.mEditorWidgetV2Config );
        break;
    }

    if ( mFieldsList->item( i, attrWMSCol )->checkState() == Qt::Unchecked )
    {
      excludeAttributesWMS.insert( mFieldsList->item( i, attrNameCol )->text() );
    }
    if ( mFieldsList->item( i, attrWFSCol )->checkState() == Qt::Unchecked )
    {
      excludeAttributesWFS.insert( mFieldsList->item( i, attrNameCol )->text() );
    }
  }

  //tabs and groups
  mLayer->clearAttributeEditorWidgets();
  for ( int t = 0; t < mDesignerTree->invisibleRootItem()->childCount(); t++ )
  {
    QTreeWidgetItem* tabItem = mDesignerTree->invisibleRootItem()->child( t );

    mLayer->addAttributeEditorWidget( createAttributeEditorWidget( tabItem, mLayer ) );
  }

  mLayer->setEditorLayout(( QgsVectorLayer::EditorLayout ) mEditorLayoutComboBox->currentIndex() );
  mLayer->setEditForm( leEditForm->text() );
  mLayer->setEditFormInit( leEditFormInit->text() );
  mLayer->setFeatureFormSuppress(( QgsVectorLayer::FeatureFormSuppress )mFormSuppressCmbBx->currentIndex() );

  mLayer->setExcludeAttributesWMS( excludeAttributesWMS );
  mLayer->setExcludeAttributesWFS( excludeAttributesWFS );
}

/*
 * FieldConfig implementation
 */

QgsFieldsProperties::FieldConfig::FieldConfig()
    : mButton( NULL )
{
}

QgsFieldsProperties::FieldConfig::FieldConfig( QgsVectorLayer* layer, int idx )
    : mButton( NULL )
{
  mEditable = layer->fieldEditable( idx );
  mEditableEnabled = layer->pendingFields().fieldOrigin( idx ) != QgsFields::OriginJoin;
  mLabelOnTop = layer->labelOnTop( idx );
  mValueRelationData = layer->valueRelation( idx );
  mValueMap = layer->valueMap( idx );
  mRange = layer->range( idx );
  mCheckedState = layer->checkedState( idx );
  mEditType = layer->editType( idx );
  mDateFormat = layer->dateFormat( idx );
  mWidgetSize = layer->widgetSize( idx );
  mEditorWidgetV2Type = layer->editorWidgetV2( idx );
  mEditorWidgetV2Config = layer->editorWidgetV2Config( idx );
}

/*
 * DragList implementation
 */

QStringList QgsFieldsProperties::DragList::mimeTypes() const
{
  return QStringList() << QLatin1String( "application/x-qgsattributetabledesignerelement" );
}

QMimeData* QgsFieldsProperties::DragList::mimeData( const QList<QTableWidgetItem*> items ) const
{
  if ( items.count() <= 0 )
    return NULL;

  QStringList types = mimeTypes();

  if ( types.isEmpty() )
    return NULL;

  QMimeData* data = new QMimeData();
  QString format = types.at( 0 );
  QByteArray encoded;
  QDataStream stream( &encoded, QIODevice::WriteOnly );

  Q_FOREACH( const QTableWidgetItem* item, items )
  {
    // Relevant information is always in the UserRole of the first column
    if ( item && item->column() == 0 )
    {
      DesignerTreeItemData itemData = item->data( DesignerTreeRole ).value<DesignerTreeItemData>();
      stream << itemData;
    }
  }

  data->setData( format, encoded );

  return data;
}

/*
 * DesignerTree implementation
 */

QTreeWidgetItem* QgsFieldsProperties::DesignerTree::addContainer( QTreeWidgetItem* parent, QString title )
{
  QTreeWidgetItem *newItem = new QTreeWidgetItem( QStringList() << title );
  newItem->setBackground( 0 , QBrush( Qt::lightGray ) );
  newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled );
  newItem->setData( 0, DesignerTreeRole, DesignerTreeItemData( DesignerTreeItemData::Container, title ).asQVariant() );
  parent->addChild( newItem );
  newItem->setExpanded( true );
  return newItem;
}

QTreeWidgetItem* QgsFieldsProperties::DesignerTree::addItem( QTreeWidgetItem* parent, DesignerTreeItemData data )
{
  QTreeWidgetItem* newItem = new QTreeWidgetItem( QStringList() << data.name() );
  newItem->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled );
  if ( data.type() == DesignerTreeItemData::Container )
  {
    newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled );
    newItem->setBackground( 0 , QBrush( Qt::lightGray ) );

#if 0
    switch ( data.type() )
    {
      case DesignerTreeItemData::Field:
        newItem->setIcon( 0, QgsApplication::getThemeIcon( "/mFieldIcon.svg" ) );
        break;

      case DesignerTreeItemData::Relation:
        newItem->setIcon( 0, QgsApplication::getThemeIcon( "/mRelationIcon.svg" ) );
        break;

      case DesignerTreeItemData::Container:
        newItem->setIcon( 0, QgsApplication::getThemeIcon( "/mContainerIcon.svg" ) );
        break;
    }
#endif
  }
  newItem->setData( 0 , DesignerTreeRole, data.asQVariant() );
  parent->addChild( newItem );

  return newItem;
}

/**
 * Is called when mouse is moved over attributes tree before a
 * drop event. Used to inhibit dropping fields onto the root item.
 */

void QgsFieldsProperties::DesignerTree::dragMoveEvent( QDragMoveEvent *event )
{
  QTreeWidgetItem* targetItem = itemAt( event->pos() );
  const QMimeData* data = event->mimeData();

  if ( data->hasFormat( "application/x-qgsattributetabledesignerelement" ) )
  {
    DesignerTreeItemData itemElement;

    QByteArray itemData = data->data( "application/x-qgsattributetabledesignerelement" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );
    stream >> itemElement;

    // Forbid dropping fields on root item
    if ( itemElement.type() == DesignerTreeItemData::Field && !targetItem )
    {
      event->ignore();
      return;
    }

    // Inner drag and drop actions are always MoveAction
    if ( event->source() == this )
    {
      event->setDropAction( Qt::MoveAction );
    }
  }
  else
  {
    event->ignore();
  }

  QTreeWidget::dragMoveEvent( event );
}


bool QgsFieldsProperties::DesignerTree::dropMimeData( QTreeWidgetItem* parent, int index, const QMimeData * data, Qt::DropAction action )
{
  Q_UNUSED( index )
  bool bDropSuccessful = false;

  if ( action == Qt::IgnoreAction )
  {
    bDropSuccessful = true;
  }
  else if ( data->hasFormat( "application/x-qgsattributetabledesignerelement" ) )
  {
    QByteArray itemData = data->data( "application/x-qgsattributetabledesignerelement" );
    QDataStream stream( &itemData, QIODevice::ReadOnly );
    DesignerTreeItemData itemElement;

    while ( !stream.atEnd() )
    {
      stream >> itemElement;

      if ( parent )
      {
        addItem( parent, itemElement );
        bDropSuccessful = true;
      }
      else // Should never happen as we ignore drops of fields onto the root element in dragMoveEvent, but actually does happen. Qt?
      {
        // addItem( invisibleRootItem(), itemName );
        // bDropSuccessful = true;
      }
    }
  }

  return bDropSuccessful;
}

void QgsFieldsProperties::DesignerTree::dropEvent( QDropEvent* event )
{
  if ( !event->mimeData()->hasFormat( "application/x-qgsattributetabledesignerelement" ) )
    return;

  if ( event->source() == this )
  {
    event->setDropAction( Qt::MoveAction );
  }

  QTreeWidget::dropEvent( event );
}

QStringList QgsFieldsProperties::DesignerTree::mimeTypes() const
{
  return QStringList() << QLatin1String( "application/x-qgsattributetabledesignerelement" );
}

QMimeData* QgsFieldsProperties::DesignerTree::mimeData( const QList<QTreeWidgetItem*> items ) const
{
  if ( items.count() <= 0 )
    return NULL;

  QStringList types = mimeTypes();

  if ( types.isEmpty() )
    return NULL;

  QMimeData* data = new QMimeData();
  QString format = types.at( 0 );
  QByteArray encoded;
  QDataStream stream( &encoded, QIODevice::WriteOnly );

  Q_FOREACH( const QTreeWidgetItem* item, items )
  {
    if ( item )
    {
      // Relevant information is always in the DesignerTreeRole of the first column
      DesignerTreeItemData itemData = item->data( 0, DesignerTreeRole ).value<DesignerTreeItemData>();
      stream << itemData;
    }
  }

  data->setData( format, encoded );

  return data;
}

/*
 * Serialization helpers for DesigerTreeItemData so we can stuff this easily into QMimeData
 */

QDataStream& operator<<( QDataStream& stream, const QgsFieldsProperties::DesignerTreeItemData& data )
{
  stream << ( quint32 )data.type() << data.name();
  return stream;
}

QDataStream& operator>>( QDataStream& stream, QgsFieldsProperties::DesignerTreeItemData& data )
{
  QString name;
  quint32 type;

  stream >> type >> name;

  data.setType(( QgsFieldsProperties::DesignerTreeItemData::Type )type );
  data.setName( name );

  return stream;
}
