/***************************************************************************
                              qgshttprequesthandler.h
                              -----------------------
  begin                : June 29, 2007
  copyright            : (C) 2007 by Marco Hugentobler
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

#ifndef QGSHTTPREQUESTHANDLER_H
#define QGSHTTPREQUESTHANDLER_H

#include "qgsrequesthandler.h"
#include <QColor>
#include <QPair>

typedef QList< QPair<QRgb, int> > QgsColorBox; //Color / number of pixels
typedef QMultiMap< int, QgsColorBox > QgsColorBoxMap; // sum of pixels / color box

/**Base class for request handler using HTTP.
It provides a method to send data to the client*/
class QgsHttpRequestHandler: public QgsRequestHandler
{
  public:
    QgsHttpRequestHandler();
    ~QgsHttpRequestHandler();

    virtual void sendGetMapResponse( const QString& service, QImage* img, int imageQuality ) const;
    virtual void sendGetCapabilitiesResponse( const QDomDocument& doc ) const;
    virtual void sendGetFeatureInfoResponse( const QDomDocument& infoDoc, const QString& infoFormat ) const;
    virtual void sendServiceException( const QgsMapServiceException& ex ) const;
    virtual void sendGetStyleResponse( const QDomDocument& doc ) const;
    virtual void sendGetPrintResponse( QByteArray* ba ) const;
    virtual bool startGetFeatureResponse( QByteArray* ba, const QString& infoFormat ) const;
    virtual void sendGetFeatureResponse( QByteArray* ba ) const;
    virtual void endGetFeatureResponse( QByteArray* ba ) const;
    virtual void sendGetCoverageResponse( QByteArray* ba ) const;

  protected:
    void sendHttpResponse( QByteArray* ba, const QString& format ) const;
    /**Converts format to official mimetype (e.g. 'jpg' to 'image/jpeg')
      @return mime string (or the entered string if not found)*/
    QString formatToMimeType( const QString& format ) const;

    void requestStringToParameterMap( const QString& request, QMap<QString, QString>& parameters );
    /**Read CONTENT_LENGTH characters from stdin*/
    QString readPostBody() const;

  private:
    static void medianCut( QVector<QRgb>& colorTable, int nColors, const QImage& inputImage );
    static void imageColors( QHash<QRgb, int>& colors, const QImage& image );
    static void splitColorBox( QgsColorBox& colorBox, QgsColorBoxMap& colorBoxMap,
                               QMap<int, QgsColorBox>::iterator colorBoxMapIt );
    static bool minMaxRange( const QgsColorBox& colorBox, int& redRange, int& greenRange, int& blueRange, int& alphaRange );
    static bool redCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 );
    static bool greenCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 );
    static bool blueCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 );
    static bool alphaCompare( const QPair<QRgb, int>& c1, const QPair<QRgb, int>& c2 );
    /**Calculates a representative color for a box (pixel weighted average)*/
    static QRgb boxColor( const QgsColorBox& box, int boxPixels );
};

#endif
