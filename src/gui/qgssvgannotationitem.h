/***************************************************************************
                              qgssvgannotationitem.h
                              ------------------------
  begin                : November, 2012
  copyright            : (C) 2012 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSVGANNOTATIONITEM_H
#define QGSSVGANNOTATIONITEM_H

#include "qgsannotationitem.h"
#include <QSvgRenderer>

class GUI_EXPORT QgsSvgAnnotationItem: public QgsAnnotationItem
{
    QGS_ANNOTATION_ITEM( QgsSvgAnnotationItem, "SVGAnnotationItem" )
  public:

    QgsSvgAnnotationItem( QgsMapCanvas* canvas );
    ~QgsSvgAnnotationItem();

    QgsSvgAnnotationItem* clone( QgsMapCanvas *canvas ) override { return new QgsSvgAnnotationItem( canvas, this ); }

    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;

    void paint( QPainter* painter ) override;

    void setFilePath( const QString& file );
    QString filePath() const { return mFilePath; }

  protected:
    QgsSvgAnnotationItem( QgsMapCanvas* canvas, QgsSvgAnnotationItem* source );

    QSvgRenderer mSvgRenderer;
    QString mFilePath;

    void _showItemEditor() override;
};

#endif // QGSSVGANNOTATIONITEM_H
