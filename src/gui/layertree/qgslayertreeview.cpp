/***************************************************************************
  qgslayertreeview.cpp
  --------------------------------------
  Date                 : May 2014
  Copyright            : (C) 2014 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslayertreeview.h"

#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgslayertreemodellegendnode.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgsmaplayer.h"

#include <QMenu>
#include <QContextMenuEvent>

QgsLayerTreeView::QgsLayerTreeView( QWidget *parent )
    : QTreeView( parent )
    , mDefaultActions( 0 )
    , mMenuProvider( 0 )
{
  setHeaderHidden( true );

  setDragEnabled( true );
  setAcceptDrops( true );
  setDropIndicatorShown( true );
  setEditTriggers( EditKeyPressed );
  setExpandsOnDoubleClick( false ); // normally used for other actions

  setSelectionMode( ExtendedSelection );

  connect( this, SIGNAL( collapsed( QModelIndex ) ), this, SLOT( updateExpandedStateToNode( QModelIndex ) ) );
  connect( this, SIGNAL( expanded( QModelIndex ) ), this, SLOT( updateExpandedStateToNode( QModelIndex ) ) );
}

QgsLayerTreeView::~QgsLayerTreeView()
{
  delete mMenuProvider;
}

void QgsLayerTreeView::setModel( QAbstractItemModel* model )
{
  if ( !qobject_cast<QgsLayerTreeModel*>( model ) )
    return;

  connect( model, SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( modelRowsInserted( QModelIndex, int, int ) ) );
  connect( model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), this, SLOT( modelRowsRemoved() ) );

  QTreeView::setModel( model );

  connect( layerTreeModel()->rootGroup(), SIGNAL( expandedChanged( QgsLayerTreeNode*, bool ) ), this, SLOT( onExpandedChanged( QgsLayerTreeNode*, bool ) ) );

  connect( selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), this, SLOT( onCurrentChanged() ) );

  connect( layerTreeModel(), SIGNAL( modelReset() ), this, SLOT( onModelReset() ) );

  updateExpandedStateFromNode( layerTreeModel()->rootGroup() );
}

QgsLayerTreeModel *QgsLayerTreeView::layerTreeModel() const
{
  return qobject_cast<QgsLayerTreeModel*>( model() );
}

QgsLayerTreeViewDefaultActions* QgsLayerTreeView::defaultActions()
{
  if ( !mDefaultActions )
    mDefaultActions = new QgsLayerTreeViewDefaultActions( this );
  return mDefaultActions;
}

void QgsLayerTreeView::setMenuProvider( QgsLayerTreeViewMenuProvider* menuProvider )
{
  delete mMenuProvider;
  mMenuProvider = menuProvider;
}

QgsMapLayer* QgsLayerTreeView::currentLayer() const
{
  return layerForIndex( currentIndex() );
}

void QgsLayerTreeView::setCurrentLayer( QgsMapLayer* layer )
{
  if ( !layer )
  {
    setCurrentIndex( QModelIndex() );
    return;
  }

  QgsLayerTreeLayer* nodeLayer = layerTreeModel()->rootGroup()->findLayer( layer->id() );
  if ( !nodeLayer )
    return;

  setCurrentIndex( layerTreeModel()->node2index( nodeLayer ) );
}

void QgsLayerTreeView::setLayerVisible( QgsMapLayer* layer, bool visible )
{
  if ( !layer )
    return;
  QgsLayerTreeLayer* nodeLayer = layerTreeModel()->rootGroup()->findLayer( layer->id() );
  if ( !nodeLayer )
    return;
  nodeLayer->setVisible( visible ? Qt::Checked : Qt::Unchecked );
}

void QgsLayerTreeView::contextMenuEvent( QContextMenuEvent *event )
{
  if ( !mMenuProvider )
    return;

  QModelIndex idx = indexAt( event->pos() );
  if ( !idx.isValid() )
    setCurrentIndex( QModelIndex() );

  QMenu* menu = mMenuProvider->createContextMenu();
  if ( menu && menu->actions().count() != 0 )
    menu->exec( mapToGlobal( event->pos() ) );
  delete menu;
}


void QgsLayerTreeView::modelRowsInserted( QModelIndex index, int start, int end )
{
  QgsLayerTreeNode* parentNode = layerTreeModel()->index2node( index );
  if ( !parentNode )
    return;

  if ( QgsLayerTree::isLayer( parentNode ) )
  {
    // if ShowLegendAsTree flag is enabled in model, we may need to expand some legend nodes
    QStringList expandedNodeKeys = parentNode->customProperty( "expandedLegendNodes" ).toStringList();
    if ( expandedNodeKeys.isEmpty() )
      return;

    foreach ( QgsLayerTreeModelLegendNode* legendNode, layerTreeModel()->layerLegendNodes( QgsLayerTree::toLayer( parentNode ) ) )
    {
      QString ruleKey = legendNode->data( QgsLayerTreeModelLegendNode::RuleKeyRole ).toString();
      if ( expandedNodeKeys.contains( ruleKey ) )
        setExpanded( layerTreeModel()->legendNode2index( legendNode ), true );
    }
    return;
  }

  QList<QgsLayerTreeNode*> children = parentNode->children();
  for ( int i = start; i <= end; ++i )
  {
    updateExpandedStateFromNode( children[i] );
  }

  // make sure we still have correct current layer
  onCurrentChanged();
}

void QgsLayerTreeView::modelRowsRemoved()
{
  // make sure we still have correct current layer
  onCurrentChanged();
}

void QgsLayerTreeView::updateExpandedStateToNode( QModelIndex index )
{
  if ( QgsLayerTreeNode* node = layerTreeModel()->index2node( index ) )
  {
    node->setExpanded( isExpanded( index ) );
  }
  else if ( QgsLayerTreeModelLegendNode* node = layerTreeModel()->index2legendNode( index ) )
  {
    QString ruleKey = node->data( QgsLayerTreeModelLegendNode::RuleKeyRole ).toString();
    QStringList lst = node->layerNode()->customProperty( "expandedLegendNodes" ).toStringList();
    bool expanded = isExpanded( index );
    bool isInList = lst.contains( ruleKey );
    if ( expanded && !isInList )
    {
      lst.append( ruleKey );
      node->layerNode()->setCustomProperty( "expandedLegendNodes", lst );
    }
    else if ( !expanded && isInList )
    {
      lst.removeAll( ruleKey );
      node->layerNode()->setCustomProperty( "expandedLegendNodes", lst );
    }
  }
}

void QgsLayerTreeView::onCurrentChanged()
{
  QgsMapLayer* layerCurrent = layerForIndex( currentIndex() );
  QString layerCurrentID = layerCurrent ? layerCurrent->id() : QString();
  if ( mCurrentLayerID == layerCurrentID )
    return;

  // update the current index in model (the item will be underlined)
  QModelIndex nodeLayerIndex;
  if ( layerCurrent )
  {
    QgsLayerTreeLayer* nodeLayer = layerTreeModel()->rootGroup()->findLayer( layerCurrentID );
    if ( nodeLayer )
      nodeLayerIndex = layerTreeModel()->node2index( nodeLayer );
  }
  layerTreeModel()->setCurrentIndex( nodeLayerIndex );

  mCurrentLayerID = layerCurrentID;
  emit currentLayerChanged( layerCurrent );
}

void QgsLayerTreeView::onExpandedChanged( QgsLayerTreeNode* node, bool expanded )
{
  QModelIndex idx = layerTreeModel()->node2index( node );
  if ( isExpanded( idx ) != expanded )
    setExpanded( idx, expanded );
}

void QgsLayerTreeView::onModelReset()
{
  updateExpandedStateFromNode( layerTreeModel()->rootGroup() );
}

void QgsLayerTreeView::updateExpandedStateFromNode( QgsLayerTreeNode* node )
{
  QModelIndex idx = layerTreeModel()->node2index( node );
  setExpanded( idx, node->isExpanded() );

  foreach ( QgsLayerTreeNode* child, node->children() )
    updateExpandedStateFromNode( child );
}

QgsMapLayer* QgsLayerTreeView::layerForIndex( const QModelIndex& index ) const
{
  QgsLayerTreeNode* node = layerTreeModel()->index2node( index );
  if ( node )
  {
    if ( QgsLayerTree::isLayer( node ) )
      return QgsLayerTree::toLayer( node )->layer();
  }
  else
  {
    // possibly a legend node
    QgsLayerTreeModelLegendNode* legendNode = layerTreeModel()->index2legendNode( index );
    if ( legendNode )
      return legendNode->layerNode()->layer();
  }

  return 0;
}

QgsLayerTreeNode* QgsLayerTreeView::currentNode() const
{
  return layerTreeModel()->index2node( selectionModel()->currentIndex() );
}

QgsLayerTreeGroup* QgsLayerTreeView::currentGroupNode() const
{
  QgsLayerTreeNode* node = currentNode();
  if ( QgsLayerTree::isGroup( node ) )
    return QgsLayerTree::toGroup( node );
  else if ( QgsLayerTree::isLayer( node ) )
  {
    QgsLayerTreeNode* parent = node->parent();
    if ( QgsLayerTree::isGroup( parent ) )
      return QgsLayerTree::toGroup( parent );
  }

  if ( QgsLayerTreeModelLegendNode* legendNode = layerTreeModel()->index2legendNode( selectionModel()->currentIndex() ) )
  {
    QgsLayerTreeLayer* parent = legendNode->layerNode();
    if ( QgsLayerTree::isGroup( parent->parent() ) )
      return QgsLayerTree::toGroup( parent->parent() );
  }

  return 0;
}

QList<QgsLayerTreeNode*> QgsLayerTreeView::selectedNodes( bool skipInternal ) const
{
  return layerTreeModel()->indexes2nodes( selectionModel()->selectedIndexes(), skipInternal );
}

QList<QgsLayerTreeLayer*> QgsLayerTreeView::selectedLayerNodes() const
{
  QList<QgsLayerTreeLayer*> layerNodes;
  foreach ( QgsLayerTreeNode* node, selectedNodes() )
  {
    if ( QgsLayerTree::isLayer( node ) )
      layerNodes << QgsLayerTree::toLayer( node );
  }
  return layerNodes;
}

QList<QgsMapLayer*> QgsLayerTreeView::selectedLayers() const
{
  QList<QgsMapLayer*> list;
  foreach ( QgsLayerTreeLayer* node, selectedLayerNodes() )
  {
    if ( node->layer() )
      list << node->layer();
  }
  return list;
}


void QgsLayerTreeView::refreshLayerSymbology( const QString& layerId )
{
  QgsLayerTreeLayer* nodeLayer = layerTreeModel()->rootGroup()->findLayer( layerId );
  if ( nodeLayer )
    layerTreeModel()->refreshLayerLegend( nodeLayer );
}


void QgsLayerTreeViewMenuProvider::addLegendLayerAction( QAction* action, QString menu, QString id,
    QgsMapLayer::LayerType type, bool allLayers )
{
  mLegendLayerActionMap[type].append( LegendLayerAction( action, menu, id, allLayers ) );
}

bool QgsLayerTreeViewMenuProvider::removeLegendLayerAction( QAction* action )
{
  QMap< QgsMapLayer::LayerType, QList< LegendLayerAction > >::iterator it;
  for ( it = mLegendLayerActionMap.begin();
        it != mLegendLayerActionMap.end(); ++it )
  {
    for ( int i = 0; i < it->count(); i++ )
    {
      if (( *it )[i].action == action )
      {
        ( *it ).removeAt( i );
        return true;
      }
    }
  }
  return false;
}

void QgsLayerTreeViewMenuProvider::addLegendLayerActionForLayer( QAction* action, QgsMapLayer* layer )
{
  if ( !action || !layer )
    return;

  legendLayerActions( layer->type() );
  if ( !mLegendLayerActionMap.contains( layer->type() ) )
    return;

  QMap< QgsMapLayer::LayerType, QList< LegendLayerAction > >::iterator it
  = mLegendLayerActionMap.find( layer->type() );
  for ( int i = 0; i < it->count(); i++ )
  {
    if (( *it )[i].action == action )
    {
      ( *it )[i].layers.append( layer );
      return;
    }
  }
}

void QgsLayerTreeViewMenuProvider::addLegendLayerActionForLayer( const QString& id, QgsMapLayer* layer )
{
  if ( id.isEmpty() || !layer )
    return;

  legendLayerActions( layer->type() );
  if ( !mLegendLayerActionMap.contains( layer->type() ) )
    return;

  QMap< QgsMapLayer::LayerType, QList< LegendLayerAction > >::iterator it
  = mLegendLayerActionMap.find( layer->type() );
  for ( int i = 0; i < it->count(); i++ )
  {
    if (( *it )[i].id == id )
    {
      ( *it )[i].layers.append( layer );
      return;
    }
  }
}

void QgsLayerTreeViewMenuProvider::removeLegendLayerActionsForLayer( QgsMapLayer* layer )
{
  if ( ! layer || ! mLegendLayerActionMap.contains( layer->type() ) )
    return;

  QMap< QgsMapLayer::LayerType, QList< LegendLayerAction > >::iterator it
  = mLegendLayerActionMap.find( layer->type() );
  for ( int i = 0; i < it->count(); i++ )
  {
    ( *it )[i].layers.removeAll( layer );
  }
}

QList< LegendLayerAction > QgsLayerTreeViewMenuProvider::legendLayerActions( QgsMapLayer::LayerType type ) const
{
#ifdef QGISDEBUG
  if ( mLegendLayerActionMap.contains( type ) )
  {
    QgsDebugMsg( QString( "legendLayerActions for layers of type %1:" ).arg( type ) );

    foreach ( LegendLayerAction lyrAction, mLegendLayerActionMap[ type ] )
    {
      QgsDebugMsg( QString( "%1/%2 - %3 layers" ).arg( lyrAction.menu ).arg( lyrAction.action->text() ).arg( lyrAction.layers.count() ) );
    }
  }
#endif

  return mLegendLayerActionMap.contains( type ) ? mLegendLayerActionMap.value( type ) : QList< LegendLayerAction >();
}

void QgsLayerTreeViewMenuProvider::addCustomLayerActions( QMenu* menu, QgsMapLayer* layer )
{
  if ( !layer )
    return;

  // add custom layer actions - should this go at end?
  QList< LegendLayerAction > lyrActions = legendLayerActions( layer->type() );

  if ( ! lyrActions.isEmpty() )
  {
    menu->addSeparator();
    QList<QMenu*> theMenus;
    for ( int i = 0; i < lyrActions.count(); i++ )
    {
      if ( lyrActions[i].allLayers || lyrActions[i].layers.contains( layer ) )
      {
        if ( lyrActions[i].menu.isEmpty() )
        {
          menu->addAction( lyrActions[i].action );
        }
        else
        {
          // find or create menu for given menu name
          // adapted from QgisApp::getPluginMenu( QString menuName )
          QString menuName = lyrActions[i].menu;
#ifdef Q_OS_MAC
          // Mac doesn't have '&' keyboard shortcuts.
          menuName.remove( QChar( '&' ) );
#endif
          QAction* before = 0;
          QMenu* newMenu = 0;
          QString dst = menuName;
          dst.remove( QChar( '&' ) );
          foreach ( QMenu* menu, theMenus )
          {
            QString src = menu->title();
            src.remove( QChar( '&' ) );
            int comp = dst.localeAwareCompare( src );
            if ( comp < 0 )
            {
              // Add item before this one
              before = menu->menuAction();
              break;
            }
            else if ( comp == 0 )
            {
              // Plugin menu item already exists
              newMenu = menu;
              break;
            }
          }
          if ( ! newMenu )
          {
            // It doesn't exist, so create
            newMenu = new QMenu( menuName );
            theMenus.append( newMenu );
            // Where to put it? - we worked that out above...
            menu->insertMenu( before, newMenu );
          }
          // QMenu* menu = getMenu( lyrActions[i].menu, &theBeforeSep, &theAfterSep, &theMenu );
          newMenu->addAction( lyrActions[i].action );
        }
      }
    }
    menu->addSeparator();
  }
}
