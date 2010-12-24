/***************************************************************************
                              qgsgetrequesthandler.h
                 class for reading from/ writing to HTTP GET
                              -------------------
  begin                : May 16, 2006
  copyright            : (C) 2006 by Marco Hugentobler & Ionut Iosifescu
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgshttprequesthandler.h"

class QgsGetRequestHandler: public QgsHttpRequestHandler
{
  public:
    QgsGetRequestHandler();
    std::map<QString, QString> parseInput();
    /**Sends the image back (but does not delete it)*/
    void sendGetMapResponse( const QString& service, QImage* img ) const;
    void sendGetCapabilitiesResponse( const QDomDocument& doc ) const;
    void sendGetFeatureInfoResponse( const QDomDocument& infoDoc, const QString& infoFormat ) const;
    void sendServiceException( const QgsMapServiceException& ex ) const;
    void sendGetStyleResponse( const QDomDocument& doc ) const;
    void sendGetPrintResponse( QByteArray* ba, const QString& formatString ) const;
};
