/***************************************************************************
                          qgsnewconnection.h  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */
#ifndef QGSNEWCONNECTION_H
#define QGSNEWCONNECTION_H
#ifdef WIN32
#include "qgsnewconnectionbase.h"
#else
#include "qgsnewconnectionbase.uic.h"
#endif
/*! \class QgsNewConnection
 * \brief Dialog to allow the user to configure and save connection
 * information for a PostgresQl database
 */
class QgsNewConnection : public QgsNewConnectionBase 
{
  Q_OBJECT
  public:
    //! Constructor
    QgsNewConnection(QWidget *parent = 0, const QString& connName = QString::null, bool modal = true);
    //! Destructor
    ~QgsNewConnection();
    //! Tests the connection using the parameters supplied
    void testConnection();
    //! Saves the connection to ~/.qt/qgisrc
    void saveConnection();
  public slots:
    void helpInfo();
  private:
    static const int context_id = 821572257;
};

#endif //  QGSNEWCONNECTIONBASE_H
