/***************************************************************************
    qgsredliningrendererv2.h
    ---------------------
    begin                : Sep 2015
    copyright            : (C) 2015 Sandro Mani
    email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSREDLININGRENDERERV2_H
#define QGSREDLININGRENDERERV2_H

#include "qgsrendererv2.h"
#include "qgssymbolv2.h"
#include <QScopedPointer>

class QgsAbstractGeometryV2;

class CORE_EXPORT QgsRedliningRendererV2 : public QgsFeatureRendererV2
{
  public:

    QgsRedliningRendererV2( );
    QgsFeatureRendererV2* clone() const override { return new QgsRedliningRendererV2(); }

    QgsSymbolV2* symbolForFeature( QgsFeature& feature ) override { return originalSymbolForFeature( feature ); }
    QgsSymbolV2* originalSymbolForFeature( QgsFeature& feature ) override;
    QgsSymbolV2List symbols() override { return QgsSymbolV2List(); }

    void startRender( QgsRenderContext& context, const QgsFields& fields ) override;
    bool renderFeature( QgsFeature &feature, QgsRenderContext &context, int layer, bool selected, bool drawVertexMarker ) override;
    void stopRender( QgsRenderContext& context ) override;

    QList<QString> usedAttributes() override { return QList<QString>(); }
    QString dump() const override { return "QgsRedliningRendererV2"; }

    int capabilities() override { return SymbolLevels; }

  protected:
    QScopedPointer<QgsMarkerSymbolV2> mMarkerSymbol;
    QScopedPointer<QgsLineSymbolV2> mLineSymbol;
    QScopedPointer<QgsFillSymbolV2> mFillSymbol;

    void drawVertexMarkers( QgsAbstractGeometryV2* geom, QgsRenderContext& context );
};

#endif // QGSREDLININGRENDERERV2_H
