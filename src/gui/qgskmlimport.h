/***************************************************************************
 *  qgskmlimport.h                                                         *
 *  -----------                                                            *
 *  begin                : March 2018                                      *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSKMLIMPORT_H
#define QGSKMLIMPORT_H

#include "qgspointv2.h"
#include <QImage>
#include <QObject>

class QDomDocument;
class QDomElement;
class QuaZip;
class QgsMapCanvas;
class QgsRedliningLayer;

class GUI_EXPORT QgsKMLImport : public QObject
{
    Q_OBJECT
  public:
    QgsKMLImport( QgsMapCanvas *canvas, QgsRedliningLayer *redliningLayer );
    bool importFile( const QString& filename , QString &errMsg );

  private:
    struct StyleData
    {
      int outlineSize = 1;
      Qt::PenStyle outlineStyle = Qt::SolidLine;
      Qt::BrushStyle fillStyle = Qt::SolidPattern;
      QColor outlineColor = Qt::black;
      QColor fillColor = Qt::white;
      QString icon;
      QPointF hotSpot;
    };
    struct TileData
    {
      QString iconHref;
      QgsRectangle bbox;
      QSize size;
    };
    struct OverlayData
    {
      QgsRectangle bbox;
      QList<TileData> tiles;
    };

    bool importDocument( const QDomDocument& doc, QString &errMsg, QuaZip* zip = nullptr );
    void buildVSIVRT( const QString& name, OverlayData &overlayData, QuaZip* kmzZip ) const;
    QList<QgsPointV2> parseCoordinates( const QDomElement &geomEl ) const;
    StyleData parseStyle( const QDomElement& styleEl, QuaZip *zip ) const;
    QList<QgsAbstractGeometryV2 *> parseGeometries( const QDomElement& containerEl );
    QMap<QString, QString> parseExtendedData( const QDomElement& placemarkEl );
    QColor parseColor( const QString& abgr ) const;

    QgsMapCanvas* mCanvas;
    QgsRedliningLayer* mRedliningLayer;
};


#endif // QGSKMLIMPORT_H
