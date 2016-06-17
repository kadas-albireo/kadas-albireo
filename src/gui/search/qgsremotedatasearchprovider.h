/***************************************************************************
 *  qgsremotedatasearchprovider.h                                       *
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


#ifndef QGSREMOTEDATASEARCHPROVIDER_HPP
#define QGSREMOTEDATASEARCHPROVIDER_HPP

#include "qgssearchprovider.h"
#include <QMap>
#include <QRegExp>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

class GUI_EXPORT QgsRemoteDataSearchProvider : public QgsSearchProvider
{
    Q_OBJECT
  public:
    QgsRemoteDataSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString& searchtext, const SearchRegion& searchRegion ) override;
    void cancelSearch() override;

  private:
    static const int sSearchTimeout;
    static const int sResultCountLimit;
    static const QByteArray sGeoAdminUrl;

    QList<QNetworkReply*> mNetReplies;
    QgsGeometry* mReplyFilter;
    QRegExp mPatBox;
    QTimer mTimeoutTimer;

  private slots:
    void replyFinished();
    void searchTimeout() { cancelSearch(); }
};

#endif // QGSREMOTEDATASEARCHPROVIDER_HPP
