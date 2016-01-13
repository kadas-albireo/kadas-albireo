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
  }
  else if ( QgsLayerTreeNode* node = mView->layerTreeModel()->index2node( idx ) )
  {
    menu->addAction( actions->actionTransparency( mMainWidget->mapCanvas(), menu ) );
    menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), mMainWidget, SLOT( removeLayer() ) );
    if ( QgsLayerTree::isLayer( node ) )
    {
      QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();
      QgsRasterLayer *rlayer = qobject_cast<QgsRasterLayer *>( layer );
      menu->addAction( actions->actionZoomToLayer( mMainWidget->mapCanvas(), menu ) );

      if ( layer->type() != QgsMapLayer::RedliningLayer )
      {
        if ( rlayer )
        {
          menu->addAction( actions->actionUseAsHightMap( mMainWidget->mapCanvas() ) );
        }
      }
      menu->addAction( tr( "&Properties" ), mMainWidget, SLOT( layerProperties() ) );
    }
  }
  return menu;
}
