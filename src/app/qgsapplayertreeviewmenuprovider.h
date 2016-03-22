#ifndef QGSAPPLAYERTREEVIEWMENUPROVIDER_H
#define QGSAPPLAYERTREEVIEWMENUPROVIDER_H

#include <QObject>

#include "qgslayertreeview.h"
#include "qgsmaplayer.h"

class QgsMapCanvas;

class QgsAppLayerTreeViewMenuProvider : public QObject, public QgsLayerTreeViewMenuProvider
{
    Q_OBJECT
  public:
    QgsAppLayerTreeViewMenuProvider( QgsLayerTreeView* view, QgsMapCanvas* canvas );

    QMenu* createContextMenu() override;

  protected:
    QgsLayerTreeView* mView;
    QgsMapCanvas* mCanvas;
};

#endif // QGSAPPLAYERTREEVIEWMENUPROVIDER_H
