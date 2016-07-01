/***************************************************************************
 *  qgskmlexportdialog.h                                                   *
 *  -----------                                                            *
 *  begin                : October 2015                                    *
 *  copyright            : (C) 2015 by Marco Hugentobler / Sourcepole AG   *
 *  email                : marco@sourcepole.ch                             *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSKMLEXPORT_H
#define QGSKMLEXPORT_H

#include <QList>
#include <QObject>

class QgsFeature;
class QgsFeatureRendererV2;
class QgsKMLPalLabeling;
class QgsMapLayer;
class QgsMapSettings;
class QgsRectangle;
class QgsRenderContext;
class QgsVectorLayer;
class QColor;
class QImage;
class QProgressDialog;
class QTextStream;
class QuaZip;

class GUI_EXPORT QgsKMLExport : public QObject
{
  public:
    bool exportToFile( const QString& filename, const QList<QgsMapLayer*>& layers, bool exportAnnotations, const QgsMapSettings& settings );
    static QString convertColor( const QColor& c );

  private:
    void writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc );
    void writeTiles( QgsMapLayer* mapLayer, const QgsRectangle &layerExtent, QTextStream& outStream, int drawingOrder, QuaZip* quaZip, QProgressDialog *progress );
    void writeGroundOverlay( QTextStream& outStream, const QString& name, const QString& href, const QgsRectangle& latLongBox, int drawingOrder );
    void writeBillboards( const QString& layerId, QTextStream& outStream, QuaZip* quaZip );
    void renderTile( QImage& img, const QgsRectangle& extent, QgsMapLayer* mapLayer );
    void addStyle( QTextStream& outStream, QgsFeature& f, QgsFeatureRendererV2& r, QgsRenderContext& rc );

};

#endif // QGSKMLEXPORT_H
