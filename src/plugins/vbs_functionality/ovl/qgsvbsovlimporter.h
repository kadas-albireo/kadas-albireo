/***************************************************************************
 *  qgsvbsovlimporter.h                                                    *
 *  -------------------                                                    *
 *  begin                : Oct 07, 2015                                    *
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

#ifndef QGSVBSOVLIMPORTER_H
#define QGSVBSOVLIMPORTER_H

#include <QObject>

class QColor;
class QDomElement;
class QFont;
class QgisInterface;
class QgsPointV2;
class QgsRedliningLayer;

class QgsVBSOvlImporter : public QObject
{
  public:
    QgsVBSOvlImporter( QgisInterface* iface, QObject* parent = 0 )
        : QObject( parent ), mIface( iface ) {}
    void import( QString file = QString() ) const;

  private:
    enum WidthHeightUnit
    {
      MapPixels = 1,
      ScreenPixels = 2,
      Meters = 3
    };
    enum GraphicDelimiter
    {
      NoDelimiter = 1,
      OutArrowDelimiter = 2,
      InArrowDelimiter = 3,
      MeasureDelimiter = 4,
      EmptyCircleDelimiter = 5,
      FilledCircleDelimiter = 6
    };

    QgisInterface* mIface;

    void parseRectangleTriangleCircle( QDomElement& object ) const;
    void parseLine( QDomElement& object ) const;
    void parsePolygon( QDomElement& object ) const;
    void parseText( QDomElement& object ) const;

    static void parseGraphic( QDomElement& attribute, QList<QgsPointV2> &points );
    static void parseGraphicTooltip( QDomElement& attribute, QString& tooltip );
    static void parseGraphicLineAttributes( QDomElement& attribute, QColor& outline, int& lineSize, Qt::PenStyle& lineStyle );
    static void parseGraphicFillAttributes( QDomElement& attribute, QColor& fill, Qt::BrushStyle& fillStyle );
    static void parseGraphicDimmAttributes( QDomElement& attribute, QColor &dimm, double& factor );
    static void parseGraphicSinglePointAttributes( QDomElement& attribute, double& width, double& height, double& rotation );
    static void parseGraphicWHUSetable( QDomElement& attribute, WidthHeightUnit& whu );
    static void parseGraphicDelimiter( QDomElement& attribute, GraphicDelimiter& startDelimiter, GraphicDelimiter &endDelimiter );
    static void parseGraphicRoundable( QDomElement& attribute, bool& roundable );
    static void parseGraphicCloseable( QDomElement& attribute, bool& closeable );
    static void parseGraphicTextAttributes( QDomElement& attribute, QString& text, QColor& textColor, QFont &font );
    static void applyDimm( double factor, const QColor &dimmColor, QColor *outline = 0, QColor *fill = 0 );

    static Qt::PenStyle convertLineStyle( int lineStyle );
    static Qt::BrushStyle convertFillStyle( int fillStyle );
};

#endif // QGSVBSOVLIMPORTER_H
