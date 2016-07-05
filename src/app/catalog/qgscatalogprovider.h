/***************************************************************************
 *  qgscatalogprovider.h                                                   *
 *  --------------------                                                   *
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

#ifndef QGSCATALOGPROVIDER_H
#define QGSCATALOGPROVIDER_H

#include <QObject>
#include <QMap>

class QgsCatalogBrowser;
class QDomDocument;
class QDomElement;
class QDomNode;
class QMimeData;
class QStandardItem;

class APP_EXPORT QgsCatalogProvider : public QObject
{
    Q_OBJECT
  public:
    QgsCatalogProvider( QgsCatalogBrowser* browser );
    virtual void load() = 0;

  signals:
    void finished();

  protected:
    QgsCatalogBrowser* mBrowser;

    QList<QDomNode> childrenByTagName( const QDomElement& element, const QString& tagName ) const;
    QMap<QString, QString> parseWMTSTileMatrixSets( const QDomDocument& doc ) const;
    void parseWMTSLayerCapabilities( const QDomNode& layerItem, const QMap<QString, QString>& tileMatrixSetMap, const QString& url, const QString& extraParams, QString& title, QString& layerid, QMimeData*& mimeData ) const;
    QStringList parseWMSFormats( const QDomDocument& doc ) const;
    QString parseWMSNestedLayer( const QDomNode& layerItem ) const;
    void parseWMSLayerCapabilities( const QDomNode& layerItem, const QStringList& imgFormats, const QString& url, QString& title, QMimeData*& mimeData ) const;
    QStandardItem* getCategoryItem( const QStringList& titles , const QStringList &sortIndices );
};

#endif // QGSCATALOGPROVIDER_H
