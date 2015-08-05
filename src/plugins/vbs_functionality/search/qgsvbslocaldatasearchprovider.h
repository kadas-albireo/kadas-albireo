/***************************************************************************
 *  qgsvbsloccaldatasearchprovider.h                                       *
 *  -------------------                                                    *
 *  begin                : Aug 03, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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


#ifndef QGSVBSLOCALDATASEARCHPROVIDER_HPP
#define QGSVBSLOCALDATASEARCHPROVIDER_HPP

#include "qgsvbssearchprovider.h"
#include <QMutex>
#include <QPointer>


class QgsMapLayer;
class QgsVBSLocalDataSearchCrawler;

class QgsVBSLocalDataSearchProvider : public QgsVBSSearchProvider
{
    Q_OBJECT
  public:
    QgsVBSLocalDataSearchProvider( QgisInterface* iface );
    void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) override;
    void cancelSearch() override;

  private:
    QPointer<QgsVBSLocalDataSearchCrawler> mCrawler;
};


class QgsVBSLocalDataSearchCrawler : public QObject
{
    Q_OBJECT
  public:
    QgsVBSLocalDataSearchCrawler( const QString& searchText,
                                  const QgsVBSSearchProvider::SearchRegion& searchRegion,
                                  QList<QgsMapLayer*> layers, QObject* parent = 0 )
        : QObject( parent ), mSearchText( searchText ), mSearchRegion( searchRegion ), mLayers( layers ), mAborted( false ) {}

    void abort();

  public slots:
    void run();

  signals:
    void searchResultFound( QgsVBSSearchProvider::SearchResult result );
    void searchFinished();

  private:
    QString mSearchText;
    QgsVBSSearchProvider::SearchRegion mSearchRegion;
    QList<QgsMapLayer*> mLayers;
    QMutex mAbortMutex;
    bool mAborted;
};

#endif // QGSVBSLOCALDATASEARCHPROVIDER_HPP
