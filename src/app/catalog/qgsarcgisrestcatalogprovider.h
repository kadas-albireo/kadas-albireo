/***************************************************************************
 *  qgsarcgisrestcatalogprovider.h                                         *
 *  ------------------------------                                         *
 *  begin                : January, 2016                                   *
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

#ifndef QGSARCGISRESTCATALOGPROVIDER_H
#define QGSARCGISRESTCATALOGPROVIDER_H

#include "qgscatalogprovider.h"
#include <QStringList>

class QStandardItem;

class APP_EXPORT QgsArcGisRestCatalogProvider : public QgsCatalogProvider
{
    Q_OBJECT
  public:
    QgsArcGisRestCatalogProvider( const QString& baseUrl, QgsCatalogBrowser* browser );
    void load() override;
  private:
    QString mBaseUrl;
    int mPendingTasks;

    void endTask();
    QStandardItem* getCategoryItem( const QStringList& titles );
    void parseFolder( const QString& path, const QStringList& catTitles = QStringList() );
    void parseService( const QString& path, const QStringList& catTitles );
    void parseWMTS( const QString& path, const QStringList& catTitles );
    void parseWMS( const QString& path, const QStringList& catTitles );

  private slots:
    void parseFolderDo();
    void parseServiceDo();
    void parseWMTSDo();
    void parseWMSDo();
};

#endif // QGSARCGISRESTCATALOGPROVIDER_H
