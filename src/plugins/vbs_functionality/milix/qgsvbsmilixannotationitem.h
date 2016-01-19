/***************************************************************************
                              qgsvbsmilixannotationitem.h
                              ---------------------------
  begin                : Oct 01, 2015
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

#ifndef QGSVBSMILIXANNOTATIONITEM_H
#define QGSVBSMILIXANNOTATIONITEM_H

#include "qgsannotationitem.h"

class QgsVBSCoordinateDisplayer;

class QgsVBSMilixAnnotationItem : public QgsAnnotationItem
{
    QGS_ANNOTATION_ITEM( QgsVBSMilixAnnotationItem, "MilixAnnotationItem" )
  public:

    QgsVBSMilixAnnotationItem( QgsMapCanvas* canvas );
    void setSymbolXml( const QString& symbolXml );
    const QString& symbolXml() const { return mSymbolXml; }
    void setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;
    void addPoint( const QgsPoint& p );
    void modifyPoint( int index, const QgsPoint& p );
    QList<QPoint> points() const;
    void updatePixmap( const QPixmap& pixmap, const QPoint &offset );

    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;
    void paint( QPainter* painter ) override;

  private:
    QString mSymbolXml;
    QPixmap mPixmap;
    QList<QgsPoint> mGeoPoints;

    void renderPixmap();
    void _showItemEditor() override;
};

#endif // QGSVBSMILIXANNOTATIONITEM_H
