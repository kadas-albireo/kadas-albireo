/***************************************************************************
                         qgsdxfpallabeling.h
                         -------------------
    begin                : January 2014
    copyright            : (C) 2014 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSDXFPALLABELING_H
#define QGSDXFPALLABELING_H

#include "qgspallabeling.h"
#include "qgsmaprenderer.h"
#include "qgsrendercontext.h"

class QgsDxfExport;

class CORE_EXPORT QgsDxfPalLabeling: public QgsPalLabeling
{
  public:
    QgsDxfPalLabeling( QgsDxfExport* dxf, const QgsRectangle& bbox, double scale );
    ~QgsDxfPalLabeling();

    QgsRenderContext& renderContext() { return mRenderContext; }
    void drawLabel( pal::LabelPosition* label, QgsRenderContext& context, QgsPalLayerSettings& tmpLyr, DrawLabelType drawType, double dpiRatio = 1.0 );

  private:
    QgsDxfExport* mDxfExport;
    QgsMapRenderer mMapRenderer;
    QgsRenderContext mRenderContext;

    //only used for render context
    QImage* mImage;
    QPainter* mPainter;
};

#endif // QGSDXFPALLABELING_H
