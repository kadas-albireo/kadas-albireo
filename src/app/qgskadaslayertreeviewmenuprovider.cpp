#include "qgskadaslayertreeviewmenuprovider.h"
#include "qgsapplication.h"
#include "qgskadasmainwidget.h"
#include "qgslayertreeviewdefaultactions.h"
#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgsrasterlayer.h"
#include "qgsvectorlayer.h"
#include <QMenu>

QgsKadasLayerTreeViewMenuProvider::QgsKadasLayerTreeViewMenuProvider( QgsLayerTreeView* view, QgsKadasMainWidget* mainWidget ):
    mView( view ), mMainWidget( mainWidget )
{
}

QgsKadasLayerTreeViewMenuProvider::QgsKadasLayerTreeViewMenuProvider(): mView( 0 ), mMainWidget( 0 )
{
}

QgsKadasLayerTreeViewMenuProvider::~QgsKadasLayerTreeViewMenuProvider()
{
}

QMenu* QgsKadasLayerTreeViewMenuProvider::createContextMenu()
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
      QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );

      if ( layer->type() != QgsMapLayer::RedliningLayer )
      {
        if ( vlayer )
        {
          menu->addAction( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ), tr( "&Open Attribute Table" ),
                           mMainWidget, SLOT( attributeTable() ) );
        }
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
