/***************************************************************************
 *  qgsvbslocsearchprovider.h                                              *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
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


#ifndef QGSVBSLOCSEARCHPROVIDER_HPP
#define QGSVBSLOCSEARCHPROVIDER_HPP

#include "qgsvbssearchprovider.h"
#include <QMap>
#include <QRegExp>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

class QgsVBSLocSearchProvider : public QgsVBSSearchProvider
{
    Q_OBJECT
  public:
    QgsVBSLocSearchProvider();
    void startSearch( const QString& searchtext ) override;
    void cancelSearch() override;

  private:
    static const int sSearchTimeout;
    static const int sResultCountLimit;
    static const QByteArray sGeoAdminUrl;
    static const QByteArray sGeoAdminReferrer;

    QNetworkReply* mNetReply;
    QMap<QString, QString> mCategoryMap;
    QRegExp mPatBox;
    QTimer mTimeoutTimer;

  private slots:
    void replyFinished();
};

#endif // QGSVBSLOCSEARCHPROVIDER_HPP
