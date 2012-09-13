/***************************************************************************
    qgscategorizedsymbolrendererv2widget.cpp
    ---------------------
    begin                : November 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder.sk at gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgscategorizedsymbolrendererv2widget.h"

#include "qgscategorizedsymbolrendererv2.h"

#include "qgssymbolv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectorcolorrampv2.h"
#include "qgsstylev2.h"

#include "qgssymbolv2selectordialog.h"

#include "qgsvectorlayer.h"

#include "qgsproject.h"

#include <QMenu>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>


QgsRendererV2Widget* QgsCategorizedSymbolRendererV2Widget::create( QgsVectorLayer* layer, QgsStyleV2* style, QgsFeatureRendererV2* renderer )
{
  return new QgsCategorizedSymbolRendererV2Widget( layer, style, renderer );
}

QgsCategorizedSymbolRendererV2Widget::QgsCategorizedSymbolRendererV2Widget( QgsVectorLayer* layer, QgsStyleV2* style, QgsFeatureRendererV2* renderer )
    : QgsRendererV2Widget( layer, style )
{

  // try to recognize the previous renderer
  // (null renderer means "no previous renderer")
  if ( !renderer || renderer->type() != "categorizedSymbol" )
  {
    // we're not going to use it - so let's delete the renderer
    delete renderer;

    mRenderer = new QgsCategorizedSymbolRendererV2( "", QgsCategoryList() );
  }
  else
  {
    mRenderer = static_cast<QgsCategorizedSymbolRendererV2*>( renderer );
  }

  QString attrName = mRenderer->classAttribute();
  mOldClassificationAttribute = attrName;

  // setup user interface
  setupUi( this );

  populateColumns();

  cboCategorizedColorRamp->populate( mStyle );

  // set project default color ramp
  QString defaultColorRamp = QgsProject::instance()->readEntry( "DefaultStyles", "/ColorRamp", "" );
  if ( defaultColorRamp != "" )
  {
    int index = cboCategorizedColorRamp->findText( defaultColorRamp, Qt::MatchCaseSensitive );
    if ( index >= 0 )
      cboCategorizedColorRamp->setCurrentIndex( index );
  }

  QStandardItemModel* m = new QStandardItemModel( this );
  QStringList labels;
  labels << tr( "Symbol" ) << tr( "Value" ) << tr( "Label" );
  m->setHorizontalHeaderLabels( labels );
  viewCategories->setModel( m );

  mCategorizedSymbol = QgsSymbolV2::defaultSymbol( mLayer->geometryType() );

  connect( cboCategorizedColumn, SIGNAL( currentIndexChanged( int ) ), this, SLOT( categoryColumnChanged() ) );

  connect( viewCategories, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( categoriesDoubleClicked( const QModelIndex & ) ) );
  connect( viewCategories, SIGNAL( customContextMenuRequested( const QPoint& ) ),  this, SLOT( contextMenuViewCategories( const QPoint& ) ) );

  connect( btnChangeCategorizedSymbol, SIGNAL( clicked() ), this, SLOT( changeCategorizedSymbol() ) );
  connect( btnAddCategories, SIGNAL( clicked() ), this, SLOT( addCategories() ) );
  connect( btnDeleteCategory, SIGNAL( clicked() ), this, SLOT( deleteCategory() ) );
  connect( btnDeleteAllCategories, SIGNAL( clicked() ), this, SLOT( deleteAllCategories() ) );
  connect( btnAddCategory, SIGNAL( clicked() ), this, SLOT( addCategory() ) );
  connect( m, SIGNAL( itemChanged( QStandardItem * ) ), this, SLOT( changeCurrentValue( QStandardItem * ) ) );

  // update GUI from renderer
  updateUiFromRenderer();

  // menus for data-defined rotation/size
  QMenu* advMenu = new QMenu;

  advMenu->addAction( tr( "Symbol levels..." ), this, SLOT( showSymbolLevels() ) );

  mDataDefinedMenus = new QgsRendererV2DataDefinedMenus( advMenu, mLayer->pendingFields(),
      mRenderer->rotationField(), mRenderer->sizeScaleField(), mRenderer->scaleMethod() );
  connect( mDataDefinedMenus, SIGNAL( rotationFieldChanged( QString ) ), this, SLOT( rotationFieldChanged( QString ) ) );
  connect( mDataDefinedMenus, SIGNAL( sizeScaleFieldChanged( QString ) ), this, SLOT( sizeScaleFieldChanged( QString ) ) );
  connect( mDataDefinedMenus, SIGNAL( scaleMethodChanged( QgsSymbolV2::ScaleMethod ) ), this, SLOT( scaleMethodChanged( QgsSymbolV2::ScaleMethod ) ) );
  btnAdvanced->setMenu( advMenu );
}

QgsCategorizedSymbolRendererV2Widget::~QgsCategorizedSymbolRendererV2Widget()
{
  delete mRenderer;
}

void QgsCategorizedSymbolRendererV2Widget::updateUiFromRenderer()
{

  updateCategorizedSymbolIcon();
  populateCategories();

  // set column
  disconnect( cboCategorizedColumn, SIGNAL( currentIndexChanged( int ) ), this, SLOT( categoryColumnChanged() ) );
  QString attrName = mRenderer->classAttribute();
  mOldClassificationAttribute = attrName;
  int idx = cboCategorizedColumn->findText( attrName, Qt::MatchExactly );
  cboCategorizedColumn->setCurrentIndex( idx >= 0 ? idx : 0 );
  connect( cboCategorizedColumn, SIGNAL( currentIndexChanged( int ) ), this, SLOT( categoryColumnChanged() ) );

  // set source symbol
  if ( mRenderer->sourceSymbol() )
  {
    delete mCategorizedSymbol;
    mCategorizedSymbol = mRenderer->sourceSymbol()->clone();
    updateCategorizedSymbolIcon();
  }

  // set source color ramp
  if ( mRenderer->sourceColorRamp() )
  {
    cboCategorizedColorRamp->setSourceColorRamp( mRenderer->sourceColorRamp() );
  }

}

QgsFeatureRendererV2* QgsCategorizedSymbolRendererV2Widget::renderer()
{
  return mRenderer;
}

void QgsCategorizedSymbolRendererV2Widget::changeCategorizedSymbol()
{
  QgsSymbolV2SelectorDialog dlg( mCategorizedSymbol, mStyle, mLayer, this );
  if ( !dlg.exec() )
    return;

  updateCategorizedSymbolIcon();
}

void QgsCategorizedSymbolRendererV2Widget::updateCategorizedSymbolIcon()
{
  QIcon icon = QgsSymbolLayerV2Utils::symbolPreviewIcon( mCategorizedSymbol, btnChangeCategorizedSymbol->iconSize() );
  btnChangeCategorizedSymbol->setIcon( icon );
}

void QgsCategorizedSymbolRendererV2Widget::populateCategories()
{
  QStandardItemModel* m = qobject_cast<QStandardItemModel*>( viewCategories->model() );
  m->clear();

  QStringList labels;
  labels << tr( "Symbol" ) << tr( "Value" ) << tr( "Label" );
  m->setHorizontalHeaderLabels( labels );

  int i, count = mRenderer->categories().count();

  // TODO: sort?? utils.sortVariantList(keys);

  for ( i = 0; i < count; i++ )
  {
    const QgsRendererCategoryV2 &cat = mRenderer->categories()[i];
    addCategory( cat );
  }

  viewCategories->resizeColumnToContents( 0 );
  viewCategories->resizeColumnToContents( 1 );
  viewCategories->resizeColumnToContents( 2 );
}

void QgsCategorizedSymbolRendererV2Widget::populateColumns()
{
  cboCategorizedColumn->clear();
  const QgsFieldMap& flds = mLayer->pendingFields();
  QgsFieldMap::ConstIterator it = flds.begin();
  for ( ; it != flds.end(); ++it )
  {
    cboCategorizedColumn->addItem( it->name() );
  }
}


void QgsCategorizedSymbolRendererV2Widget::addCategory( const QgsRendererCategoryV2 &cat )
{
  QSize iconSize( 16, 16 );

  QIcon icon = QgsSymbolLayerV2Utils::symbolPreviewIcon( cat.symbol(), iconSize );
  QStandardItem *symbolItem = new QStandardItem( icon, "" );
  symbolItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

  QStandardItem *valueItem = new QStandardItem( cat.value().toString() );
  valueItem->setData( cat.value() ); // set attribute value as user role

  QList<QStandardItem *> list;
  list << symbolItem << valueItem << new QStandardItem( cat.label() );
  qobject_cast<QStandardItemModel *>( viewCategories->model() )->appendRow( list );
}

void QgsCategorizedSymbolRendererV2Widget::categoryColumnChanged()
{
  mRenderer->setClassAttribute( cboCategorizedColumn->currentText() );
}

void QgsCategorizedSymbolRendererV2Widget::categoriesDoubleClicked( const QModelIndex & idx )
{
  if ( idx.isValid() && idx.column() == 0 )
    changeCategorySymbol();
}

void QgsCategorizedSymbolRendererV2Widget::changeCategorySymbol()
{
  QVariant k = currentCategory();
  if ( !k.isValid() )
    return;

  int catIdx = mRenderer->categoryIndexForValue( k );
  QgsSymbolV2* newSymbol = mRenderer->categories()[catIdx].symbol()->clone();

  QgsSymbolV2SelectorDialog dlg( newSymbol, mStyle, mLayer, this );
  if ( !dlg.exec() )
  {
    delete newSymbol;
    return;
  }

  mRenderer->updateCategorySymbol( catIdx, newSymbol );

  populateCategories();
}

static void _createCategories( QgsCategoryList& cats, QList<QVariant>& values, QgsSymbolV2* symbol, QgsVectorColorRampV2* ramp )
{
  // sort the categories first
  //TODO: make the order configurable?
  QgsSymbolLayerV2Utils::sortVariantList( values, Qt::AscendingOrder );

  int num = values.count();

  bool hasNull = false;

  for ( int i = 0; i < num; i++ )
  {
    QVariant value = values[i];
    if ( value.toString().isNull() )
    {
      hasNull = true;
    }
    double x = i / ( double ) num;
    QgsSymbolV2* newSymbol = symbol->clone();
    newSymbol->setColor( ramp->color( x ) );

    cats.append( QgsRendererCategoryV2( value, newSymbol, value.toString() ) );
  }

  // add null (default) value if not exists
  if ( !hasNull )
  {
    QgsSymbolV2* newSymbol = symbol->clone();
    newSymbol->setColor( ramp->color( 1 ) );
    cats.append( QgsRendererCategoryV2( QVariant( "" ), newSymbol, QString() ) );
  }
}

void QgsCategorizedSymbolRendererV2Widget::addCategories()
{
  QString attrName = cboCategorizedColumn->currentText();
  int idx = mLayer->fieldNameIndex( attrName );
  QList<QVariant> unique_vals;
  mLayer->uniqueValues( idx, unique_vals );

  //DlgAddCategories dlg(mStyle, createDefaultSymbol(), unique_vals, this);
  //if (!dlg.exec())
  //  return;

  QgsVectorColorRampV2* ramp = cboCategorizedColorRamp->currentColorRamp();

  if ( ramp == NULL )
  {
    if ( cboCategorizedColorRamp->count() == 0 )
      QMessageBox::critical( this, tr( "Error" ), tr( "There are no available color ramps. You can add them in Style Manager." ) );
    else
      QMessageBox::critical( this, tr( "Error" ), tr( "The selected color ramp is not available." ) );
    return;
  }

  QgsCategoryList cats;
  _createCategories( cats, unique_vals, mCategorizedSymbol, ramp );

  bool deleteExisting = false;

  if ( !mOldClassificationAttribute.isEmpty() &&
       attrName != mOldClassificationAttribute &&
       mRenderer->categories().count() > 0 )
  {
    int res = QMessageBox::question( this,
                                     tr( "Confirm Delete" ),
                                     tr( "The classification field was changed from '%1' to '%2'.\n"
                                         "Should the existing classes be deleted before classification?" )
                                     .arg( mOldClassificationAttribute ).arg( attrName ),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
    if ( res == QMessageBox::Cancel )
    {
      return;
    }

    deleteExisting = ( res == QMessageBox::Yes );
  }

  if ( !deleteExisting )
  {
    QgsCategoryList prevCats = mRenderer->categories();
    for ( int i = 0; i < cats.size(); ++i )
    {
      bool contains = false;
      QVariant value = cats.at( i ).value();
      for ( int j = 0; j < prevCats.size() && !contains; ++j )
      {
        if ( prevCats.at( j ).value() == value )
        {
          contains = true;
          break;
        }
      }

      if ( !contains )
        prevCats.append( cats.at( i ) );
    }
    cats = prevCats;
  }

  mOldClassificationAttribute = attrName;

  // TODO: if not all categories are desired, delete some!
  /*
  if (not dlg.readAllCats.isChecked())
  {
    cats2 = {}
    for item in dlg.listCategories.selectedItems():
      for k,c in cats.iteritems():
        if item.text() == k.toString():
          break
      cats2[k] = c
    cats = cats2
  }
  */

  // recreate renderer
  QgsCategorizedSymbolRendererV2 *r = new QgsCategorizedSymbolRendererV2( attrName, cats );
  r->setSourceSymbol( mCategorizedSymbol->clone() );
  r->setSourceColorRamp( ramp->clone() );
  r->setScaleMethod( mRenderer->scaleMethod() );
  r->setSizeScaleField( mRenderer->sizeScaleField() );
  r->setRotationField( mRenderer->rotationField() );
  delete mRenderer;
  mRenderer = r;

  populateCategories();
}

int QgsCategorizedSymbolRendererV2Widget::currentCategoryRow()
{
  QModelIndex idx = viewCategories->selectionModel()->currentIndex();
  if ( !idx.isValid() )
    return -1;
  return idx.row();
}

QVariant QgsCategorizedSymbolRendererV2Widget::currentCategory()
{
  int row = currentCategoryRow();
  if ( row == -1 )
    return QVariant();
  QStandardItemModel* m = qobject_cast<QStandardItemModel*>( viewCategories->model() );
  return m->item( row, 1 )->data();
}

void QgsCategorizedSymbolRendererV2Widget::deleteCategory()
{
  QVariant k = currentCategory();
  if ( !k.isValid() )
    return;

  int idx = mRenderer->categoryIndexForValue( k );
  if ( idx < 0 )
    return;

  mRenderer->deleteCategory( idx );

  populateCategories();
}

void QgsCategorizedSymbolRendererV2Widget::deleteAllCategories()
{
  mRenderer->deleteAllCategories();
  populateCategories();
}

void QgsCategorizedSymbolRendererV2Widget::changeCurrentValue( QStandardItem * item )
{
  int idx = item->row();
  QString newtext = item->text();
  if ( item->column() == 1 )
  {
    QVariant value = newtext;
    // try to preserve variant type for this value
    QVariant::Type t = item->data().type();
    if ( t == QVariant::Int )
      value = newtext.toInt();
    else if ( t == QVariant::Double )
      value = newtext.toDouble();
    mRenderer->updateCategoryValue( idx, value );
    item->setData( value );
  }
  else if ( item->column() == 2 )
  {
    mRenderer->updateCategoryLabel( idx, newtext );
  }
}

void QgsCategorizedSymbolRendererV2Widget::addCategory()
{
  QgsSymbolV2 *symbol = QgsSymbolV2::defaultSymbol( mLayer->geometryType() );
  QgsRendererCategoryV2 cat( QString(), symbol, QString() );
  addCategory( cat );
  mRenderer->addCategory( cat );
}

void QgsCategorizedSymbolRendererV2Widget::rotationFieldChanged( QString fldName )
{
  mRenderer->setRotationField( fldName );
}

void QgsCategorizedSymbolRendererV2Widget::sizeScaleFieldChanged( QString fldName )
{
  mRenderer->setSizeScaleField( fldName );
}

void QgsCategorizedSymbolRendererV2Widget::scaleMethodChanged( QgsSymbolV2::ScaleMethod scaleMethod )
{
  mRenderer->setScaleMethod( scaleMethod );
}

QList<QgsSymbolV2*> QgsCategorizedSymbolRendererV2Widget::selectedSymbols()
{
  QList<QgsSymbolV2*> selectedSymbols;

  QItemSelectionModel* m = viewCategories->selectionModel();
  QModelIndexList selectedIndexes = m->selectedRows( 1 );

  if ( m && selectedIndexes.size() > 0 )
  {
    const QgsCategoryList& categories = mRenderer->categories();
    QModelIndexList::const_iterator indexIt = selectedIndexes.constBegin();
    for ( ; indexIt != selectedIndexes.constEnd(); ++indexIt )
    {
      QStandardItem* currentItem = qobject_cast<const QStandardItemModel*>( m->model() )->itemFromIndex( *indexIt );
      if ( currentItem )
      {
        QgsSymbolV2* s = categories[mRenderer->categoryIndexForValue( currentItem->data() )].symbol();
        if ( s )
        {
          selectedSymbols.append( s );
        }
      }
    }
  }
  return selectedSymbols;
}

void QgsCategorizedSymbolRendererV2Widget::showSymbolLevels()
{
  showSymbolLevelsDialog( mRenderer );
}
