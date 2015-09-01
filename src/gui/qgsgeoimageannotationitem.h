/***************************************************************************
                              qgsgeoimageannotationitem.h
                              ------------------------
  begin                : August, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGEOIMAGEANNOTATIONITEM_H
#define QGSGEOIMAGEANNOTATIONITEM_H

#include "qgsannotationitem.h"
#include <QSvgRenderer>

class GUI_EXPORT QgsGeoImageAnnotationItem: public QgsAnnotationItem
{
  public:
    static QgsGeoImageAnnotationItem* create( QgsMapCanvas* canvas, const QString& filePath );

    QgsGeoImageAnnotationItem( QgsMapCanvas* canvas );
    void setFilePath( const QString& filePath );


    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;

    void paint( QPainter* painter ) override;

  private:
    QString mFilePath;
    QImage mImage;

    static bool readGeoPos( const QString& filePath, QgsPoint& wgs84Pos );

    void _showItemEditor() override;
};

#endif // QGSGEOIMAGEANNOTATIONITEM_H
