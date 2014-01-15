/***************************************************************************
    qgswmsconnection.h  -  WMS connection
                             -------------------
    begin                : 3 April 2005
    original             : (C) 2005 by Brendan Morley email  : morb at ozemail dot com dot au
    wms search           : (C) 2009 Mathias Walker <mwa at sourcepole.ch>, Sourcepole AG
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSWMSCONNECTION_H
#define QGSWMSCONNECTION_H
#include "qgsdatasourceuri.h"
#include "qgisgui.h"
//#include "qgscontexthelp.h"

#include <QStringList>
#include <QPushButton>

class QgisApp;
/*class QButtonGroup;*/
/*class QgsNumericSortTreeWidgetItem;*/
class QDomDocument;
class QDomElement;

/*!
 * \brief   Connections management
 */
class QgsWMSConnection : public QObject
{
    Q_OBJECT

  public:
    //! Constructor
    QgsWMSConnection( QString theConnName );
    //! Destructor
    ~QgsWMSConnection();

    static QStringList connectionList();

    static void deleteConnection( QString name );

    static QString selectedConnection();
    static void setSelectedConnection( QString name );


  public:
    QString connectionInfo();
    QString mConnName;
    QString mConnectionInfo;
    QgsDataSourceURI uri();
    QgsDataSourceURI mUri;
};


#endif // QGSWMSCONNECTION_H
