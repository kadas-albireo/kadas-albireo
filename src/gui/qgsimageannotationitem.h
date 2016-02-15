/***************************************************************************
                              qgsimageannotationitem.h
                              ------------------------
  begin                : November, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : manisandro@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSIMAGEANNOTATIONITEM_H
#define QGSIMAGEANNOTATIONITEM_H

#include "qgsannotationitem.h"

class GUI_EXPORT QgsImageAnnotationItem: public QgsAnnotationItem
{
    QGS_ANNOTATION_ITEM( QgsImageAnnotationItem, "ImageAnnotationItem" )
  public:

    QgsImageAnnotationItem( QgsMapCanvas* canvas );

    QgsImageAnnotationItem* clone( QgsMapCanvas *canvas ) override { return new QgsImageAnnotationItem( canvas, this ); }

    void setImage( const QImage& image );

    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;

    void paint( QPainter* painter ) override;

  protected:
    QgsImageAnnotationItem( QgsMapCanvas* canvas, QgsImageAnnotationItem* source );

  private:
    QImage mImage;

    void _showItemEditor() override;
};

#endif // QGSIMAGEANNOTATIONITEM_H
