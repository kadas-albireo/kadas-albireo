#ifndef QGSKADASLAYERTREEVIEWMENUPROVIDER_H
#define QGSKADASLAYERTREEVIEWMENUPROVIDER_H

#include "qgslayertreeview.h"

class QgsKadasMainWidget;

class QgsKadasLayerTreeViewMenuProvider: public QObject, public QgsLayerTreeViewMenuProvider
{
    Q_OBJECT
  public:
    QgsKadasLayerTreeViewMenuProvider( QgsLayerTreeView* view, QgsKadasMainWidget* mainWidget );
    ~QgsKadasLayerTreeViewMenuProvider();

    QMenu* createContextMenu() override;

  private:
    QgsKadasLayerTreeViewMenuProvider();

    QgsLayerTreeView* mView;
    QgsKadasMainWidget* mMainWidget;
};

#endif // QGSKADASLAYERTREEVIEWMENUPROVIDER_H
