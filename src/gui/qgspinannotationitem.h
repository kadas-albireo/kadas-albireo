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
#include "qgscoordinateutils.h"

class QgsCoordinateDisplayer;

class GUI_EXPORT QgsPinAnnotationItem: public QgsSvgAnnotationItem
{
    Q_OBJECT
  public:

    QgsPinAnnotationItem( QgsMapCanvas* canvas, QgsCoordinateUtils::TargetFormat targetFormat, const QString& targetEPSG = QString() );

    QgsPinAnnotationItem* clone( QgsMapCanvas *canvas ) override { return new QgsPinAnnotationItem( canvas, this ); }

    void setMapPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;
    void showContextMenu( const QPoint& screenPos );

    void writeXML( QDomDocument& doc ) const override;

  public slots:
    void changeCoordinateFormatter( QgsCoordinateUtils::TargetFormat targetFormat, const QString& targetEPSG = QString() );

  protected:
    QgsPinAnnotationItem( QgsMapCanvas* canvas, QgsPinAnnotationItem* source );

  private:
    QgsCoordinateUtils::TargetFormat mTargetFormat;
    QString mTargetEPSG;

    void updateToolTip();

  private slots:
    void copyPosition();
};

#endif // QGSPINANNOTATIONITEM_H
