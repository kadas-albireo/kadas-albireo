/***************************************************************************
 *  qgsgeoadminrestcatalogprovider.h                                       *
 *  --------------------------------                                       *
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

#ifndef QGSGEOADMINRESTCATALOGPROVIDER_H
#define QGSGEOADMINRESTCATALOGPROVIDER_H

#include "qgscatalogprovider.h"

class QStandardItem;

class APP_EXPORT QgsGeoAdminRestCatalogProvider : public QgsCatalogProvider
{
    Q_OBJECT
  public:
    QgsGeoAdminRestCatalogProvider( const QString& baseUrl, QgsCatalogBrowser* browser );
    void load() override;
  private slots:
    void replyFinished();
  private:
    QString mBaseUrl;

    void parseTheme( QStandardItem* parent, const QDomElement& theme, QMap<QString, QStandardItem*>& layerParentMap );
};

#endif // QGSGEOADMINRESTCATALOGPROVIDER_H
