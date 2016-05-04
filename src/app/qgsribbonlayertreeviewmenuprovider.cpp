#include "qgsribbonlayertreeviewmenuprovider.h"
#include "qgsapplication.h"
#include "qgsribbonapp.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgsrasterlayer.h"
#include "qgsvectorlayer.h"
#include <QMenu>

QgsRibbonLayerTreeViewMenuProvider::QgsRibbonLayerTreeViewMenuProvider( QgsLayerTreeView* view, QgsRibbonApp* mainWidget ):
    mView( view ), mMainWidget( mainWidget )
{
}

QgsRibbonLayerTreeViewMenuProvider::QgsRibbonLayerTreeViewMenuProvider(): mView( 0 ), mMainWidget( 0 )
{
}

QgsRibbonLayerTreeViewMenuProvider::~QgsRibbonLayerTreeViewMenuProvider()
{
}

QMenu* QgsRibbonLayerTreeViewMenuProvider::createContextMenu()
{
  if ( !mView || !mMainWidget )
  {
    return 0;
  }

  QgsLayerTreeViewDefaultActions* actions = mView->defaultActions();

  QMenu* menu = new QMenu;
  QModelIndex idx = mView->currentIndex();
  if ( !idx.isValid() )
  {
    menu->addAction( actions->actionAddGroup( menu ) );
  }
  else if ( QgsLayerTreeNode* node = mView->layerTreeModel()->index2node( idx ) )
  {
    if ( QgsLayerTree::isLayer( node ) )
    {
      QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();
      if ( layer->type() == QgsMapLayer::VectorLayer || layer->type() == QgsMapLayer::RasterLayer || layer->type() == QgsMapLayer::RedliningLayer )
      {
        menu->addAction( actions->actionTransparency( mMainWidget->mapCanvas(), menu ) );
      }
    }
    else if ( QgsLayerTree::isGroup( node ) )
    {
      menu->addAction( actions->actionRenameGroupOrLayer( menu ) );
      menu->addAction( actions->actionMutuallyExclusiveGroup( menu ) );
    }
    menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), mMainWidget, SLOT( removeLayer() ) );
    if ( QgsLayerTree::isLayer( node ) )
    {
      QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();
      addCustomLayerActions( menu, layer );
      menu->addAction( actions->actionZoomToLayer( mMainWidget->mapCanvas(), menu ) );
      if ( layer->type() == QgsMapLayer::RasterLayer )
      {
        menu->addAction( actions->actionUseAsHightMap( mMainWidget->mapCanvas() ) );
      }
      else if ( layer->type() == QgsMapLayer::VectorLayer || layer->type() == QgsMapLayer::RedliningLayer )
      {
//        menu->addAction( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ), tr( "&Open Attribute Table" ),
//                         QgisApp::instance(), SLOT( attributeTable() ) );
      }

      menu->addAction( tr( "&Properties" ), mMainWidget, SLOT( layerProperties() ) );
    }
  }
  return menu;
}
