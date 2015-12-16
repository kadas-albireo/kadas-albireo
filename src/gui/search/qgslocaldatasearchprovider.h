/***************************************************************************
 *  qgsloccaldatasearchprovider.h                                       *
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


#ifndef QGSLOCALDATASEARCHPROVIDER_HPP
#define QGSLOCALDATASEARCHPROVIDER_HPP

#include "qgssearchprovider.h"
#include <QMutex>
#include <QPointer>

class QgsFeature;
class QgsMapLayer;
class QgsVectorLayer;
class QgsLocalDataSearchCrawler;

class GUI_EXPORT QgsLocalDataSearchProvider : public QgsSearchProvider
{
    Q_OBJECT
  public:
    QgsLocalDataSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) override;
    void cancelSearch() override;

  private:
    QPointer<QgsLocalDataSearchCrawler> mCrawler;
};


class GUI_EXPORT QgsLocalDataSearchCrawler : public QObject
{
    Q_OBJECT
  public:
    QgsLocalDataSearchCrawler( const QString& searchText,
                                  const QgsSearchProvider::SearchRegion& searchRegion,
                                  QList<QgsMapLayer*> layers, QObject* parent = 0 )
        : QObject( parent ), mSearchText( searchText ), mSearchRegion( searchRegion ), mLayers( layers ), mAborted( false ) {}

    void abort();

  public slots:
    void run();

  signals:
    void searchResultFound( QgsSearchProvider::SearchResult result );
    void searchFinished();

  private:
    static const int sResultCountLimit;

    QString mSearchText;
    QgsSearchProvider::SearchRegion mSearchRegion;
    QList<QgsMapLayer*> mLayers;
    QMutex mAbortMutex;
    bool mAborted;

    void buildResult( const QgsFeature& feature, QgsVectorLayer *layer );
};

#endif // QGSLOCALDATASEARCHPROVIDER_HPP
