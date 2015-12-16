#ifndef QGSRIBBONLAYERTREEVIEWMENUPROVIDER_H
#define QGSRIBBONLAYERTREEVIEWMENUPROVIDER_H

#include "qgslayertreeview.h"

class QgsRibbonApp;

class QgsRibbonLayerTreeViewMenuProvider: public QObject, public QgsLayerTreeViewMenuProvider
{
    Q_OBJECT
  public:
    QgsRibbonLayerTreeViewMenuProvider( QgsLayerTreeView* view, QgsRibbonApp* mainWidget );
    ~QgsRibbonLayerTreeViewMenuProvider();

    QMenu* createContextMenu() override;

  private:
    QgsRibbonLayerTreeViewMenuProvider();

    QgsLayerTreeView* mView;
    QgsRibbonApp* mMainWidget;
};

#endif // QGSRIBBONLAYERTREEVIEWMENUPROVIDER_H
