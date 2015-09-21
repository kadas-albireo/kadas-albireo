#include "qgsapplayertreeviewmenuprovider.h"


#include "qgsclassicapp.h"
#include "qgsapplication.h"
#include "qgsclipboard.h"
#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgsmaplayerstyleguiutils.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgslayertreeregistrybridge.h"

#include <QMenu>


QgsAppLayerTreeViewMenuProvider::QgsAppLayerTreeViewMenuProvider( QgsLayerTreeView* view, QgsMapCanvas* canvas )
    : mView( view )
    , mCanvas( canvas )
{
}


QMenu* QgsAppLayerTreeViewMenuProvider::createContextMenu()
{
  QMenu* menu = new QMenu;

  QgsLayerTreeViewDefaultActions* actions = mView->defaultActions();

  QModelIndex idx = mView->currentIndex();
  if ( !idx.isValid() )
  {
    // global menu
    menu->addAction( actions->actionAddGroup( menu ) );

    menu->addAction( QgsApplication::getThemeIcon( "/mActionExpandTree.png" ), tr( "&Expand All" ), mView, SLOT( expandAll() ) );
    menu->addAction( QgsApplication::getThemeIcon( "/mActionCollapseTree.png" ), tr( "&Collapse All" ), mView, SLOT( collapseAll() ) );

    // TODO: update drawing order
  }
  else if ( QgsLayerTreeNode* node = mView->layerTreeModel()->index2node( idx ) )
  {
    // layer or group selected
    if ( QgsLayerTree::isGroup( node ) )
    {
      menu->addAction( actions->actionZoomToGroup( mCanvas, menu ) );

      menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), QgsClassicApp::instance(), SLOT( removeLayer() ) );

      menu->addAction( QgsApplication::getThemeIcon( "/mActionSetCRS.png" ),
                       tr( "&Set Group CRS" ), QgsClassicApp::instance(), SLOT( legendGroupSetCRS() ) );

      menu->addAction( actions->actionRenameGroupOrLayer( menu ) );

      menu->addAction( actions->actionMutuallyExclusiveGroup( menu ) );

      if ( mView->selectedNodes( true ).count() >= 2 )
        menu->addAction( actions->actionGroupSelected( menu ) );

      menu->addAction( tr( "Save As Layer Definition File..." ), QgsClassicApp::instance(), SLOT( saveAsLayerDefinition() ) );

      menu->addAction( actions->actionAddGroup( menu ) );

      menu->addAction( tr( "&Properties" ), QgsClassicApp::instance(), SLOT( groupProperties() ) );
    }
    else if ( QgsLayerTree::isLayer( node ) )
    {
      QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();
      QgsRasterLayer *rlayer = qobject_cast<QgsRasterLayer *>( layer );
      QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );


      if ( rlayer || vlayer )
      {
        menu->addAction( actions->actionTransparency( mCanvas, menu ) );
        menu->addSeparator();
      }

      menu->addAction( actions->actionZoomToLayer( mCanvas, menu ) );
      menu->addAction( actions->actionShowInOverview( menu ) );

      if ( rlayer )
      {
        menu->addAction( tr( "&Zoom to Best Scale (100%)" ), QgsClassicApp::instance(), SLOT( legendLayerZoomNative() ) );
        menu->addAction( actions->actionUseAsHightMap( menu ) );

        if ( rlayer->rasterType() != QgsRasterLayer::Palette )
          menu->addAction( tr( "&Stretch Using Current Extent" ), QgsClassicApp::instance(), SLOT( legendLayerStretchUsingCurrentExtent() ) );
      }

      menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), QgsClassicApp::instance(), SLOT( removeLayer() ) );

      if ( layer->type() != QgsMapLayer::RedliningLayer )
      {
        // duplicate layer
        QAction* duplicateLayersAction = menu->addAction( QgsApplication::getThemeIcon( "/mActionDuplicateLayer.svg" ), tr( "&Duplicate" ), QgsClassicApp::instance(), SLOT( duplicateLayers() ) );

        if ( !vlayer || vlayer->geometryType() != QGis::NoGeometry )
        {
          // set layer scale visibility
          menu->addAction( tr( "&Set Layer Scale Visibility" ), QgsClassicApp::instance(), SLOT( setLayerScaleVisibility() ) );

          // set layer crs
          menu->addAction( QgsApplication::getThemeIcon( "/mActionSetCRS.png" ), tr( "&Set Layer CRS" ), QgsClassicApp::instance(), SLOT( setLayerCRS() ) );

          // assign layer crs to project
          menu->addAction( QgsApplication::getThemeIcon( "/mActionSetProjectCRS.png" ), tr( "Set &Project CRS from Layer" ), QgsClassicApp::instance(), SLOT( setProjectCRSFromLayer() ) );
        }

        // style-related actions
        if ( layer && mView->selectedLayerNodes().count() == 1 )
        {
          QMenu *menuStyleManager = new QMenu( tr( "Styles" ) );

          QgsClassicApp *app = QgsClassicApp::instance();
          menuStyleManager->addAction( tr( "Copy Style" ), app, SLOT( copyStyle() ) );
          if ( app->clipboard()->hasFormat( QGSCLIPBOARD_STYLE_MIME ) )
          {
            menuStyleManager->addAction( tr( "Paste Style" ), app, SLOT( pasteStyle() ) );
          }

          menuStyleManager->addSeparator();
          QgsMapLayerStyleGuiUtils::instance()->addStyleManagerActions( menuStyleManager, layer );

          menu->addMenu( menuStyleManager );
        }

        menu->addSeparator();

        if ( vlayer )
        {
          QAction *toggleEditingAction = QgsClassicApp::instance()->actionToggleEditing();
          QAction *saveLayerEditsAction = QgsClassicApp::instance()->actionSaveActiveLayerEdits();
          QAction *allEditsAction = QgsClassicApp::instance()->actionAllEdits();

          // attribute table
          menu->addAction( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ), tr( "&Open Attribute Table" ),
                           QgsClassicApp::instance(), SLOT( attributeTable() ) );

          // allow editing
          int cap = vlayer->dataProvider()->capabilities();
          if ( cap & QgsVectorDataProvider::EditingCapabilities )
          {
            if ( toggleEditingAction )
            {
              menu->addAction( toggleEditingAction );
              toggleEditingAction->setChecked( vlayer->isEditable() );
            }
            if ( saveLayerEditsAction && vlayer->isModified() )
            {
              menu->addAction( saveLayerEditsAction );
            }
          }

          if ( allEditsAction->isEnabled() )
            menu->addAction( allEditsAction );

          // disable duplication of memory layers
          if ( vlayer->storageType() == "Memory storage" && mView->selectedLayerNodes().count() == 1 )
            duplicateLayersAction->setEnabled( false );

          // save as vector file
          menu->addAction( tr( "Save As..." ), QgsClassicApp::instance(), SLOT( saveAsFile() ) );
          menu->addAction( tr( "Save As Layer Definition File..." ), QgsClassicApp::instance(), SLOT( saveAsLayerDefinition() ) );

          if ( !vlayer->isEditable() && vlayer->dataProvider()->supportsSubsetString() && vlayer->vectorJoins().isEmpty() )
            menu->addAction( tr( "&Filter..." ), QgsClassicApp::instance(), SLOT( layerSubsetString() ) );

          menu->addAction( actions->actionShowFeatureCount( menu ) );

          menu->addSeparator();
        }
        else if ( rlayer )
        {
          menu->addAction( tr( "Save As..." ), QgsClassicApp::instance(), SLOT( saveAsRasterFile() ) );
          menu->addAction( tr( "Save As Layer Definition File..." ), QgsClassicApp::instance(), SLOT( saveAsLayerDefinition() ) );
        }
        else if ( layer && layer->type() == QgsMapLayer::PluginLayer && mView->selectedLayerNodes().count() == 1 )
        {
          // disable duplication of plugin layers
          duplicateLayersAction->setEnabled( false );
        }
      }

      addCustomLayerActions( menu, layer );

      if ( layer->type() != QgsMapLayer::RedliningLayer )
      {
        if ( layer && QgsProject::instance()->layerIsEmbedded( layer->id() ).isEmpty() )
          menu->addAction( tr( "&Properties" ), QgsClassicApp::instance(), SLOT( layerProperties() ) );

        if ( node->parent() != mView->layerTreeModel()->rootGroup() )
          menu->addAction( actions->actionMakeTopLevel( menu ) );

        menu->addAction( actions->actionRenameGroupOrLayer( menu ) );

        if ( mView->selectedNodes( true ).count() >= 2 )
          menu->addAction( actions->actionGroupSelected( menu ) );
      }
    }

  }
  else
  {
    // symbology item?
  }

  return menu;
}
