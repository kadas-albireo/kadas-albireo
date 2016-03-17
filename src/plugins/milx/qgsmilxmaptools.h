/***************************************************************************
 *  qgsmilxmaptoolS.h                                                      *
 *  -----------------                                                      *
 *  begin                : Oct 01, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMILXMAPTOOLS_H
#define QGSMILXMAPTOOLS_H

#include "qgsmaptoolannotation.h"
#include <QPointer>

class QgsMilXAnnotationItem;
class QgsMilXItem;
class QgsMilXLayer;

class QgsMilXCreateTool : public QgsMapTool
{
  public:
    QgsMilXCreateTool( QgsMapCanvas* canvas, QgsMilXLayer *layer, const QString& symbolXml, const QString& symbolMilitaryName, int nMinPoints, bool hasVariablePoints, const QPixmap& preview );
    ~QgsMilXCreateTool();
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;

  private:
    QString mSymbolXml;
    QString mSymbolMilitaryName;
    int mMinNPoints;
    int mNPressedPoints;
    bool mHasVariablePoints;
    QgsMilXAnnotationItem* mItem;
    QgsMilXLayer* mLayer;
};

class QgsMilXEditTool : public QgsMapToolPan
{
    Q_OBJECT
  public:
    QgsMilXEditTool( QgsMapCanvas* canvas, QgsMilXLayer* layer, QgsMilXItem* item );
    ~QgsMilXEditTool();
    void canvasReleaseEvent( QMouseEvent * e );

  private:
    QPointer<QgsMilXAnnotationItem> mItem;
    QgsMilXLayer* mLayer;

};

#endif // QGSMILXMAPTOOLS_H

