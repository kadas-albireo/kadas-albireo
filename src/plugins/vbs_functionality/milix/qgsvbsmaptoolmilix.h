/***************************************************************************
 *  qgsvbsmaptoolmilix.h                                                   *
 *  -------------------                                                    *
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

#ifndef QGSVBSMAPTOOLMILIX_H
#define QGSVBSMAPTOOLMILIX_H

#include "qgsmaptoolannotation.h"

class QgsVBSMilixAnnotationItem;
class QgsVBSMilixLayer;

class QgsVBSMapToolMilix : public QgsMapTool
{
  public:
    QgsVBSMapToolMilix( QgsMapCanvas* canvas, QgsVBSMilixLayer *layer, const QString& symbolXml, const QString& symbolMilitaryName, int nMinPoints, bool hasVariablePoints, const QPixmap& preview );
    ~QgsVBSMapToolMilix();
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;

  private:
    QString mSymbolXml;
    QString mSymbolMilitaryName;
    int mMinNPoints;
    int mNPressedPoints;
    bool mHasVariablePoints;
    QPixmap mPreview;
    QgsVBSMilixAnnotationItem* mItem;
    QgsVBSMilixLayer* mLayer;
};

#endif // QGSVBSMAPTOOLMILIX_H

