/***************************************************************************
                              qgspinannotationitem.h
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

#ifndef QGSPINANNOTATIONITEM_H
#define QGSPINANNOTATIONITEM_H

#include "qgssvgannotationitem.h"

class QgsCoordinateDisplayer;

class GUI_EXPORT QgsPinAnnotationItem: public QgsSvgAnnotationItem
{
    Q_OBJECT

    QGS_ANNOTATION_ITEM( QgsPinAnnotationItem, "PinAnnotationItem" )
  public:

    QgsPinAnnotationItem( QgsMapCanvas* canvas );
    ~QgsPinAnnotationItem();

    QgsPinAnnotationItem* clone( QgsMapCanvas *canvas ) override { return new QgsPinAnnotationItem( canvas, this ); }

    void setMapPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;
    void showContextMenu( const QPoint& screenPos );

    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;

    const QString& getName() const { return mName; }
    const QString& getRemarks() const { return mRemarks; }

  protected:
    QgsPinAnnotationItem( QgsMapCanvas* canvas, QgsPinAnnotationItem* source );

  private:
    QString mName;
    QString mRemarks;

    void _showItemEditor() override;

  private slots:
    void copyPosition();
    void updateToolTip();
};

#endif // QGSPINANNOTATIONITEM_H
