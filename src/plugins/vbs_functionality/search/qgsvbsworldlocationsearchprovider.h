/***************************************************************************
 *  qgsvbsworldlocationsearchprovider.h                                    *
 *  -------------------                                                    *
 *  begin                : Sep 16, 2015                                    *
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


#ifndef QGSWORLDVBSLOCATIONSEARCHPROVIDER_HPP
#define QGSWORLDVBSLOCATIONSEARCHPROVIDER_HPP

#include "qgsvbssearchprovider.h"
#include <QMap>
#include <QRegExp>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

class QgsVBSWorldLocationSearchProvider : public QgsVBSSearchProvider
{
    Q_OBJECT
  public:
    QgsVBSWorldLocationSearchProvider( QgisInterface* iface );
    void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) override;
    void cancelSearch() override;

  private:
    static const int sSearchTimeout;
    static const int sResultCountLimit;
    static const QByteArray sGeoAdminUrl;

    QNetworkReply* mNetReply;
    QMap<QString, QString> mCategoryMap;
    QRegExp mPatBox;
    QTimer mTimeoutTimer;

  private slots:
    void replyFinished();
};

#endif // QGSWORLDVBSLOCATIONSEARCHPROVIDER_HPP
