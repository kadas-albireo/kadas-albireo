#ifndef QGSWMSDATAITEMS_H
#define QGSWMSDATAITEMS_H

#include "qgswmsprovider.h"

class QgsWMSConnectionItem : public QgsDataCollectionItem
{
  public:
    QgsWMSConnectionItem( QgsDataItem* parent, QString name, QString path );
    ~QgsWMSConnectionItem();

    QVector<QgsDataItem*> createChildren();
    virtual bool equal( const QgsDataItem *other );

    QgsWmsCapabilitiesProperty mCapabilitiesProperty;
    QString mConnInfo;
    QVector<QgsWmsLayerProperty> mLayerProperties;
};

// WMS Layers may be nested, so that they may be both QgsDataCollectionItem and QgsLayerItem
// We have to use QgsDataCollectionItem and support layer methods if necessary
class QgsWMSLayerItem : public QgsLayerItem
{
    Q_OBJECT
  public:
    QgsWMSLayerItem( QgsDataItem* parent, QString name, QString path,
                     QgsWmsCapabilitiesProperty capabilitiesProperty, QString connInfo, QgsWmsLayerProperty layerProperties );
    ~QgsWMSLayerItem();

    QString createUri();

    QgsWmsCapabilitiesProperty mCapabilitiesProperty;
    QString mConnInfo;
    QgsWmsLayerProperty mLayerProperty;
};

class QgsWMSRootItem : public QgsDataCollectionItem
{
    Q_OBJECT
  public:
    QgsWMSRootItem( QgsDataItem* parent, QString name, QString path );
    ~QgsWMSRootItem();

    QVector<QgsDataItem*> createChildren();

    virtual QWidget * paramWidget();

  public slots:
    void connectionsChanged();
};

#endif // QGSWMSDATAITEMS_H
