/***************************************************************************
 *  qgsvbscatalogprovider.h                                                *
 *  -----------------------                                                *
 *  begin                : April, 2016                                     *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVBSCATALOGPROVIDER_H
#define QGSVBSCATALOGPROVIDER_H

#include "qgscatalogprovider.h"

class QStandardItem;

class APP_EXPORT QgsVBSCatalogProvider : public QgsCatalogProvider
{
    Q_OBJECT
  public:
    QgsVBSCatalogProvider( const QString& baseUrl, QgsCatalogBrowser* browser );
    void load() override;
  private slots:
    void replyFinished();
  private:
    struct ResultEntry
    {
      ResultEntry() {}
      ResultEntry( const QString& _category, const QString& _title )
          : category( _category ), title( _title ) {}
      ResultEntry( const ResultEntry& entry )
          : category( entry.category ), title( entry.title ) {}
      QString category;
      QString title;
    };
    typedef QMap< QString, ResultEntry > EntryMap;

    QString mBaseUrl;
    int mPendingTasks;

    void endTask();

    void readWMTSCapabilities( const QString& wmtsUrl, const EntryMap &entries );
    void readWMSCapabilities( const QString& wmsUrl, const EntryMap &entries );
    bool searchMatchingWMSLayer( const QDomNode& layerItem, const EntryMap& entries, const QString &url, const QStringList &imgFormats );

  private slots:
    void readWMTSCapabilitiesDo();
    void readWMSCapabilitiesDo();
};

#endif // QGSGEOADMINRESTCATALOGPROVIDER_H
