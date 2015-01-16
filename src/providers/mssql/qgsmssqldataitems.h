/***************************************************************************
                         qgsmssqldataitems.h  -  description
                         -------------------
    begin                : 2011-10-08
    copyright            : (C) 2011 by Tamas Szekeres
    email                : szekerest at gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#ifndef QGSMSSQLDATAITEMS_H
#define QGSMSSQLDATAITEMS_H

#include "qgsdataitem.h"

#include "qgsmssqlsourceselect.h"

class QgsMssqlRootItem;
class QgsMssqlConnectionItem;
class QgsMssqlSchemaItem;
class QgsMssqlLayerItem;

class QgsMssqlRootItem : public QgsDataCollectionItem
{
    Q_OBJECT
  public:
    QgsMssqlRootItem( QgsDataItem* parent, QString name, QString path );
    ~QgsMssqlRootItem();

    QVector<QgsDataItem*> createChildren() override;

    virtual QWidget * paramWidget() override;

    virtual QList<QAction*> actions() override;

  public slots:
    void connectionsChanged();
    void newConnection();
};

class QgsMssqlConnectionItem : public QgsDataCollectionItem
{
    Q_OBJECT
  public:
    QgsMssqlConnectionItem( QgsDataItem* parent, QString name, QString path );
    ~QgsMssqlConnectionItem();

    QVector<QgsDataItem*> createChildren() override;
    virtual bool equal( const QgsDataItem *other ) override;
    virtual QList<QAction*> actions() override;

    virtual bool acceptDrop() override { return true; }
    virtual bool handleDrop( const QMimeData * data, Qt::DropAction action ) override;
    void refresh() override;

    QString connInfo() const { return mConnInfo; };

  signals:
    void addGeometryColumn( QgsMssqlLayerProperty );

  public slots:
    void editConnection();
    void deleteConnection();
    void setAllowGeometrylessTables( bool allow );

    void setLayerType( QgsMssqlLayerProperty layerProperty );

  private:
    QString mConnInfo;
    QString mService;
    QString mHost;
    QString mDatabase;
    QString mUsername;
    QString mPassword;
    bool mUseGeometryColumns;
    bool mUseEstimatedMetadata;
    bool mAllowGeometrylessTables;

    void readConnectionSettings();
};

class QgsMssqlSchemaItem : public QgsDataCollectionItem
{
    Q_OBJECT
  public:
    QgsMssqlSchemaItem( QgsDataItem* parent, QString name, QString path );
    ~QgsMssqlSchemaItem();

    QVector<QgsDataItem*> createChildren() override;

    QgsMssqlLayerItem* addLayer( QgsMssqlLayerProperty layerProperty, bool refresh );
    void refresh() override {}; // do not refresh directly
    void addLayers( QgsDataItem* newLayers );
};

class QgsMssqlLayerItem : public QgsLayerItem
{
    Q_OBJECT

  public:
    QgsMssqlLayerItem( QgsDataItem* parent, QString name, QString path, QgsLayerItem::LayerType layerType, QgsMssqlLayerProperty layerProperties );
    ~QgsMssqlLayerItem();

    QString createUri();

    QgsMssqlLayerItem* createClone();
    bool Used;

  private:
    QgsMssqlLayerProperty mLayerProperty;
};

#endif // QGSMSSQLDATAITEMS_H
