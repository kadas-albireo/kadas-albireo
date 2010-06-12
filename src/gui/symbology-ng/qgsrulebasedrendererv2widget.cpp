/***************************************************************************
    qgsrulebasedrendererv2widget.cpp - Settings widget for rule-based renderer
    ---------------------
    begin                : May 2010
    copyright            : (C) 2010 by Martin Dobias
    email                : wonder.sk at gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrulebasedrendererv2widget.h"

#include "qgsrulebasedrendererv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgssymbolv2.h"
#include "qgsvectorlayer.h"
#include "qgsapplication.h"
#include "qgssearchtreenode.h"
#include "qgssymbolv2selectordialog.h"

#include <QMenu>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QMessageBox>

QgsRendererV2Widget* QgsRuleBasedRendererV2Widget::create( QgsVectorLayer* layer, QgsStyleV2* style, QgsFeatureRendererV2* renderer )
{
  return new QgsRuleBasedRendererV2Widget( layer, style, renderer );
}

QgsRuleBasedRendererV2Widget::QgsRuleBasedRendererV2Widget( QgsVectorLayer* layer, QgsStyleV2* style, QgsFeatureRendererV2* renderer )
    : QgsRendererV2Widget( layer, style )
{

  // try to recognize the previous renderer
  // (null renderer means "no previous renderer")
  if ( !renderer || renderer->type() != "RuleRenderer" )
  {
    // we're not going to use it - so let's delete the renderer
    delete renderer;

    // some default options
    QgsSymbolV2* symbol = QgsSymbolV2::defaultSymbol( mLayer->geometryType() );

    mRenderer = new QgsRuleBasedRendererV2( symbol );
  }
  else
  {
    mRenderer = static_cast<QgsRuleBasedRendererV2*>( renderer );
  }

  setupUi( this );

  treeRules->setRenderer( mRenderer );

  mRefineMenu = new QMenu( btnRefineRule );
  mRefineMenu->addAction( tr( "Add scales" ), this, SLOT( refineRuleScales() ) );
  mRefineMenu->addAction( tr( "Add categories" ), this, SLOT( refineRuleCategories() ) );
  mRefineMenu->addAction( tr( "Add ranges" ), this, SLOT( refineRuleRanges() ) );
  btnRefineRule->setMenu( mRefineMenu );

  btnAddRule->setIcon( QIcon( QgsApplication::iconPath( "symbologyAdd.png" ) ) );
  btnEditRule->setIcon( QIcon( QgsApplication::iconPath( "symbologyEdit.png" ) ) );
  btnRemoveRule->setIcon( QIcon( QgsApplication::iconPath( "symbologyRemove.png" ) ) );

  connect( treeRules, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( editRule() ) );

  connect( btnAddRule, SIGNAL( clicked() ), this, SLOT( addRule() ) );
  connect( btnEditRule, SIGNAL( clicked() ), this, SLOT( editRule() ) );
  connect( btnRemoveRule, SIGNAL( clicked() ), this, SLOT( removeRule() ) );

  connect( radNoGrouping, SIGNAL( clicked() ), this, SLOT( setGrouping() ) );
  connect( radGroupFilter, SIGNAL( clicked() ), this, SLOT( setGrouping() ) );
  connect( radGroupScale, SIGNAL( clicked() ), this, SLOT( setGrouping() ) );

  treeRules->populateRules();
}

QgsRuleBasedRendererV2Widget::~QgsRuleBasedRendererV2Widget()
{
  delete mRenderer;
}

QgsFeatureRendererV2* QgsRuleBasedRendererV2Widget::renderer()
{
  return mRenderer;
}

void QgsRuleBasedRendererV2Widget::setGrouping()
{
  QObject* origin = sender();
  if ( origin == radNoGrouping )
    treeRules->setGrouping( QgsRendererRulesTreeWidget::NoGrouping );
  else if ( origin == radGroupFilter )
    treeRules->setGrouping( QgsRendererRulesTreeWidget::GroupingByFilter );
  else if ( origin == radGroupScale )
    treeRules->setGrouping( QgsRendererRulesTreeWidget::GroupingByScale );
}

void QgsRuleBasedRendererV2Widget::addRule()
{
  QgsRuleBasedRendererV2::Rule newrule( QgsSymbolV2::defaultSymbol( mLayer->geometryType() ) );

  QgsRendererRulePropsDialog dlg( newrule, mLayer, mStyle );
  if ( dlg.exec() )
  {
    // add rule
    dlg.updateRuleFromGui();
    mRenderer->addRule( dlg.rule() );
    treeRules->populateRules();
  }
}



void QgsRuleBasedRendererV2Widget::editRule()
{
  QTreeWidgetItem * item = treeRules->currentItem();
  if ( ! item ) return;

  int rule_index = item->data( 0, Qt::UserRole + 1 ).toInt();
  if ( rule_index < 0 )
  {
    QMessageBox::information( this, tr( "Edit rule" ), tr( "Groups of rules cannot be edited." ) );
    return;
  }
  QgsRuleBasedRendererV2::Rule& rule = mRenderer->ruleAt( rule_index );

  QgsRendererRulePropsDialog dlg( rule, mLayer, mStyle );
  if ( dlg.exec() )
  {
    // update rule
    dlg.updateRuleFromGui();
    mRenderer->updateRuleAt( rule_index, dlg.rule() );

    treeRules->populateRules();
  }
}

void QgsRuleBasedRendererV2Widget::removeRule()
{
  QTreeWidgetItem * item = treeRules->currentItem();
  if ( ! item ) return;

  int rule_index = item->data( 0, Qt::UserRole + 1 ).toInt();
  if ( rule_index < 0 )
  {
    QList<int> rule_indexes;
    // this is a group
    for ( int i = 0; i < item->childCount(); i++ )
    {
      int idx = item->child( i )->data( 0, Qt::UserRole + 1 ).toInt();
      rule_indexes.append( idx );
    }
    // delete in decreasing order to avoid shifting of indexes
    qSort( rule_indexes.begin(), rule_indexes.end(), qGreater<int>() );
    foreach( int idx, rule_indexes )
    mRenderer->removeRuleAt( idx );
  }
  else
  {
    // this is a rule
    mRenderer->removeRuleAt( rule_index );
  }

  treeRules->populateRules();
}


#include "qgscategorizedsymbolrendererv2.h"
#include "qgscategorizedsymbolrendererv2widget.h"
#include "qgsgraduatedsymbolrendererv2.h"
#include "qgsgraduatedsymbolrendererv2widget.h"
#include <QDialogButtonBox>
#include <QInputDialog>

void QgsRuleBasedRendererV2Widget::refineRule( int type )
{
  QTreeWidgetItem * item = treeRules->currentItem();
  if ( ! item ) return;

  int rule_index = item->data( 0, Qt::UserRole + 1 ).toInt();
  if ( rule_index < 0 ) return;

  QgsRuleBasedRendererV2::Rule& initialRule = mRenderer->ruleAt( rule_index );


  QList<QgsRuleBasedRendererV2::Rule> refinedRules;
  if ( type == 0 ) // categories
    refinedRules = refineRuleCategoriesGui( initialRule );
  else if ( type == 1 ) // ranges
    refinedRules = refineRuleRangesGui( initialRule );
  else // scales
    refinedRules = refineRuleScalesGui( initialRule );

  if ( refinedRules.count() == 0 )
    return;

  // delete original rule
  mRenderer->removeRuleAt( rule_index );

  // add new rules
  for ( int i = 0; i < refinedRules.count(); i++ )
  {
    mRenderer->insertRule( rule_index + i, refinedRules.at( i ) );
  }

  treeRules->populateRules();
}

void QgsRuleBasedRendererV2Widget::refineRuleCategories()
{
  refineRule( 0 );
}

void QgsRuleBasedRendererV2Widget::refineRuleRanges()
{
  refineRule( 1 );
}

void QgsRuleBasedRendererV2Widget::refineRuleScales()
{
  refineRule( 2 );
}

QList<QgsRuleBasedRendererV2::Rule> QgsRuleBasedRendererV2Widget::refineRuleCategoriesGui( QgsRuleBasedRendererV2::Rule& initialRule )
{
  QDialog dlg;
  dlg.setWindowTitle( tr( "Refine a rule to categories" ) );
  QVBoxLayout* l = new QVBoxLayout();
  QgsCategorizedSymbolRendererV2Widget* w = new QgsCategorizedSymbolRendererV2Widget( mLayer, mStyle, NULL );
  l->addWidget( w );
  QDialogButtonBox* bb = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  l->addWidget( bb );
  connect( bb, SIGNAL( accepted() ), &dlg, SLOT( accept() ) );
  connect( bb, SIGNAL( rejected() ), &dlg, SLOT( reject() ) );
  dlg.setLayout( l );

  if ( !dlg.exec() )
    return QList<QgsRuleBasedRendererV2::Rule>();

  // create new rules
  QgsCategorizedSymbolRendererV2* r = static_cast<QgsCategorizedSymbolRendererV2*>( w->renderer() );
  return QgsRuleBasedRendererV2::refineRuleCategories( initialRule, r );
}


QList<QgsRuleBasedRendererV2::Rule> QgsRuleBasedRendererV2Widget::refineRuleRangesGui( QgsRuleBasedRendererV2::Rule& initialRule )
{
  QDialog dlg;
  dlg.setWindowTitle( tr( "Refine a rule to ranges" ) );
  QVBoxLayout* l = new QVBoxLayout();
  QgsGraduatedSymbolRendererV2Widget* w = new QgsGraduatedSymbolRendererV2Widget( mLayer, mStyle, NULL );
  l->addWidget( w );
  QDialogButtonBox* bb = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  l->addWidget( bb );
  connect( bb, SIGNAL( accepted() ), &dlg, SLOT( accept() ) );
  connect( bb, SIGNAL( rejected() ), &dlg, SLOT( reject() ) );
  dlg.setLayout( l );

  if ( !dlg.exec() )
    return QList<QgsRuleBasedRendererV2::Rule>();

  // create new rules
  QgsGraduatedSymbolRendererV2* r = static_cast<QgsGraduatedSymbolRendererV2*>( w->renderer() );
  return QgsRuleBasedRendererV2::refineRuleRanges( initialRule, r );
}

QList<QgsRuleBasedRendererV2::Rule> QgsRuleBasedRendererV2Widget::refineRuleScalesGui( QgsRuleBasedRendererV2::Rule& initialRule )
{
  QString txt = QInputDialog::getText( this,
                                       tr( "Scale refinement" ),
                                       tr( "Please enter scale denominators at which will split the rule, separate them by commas (e.g. 1000,5000):" ) );
  if ( txt.isEmpty() )
    return QList<QgsRuleBasedRendererV2::Rule>();

  QList<int> scales;
  bool ok;
  foreach( QString item, txt.split( ',' ) )
  {
    int scale = item.toInt( &ok );
    if ( ok )
      scales.append( scale );
    else
      QMessageBox::information( this, tr( "Error" ), QString( tr( "\"%1\" is not valid scale denominator, ignoring it." ) ).arg( item ) );
  }

  return QgsRuleBasedRendererV2::refineRuleScales( initialRule, scales );
}


///////////

QgsRendererRulePropsDialog::QgsRendererRulePropsDialog( const QgsRuleBasedRendererV2::Rule& rule, QgsVectorLayer* layer, QgsStyleV2* style )
    : mRule( rule ), mLayer( layer )
{
  setupUi( this );

  editFilter->setText( mRule.filterExpression() );

  if ( mRule.dependsOnScale() )
  {
    groupScale->setChecked( true );
    spinMinScale->setValue( rule.scaleMinDenom() );
    spinMaxScale->setValue( rule.scaleMaxDenom() );
  }

  QgsSymbolV2SelectorDialog* symbolSel = new QgsSymbolV2SelectorDialog( mRule.symbol(), style, this, true );
  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget( symbolSel );
  groupSymbol->setLayout( l );

  connect( btnTestFilter, SIGNAL( clicked() ), this, SLOT( testFilter() ) );
}

void QgsRendererRulePropsDialog::testFilter()
{
  QgsSearchString filterParsed;
  if ( ! filterParsed.setString( editFilter->text() ) )
  {
    QMessageBox::critical( this, tr( "Error" ),  tr( "Filter expression parsing error:\n" ) + filterParsed.parserErrorMsg() );
    return;
  }

  QgsSearchTreeNode* tree = filterParsed.tree();
  if ( ! tree )
  {
    QMessageBox::critical( this, tr( "Error" ), tr( "Filter is empty" ) );
    return;
  }

  QApplication::setOverrideCursor( Qt::WaitCursor );

  const QgsFieldMap& fields = mLayer->pendingFields();
  mLayer->select( fields.keys(), QgsRectangle(), false );

  int count = 0;
  QgsFeature f;
  while ( mLayer->nextFeature( f ) )
  {
    if ( tree->checkAgainst( fields, f.attributeMap() ) )
      count++;
    if ( tree->hasError() )
      break;
  }

  QApplication::restoreOverrideCursor();

  QMessageBox::information( this, tr( "Filter" ), tr( "Filter returned %1 features" ).arg( count ) );
}

void QgsRendererRulePropsDialog::updateRuleFromGui()
{
  mRule.setFilterExpression( editFilter->text() );
  mRule.setScaleMinDenom( groupScale->isChecked() ? spinMinScale->value() : 0 );
  mRule.setScaleMaxDenom( groupScale->isChecked() ? spinMaxScale->value() : 0 );
}

////////

QgsRendererRulesTreeWidget::QgsRendererRulesTreeWidget( QWidget* parent )
    : QTreeWidget( parent ), mR( NULL ), mGrouping( NoGrouping )
{
  setSelectionMode( QAbstractItemView::SingleSelection );
  /*
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
  */
}

void QgsRendererRulesTreeWidget::setRenderer( QgsRuleBasedRendererV2* r )
{
  mR = r;
}

void QgsRendererRulesTreeWidget::setGrouping( Grouping g )
{
  mGrouping = g;
  setRootIsDecorated( mGrouping != NoGrouping );
  populateRules();
}

void QgsRendererRulesTreeWidget::populateRules()
{
  if ( !mR ) return;

  clear();
  if ( mGrouping == NoGrouping )
    populateRulesNoGrouping();
  else if ( mGrouping == GroupingByScale )
    populateRulesGroupByScale();
  else
    populateRulesGroupByFilter();
}

void QgsRendererRulesTreeWidget::populateRulesNoGrouping()
{
  QList<QTreeWidgetItem *> lst;
  for ( int i = 0; i < mR->ruleCount(); ++i )
  {
    QgsRuleBasedRendererV2::Rule& rule = mR->ruleAt( i );

    QTreeWidgetItem* item = new QTreeWidgetItem;
    QString txt = rule.filterExpression();
    if ( txt.isEmpty() ) txt = tr( "(no filter)" );
    if ( rule.dependsOnScale() )
    {
      txt += QString( ", scale <1:%1, 1:%2>" ).arg( rule.scaleMinDenom() ).arg( rule.scaleMaxDenom() );
    }

    item->setText( 0, txt );
    item->setData( 0, Qt::UserRole + 1, i );
    item->setIcon( 0, QgsSymbolLayerV2Utils::symbolPreviewIcon( rule.symbol(), QSize( 16, 16 ) ) );

    lst << item;
  }

  addTopLevelItems( lst );
}

void QgsRendererRulesTreeWidget::populateRulesGroupByScale()
{
  QMap< QPair<int, int>, QTreeWidgetItem*> scale_items;

  for ( int i = 0; i < mR->ruleCount(); ++i )
  {
    QgsRuleBasedRendererV2::Rule& rule = mR->ruleAt( i );

    QPair<int, int> scale = qMakePair( rule.scaleMinDenom(), rule.scaleMaxDenom() );
    if ( ! scale_items.contains( scale ) )
    {
      QString txt;
      if ( rule.dependsOnScale() )
        txt = QString( "scale <1:%1, 1:%2>" ).arg( rule.scaleMinDenom() ).arg( rule.scaleMaxDenom() );
      else
        txt = "any scale";

      QTreeWidgetItem* scale_item = new QTreeWidgetItem;
      scale_item->setText( 0, txt );
      scale_item->setData( 0, Qt::UserRole + 1, -2 );
      scale_item->setFlags( scale_item->flags() & ~Qt::ItemIsDragEnabled ); // groups cannot be dragged
      scale_items[scale] = scale_item;
    }

    QString filter = rule.filterExpression();

    QTreeWidgetItem* item = new QTreeWidgetItem( scale_items[scale] );
    item->setText( 0, filter.isEmpty() ? tr( "(no filter)" ) : filter );
    item->setData( 0, Qt::UserRole + 1, i );
    item->setIcon( 0, QgsSymbolLayerV2Utils::symbolPreviewIcon( rule.symbol(), QSize( 16, 16 ) ) );
  }
  addTopLevelItems( scale_items.values() );
}

void QgsRendererRulesTreeWidget::populateRulesGroupByFilter()
{
  QMap<QString, QTreeWidgetItem *> filter_items;

  for ( int i = 0; i < mR->ruleCount(); ++i )
  {
    QgsRuleBasedRendererV2::Rule& rule = mR->ruleAt( i );

    QString filter = rule.filterExpression();
    if ( ! filter_items.contains( filter ) )
    {
      QTreeWidgetItem* filter_item = new QTreeWidgetItem;
      filter_item->setText( 0, filter.isEmpty() ? tr( "(no filter)" ) : filter );
      filter_item->setData( 0, Qt::UserRole + 1, -1 );
      filter_item->setFlags( filter_item->flags() & ~Qt::ItemIsDragEnabled ); // groups cannot be dragged
      filter_items[filter] = filter_item;
    }

    QString txt;
    if ( rule.dependsOnScale() )
      txt = QString( "scale <1:%1, 1:%2>" ).arg( rule.scaleMinDenom() ).arg( rule.scaleMaxDenom() );
    else
      txt = "any scale";

    QTreeWidgetItem* item = new QTreeWidgetItem( filter_items[filter] );
    item->setText( 0, txt );
    item->setData( 0, Qt::UserRole + 1, i );
    item->setIcon( 0, QgsSymbolLayerV2Utils::symbolPreviewIcon( rule.symbol(), QSize( 16, 16 ) ) );

  }
  addTopLevelItems( filter_items.values() );
}
