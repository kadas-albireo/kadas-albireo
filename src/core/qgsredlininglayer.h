/***************************************************************************
    qgsredlininglayer.h - Redlining support
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSREDLININGLAYER_H
#define QGSREDLININGLAYER_H

#include "qgsvectorlayer.h"

class CORE_EXPORT QgsRedliningLayer : public QgsVectorLayer
{
    Q_OBJECT
  public:
    QgsRedliningLayer();
    bool addShape( QgsGeometry* geometry, const QColor& outline, const QColor& fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle , const QString &flags = QString() );
    bool addText( const QString &text, const QgsPointV2 &pos, const QColor& color, const QFont& font );
    void read( const QDomElement& redliningElem );
    void write( QDomElement& redliningElem );
};

#endif // QGSREDLININGLAYER_H
