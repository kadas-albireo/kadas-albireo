/***************************************************************************
  qgsvisibilitypresets.cpp
  --------------------------------------
  Date                 : September 2014
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

#include "qgsvisibilitypresets.h"

#include "qgslayertree.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgslayertreemodel.h"
#include "qgslayertreemodellegendnode.h"
#include "qgslayertreeview.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaplayerstylemanager.h"
#include "qgsproject.h"
#include "qgsrendererv2.h"
#include "qgsvectorlayer.h"
#include "qgisapp.h"

#include <QInputDialog>


QgsVisibilityPresets* QgsVisibilityPresets::sInstance;


QgsVisibilityPresets::QgsVisibilityPresets()
    : mMenu( new QMenu )
{

  mMenu->addAction( QgisApp::instance()->actionShowAllLayers() );
  mMenu->addAction( QgisApp::instance()->actionHideAllLayers() );
  mMenu->addAction( QgisApp::instance()->actionShowSelectedLayers() );
  mMenu->addAction( QgisApp::instance()->actionHideSelectedLayers() );
  mMenu->addSeparator();

  mMenu->addAction( tr( "Add Preset..." ), this, SLOT( addPreset() ) );
  mMenuSeparator = mMenu->addSeparator();

  mActionRemoveCurrentPreset = mMenu->addAction( tr( "Remove Current Preset" ), this, SLOT( removeCurrentPreset() ) );

  connect( mMenu, SIGNAL( aboutToShow() ), this, SLOT( menuAboutToShow() ) );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ),
           this, SLOT( registryLayersRemoved( QStringList ) ) );

  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument & ) ),
           this, SLOT( readProject( const QDomDocument & ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument & ) ),
           this, SLOT( writeProject( QDomDocument & ) ) );
}

void QgsVisibilityPresets::addVisibleLayersToPreset( QgsLayerTreeGroup* parent, QgsVisibilityPresets::PresetRecord& rec )
{
  foreach ( QgsLayerTreeNode* node, parent->children() )
  {
    if ( QgsLayerTree::isGroup( node ) )
      addVisibleLayersToPreset( QgsLayerTree::toGroup( node ), rec );
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer* nodeLayer = QgsLayerTree::toLayer( node );
      if ( nodeLayer->isVisible() )
        rec.mVisibleLayerIDs << nodeLayer->layerId();
    }
  }
}

void QgsVisibilityPresets::addPerLayerCheckedLegendSymbols( QgsVisibilityPresets::PresetRecord& rec )
{
  QgsLayerTreeModel* model = QgisApp::instance()->layerTreeView()->layerTreeModel();

  foreach ( QString layerID, rec.mVisibleLayerIDs )
  {
    QgsLayerTreeLayer* nodeLayer = model->rootGroup()->findLayer( layerID );
    if ( !nodeLayer )
      continue;

    bool hasCheckableItems = false;
    bool someItemsUnchecked = false;
    QSet<QString> checkedItems;
    foreach ( QgsLayerTreeModelLegendNode* legendNode, model->layerLegendNodes( nodeLayer ) )
    {
      if ( legendNode->flags() & Qt::ItemIsUserCheckable )
      {
        hasCheckableItems = true;

        if ( legendNode->data( Qt::CheckStateRole ).toInt() == Qt::Checked )
          checkedItems << legendNode->data( QgsLayerTreeModelLegendNode::RuleKeyRole ).toString();
        else
          someItemsUnchecked = true;
      }
    }

    if ( hasCheckableItems && someItemsUnchecked )
      rec.mPerLayerCheckedLegendSymbols.insert( nodeLayer->layerId(), checkedItems );
  }
}

void QgsVisibilityPresets::addPerLayerCurrentStyle( QgsVisibilityPresets::PresetRecord& rec )
{
  QgsLayerTreeModel* model = QgisApp::instance()->layerTreeView()->layerTreeModel();

  foreach ( QString layerID, rec.mVisibleLayerIDs )
  {
    QgsLayerTreeLayer* nodeLayer = model->rootGroup()->findLayer( layerID );
    if ( !nodeLayer )
      continue;

    rec.mPerLayerCurrentStyle[layerID] = nodeLayer->layer()->styleManager()->currentStyle();
  }
}

QgsVisibilityPresets::PresetRecord QgsVisibilityPresets::currentState()
{
  PresetRecord rec;
  QgsLayerTreeGroup* root = QgsProject::instance()->layerTreeRoot();
  addVisibleLayersToPreset( root, rec );
  addPerLayerCheckedLegendSymbols( rec );
  addPerLayerCurrentStyle( rec );
  return rec;
}

QgsVisibilityPresets* QgsVisibilityPresets::instance()
{
  if ( !sInstance )
    sInstance = new QgsVisibilityPresets();

  return sInstance;
}

void QgsVisibilityPresets::addPreset( const QString& name )
{
  mPresets.insert( name, currentState() );

  reconnectToLayersStyleManager();
}

void QgsVisibilityPresets::updatePreset( const QString& name )
{
  if ( !mPresets.contains( name ) )
    return;

  mPresets[name] = currentState();

  reconnectToLayersStyleManager();
}

void QgsVisibilityPresets::removePreset( const QString& name )
{
  mPresets.remove( name );

  reconnectToLayersStyleManager();
}

void QgsVisibilityPresets::clear()
{
  mPresets.clear();

  reconnectToLayersStyleManager();
}

QStringList QgsVisibilityPresets::presets() const
{
  return mPresets.keys();
}

QStringList QgsVisibilityPresets::presetVisibleLayers( const QString& name ) const
{
  QSet<QString> visibleIds = mPresets.value( name ).mVisibleLayerIDs;

  // also make sure to order the layers according to map canvas order
  QgsLayerTreeMapCanvasBridge* bridge = QgisApp::instance()->layerTreeCanvasBridge();
  QStringList order = bridge->hasCustomLayerOrder() ? bridge->customLayerOrder() : bridge->defaultLayerOrder();
  QStringList order2;
  foreach ( QString layerID, order )
  {
    if ( visibleIds.contains( layerID ) )
      order2 << layerID;
  }

  return order2;
}


void QgsVisibilityPresets::applyPresetCheckedLegendNodesToLayer( const QString& name, const QString& layerID )
{
  if ( !mPresets.contains( name ) )
    return;

  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerID );
  if ( !layer )
    return;

  const PresetRecord& rec = mPresets[name];

  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>( layer );
  if ( !vlayer || !vlayer->rendererV2() )
    return;

  if ( !vlayer->rendererV2()->legendSymbolItemsCheckable() )
    return; // no need to do anything

  bool someNodesUnchecked = rec.mPerLayerCheckedLegendSymbols.contains( layerID );

  foreach ( const QgsLegendSymbolItemV2& item, vlayer->rendererV2()->legendSymbolItemsV2() )
  {
    bool checked = vlayer->rendererV2()->legendSymbolItemChecked( item.ruleKey() );
    bool shouldBeChecked = someNodesUnchecked ? rec.mPerLayerCheckedLegendSymbols[layerID].contains( item.ruleKey() ) : true;
    if ( checked != shouldBeChecked )
      vlayer->rendererV2()->checkLegendSymbolItem( item.ruleKey(), shouldBeChecked );
  }
}


QMap<QString, QString> QgsVisibilityPresets::presetStyleOverrides( const QString& presetName )
{
  QMap<QString, QString> styleOverrides;
  if ( !mPresets.contains( presetName ) )
    return styleOverrides;

  QStringList lst = QgsVisibilityPresets::instance()->presetVisibleLayers( presetName );
  const QgsVisibilityPresets::PresetRecord& rec = mPresets[presetName];
  foreach ( const QString& layerID, lst )
  {
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerID );
    if ( !layer )
      continue;

    // use either the stored style name or the current one if none has been stored
    QString overrideStyleName = rec.mPerLayerCurrentStyle.value( layerID, layer->styleManager()->currentStyle() );

    // store original style and temporarily apply a style
    layer->styleManager()->setOverrideStyle( overrideStyleName );

    // set the checked legend nodes
    applyPresetCheckedLegendNodesToLayer( presetName, layerID );

    // save to overrides
    QgsMapLayerStyle layerStyle;
    layerStyle.readFromLayer( layer );
    styleOverrides[layerID] = layerStyle.xmlData();

    layer->styleManager()->restoreOverrideStyle();
  }
  return styleOverrides;
}


QMenu* QgsVisibilityPresets::menu()
{
  return mMenu;
}


void QgsVisibilityPresets::addPreset()
{
  bool ok;
  QString name = QInputDialog::getText( 0, tr( "Visibility Presets" ), tr( "Name of the new preset" ), QLineEdit::Normal, QString(), &ok );
  if ( !ok && name.isEmpty() )
    return;

  addPreset( name );
}


void QgsVisibilityPresets::presetTriggerred()
{
  QAction* actionPreset = qobject_cast<QAction*>( sender() );
  if ( !actionPreset )
    return;

  applyState( actionPreset->text() );
}


void QgsVisibilityPresets::applyStateToLayerTreeGroup( QgsLayerTreeGroup* parent, const PresetRecord& rec )
{
  foreach ( QgsLayerTreeNode* node, parent->children() )
  {
    if ( QgsLayerTree::isGroup( node ) )
      applyStateToLayerTreeGroup( QgsLayerTree::toGroup( node ), rec );
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsLayerTreeLayer* nodeLayer = QgsLayerTree::toLayer( node );
      bool isVisible = rec.mVisibleLayerIDs.contains( nodeLayer->layerId() );
      nodeLayer->setVisible( isVisible ? Qt::Checked : Qt::Unchecked );

      if ( isVisible )
      {
        if ( rec.mPerLayerCurrentStyle.contains( nodeLayer->layerId() ) )
        {
          // apply desired style first
          nodeLayer->layer()->styleManager()->setCurrentStyle( rec.mPerLayerCurrentStyle[nodeLayer->layerId()] );
        }

        QgsLayerTreeModel* model = QgisApp::instance()->layerTreeView()->layerTreeModel();
        if ( rec.mPerLayerCheckedLegendSymbols.contains( nodeLayer->layerId() ) )
        {
          const QSet<QString>& checkedNodes = rec.mPerLayerCheckedLegendSymbols[nodeLayer->layerId()];
          // some nodes are not checked
          foreach ( QgsLayerTreeModelLegendNode* legendNode, model->layerLegendNodes( nodeLayer ) )
          {
            Qt::CheckState shouldHaveState = checkedNodes.contains( legendNode->data( QgsLayerTreeModelLegendNode::RuleKeyRole ).toString() ) ? Qt::Checked : Qt::Unchecked;
            if (( legendNode->flags() & Qt::ItemIsUserCheckable ) &&
                legendNode->data( Qt::CheckStateRole ).toInt() != shouldHaveState )
              legendNode->setData( shouldHaveState, Qt::CheckStateRole );
          }
        }
        else
        {
          // all nodes should be checked
          foreach ( QgsLayerTreeModelLegendNode* legendNode, model->layerLegendNodes( nodeLayer ) )
          {
            if (( legendNode->flags() & Qt::ItemIsUserCheckable ) &&
                legendNode->data( Qt::CheckStateRole ).toInt() != Qt::Checked )
              legendNode->setData( Qt::Checked, Qt::CheckStateRole );
          }
        }
      }
    }
  }
}


void QgsVisibilityPresets::applyState( const QString& presetName )
{
  if ( !mPresets.contains( presetName ) )
    return;

  applyStateToLayerTreeGroup( QgsProject::instance()->layerTreeRoot(), mPresets[presetName] );

  // also make sure that the preset is up-to-date (not containing any non-existent legend items)
  if ( mPresets[presetName] == currentState() )
    return; // no need for update

  PresetRecord& rec = mPresets[presetName];
  foreach ( QString layerID, rec.mPerLayerCheckedLegendSymbols.keys() )
  {
    QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( QgsMapLayerRegistry::instance()->mapLayer( layerID ) );
    if ( !vl || !vl->rendererV2() )
      continue;

    // first verify if the layer still has checkable legend items
    // if not, remove the entry for the layer

    if ( !vl->rendererV2()->legendSymbolItemsCheckable() )
    {
      rec.mPerLayerCheckedLegendSymbols.remove( layerID );
      continue;
    }

    // now verify that all recorded legend items still exist
    // if not, remove them from the layer's entry

    QSet<QString> validRuleKeys;
    foreach ( const QgsLegendSymbolItemV2& item, vl->rendererV2()->legendSymbolItemsV2() )
      validRuleKeys << item.ruleKey();

    QSet<QString> invalidRuleKeys;
    foreach ( QString ruleKey, rec.mPerLayerCheckedLegendSymbols[layerID] )
      if ( !validRuleKeys.contains( ruleKey ) )
        invalidRuleKeys << ruleKey;

    foreach ( QString invalidRuleKey, invalidRuleKeys )
      rec.mPerLayerCheckedLegendSymbols[layerID].remove( invalidRuleKey );
  }

  // fix non-existent layer styles
  foreach ( QString layerID, rec.mPerLayerCurrentStyle.keys() )
  {
    QgsMapLayer* ml = QgsMapLayerRegistry::instance()->mapLayer( layerID );
    if ( !ml )
      continue;

    QString name = rec.mPerLayerCurrentStyle[layerID];
    if ( !ml->styleManager()->styles().contains( name ) )
      rec.mPerLayerCurrentStyle[layerID] = ml->styleManager()->currentStyle();
  }
}

void QgsVisibilityPresets::reconnectToLayersStyleManager()
{
  // disconnect( 0, 0, this, SLOT( layerStyleRenamed( QString, QString ) ) );

  QSet<QString> layerIDs;
  foreach ( const QString& grpName, mPresets.keys() )
  {
    const PresetRecord& rec = mPresets[grpName];
    foreach ( const QString& layerID, rec.mPerLayerCurrentStyle.keys() )
      layerIDs << layerID;
  }

  foreach ( const QString& layerID, layerIDs )
  {
    if ( QgsMapLayer* ml = QgsMapLayerRegistry::instance()->mapLayer( layerID ) )
      connect( ml->styleManager(), SIGNAL( styleRenamed( QString, QString ) ), this, SLOT( layerStyleRenamed( QString, QString ) ) );
  }
}


void QgsVisibilityPresets::removeCurrentPreset()
{
  foreach ( QAction* a, mMenuPresetActions )
  {
    if ( a->isChecked() )
    {
      removePreset( a->text() );
      break;
    }
  }
}


void QgsVisibilityPresets::menuAboutToShow()
{
  qDeleteAll( mMenuPresetActions );
  mMenuPresetActions.clear();

  PresetRecord rec = currentState();
  bool hasCurrent = false;

  foreach ( const QString& grpName, mPresets.keys() )
  {
    QAction* a = new QAction( grpName, mMenu );
    a->setCheckable( true );
    if ( rec == mPresets[grpName] )
    {
      a->setChecked( true );
      hasCurrent = true;
    }
    connect( a, SIGNAL( triggered() ), this, SLOT( presetTriggerred() ) );
    mMenuPresetActions.append( a );
  }
  mMenu->insertActions( mMenuSeparator, mMenuPresetActions );

  mActionRemoveCurrentPreset->setEnabled( hasCurrent );
}


void QgsVisibilityPresets::readProject( const QDomDocument& doc )
{
  clear();

  QDomElement visPresetsElem = doc.firstChildElement( "qgis" ).firstChildElement( "visibility-presets" );
  if ( visPresetsElem.isNull() )
    return;

  QDomElement visPresetElem = visPresetsElem.firstChildElement( "visibility-preset" );
  while ( !visPresetElem.isNull() )
  {
    QString presetName = visPresetElem.attribute( "name" );
    PresetRecord rec;
    QDomElement visPresetLayerElem = visPresetElem.firstChildElement( "layer" );
    while ( !visPresetLayerElem.isNull() )
    {
      QString layerID = visPresetLayerElem.attribute( "id" );
      if ( QgsMapLayerRegistry::instance()->mapLayer( layerID ) )
      {
        rec.mVisibleLayerIDs << layerID; // only use valid layer IDs
        if ( visPresetLayerElem.hasAttribute( "style" ) )
          rec.mPerLayerCurrentStyle[layerID] = visPresetLayerElem.attribute( "style" );
      }
      visPresetLayerElem = visPresetLayerElem.nextSiblingElement( "layer" );
    }

    QDomElement checkedLegendNodesElem = visPresetElem.firstChildElement( "checked-legend-nodes" );
    while ( !checkedLegendNodesElem.isNull() )
    {
      QSet<QString> checkedLegendNodes;

      QDomElement checkedLegendNodeElem = checkedLegendNodesElem.firstChildElement( "checked-legend-node" );
      while ( !checkedLegendNodeElem.isNull() )
      {
        checkedLegendNodes << checkedLegendNodeElem.attribute( "id" );
        checkedLegendNodeElem = checkedLegendNodeElem.nextSiblingElement( "checked-legend-node" );
      }

      QString layerID = checkedLegendNodesElem.attribute( "id" );
      if ( QgsMapLayerRegistry::instance()->mapLayer( layerID ) ) // only use valid IDs
        rec.mPerLayerCheckedLegendSymbols.insert( layerID, checkedLegendNodes );
      checkedLegendNodesElem = checkedLegendNodesElem.nextSiblingElement( "checked-legend-nodes" );
    }

    mPresets.insert( presetName, rec );

    visPresetElem = visPresetElem.nextSiblingElement( "visibility-preset" );
  }

  reconnectToLayersStyleManager();
}

void QgsVisibilityPresets::writeProject( QDomDocument& doc )
{
  QDomElement visPresetsElem = doc.createElement( "visibility-presets" );
  foreach ( const QString& grpName, mPresets.keys() )
  {
    const PresetRecord& rec = mPresets[grpName];
    QDomElement visPresetElem = doc.createElement( "visibility-preset" );
    visPresetElem.setAttribute( "name", grpName );
    foreach ( QString layerID, rec.mVisibleLayerIDs )
    {
      QDomElement layerElem = doc.createElement( "layer" );
      layerElem.setAttribute( "id", layerID );
      if ( rec.mPerLayerCurrentStyle.contains( layerID ) )
        layerElem.setAttribute( "style", rec.mPerLayerCurrentStyle[layerID] );
      visPresetElem.appendChild( layerElem );
    }

    foreach ( QString layerID, rec.mPerLayerCheckedLegendSymbols.keys() )
    {
      QDomElement checkedLegendNodesElem = doc.createElement( "checked-legend-nodes" );
      checkedLegendNodesElem.setAttribute( "id", layerID );
      foreach ( QString checkedLegendNode, rec.mPerLayerCheckedLegendSymbols[layerID] )
      {
        QDomElement checkedLegendNodeElem = doc.createElement( "checked-legend-node" );
        checkedLegendNodeElem.setAttribute( "id", checkedLegendNode );
        checkedLegendNodesElem.appendChild( checkedLegendNodeElem );
      }
      visPresetElem.appendChild( checkedLegendNodesElem );
    }

    visPresetsElem.appendChild( visPresetElem );
  }

  doc.firstChildElement( "qgis" ).appendChild( visPresetsElem );
}

void QgsVisibilityPresets::registryLayersRemoved( QStringList layerIDs )
{
  foreach ( QString layerID, layerIDs )
  {
    foreach ( QString presetName, mPresets.keys() )
    {
      PresetRecord& rec = mPresets[presetName];
      rec.mVisibleLayerIDs.remove( layerID );
      rec.mPerLayerCheckedLegendSymbols.remove( layerID );
      rec.mPerLayerCurrentStyle.remove( layerID );
    }
  }
}

void QgsVisibilityPresets::layerStyleRenamed( const QString& oldName, const QString& newName )
{
  QgsMapLayerStyleManager* styleMgr = qobject_cast<QgsMapLayerStyleManager*>( sender() );
  if ( !styleMgr )
    return;

  QString layerID = styleMgr->layer()->id();

  foreach ( QString presetName, mPresets.keys() )
  {
    PresetRecord& rec = mPresets[presetName];

    if ( rec.mPerLayerCurrentStyle.contains( layerID ) )
    {
      QString styleName = rec.mPerLayerCurrentStyle[layerID];
      if ( styleName == oldName )
        rec.mPerLayerCurrentStyle[layerID] = newName;
    }
  }
}
